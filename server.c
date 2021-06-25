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

/// Holds data (void *args) that is sent to accept_connections(void *args)
struct data {
	int server;
	int nconnections;
	struct pollfd *connections;
};

/// Createas a new socket (server). For more datarmation see `$ man socket`.
/// - Returns: The socket file descriptor.
int create_socket() {
	int server = socket(AF_UNIX, SOCK_STREAM, PF_UNIX);
	
	struct sockaddr_un addr = {
		AF_UNIX,
		SOCKET_PATH
	};

	bind(server, (struct sockaddr *) &addr, sizeof(addr));
	listen(server, SOCKET_BACKLOG);
	return server;
};

/// Accepts connections to the server socket.
/// - Parameter *args: struct data.
static void *accept_connections(void *args) {	
	struct data *data = args;
	while(1) {
		int fd = accept(data->server, (struct sockaddr *) NULL, NULL); 
		if (fd == -1) {
			handle_error("Accept");
		};
		
		struct pollfd connection;
		connection.fd = fd;
		connection.events = POLLIN;
		
		data->connections = realloc(data->connections, (data->nconnections+1) * sizeof(struct pollfd));
		data->connections[data->nconnections] = connection;
		data->nconnections += 1;
	};
};

///	Reads from the connected socket.
/// - Parameter fd: The connected socket file descriptor.
/// - Parameter length: The buffer length.
/// - Returns: A pointer to the message buffer.
/// - Warning: The pointer points to memory in the stack.   
char socket_read(int fd, char **buffer) {
	int bytes = read(fd, *buffer, sizeof(*buffer));
};

/// Writes to the connected socket.
/// - Parameter fd: The connected socket file descriptor.
/// - Parameter *message: A pointer to the message buffer.
void socket_write(int fd, char *message) {
	ssize_t write_status = write(fd, (const char *) message, sizeof(message));
};

int main() {
	
	int server = create_socket();
	
	/// Accepted socket connections in a separate thread.
	struct data *data = malloc(sizeof(struct data));
	data->server = server;
	data->nconnections = 0;
	data->connections = malloc(sizeof(struct pollfd));
	pthread_t thread_id = 0;
	thread(thread_id,  &accept_connections, (void *) &data);	

	/// Start polling for events on socket connections.
	while(1) {
		const int POLL_RESULT_ERROR = -1;
		const int POLL_RESULT_TIMEOUT = 0;
		const int POLL_RESULT_SUCCESS = 1;
		const int POLL_TIMEOUT = 0.05 * 60 * 1000;
		int rpoll = poll(data->connections, data->nconnections, POLL_TIMEOUT);
		
		if (rpoll == POLL_RESULT_ERROR) {
			handle_error("Poll");
		};

		/// Restart loop after timeout. 
		if (rpoll == POLL_RESULT_TIMEOUT) {
			continue;	
		};
		
		/// One or more connections have events available. 
		if (rpoll == POLL_RESULT_SUCCESS) {
			for (int i = 0; i < data->nconnections; i++) {
				
				struct pollfd connection = data->connections[i];
			
				/// No events available.	
				if (connection.revents == 0) {
					continue;			
				};

				/// Probably, client connection terminated.
				if (connection.revents != POLLIN) {
					/// Close connection.
					close(connection.fd);
					
					/// Create new connection array without closed connection.
					int count = 0;
					int _nconnections = data->nconnections - 1;
					struct pollfd *pfds = calloc(_nconnections, sizeof(struct pollfd));
					for (int a = 0; a < _nconnections; a++) {
						if (a == i) {
							count++;
						};
						pfds[a] = data->connections[count];
						count++;
					};

					/// Replace old connection array with resized one.
					free(data->connections);
					data->connections = calloc(_nconnections, sizeof(struct pollfd));
					data->connections = pfds;
					data->nconnections = _nconnections;
					continue;
				};

				/// Read from socket.
				char *buffer = malloc(1024);
				socket_read(connection.fd, &buffer);				
				
				/// Write to socket.
				for (int j = 0; j < data->nconnections; j++) {
					if (j != i) {
						socket_write(data->connections[j].fd, buffer);
					};
				};
				free(buffer);
			};

			continue;
		};	
	};
	
	free(data->connections);
	free(data);	
	remove(SOCKET_PATH);
	close(server);
	exit(EXIT_SUCCESS);
}
