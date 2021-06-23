#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include "thread.h"

#define SOCKET_PATH "/tmp/socket"
#define SOCKET_BACKLOG 2
#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

/*
 * Holds data (void *args) that is sent to accept_connections(void *args)
 */
struct thread_info {
	// The connected socket file descriptor.
	int socket_fd;

	// The username on the connected socket.
	char *username;
};

/*
 * Writes to a socket.
 *
 * - Parameter args: A struct thread_info.
 */
static void *write_to_socket(void *args) {	
	struct thread_info *info = (struct thread_info *) args;
	char message[1024];
	while (fgets(message, sizeof(message), stdin)) {
		int length = strlen(info->username) + strlen(message) + 2;
		char username[length];
		strcpy(username, info->username);
		strcat(username, ": ");
		strcat(username, message);
		ssize_t write_status = write(info->socket_fd, (const char *) &username, strlen(username));
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

	/*
	 * Get socket description.
	 */
	int socket_fd = socket(AF_UNIX, SOCK_STREAM, PF_UNIX);
	struct sockaddr_un addr = { 
			AF_UNIX,
			SOCKET_PATH,
	};

	/*
	 * Connect to a socket.
	 */
	int socket_conn_fd = connect(socket_fd, (struct sockaddr *) &addr, sizeof(addr));
	if (socket_conn_fd == -1) {
		handle_error("connect");
	}	

	/*
	 * Wait for user input and write it to the socket.
	 */
	pthread_t thread_id = 0;
	struct thread_info *info = malloc(sizeof(struct thread_info));
	info->socket_fd = socket_fd;
	info->username = username;
	thread(thread_id, &write_to_socket, (void *) info);	

	struct pollfd pfd;
	pfd.fd = socket_fd;
	pfd.events = POLLIN;
	struct pollfd pollfds[1] = { pfd };	

	while(1) {
		int POLL_RESULT_ERROR = -1;
		int POLL_RESULT_TIMEOUT = 0;
		int POLL_RESULT_SUCCESS = 1;
		int POLL_TIMEOUT = 0.05 * 60 * 1000;
		int result = poll(pollfds, 1, POLL_TIMEOUT);
		
		if (result == POLL_RESULT_ERROR) {
			handle_error("Poll");
		};

		if (result == POLL_RESULT_TIMEOUT) {
			continue;	
		};
	 	
		/*
		 * Successful result means server socket has events available. 
		 */
		if (result == POLL_RESULT_SUCCESS) {
			if (pollfds[0].revents == 0) {
				continue;			
			};

			if (pollfds[0].revents != POLLIN) {
				break;
			};	
		
			/*
			 * Read data from server socket.
			 */
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

	remove(SOCKET_PATH);
	close(socket_fd);
	exit(EXIT_SUCCESS);

	return 0;
}
