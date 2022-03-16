#include <stdlib.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>

#include "displ.h"

#include "lid.h"
#include "listeners.h"
#include "log.h"
#include "process.h"
#include "types.h"

void connect_display(struct Displ *displ) {

	if (!(displ->display = wl_display_connect(NULL))) {
		log_error("\nUnable to connect to the compositor. Check or set the WAYLAND_DISPLAY environment variable. exiting");
		exit(EXIT_FAILURE);
	}

	displ->registry = wl_display_get_registry(displ->display);

	wl_registry_add_listener(displ->registry, registry_listener(), displ);

	if (wl_display_roundtrip(displ->display) == -1) {
		log_error("\nwl_display_roundtrip failed -1, exiting");
		exit_fail();
	}

	if (!displ->output_manager) {
		log_error("\ncompositor does not support WLR output manager protocol, exiting");
		exit(EXIT_FAILURE);
	}
}

void destroy_display(struct Displ *displ) {
	if (!displ)
		return;

	if (displ->output_manager && displ->output_manager->zwlr_output_manager) {
		wl_proxy_destroy((struct wl_proxy*) displ->output_manager->zwlr_output_manager);
	}

	wl_registry_destroy(displ->registry);

	wl_display_disconnect(displ->display);

	destroy_lid(displ->lid);
	displ->lid = NULL;

	free_displ(displ);
}

