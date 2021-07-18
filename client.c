#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>

#define MAXBUFFER 1024
#define POLL_RESULT_ERROR -1
#define POLL_RESULT_TIMEOUT 0
#define POLL_RESULT_SUCCESS 1
#define POLL_TIMEOUT 0.05 * 60 * 1000
#define herror(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef enum { FALSE, TRUE } bool;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieves a node of addrinfo based on hostname (IP address or URL) and port number.
/// For more information:
/// $ man inet
/// $ man getaddrinfo
/// - Returns: A node of addrinfo.
//////////////////////////////////////////////////////////////////////////////////////////
struct addrinfo *getaddress(const char hostname[], const char port[]) {
  struct addrinfo *address;
  struct addrinfo hints;

  memset(&hints, 0, sizeof hints); // Clean the struct.

  hints.ai_family = AF_UNSPEC; // IPV4 & IPV6.
  hints.ai_socktype = SOCK_STREAM; // TCP type.
  hints.ai_flags = AI_PASSIVE | AI_CANONNAME; // Wildcard IP and canonical name.
  hints.ai_protocol = 0; // Any protocol.
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  const char status = getaddrinfo(hostname, port, &hints, &address);
  if (status != 0) {
    fprintf(stderr, "getaddrresult error: %s\n", gai_strerror(status));
    exit(EXIT_FAILURE);
  }

  return address;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Creates  and binds a socket that listen to new socket connections.
/// - Parameter *address: A pointer to a node of addrinfo.
/// - Returns: The socker file descriptor.
//////////////////////////////////////////////////////////////////////////////////////////
int upsocket(struct addrinfo *address) {
  const char fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);

  if (fd < 0)
    herror("Accept");

  const char cr = connect(fd, address->ai_addr, address->ai_addrlen);
  if (cr < 0) 
    herror("Connect");

  freeaddrinfo(address);
  return fd;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// A program that connects to a socket.
/// - Parameter argv[1]: The username of the connection socket.
/// - Parameter argv[2]: The socket hostname.
/// - Parameter argv[3]: The socket port.
//////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char * argv[]) {
  if (argc <= 3) {
    printf("Arguments are username, hostname and port.\n");
    exit(EXIT_FAILURE);
  }

  char *username = argv[1];
  const char *hostname = argv[2];
  const char *port = argv[3];
  struct addrinfo *address = getaddress(hostname, port);
  const char client = upsocket(address);
  bool isrunning = TRUE;
 
  struct pollfd sfd;
  sfd.fd = client;
  sfd.events = POLLIN;

  struct pollfd ifd;
  ifd.fd = STDIN_FILENO;
  ifd.events = POLLIN;

  const int nfds = 2;
  
  struct pollfd pollfds[2] = { sfd, ifd };	

  while(isrunning) {

    int rpoll = poll(pollfds, nfds, POLL_TIMEOUT);

    if (rpoll == POLL_RESULT_ERROR) {
      herror("Poll");
    }

    if (rpoll == POLL_RESULT_TIMEOUT) {
      continue;	
    }

    if (rpoll == POLL_RESULT_SUCCESS) {
      for (int i = 0; i < nfds; i++) {
     
        if (pollfds[i].revents == 0) {
          continue;			
        }

        if (pollfds[i].revents != POLLIN) {
          isrunning = FALSE;
          break;
        }

        if (pollfds[i].revents == POLLIN) {
          char buffer[MAXBUFFER];
          int bytes = read(pollfds[i].fd, buffer, MAXBUFFER);
          buffer[bytes] = 0x00;

          if (bytes <= 0) {
            isrunning = FALSE;
          }
          
          if (pollfds[i].fd == STDIN_FILENO) {
            const int length = strlen(username) + strlen(buffer) + 3;
            char message[length];
            strcpy(message, username);
            strcat(message, ": ");
            strcat(message, buffer);
            char wr = write(client, message, length);
            if (wr < 0)
              herror("Write");
          } else {
            printf("%s", buffer); 
          }

          memset(buffer, 0, MAXBUFFER);
        }
      } 
    }	
  }

  close(client);
  close(STDIN_FILENO);
  exit(EXIT_SUCCESS);
}
