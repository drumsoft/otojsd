#ifndef CODESERVER_H
#define CODESERVER_H
// Otoperl::otoperld::codeserver - mini http server for otoperld.

#include <stdbool.h>
#include <netinet/in.h>

typedef struct {
	int port;
	bool findfreeport;
	struct in_addr allow_addr;
	struct in_addr allow_mask;
	int listen_fd;
	const char *(*callback)(const char *code);
	bool verbose;
	const char *document_root;
} codeserver;

codeserver *codeserver_init(int port, bool findfreeport, const char *allow, bool verbose, const char *document_root, const char *(*callback)(const char *code));
bool codeserver_start(codeserver *self);
bool codeserver_run(codeserver *self);
void codeserver_stop(codeserver *self);

#endif