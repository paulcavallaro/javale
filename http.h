#ifndef _http_h
#define _http_h

struct HttpRequest {
	char **headers;
	char *body;
};

typedef struct HttpRequest HttpRequest;

#endif
