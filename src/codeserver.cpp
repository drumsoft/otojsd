// OtoPerl - live sound programming environment with Perl.
// OtoPerl::otoperld - sound processing server for OtoPerl.
// Otoperl::otoperld::codeserver - mini http server for otoperld.
/*
    OtoPerl - live sound programming environment with Perl.
    Copyright (C) 2011- Haruka Kataoka

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <format>

#include "const.h"
#include "codeserver.h"
#include "logger.h"

#define BUFFERSIZE 8192

const int METHOD_UNKNOWN = 0;
const int METHOD_UNSUPPORTED = -1;
const int METHOD_GET     = 1;
const int METHOD_POST    = 2;

// -------------------------------------------------------- private function

void codeserver__error(codeserver *self, const char *err);
bool codeserver__decode_ipaddr(char *str, unsigned char *addr, unsigned char *mask);
bool codeserver__check_client_ip( codeserver *self, struct sockaddr_in *client);
void codeserver__write_port_file( codeserver *self, const char *path, int port );
void codeserver__respond(int conn_fd, int status, const char *body, const char *server_message);
int codeserver__get_http_method(const char *request);
bool codeserver__is_safe_path(const char *path);
bool codeserver__serve_file(codeserver *self, int conn_fd, const char *path);
char *codeserver__get_request_path(const char *request);
const char *codeserver__get_mime_type(const char *filename);

// ------------------------------------------------- codeserver_text decraration
struct codeserver_textnode_s {
	char *text;
	int size;
	struct codeserver_textnode_s *next;
};
typedef struct codeserver_textnode_s codeserver_textnode;

typedef struct {
	codeserver_textnode *root;
	codeserver_textnode *cur;
	int size;
} codeserver_text;

codeserver_text *codeserver_text_new();
void  codeserver_text_push(codeserver_text *self, char *buf, int size);
char *codeserver_text_join(codeserver_text *self);
void  codeserver_text_destroy(codeserver_text *self);

// --------------------------------------------------- codeserver implimentation

codeserver *codeserver_init(int port, bool findfreeport, const char *c_allow, bool verbose, const char *document_root, const char *(*callback)(const char *code)) {
	codeserver *self = (codeserver *)malloc(sizeof(codeserver));
	self->callback = callback;
	self->port = port;
	self->findfreeport = findfreeport;
	self->verbose = verbose;

	if (document_root) {
		self->document_root = strdup(document_root);
	} else {
		self->document_root = NULL;
	}
	
	char *allow = (char *)malloc(strlen(c_allow));
	if (!allow) {
		codeserver__error(self, "malloc failed(codeserver_init).");
		return NULL;
	}
	strcpy(allow, c_allow);
	unsigned char addr_c4[4]={0,0,0,0}, mask_c4[4]={0,0,0,0};
	char *mask = strchr(allow, '/');
	if (mask) {
		*mask = '\0';
		mask++;
		if (! codeserver__decode_ipaddr(allow, addr_c4, NULL) ) {
			codeserver__error(self, "invalid format: allowed address.");
			return NULL;
		}
		if (! codeserver__decode_ipaddr(mask , mask_c4, NULL) ) {
			codeserver__error(self, "invalid format: allowed mask.");
			return NULL;
		}
	}else{
		if (! codeserver__decode_ipaddr(allow, addr_c4, mask_c4) ) {
			codeserver__error(self, "invalid format: allowed address.");
			return NULL;
		}
	}
	self->allow_mask.s_addr = (*(uint32_t *)mask_c4);
	self->allow_addr.s_addr = (*(uint32_t *)addr_c4) & (self->allow_mask.s_addr);
	free(allow);
	
	return self;
}

bool codeserver_start(codeserver *self) {
	struct sockaddr_in saddr;
	unsigned int sockaddr_in_size = sizeof(struct sockaddr_in);
	
	if ((self->listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		codeserver__error(self, "socket failed."); return false;
	}
	
	bzero((char *)&saddr, sizeof(saddr));
	saddr.sin_family        = PF_INET;
	saddr.sin_addr.s_addr   = INADDR_ANY;
	//if ( ! inet_aton("192.168.0.1", &(saddr.sin_addr) ) ) exit(1);
	saddr.sin_port          = htons(self->port);
	if (self->findfreeport) {
		while ( bind(self->listen_fd, (struct sockaddr *)&saddr, sockaddr_in_size) < 0 ) {
			self->port++;
			saddr.sin_port = htons(self->port);
		}
		codeserver__write_port_file(self, PORTFILENAME, self->port);
	} else {
		if (bind(self->listen_fd, (struct sockaddr *)&saddr, sockaddr_in_size) < 0) {
			codeserver__error(self, "bind failed."); return false;
		}
	}

	if (listen(self->listen_fd, SOMAXCONN) < 0) {
		codeserver__error(self, "listen failed."); return false;
	}
	logger::log(std::format("codeserver start listening port {}.", self->port));
	return true;
}

void codeserver_stop(codeserver *self) {
	close(self->listen_fd);
	if (self->document_root) {
		free((void *)self->document_root);
	}
	free(self);
	logger::log("codeserver stop.");
}

bool codeserver_run(codeserver *self) {
	struct sockaddr_in caddr;
	int conn_fd;
	unsigned int sockaddr_in_size = sizeof(struct sockaddr_in);
	char buf[BUFFERSIZE];
	
	struct timeval accept_timeout = {0, 0};
	fd_set accept_fds;
	FD_ZERO(&accept_fds);
	FD_SET(self->listen_fd, &accept_fds);
	int selected = select(self->listen_fd+1, &accept_fds, (fd_set *)NULL, (fd_set *)NULL, &accept_timeout);
	if (selected < 0) {
		codeserver__error(self, "select accept failed."); return false;
	} else if (selected == 0) {
		return true;
	}

	if ((conn_fd = accept(self->listen_fd, (struct sockaddr *)&caddr, &sockaddr_in_size)) < 0) {
		codeserver__error(self, "accept failed."); return false;
	}

	logger::log(std::format("request from {}", inet_ntoa(caddr.sin_addr)));
	if ( ! codeserver__check_client_ip( self, &caddr ) ) {
		codeserver__respond(conn_fd, 403, "accessed denied.", "client from forbidden address.");
		if ( close(conn_fd) < 0) {
			codeserver__error(self, "close failed."); return false;
		}
		return true;
	}

	codeserver_text *cstext = codeserver_text_new();
	struct timeval recv_timeout = {0, 100000};
	fd_set recv_fds;
	FD_ZERO(&recv_fds);
	FD_SET(conn_fd, &recv_fds);
	while (1) {
		int selected = select(conn_fd+1, &recv_fds, (fd_set *)NULL, (fd_set *)NULL, &recv_timeout);
		if (selected < 0) {
			codeserver__error(self, "select recv failed."); return false;
		} else if (selected == 0) {
			break;
		}

		int rsize = recv(conn_fd, buf, BUFFERSIZE, 0);
		if (rsize == 0) {
			break;
		} else if (rsize == -1) {
			codeserver__error(self, "recv failed."); return false;
		} else {
			codeserver_text_push(cstext, buf, rsize);
			//if (rsize < BUFFERSIZE) break;
		}
	};

	char *request = NULL;
	int method;
	char *path = NULL;
	char *codestart = NULL;
	if (cstext->size == 0) {
		codeserver__respond(conn_fd, 400, "request is empty", "request is empty");
		codeserver_text_destroy(cstext);
		goto RETURN_AFTER_CLOSE;
	}
	request = codeserver_text_join(cstext);
	logger::log(std::format("{} bytes request received.", cstext->size));
	codeserver_text_destroy(cstext);

	method = codeserver__get_http_method(request);
	switch (method) {
	case METHOD_GET:
		if (!self->document_root) {
			codeserver__respond(conn_fd, 404, "not found.", "document root is not configured.");
			goto RETURN_AFTER_CLOSE;
		}
		path = codeserver__get_request_path(request);
		if (!path) {
			codeserver__respond(conn_fd, 400, "bad request format.", "failed to parse request path.");
			goto RETURN_AFTER_CLOSE;
		}
		logger::log(std::format("GET {}", path));
		if (!codeserver__is_safe_path(path)) {
			codeserver__respond(conn_fd, 403, "forbidden.", "unsafe path requested.");
			goto RETURN_AFTER_CLOSE;
		}
		codeserver__serve_file(self, conn_fd, path);
		break;
	case METHOD_POST:
		const char *ret;
		bool ret_allocated;
		codestart = strstr(request, "\r\n\r\n");
		if (codestart != NULL) {
			codestart += 4;
			if ( self->verbose )
				logger::log(std::format("[CODE START]\n{}\n[CODE END]", codestart));
			ret = self->callback(codestart);
			ret_allocated = true;
		}else{
			ret = "failed to find code.";
			ret_allocated = false;
		}

		if (ret == NULL) {
			codeserver__respond(conn_fd, 200, NULL, NULL);
		}else{
			codeserver__respond(conn_fd, 400, ret, "eval failed.");
		}

		if (ret_allocated) free((void *)ret);
		break;
	default:
		codeserver__respond(conn_fd, 400, "unsupported method.", "unsupported method.");
		break;
	}

RETURN_AFTER_CLOSE:
	if (path) free(path);
	if (request) free(request);
	if ( close(conn_fd) < 0) {
		codeserver__error(self, "close failed.");
		return false;
	}
	return true;
}

// -------------------------------------------- codeserver http helper implimentation

int codeserver__get_http_method(const char *request) {
	if (strncmp(request, "GET ", 4) == 0) {
		return METHOD_GET;
	} else if (strncmp(request, "POST ", 5) == 0) {
		return METHOD_POST;
	} else {
		return METHOD_UNKNOWN;
	}
}

void codeserver__respond(int conn_fd, int status, const char *body, const char *server_message) {
	if (server_message) {
		logger::error(std::format("[server response] {} {}", status, server_message));
	}
	char buf[256];
	switch (status) {
		case 200:
			write(conn_fd, "HTTP/1.1 200 OK\r\n", 17);
			break;
		case 400:
			write(conn_fd, "HTTP/1.1 400 Bad Request\r\n", 26);
			break;
		case 403:
			write(conn_fd, "HTTP/1.1 403 Forbidden\r\n", 24);
			break;
		case 404:
			write(conn_fd, "HTTP/1.1 404 Not Found\r\n", 24);
			break;
		case 500:
			write(conn_fd, "HTTP/1.1 500 Internal Server Error\r\n", 36);
			break;
		default:
			snprintf(buf, sizeof(buf), "HTTP/1.1 %d Unknown Error\r\n", status);
			write(conn_fd, buf, strlen(buf));
			break;
	}
	write(conn_fd, "Content-Type: text/plain;\r\n", 27);
	size_t body_length = body ? strlen(body) : 0;
	snprintf(buf, sizeof(buf), "Content-Length: %zu\r\n\r\n", body_length);
	write(conn_fd, buf, strlen(buf));
	if (body_length > 0) {
		write(conn_fd, body, body_length);
	}
}

void codeserver__error(codeserver *self, const char *err) {
	(void)self;
	perror(err);
}

// decode string address *str to binary address *addr.
// if *mask is not null, auto generated netmask will be set.
// example: '192.168' is decoded as '192.168.0.0/255.255.0.0'
bool codeserver__decode_ipaddr(char *str, unsigned char *addr, unsigned char *mask){
	char *err, *tp;
	for ( tp = strtok(str,"."); tp != NULL; tp = strtok(NULL,".") ){
		long number = strtol(tp, &err, 0);
		if (*err != '\0') return false;
		if (number < 0 || number > 255) return false;
		*addr++ = number;
		if (mask) {
			*mask++ = 255;
		}
	}
	return true;
}

bool codeserver__check_client_ip( codeserver *self, struct sockaddr_in *client) {
	return (self->allow_addr.s_addr) == 
	       (self->allow_mask.s_addr & client->sin_addr.s_addr);
}

void codeserver__write_port_file( codeserver *self, const char *path, int port ) {
	FILE *portfile = fopen(path, "w");
	if (portfile == NULL) {
		codeserver__error(self, "opening portfile failed."); return;
	}
	if (fprintf(portfile, "%d", port) < 0) {
		codeserver__error(self, "writing portfile failed."); return;
	}
	fclose(portfile);
}

// -------------------------------------------- codeserver_text implimentation
codeserver_text *codeserver_text_new() {
	codeserver_text *self = (codeserver_text *)malloc(sizeof(codeserver_text));
	self->root = NULL;
	self->cur = NULL;
	self->size = 0;
	return self;
}
void codeserver_text_push(codeserver_text *self, char *buf, int size) {
	codeserver_textnode *newnode = (codeserver_textnode *)malloc(sizeof(codeserver_textnode));
	newnode->text = (char *)malloc(size);
	memcpy(newnode->text, buf, size);
	newnode->size = size;
	newnode->next = NULL;

	if ( self->root == NULL ) {
		self->root = newnode;
		self->cur  = newnode;
	}else{
		self->cur->next = newnode;
		self->cur       = newnode;
	}
	self->size += size;
}
char *codeserver_text_join(codeserver_text *self) {
	codeserver_textnode *cur = self->root;
	char *text = (char *)malloc(self->size + 1);
	char *textcur = text;
	while( cur != NULL ) {
		memcpy(textcur, cur->text, cur->size);
		textcur += cur->size;
		cur = cur->next;
	}
	*textcur = 0;
	return text;
}
void codeserver_text_destroy(codeserver_text *self) {
	codeserver_textnode *cur = self->root;
	while( cur != NULL ) {
		codeserver_textnode *next = cur->next;
		free(cur->text);
		free(cur);
		cur = next;
	}
	free(self);
}

// -------------------------------------------- codeserver get method implimentation

char *codeserver__get_request_path(const char *request) {
	const char *start = strchr(request, ' ');
	if (!start) return NULL;
	start++;

	const char *end = strchr(start, ' ');
	if (!end) return NULL;

	int length = end - start;
	char *path = (char *)malloc(length + 1);
	strncpy(path, start, length);
	path[length] = '\0';
	return path;
}

bool codeserver__is_safe_path(const char *path) {
	if (strstr(path, "..") != NULL) {
		return false;
	}

	char *path_copy = strdup(path);
	char *token = strtok(path_copy, "/");
	while (token != NULL) {
		if (token[0] == '.') {
			free(path_copy);
			return false;
		}
		token = strtok(NULL, "/");
	}
	free(path_copy);
	return true;
}

const char *codeserver__get_mime_type(const char *filename) {
	const char *ext = strrchr(filename, '.');
	if (!ext) return "application/octet-stream";

	if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
	if (strcmp(ext, ".css") == 0) return "text/css";
	if (strcmp(ext, ".js") == 0) return "application/javascript";
	if (strcmp(ext, ".json") == 0) return "application/json";
	if (strcmp(ext, ".txt") == 0) return "text/plain";
	if (strcmp(ext, ".png") == 0) return "image/png";
	if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
	if (strcmp(ext, ".gif") == 0) return "image/gif";
	if (strcmp(ext, ".svg") == 0) return "image/svg+xml";

	return "application/octet-stream";
}

bool codeserver__serve_file(codeserver *self, int conn_fd, const char *path) {
	char full_path[1024];

	if (strcmp(path, "/") == 0) {
		snprintf(full_path, sizeof(full_path), "%s/index.html", self->document_root);
	} else {
		snprintf(full_path, sizeof(full_path), "%s%s", self->document_root, path);
	}

	struct stat file_stat;
	if (stat(full_path, &file_stat) < 0) {
		if (strcmp(path, "/") != 0) {
			char dir_path[1024];
			snprintf(dir_path, sizeof(dir_path), "%s%s", self->document_root, path);
			if (stat(dir_path, &file_stat) == 0 && S_ISDIR(file_stat.st_mode)) {
				snprintf(full_path, sizeof(full_path), "%s/index.html", dir_path);
				if (stat(full_path, &file_stat) < 0) {
					codeserver__respond(conn_fd, 404, "not found.", "index.html not found in directory.");
					return false;
				}
			} else {
				codeserver__respond(conn_fd, 404, "not found.", full_path);
				return false;
			}
		} else {
			codeserver__respond(conn_fd, 404, "not found.", std::format("index.html not found: {}", full_path).c_str());
			return false;
		}
	}

	if (S_ISDIR(file_stat.st_mode)) {
		snprintf(full_path + strlen(full_path), sizeof(full_path) - strlen(full_path), "/index.html");
		if (stat(full_path, &file_stat) < 0) {
			codeserver__respond(conn_fd, 404, "not found.", std::format("index.html not found in directory: {}", full_path).c_str());
			return false;
		}
	}

	int file_fd = open(full_path, O_RDONLY);
	if (file_fd < 0) {
		codeserver__respond(conn_fd, 500, "internal server error.", std::format("failed to open file: {}", full_path).c_str());
		return false;
	}

	const char *mime_type = codeserver__get_mime_type(full_path);
	char header[256];
	snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lld\r\n\r\n",
	         mime_type, (long long)file_stat.st_size);
	write(conn_fd, header, strlen(header));

	char buffer[8192];
	ssize_t bytes_read;
	while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
		write(conn_fd, buffer, bytes_read);
	}

	close(file_fd);
	logger::log(std::format("served file: {} ({} bytes)", (const char *)full_path, (long long)file_stat.st_size));
	return true;
}
