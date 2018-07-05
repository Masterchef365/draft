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

#include <fail.h>

/* Buffer length for incoming ID messages */
#define ID_BUF_LEN 64

/* Client names/variables */
#define CLIENTS() \
	CLIENT(joystick) \
	CLIENT(vision) \


/* Polling file descriptor mapping table (unique names 
 * for each pollfd defined in the CLIENTS() macro */
#define CLIENT(NAME) struct pollfd NAME;
typedef struct ClientFds {
	CLIENTS()
} ClientFds;
#undef CLIENT

/* Handler struct (pollfd's, server address info) */
typedef struct ClientHandler {
	int server_sockfd;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	socklen_t clilen;
	ClientFds clients;
} ClientHandler;

/* ======== External methods ======== */
/* Handle disconnect on read */
int handle_read(struct pollfd* fd, void* buf, size_t nbyte);

/* Create a new client handler at specified port */
ClientHandler create_client_handler(int portno);

/* Poll clients and accept new connections */
void handle_connections(ClientHandler* handler);

/* Desconstruct a client handler safely */
void close_server(ClientHandler* handler);

/* ======== Internal methods ======== */
/* Close a socket and set it to -1 if it received POLLHUP */
int invalidate_if_hungup (struct pollfd* fd);

/* Safely assign a new client */
void assign_new_client(struct pollfd* fd_named, int fd_new);

#endif
