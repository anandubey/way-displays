#ifndef IPC_H
#define IPC_H

#ifndef __cplusplus

#include <stdbool.h>

#else

extern "C" { //}

#endif

enum IpcRequestCommand {
	CFG_GET = 1,
	CFG_SET,
	CFG_DEL,
};

enum IpcResponseField {
	RC = 1,
	MESSAGES,
	DONE,
};

struct IpcRequest {
	enum IpcRequestCommand command;
	struct Cfg *cfg;
	int fd;
	bool bad;
};

struct IpcResponse {
	bool done;
	int rc;
	int fd;
};

int ipc_request_send(struct IpcRequest *request);

void ipc_response_send(struct IpcResponse *response);

struct IpcRequest *ipc_request_receive(int fd_sock);

struct IpcResponse *ipc_response_receive(int fd);

void free_ipc_request(struct IpcRequest *request);

void free_ipc_response(struct IpcResponse *response);

#if __cplusplus
} // extern "C"
#endif

#endif // IPC_H

