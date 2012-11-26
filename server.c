#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <sys/time.h>
#include "server.h"

void die(const char * message)
{
        if (errno) {
                perror(message);
        } else {
                fprintf(stderr, "ERROR: %s\n", message);
        }

        exit(1);
}

void events(int sockfd);

int main(int argc, char *argv[])
{
        if (argc != 2) die("USAGE: server PORT");
        long port = strtol(argv[1], NULL, 10);
        if (port <= 0) die("Invalid port");

        int status, sockfd;
        struct addrinfo hints;
        struct addrinfo *res;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        if ((status = getaddrinfo(NULL, argv[1], &hints, &res)) != 0) {
                fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
                exit(1);
        }

        sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sockfd < 0) die("Can't create socket");

        if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) die("Can't bind to port");

        if (listen(sockfd, 10) < 0) die("Failed to listen on port");

        events(sockfd);

        freeaddrinfo(res);

        return 0;
}

void events(int sockfd) {
        size_t len = 1024;
        char buf[len];
        int kq, i, recv_status, new_fd, enqueue_status, k_status;
        int nevents = 10;
        struct kevent64_s events[nevents];
        struct kevent64_s event, listen_kevent;
        struct sockaddr_storage their_addr;
        socklen_t addr_size = sizeof(their_addr);

        // Start KQueue Event Loop
        kq = kqueue();
        if (kq < 0) die("Couldn't create kqueue");

        // Add Socket Listen kevent64 to kqueue
        EV_SET64(&listen_kevent, sockfd, EVFILT_READ, EV_ADD, 0, 0, 0, 0, 0);

        k_status = kevent64(kq, &listen_kevent, 1, NULL, 0, 0, NULL);

        if (k_status < 0) die("Could not register kevent64 to listen to incoming connections");

        for (;;) {
                k_status = kevent64(kq, NULL, 0, events, nevents, 0, NULL);
                if (k_status < 0) die("kevent64 call failed");
                for (i = 0; i < k_status; i++) {
                        event = events[i];
                        fprintf(stdout, "Got an event: (%lld, %d, %d, %d, %lld, %lld, %lld, %lld)\n", event.ident, event.filter, event.flags, event.fflags, event.data, event.udata, event.ext[0], event.ext[1]);
                        if (event.ident == sockfd && event.filter == EVFILT_READ) {
                                // Got new connection accepting
                                // accept and read
                                new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &addr_size);
                                if (new_fd < 0) die("Failed to accept request");
                                fprintf(stdout, "Recived accept waiting event\n");

                                struct kevent64_s kevent;
                                EV_SET64(&kevent, new_fd, EVFILT_READ, EV_ADD, 0, 0, 0, 0, 0);
                                enqueue_status = kevent64(kq, &kevent, 1, NULL, 0, 0, NULL);
                                if (enqueue_status < 0) die("Failed to enqueue request socket");
                        } else if (event.flags & EV_EOF) {
                                // Read until empty, then close
                                fprintf(stdout, "Recived closed event\n");
                                while ((recv_status = recv(event.ident, buf, len, 0)) > 0) {
                                        fprintf(stdout, "%s", buf);
                                        memset(buf, 0, len);
                                }
                                if (close(event.ident) < 0) die("Failed to close handle");
                        } else if (event.flags & EVFILT_READ) {
                                fprintf(stdout, "Recived read waiting event\n");
                                // Read from socket
                                recv_status = recv(event.ident, buf, len, 0);
                                if (recv_status > 0) {
                                        fprintf(stdout, "%s", buf);
                                        memset(buf, 0, len);
                                }
                        }
                }
        }
}
