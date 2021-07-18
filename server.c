#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/signalfd.h>
#include <signal.h>

#define MAXBUFFER 1024
#define MAXBACKLOG 10
#define POLL_RESULT_ERROR -1
#define POLL_RESULT_TIMEOUT 0
#define POLL_RESULT_SUCCESS 1
#define POLL_TIMEOUT 0.05 * 60 * 1000
#define herror(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef enum { FALSE, TRUE } bool;

//////////////////////////////////////////////////////////////////////////////////////////
/// A struct to hold connected file descriptors.
//////////////////////////////////////////////////////////////////////////////////////////
typedef struct {
  int nconnections;
  struct pollfd *connections;
} PollData;

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
/// Prints the IP address of each node in a addrinfo struct. To be use with getaddrinfo().
/// - Paramater *address: A pointer to a addrinfo node.
//////////////////////////////////////////////////////////////////////////////////////////
void printaddress(struct addrinfo *address) {
  char ipstr[INET6_ADDRSTRLEN];

  while(address != NULL) {
    void *addr; 
    char *ipver;

    if (address->ai_family == AF_INET) {
      struct sockaddr_in *ipv4 = (struct sockaddr_in *) address->ai_addr;
      addr = &(ipv4->sin_addr);
      ipver = "IPV4";
    } else {
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) address->ai_addr;
      addr = &(ipv6->sin6_addr);
      ipver = "IPV6";
    }

    inet_ntop(address->ai_family, addr, ipstr, sizeof ipstr);
    
    printf("%s: %s \n", ipver, ipstr);

    address = address->ai_next;
  }
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

  int reuseaddr = 1;
  const char ssr = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof reuseaddr);
  if (ssr < 0) 
    herror("Accept");

  const char br = bind(fd, address->ai_addr, address->ai_addrlen);
  if (br != 0)
    herror("Accept");

  freeaddrinfo(address);
  
  const char lr = listen(fd, MAXBACKLOG);
  if (lr < 0)
    herror("Accept");
  
  return fd;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Adds a new file descriptor to the connections array.
/// - Parameter fd: The new file descriptor.
/// - Parameter **data: The PollData containing the connections array.
//////////////////////////////////////////////////////////////////////////////////////////
void addfd(char fd, PollData **data) {
  struct pollfd connection;
  connection.fd = fd;
  connection.events = POLLIN;

  PollData *polld = *data;
  polld->connections = realloc(polld->connections, (polld->nconnections+1) * sizeof(struct pollfd));
  polld->connections[polld->nconnections] = connection;
  ++polld->nconnections;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Removes a file descriptor from connections array.
/// - Parameter index: The index of the file descriptor in the connections array.
/// - Paramaeter **data: The PollData containing the connections array.
//////////////////////////////////////////////////////////////////////////////////////////
void remfd(int index, PollData **data) {
  PollData *polld = *data;
  close(polld->connections[index].fd);

  for (int a = index; a < polld->nconnections - 1; a++) {
    polld->connections[a] = polld->connections[a+1];
  }

  polld->connections = realloc(polld->connections, (polld->nconnections) * sizeof(struct pollfd));
  --polld->nconnections;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Blocks SIGINT and SIGQUIT, and creates a signal fd that can be used for POLLIN events.
/// Returns: A signal file descriptor.
//////////////////////////////////////////////////////////////////////////////////////////
char masksignal() {
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGQUIT);

  if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
    herror("sigprocmask");

  char sigfd = signalfd(-1, &mask, 0);
  if (sigfd == -1)
    herror("signalfd");

  return sigfd;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// A program that opens, binds and allows connection to be made to a socket.
/// - Parameter argv[1]: The socket hostname.
/// - Parameter argv[2]: The socket port.
//////////////////////////////////////////////////////////////////////////////////////////
int main(const int argc, const char *argv[]) {
  if (argc <= 2) {
    printf("Arguments are hostname and port.\n");
    exit(EXIT_FAILURE);
  }

  const char *hostname = argv[1];
  const char *port = argv[2];
  struct addrinfo *address = getaddress(hostname, port);
  const char server = upsocket(address);

  PollData *data = malloc(sizeof(PollData));
  data->nconnections = 0;
  data->connections = malloc(sizeof(struct pollfd));
  addfd(server, &data);

  char signal = masksignal(); 
  addfd(signal, &data);
  
  bool isrunning = TRUE;
  
  while(isrunning) {
    int nconnections = data->nconnections;
    int rpoll = poll(data->connections, data->nconnections, POLL_TIMEOUT);

    if (rpoll == POLL_RESULT_ERROR) 
      herror("Poll");

    if (rpoll == POLL_RESULT_TIMEOUT)
      continue;	

    if (rpoll == POLL_RESULT_SUCCESS) {
      for (int i = 0; i < nconnections; i++) {
        struct pollfd connection = data->connections[i];

        if (connection.revents == 0) 
          continue;	

        if (connection.revents != POLLIN) {
          isrunning = FALSE;
          break;
        }

        if (connection.fd == signal) {
          isrunning = FALSE;
          break;
        } else if (connection.fd == server) {
          struct sockaddr_storage cli_addr; 
          socklen_t cli_addrlen = sizeof cli_addr;
          const char fd = accept(server, (struct sockaddr *) &cli_addr, &cli_addrlen);
          if (fd > 0)
            addfd(fd, &data);
        } else {
          char buffer[MAXBUFFER];
          const int bytes = read(connection.fd, &buffer, MAXBUFFER);
          buffer[bytes] = '\0';
          if (bytes <= 0)
            remfd(i, &data);
          else 
            for (int j = 1; j < data->nconnections; j++) 
              if (j != i)
                send(data->connections[j].fd, buffer, strlen(buffer), 0);
        }
      }
    }	
  }

  free(data->connections);
  free(data);	
  close(server);
  exit(EXIT_SUCCESS);
}
