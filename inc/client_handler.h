#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <poll.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h> 
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* Buffer length for incoming ID messages */
#define ID_BUF_LEN 64

/* Client names/variables */
#define CLIENTS() \
	CLIENT(joystick) \
	CLIENT(vision) \
	CLIENT(cmd) \

/* TODO: Move client names out of the implementation, 
 * move all functions into a header file */

/* Polling file descriptor mapping table (unique names 
 * for each pollfd defined in the CLIENTS() macro */
#define CLIENT(NAME) struct pollfd NAME;
typedef struct ClientFds {
	CLIENTS()
} ClientFds;
#undef CLIENT

/* Handler struct (pollfd's, server address info) */
typedef struct ClientHandler {
	int server_sockfd; /* Server socket file descriptor */
	struct sockaddr_in serv_addr; /* Server address struct */
	struct sockaddr_in cli_addr; /* Client address struct */
	socklen_t clilen; /* Length of client address struct */
	ClientFds clients; /* Client polling file descriptors */
	/* Polling timeout interval (Balence of delay when accepting 
	 * new clients vs server CPU usage), 10ms is a good start */
	int timeout_ms; 
} ClientHandler;

/* ======== External methods ======== */
/* Handle disconnect on read and nonexistant connection */
int handle_read(struct pollfd* fd, void* buf, size_t nbyte);

/* Handle disconnect on write and nonexistant connection */
int handle_write(struct pollfd* fd, void* buf, size_t nbyte);

/* Remove a client safely */
void handle_remove(struct pollfd* fd);

/* Create a new client handler at specified port */
ClientHandler create_client_handler(int portno, int timeout_ms);

/* Poll clients and accept new connections */
void handle_connections(ClientHandler* handler);

/* Desconstruct a client handler safely */
void hander_close(ClientHandler* handler);

/* ======== Internal methods ======== */
/* Close a socket and set it to -1 if it received POLLHUP */
int invalidate_if_hungup (struct pollfd* fd);

/* Safely assign a new client */
void assign_new_client(struct pollfd* fd_named, int fd_new);

#endif
