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
	int client;
	char *username;
};
 
/// Writes to a socket.
/// - Parameter args: A struct data.
static void *write_to_socket(void *args) {	
	struct data *data = (struct data *) args;
	char message[1024];
	while (fgets(message, sizeof(message), stdin)) {
		int length = strlen(data->username) + strlen(message) + 2;
		char username[length];
		strcpy(username, data->username);
		strcat(username, ": ");
		strcat(username, message);
		ssize_t write_status = write(data->client, (const char *) &username, strlen(username));
		if (write_status == -1) {
			handle_error("Write");
		}
	};
};

int main(int argc, char * args[]) {
	
	if (argc <= 0) {
		printf("Please provide a username when starting this socket:");
		exit(EXIT_FAILURE);
	};	

	char *username = args[1];

	/// Get socket description.
	int client = socket(AF_UNIX, SOCK_STREAM, PF_UNIX);
	struct sockaddr_un addr = { 
			AF_UNIX,
			SOCKET_PATH,
	};

	/// Connect to a socket.
	int socket_conn_fd = connect(client, (struct sockaddr *) &addr, sizeof(addr));

	/// Wait for user input and write it to the socket.
	pthread_t tid = 0;
	struct data *data = malloc(sizeof(struct data));
	data->client = client;
	data->username = username;
	thread(tid, &write_to_socket, (void *) &data);	

	/// Socket to be monitored for events.
	struct pollfd pfd;
	pfd.fd = client;
	pfd.events = POLLIN;
	struct pollfd pollfds[1] = { pfd };	

	/// Start polling for socket events.
	while(1) {
		const int POLL_RESULT_ERROR = -1;
		const int POLL_RESULT_TIMEOUT = 0;
		const int POLL_RESULT_SUCCESS = 1;
		const int POLL_TIMEOUT = 0.05 * 60 * 1000;
		int rpoll = poll(pollfds, 1, POLL_TIMEOUT);
		
		if (rpoll == POLL_RESULT_ERROR) {
			handle_error("Poll");
		};

		if (rpoll == POLL_RESULT_TIMEOUT) {
			continue;	
		};
	 	
		/// Client socket has events available. 
		if (rpoll == POLL_RESULT_SUCCESS) {
			
			if (pollfds[0].revents == 0) {
				continue;			
			};

			if (pollfds[0].revents != POLLIN) {
				break;
			};	
		
			/// Read data from socket.
			int length = 1024;	
			char buffer[length];
			int message = read(pfd.fd, buffer, sizeof(buffer)-1);
			buffer[message] = 0x00;
			printf("%s", buffer);
			fflush(stdout);
			memset(buffer, 0, length);

			continue;
		};	
	};

	free(data);
	remove(SOCKET_PATH);
	close(client);
	exit(EXIT_SUCCESS);
}
