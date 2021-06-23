#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/socket"
#define SOCKET_BACKLOG 2
#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main() {

	/// Create a file descriptor of the socket endpoint
	int socket_desc = socket(AF_UNIX, SOCK_STREAM, PF_UNIX);

	/// Create an address family 
	struct sockaddr_un my_addr = { 
			AF_UNIX,
			SOCKET_PATH,
	};

	/// Connect to socket
	int socket_connect = connect(socket_desc, (struct sockaddr *) &my_addr, sizeof(my_addr));
	if (socket_connect == -1) {
		handle_error("connect");
	}

	char *buffer = (char *) malloc(1024);	
	int read_status = read(socket_desc, buffer, sizeof(buffer)-1);
	if (read_status == -1) {
		handle_error("read");
	}
	
	buffer[read_status] = 0x00;
	free(buffer);
	printf("%s\n", buffer);
	close(socket_desc);

	return 0;
}
