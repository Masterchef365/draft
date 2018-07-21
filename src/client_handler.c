#include <client_handler.h>

ClientHandler create_client_handler(int portno, int timeout_ms) {
	ClientHandler handler;
	bzero((char *) &handler.clients, sizeof(handler.clients));
	handler.timeout_ms = timeout_ms;

	/* Set up server structs */
	bzero((char *) &handler.serv_addr, sizeof(handler.serv_addr));
	handler.serv_addr.sin_port = htons(portno);
	handler.clilen = sizeof(handler.cli_addr);
	handler.serv_addr.sin_family = AF_INET;
	handler.serv_addr.sin_addr.s_addr = INADDR_ANY;
	handler.server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	/* Set server socket to non-blocking. This allows for non-blocking client acceptance */
	int flags = fcntl(handler.server_sockfd, F_GETFL, 0);
	fcntl(handler.server_sockfd, F_SETFL, flags | O_NONBLOCK);

	/* Reuse server port. When exited incorrectly, the server can leave behind some data. */
	int tmp;
	if (setsockopt(handler.server_sockfd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)) < 0) {
		perror("reusing server socket");
		exit(EXIT_FAILURE);
	}

	/* Bind the server socket to the address */
	if (bind(handler.server_sockfd, (struct sockaddr *) &handler.serv_addr, sizeof(handler.serv_addr)) < 0) {
		perror("binding server socket");
		exit(EXIT_FAILURE);
	}

	/* Set all existing file descriptors to -1 (invalid, will be skipped in polling) */
#define CLIENT(NAME) handler.clients.NAME.fd = -1;
	CLIENTS();
#undef CLIENT

	/* Begin listening for connections */
	listen(handler.server_sockfd, 5);

	return handler;
}

int invalidate_if_hungup (struct pollfd* fd) {
	if (fd->revents & POLLHUP && fd->fd != -1) {
		close(fd->fd);
		fd->fd = -1;
		return 1;
	} else {
		return 0;
	}
}

void assign_new_client(struct pollfd* fd_named, int fd_new) {
	fd_named->events = POLLIN | POLLHUP; /* | POLLOUT */
	fd_named->fd = fd_new;
}

void handle_remove(struct pollfd* fd) {
	close(fd->fd);
	fd->fd = -1;
}

int handle_read(struct pollfd* fd, void* buf, size_t nbyte) {
	if (fd->revents & POLLIN && fd->fd != -1) {
		int nread = read(fd->fd, buf, nbyte);
		if (nread <= 0) {
			handle_remove(fd);
			return 0;
		} else {
			return nread;
		}
	} else {
		return 0;
	}
}

int handle_write(struct pollfd* fd, void* buf, size_t nbyte) {
	if (fd->fd != -1) {
	/* if (fd->revents & POLLOUT && fd->fd != -1) { */
		int nwrite = write(fd->fd, buf, nbyte);
		if (nwrite <= 0) {
			handle_remove(fd);
			return 0;
		} else {
			return nwrite;
		}
	} else {
		return 0;
	}
}

void handle_connections(ClientHandler* handler) {
	/* Poll socket to determine if connections have activity */
	poll((struct pollfd*)&handler->clients, sizeof(ClientFds) / sizeof(struct pollfd), handler->timeout_ms);

	/* Invalidate each hungup connection */
#define CLIENT(NAME) invalidate_if_hungup(&handler->clients.NAME);
	CLIENTS();
#undef CLIENT

	/* Check to see if there's a client attempting to connect */
	int client_sockfd = accept(handler->server_sockfd, (struct sockaddr *) &handler->cli_addr, &handler->clilen);

	/* If it exists, match the new client to a name and add it to the handler */
	if (client_sockfd > -1) {
		char id_buf[ID_BUF_LEN];
		bzero(&id_buf, ID_BUF_LEN);
		read(client_sockfd, id_buf, ID_BUF_LEN);
		int did_assign = 0;
#define CLIENT(NAME) if (strncmp(id_buf, "id:" #NAME "\n", ID_BUF_LEN) == 0) { assign_new_client(&handler->clients.NAME, client_sockfd); did_assign = 1; }
		CLIENTS();
#undef CLIENT
		if (!did_assign) {
			close(client_sockfd);
			fprintf(stderr, "Removed unrecognized client: %s\n", id_buf);
		} else {
			if (errno != EAGAIN) perror("On client accept\n");
		}
	}
}

void hander_close(ClientHandler* handler) {
	/* For each client, close the fd if it's valid */
#define CLIENT(NAME) if (handler->clients.NAME.fd != -1) close (handler->clients.NAME.fd);
	CLIENTS();
#undef CLIENT
	close(handler->server_sockfd);
}


