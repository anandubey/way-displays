#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include "process.h"

#include "log.h"

#define NPBUF 11

char pbuf[NPBUF] = "";
char pid_path[PATH_MAX];

void ensure_singleton() {

	// singleton in our session only
	char *vtnr = getenv("XDG_VTNR");
	if (!vtnr) {
		log_warn("XDG_VTNR not set: way-displays will not be able to run for multiple sessions.");
	}
	snprintf(pid_path, PATH_MAX, "/tmp/way-displays.%s.pid", vtnr ? vtnr : "XDG_VTNR");

	// attempt to use existing, regardless of owner
	int fd = open(pid_path, O_RDWR | O_CLOEXEC);
	if (fd == -1 && errno != ENOENT) {
		log_error("unable to open existing pid file for writing '%s' %d: %s, exiting", pid_path, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	// create a new file
	if (fd == -1) {
		mode_t umask_prev = umask(0000);
		fd = open(pid_path, O_RDWR | O_CLOEXEC | O_CREAT, 0666);
		umask(umask_prev);
		if (fd == -1) {
			log_error("nunable to create pid file '%s' %d: %s, exiting", pid_path, errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	// lock it forever
	if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
		if (errno != EWOULDBLOCK) {
			log_error("unable to lock pid file '%s' %d: %s, exiting", pid_path, errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (read(fd, pbuf, NPBUF) == -1) {
			log_error("unable to read pid file '%s' %d: %s, exiting", pid_path, errno, strerror(errno));
			exit(EXIT_FAILURE);
		}
		log_error("another instance %s is running, exiting", pbuf);
		exit(EXIT_FAILURE);
	}

	// write the new pid
	if (ftruncate(fd, 0) == -1) {
		log_error("unable to truncate pid file '%s' %d: %s, exiting", pid_path, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
	snprintf(pbuf, NPBUF, "%d", getpid());
	if (write(fd, pbuf, NPBUF) == -1) {
		log_error("unable to write pid file '%s' %d: %s, exiting", pid_path, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

