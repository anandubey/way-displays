#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "cfg.h"
#include "client.h"
#include "displ.h"
#include "fds.h"
#include "info.h"
#include "ipc.h"
#include "layout.h"
#include "lid.h"
#include "log.h"
#include "process.h"
#include "types.h"
#include "wl_wrappers.h"

// TODO server.c
// TODO sent captured error and warning
bool process_client_request(int fd_sock, struct Displ *displ) {
	if (fd_sock == -1 || !displ || !displ->cfg) {
		return false;
	}

	bool success = true;
	struct Cfg *cfg_new = NULL;
	int fd = -1;
	char *yaml_request = NULL;
	char *yaml_response = NULL;
	ssize_t n;

	if ((fd = socket_accept(fd_sock)) == -1) {
		success = false;
		goto end;
	}

	if (!(yaml_request = socket_read(fd))) {
		success = false;
		goto end;
	}

	log_debug("Received request:\n%s\n", yaml_request);

	if (!(cfg_new = cfg_merge_deltas_yaml(displ->cfg, yaml_request))) {
		success = false;
		goto end;
	}

	cfg_fix(cfg_new);

	free_cfg(displ->cfg);
	displ->cfg = cfg_new;
	displ->cfg->dirty = true;

	yaml_response = cfg_active_yaml(displ->cfg);
	if (!yaml_response) {
		success = false;
		goto end;
	}

	log_debug("Sent response:\n%s\n", yaml_request);

	if ((n = socket_write(fd, yaml_response, strlen(yaml_response))) == -1) {
		success = false;
		goto end;
	}

	log_info("\nActive configuration:");
	print_cfg(displ->cfg);

end:
	if (fd != -1) {
		close(fd);
	}
	if (yaml_request) {
		free(yaml_request);
	}
	if (yaml_response) {
		free(yaml_response);
	}

	return success;
}

// see Wayland Protocol docs Appendix B wl_display_prepare_read_queue
int loop(struct Displ *displ) {
	bool user_changes = false;
	bool initial_run_complete = false;
	bool lid_discovery_complete = false;

	init_pfds(displ->cfg);
	for (;;) {
		user_changes = false;
		create_pfds(displ);


		// prepare for reading wayland events
		while (_wl_display_prepare_read(displ->display, FL) != 0) {
			_wl_display_dispatch_pending(displ->display, FL);
		}
		_wl_display_flush(displ->display, FL);


		if (!initial_run_complete || lid_discovery_complete) {
			if (poll(pfds, npfds, -1) < 0) {
				log_error_errno("\npoll failed, exiting");
				exit(EXIT_FAILURE);
			}
		} else {
			// takes ~1 sec hence we defer
			displ->lid = create_lid();
			update_lid(displ);
			lid_discovery_complete = true;
		}


		// subscribed signals are mostly a clean exit
		if (pfd_signal && pfd_signal->revents & pfd_signal->events) {
			struct signalfd_siginfo fdsi;
			if (read(fd_signal, &fdsi, sizeof(fdsi)) == sizeof(fdsi)) {
				if (fdsi.ssi_signo != SIGPIPE) {
					return fdsi.ssi_signo;
				} else {
					log_info("ignoring SIGPIPE");
				}
			}
		}


		// cfg directory change
		if (pfd_cfg_dir && pfd_cfg_dir->revents & pfd_cfg_dir->events) {
			if (cfg_file_written(displ->cfg->file_name)) {
				user_changes = true;
				displ->cfg = cfg_file_reload(displ->cfg);
			}
		}


		// ipc client message
		if (pfd_ipc && pfd_ipc->revents & pfd_ipc->events) {
			log_info("\nRequest from client:");
			user_changes = process_client_request(fd_ipc, displ);
		}


		// safe to always read and dispatch wayland events
		_wl_display_read_events(displ->display, FL);
		_wl_display_dispatch_pending(displ->display, FL);


		if (!displ->output_manager) {
			log_info("\nDisplay's output manager has departed, exiting");
			exit(EXIT_SUCCESS);
		}


		// dispatch libinput events only when we have received a change
		if (pfd_lid && pfd_lid->revents & pfd_lid->events) {
			user_changes = user_changes || update_lid(displ);
		}
		// always do this, to cover the initial case
		update_heads_lid_closed(displ);


		// inform of head arrivals and departures and clean them
		user_changes = user_changes || consume_arrived_departed(displ->output_manager);


		// if we have no changes in progress we can maybe react to inital or modified state
		if (is_dirty(displ) && !is_pending_output_manager(displ->output_manager)) {

			// prepare possible changes
			reset_dirty(displ);
			desire_arrange(displ);
			pend_desired(displ);

			if (is_pending_output_manager(displ->output_manager)) {

				// inform and apply
				print_heads(DELTA, displ->output_manager->heads);
				apply_desired(displ);

			} else if (user_changes) {
				log_info("\nNo changes needed");
			}
		}


		// no changes are outstanding
		if (!is_pending_output_manager(displ->output_manager)) {
			initial_run_complete = true;
		}


		destroy_pfds();
	}
}

int
server() {

	struct Displ *displ = calloc(1, sizeof(struct Displ));

	log_info("way-displays version %s", VERSION);

	// only one instance
	pid_file_create();

	// always returns a cfg, possibly default
	displ->cfg = cfg_file_load();

	// discover the output manager via a roundtrip
	connect_display(displ);

	// only stops when signalled or display goes away
	int sig = loop(displ);

	// release what remote resources we can
	destroy_display(displ);

	return sig;
}


int
main(int argc, const char **argv) {
	setlinebuf(stdout);

	if (argc > 1) {
		return client(argc, argv);
	} else {
		return server();
	}
}

