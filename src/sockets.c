#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#include "log.h"

#define SERVER_TIMEOUT_SEC 2
#define CLIENT_TIMEOUT_SEC 10

bool set_socket_timeout(int fd, struct timeval timeout) {

	if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
		log_error_errno("socket set timeout failed");
		return false;
	}

	return true;
}

int socket_accept(int fd_sock) {

	int fd = accept(fd_sock, NULL, NULL);
	if (fd == -1) {
		log_error_errno("socket accept failed");
		return -1;
	}

	struct timeval timeout = { .tv_sec = SERVER_TIMEOUT_SEC, .tv_usec = 0, };
	if (!set_socket_timeout(fd, timeout)) {
		return -1;
	}

	return fd;
}

char *socket_read(int fd) {

	// peek, as the sender may experience delay between connecting and sending
	if (recv(fd, NULL, 0, MSG_PEEK) == -1) {
		if (errno == EAGAIN) {
			log_error("socket read timeout");
		} else {
			log_error_errno("socket recv failed");
		}
		return NULL;
	}

	// total message size right now; further data will be disregarded
	int n = 0;
	if (ioctl(fd, FIONREAD, &n) == -1) {
		log_error_errno("server FIONREAD failed");
		return NULL;
	}
	if (n == 0) {
		log_error("socket no data");
		return NULL;
	}

	// read it
	char *buf = calloc(n + 1, sizeof(char));
	if (recv(fd, buf, n, 0) == -1) {
		log_error_errno("socket recv failed");
		return NULL;
	}

	return buf;
}

ssize_t socket_write(int fd, char *data, size_t len) {

	ssize_t n;
	if ((n = write(fd, data, len)) == -1) {
		log_error_errno("socket write failed");
		return -1;
	}

	return n;
}

void socket_path(struct sockaddr_un *addr) {
	size_t sun_path_size = sizeof(addr->sun_path);

	const char *xdg_vtnr = getenv("XDG_VTNR");

	char name[sun_path_size];
	if (xdg_vtnr) {
		snprintf(name, sizeof(name), "/way-displays.%s.sock", xdg_vtnr);
	} else {
		snprintf(name, sizeof(name), "/way-displays.sock");
	}

	const char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
	if (!xdg_runtime_dir || strlen(name) + strlen(xdg_runtime_dir) > sun_path_size) {
		snprintf(addr->sun_path, sun_path_size, "/tmp%s", name);
	} else {
		snprintf(addr->sun_path, sun_path_size, "%s%s", xdg_runtime_dir, name);
	}
}

int create_fd_ipc_server() {
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1) {
		log_error_errno("server socket failed, clients unavailable");
		return -1;
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	socket_path(&addr);
	unlink(addr.sun_path);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		log_error_errno("server socket bind failed, clients unavailable");
		close(fd);
		return -1;
	}

	if (listen(fd, 3) < 0) {
		log_error_errno("server socket listen failed, clients unavailable");
		close(fd);
		return -1;
	}

	struct timeval timeout = { .tv_sec = SERVER_TIMEOUT_SEC, .tv_usec = 0, };
	if (!set_socket_timeout(fd, timeout)) {
		close(fd);
		return -1;
	}

	return fd;
}

int create_fd_ipc_client() {

	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd == -1) {
		log_error_errno("socket create failed");
		return -1;
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	socket_path(&addr);

	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		log_error_errno("socket connect failed");
		close(fd);
		return -1;
	}

	struct timeval timeout = { .tv_sec = CLIENT_TIMEOUT_SEC, .tv_usec = 0, };
	if (!set_socket_timeout(fd, timeout)) {
		close(fd);
		return -1;
	}

	return fd;
}

