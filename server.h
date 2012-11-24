#ifndef _server_h
#define _server_h

struct Response {
	char **headers;
	int response_length;
	char *response;
};

typedef struct Response Response;

#endif
