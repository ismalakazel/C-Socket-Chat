#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "thread.h"

#define SOCKET_PATH "/tmp/socket"
#define SOCKET_BACKLOG 2
#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

/*
 * Holds data (void *args) that is sent to accept_connections(void *args)
 */
struct thread_info {
	/// The server socket file descriptor.
	int socket_fd;

	/// The total number of socket connections.
	int socket_conn_count;

	// An array of struct pollfd. (See `$ man poll` for more information on this struct)
	struct pollfd *socket_conn_fds;
};

/*
 * Accepts connections to the server socket.
 *
 * - Parameter *args: struct thread_info.
 */
static void *accept_connections(void *args) {	
	struct thread_info *info = args;
	while(1) {
		int fd = accept(info->socket_fd, (struct sockaddr *) NULL, NULL); 
		if (fd == -1) {
			handle_error("Accept");
		};
		
		struct pollfd newfd;
		newfd.fd = fd;
		newfd.events = POLLIN;

		int count = info->socket_conn_count;
		info->socket_conn_fds = realloc(info->socket_conn_fds, (count + 1) * sizeof(struct pollfd));	
		info->socket_conn_fds[count] = newfd;
		info->socket_conn_count += 1;

		printf("New connection accepted for file descriptor #: %d \n", fd);
	};
};

/*
 * Createas a new socket (server). For more information see `$ man socket`.
 *
 * - Returns: The socket file descriptor.
 */
int create_socket() {
	int socket_fd = socket(AF_UNIX, SOCK_STREAM, PF_UNIX);
	
	struct sockaddr_un addr = {
		AF_UNIX,
		SOCKET_PATH
	};

	int bind_status = bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr));
	if (bind_status == -1) {
		handle_error("bind");
	}

	int listen_status = listen(socket_fd, SOCKET_BACKLOG);
	if (listen_status == -1) {
		handle_error("listen");
	}
	
	return socket_fd;
};

/*
 * Reads from the connected socket.
 *
 * - Parameter fd: The connected socket file descriptor.
 * - Parameter length: The buffer length.
 * - Returns: A pointer to the message buffer.
 * - Warning: The pointer points to memory in the stack.   
 */
char *read_message(int fd, int length) {
	char buffer[length];
	int bytes = read(fd, buffer, sizeof(buffer)-1);
	buffer[bytes] = 0x00;
	char *message = buffer;
	return message;
};

/*
 * Writes to the connected socket.
 * - Parameter fd: The connected socket file descriptor.
 * - Parameter *message: A pointer to the message buffer.
 */
void write_message(int fd, char *message) {
	ssize_t write_status = write(fd, (const char *) message, strlen(message));
	if (write_status == -1) {
		handle_error("Write");
	};
};

int main() {
	
	int socket_fd = create_socket();
	
	/*
	 * Accepted socket connections in a separate thread.
	 */
	pthread_t thread_id = 0;
	struct thread_info *info = malloc(sizeof(struct thread_info));
	info->socket_fd = socket_fd;
	info->socket_conn_count = 0;
	info->socket_conn_fds;
	thread(thread_id,  &accept_connections, (void *) info);	

	/*
	 * Start polling for events on socket connections.
	 */
	while(1) {
		int POLL_RESULT_ERROR = -1;
		int POLL_RESULT_TIMEOUT = 0;
		int POLL_RESULT_SUCCESS = 1;
		int POLL_TIMEOUT = 0.05 * 60 * 1000;
		int result = poll(info->socket_conn_fds, info->socket_conn_count, POLL_TIMEOUT);
		
		if (result == POLL_RESULT_ERROR) {
			handle_error("Poll");
		};

		if (result == POLL_RESULT_TIMEOUT) {
			continue;	
		};
		
		/*
		 * Successful result means one or more connections have events available. 
		 */
		if (result == POLL_RESULT_SUCCESS) {
			for (int i = 0; i < info->socket_conn_count; i++) {
				struct pollfd fd = info->socket_conn_fds[i];
				
				if (fd.revents == 0) {
					continue;			
				};

				/*
				 * In any event other than POLLIN close and remove fd from pollfds array.
				 */
				if (fd.revents != POLLIN) {
					for (int a = i; a < info->socket_conn_count; a++) {
						info->socket_conn_fds[a] = info->socket_conn_fds[a + 1];
					};

					info->socket_conn_fds = realloc(info->socket_conn_fds, info->socket_conn_count * sizeof(struct pollfd));	
					info->socket_conn_count -= 1;
					close(fd.fd);
					continue;

				};

				char *message;
				message = read_message(info->socket_conn_fds[i].fd, 1024);				

				/*
				 * Send message to other socket connections. 
				 */
				for (int o = 0; o < info->socket_conn_count; o++) {
					if (o == i) {
						continue;
					};
					
					write_message(info->socket_conn_fds[o].fd, message);
				};

				/* 
				 * clear message data. 
				 */
				memset(message, 0, sizeof(message));
			};

			continue;
		};	
	};
	
	free(info);	
	remove(SOCKET_PATH);
	close(socket_fd);
	
	return 0;
}
