# C TCP Socket

*© Israel Pereira Tavares da Silva*

> A *socket* can be thought of as an endpoint in a two-way communication channel. Socket routines create the communication channel, and the channel is used to send data between application programs either locally or over networks. Each socket within the network has a unique name associated with it called a *socket descriptor*—a fullword integer that designates a socket and allows application programs to refer to it when needed.



* [Man](https://man7.org/linux/man-pages/man2/socket.2.html)

* [IBM](https://www.ibm.com/docs/en/zos/2.3.0?topic=services-what-is-socket)

* [Using Poll](https://www.ibm.com/docs/en/i/7.1?topic=designs-using-poll-instead-select)

* [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch)

* [Understanding and Using C Pointers: Core Techniques for Memory Management](https://www.google.com/search?q=understanding+and+using+c+pointers&source=hp&ei=qlLWYN67NvbC5OUPvYa-yAE&iflsig=AINFCbYAAAAAYNZgunrxwmx5AD1FN8M_2pWAl3UkwPQa&gs_ssp=eJzj4tFP1zc0jK9MKypKMTRg9FIqzUtJLSouScxLycxLVwBSCqXFIFayQkF-Zl4JUA4ArgsRuA&oq=understanding+and+using+c+&gs_lcp=Cgdnd3Mtd2l6EAMYADIFCC4QkwIyAggAMgIIADICCAAyBggAEBYQHjIGCAAQFhAeMgYIABAWEB4yBggAEBYQHjoCCC46CAguEMcBEKMCOgsILhDHARCjAhCTAlDjAViSNWDdSmgFcAB4AIAB5AKIAY4pkgEIMC4yNy4yLjKYAQCgAQGqAQdnd3Mtd2l6sAEA&sclient=gws-wiz)

* [Ubuntu Manual](http://manpages.ubuntu.com/manpages/cosmic/man2/signalfd.2.html#name)

While reading `Understanding and Using C Pointers: Core Techniques for Memory Managemen` I wanted to practice C programing on a small application. Since the topic of sockets is of my interest, I decided to build a server-client chat application. 

This application was built for learning purposes and its final goal is not to be an API or an interface for another application. I use it to learn and practice while reading more about sockets. There are many things to improve upon, but the building blocks are here and as I read more on the subject more pieces will be put on the right place as other pieces will be removed (whatever this means).

### How to Run

Clone the repository to your local folder.

```bash
$ git clone https://github.com/ismalakazel/C-Socket-Chat
```

On one terminal tab run the server.

```bash
$ cc -o server.out server.c && ./server.out hostname 4390
```

Open two or more terminal tabs and on each one run the client.

```bash
cc -o client.out client.c && ./client.out username hostname 4390
```

Use the client tabs to send messages to other clients on the same server. Messages are sent and received via the standard input on terminal.

```bash
$ ClientA: Hi, how are you?
$ ClientB: Good thanks.
$ WriteYourMessageHere
```
Below is a brief description of what was done in `server.c` and `client.c`.

### Server 

The first step for establishing a socket connection, or creating a socket that listens for connections, is to build a structure with the proper information about the type of sockets we are interested in.  This is done with the function call `getaddrinfo` which takes a hostname and port number to then return a struct `addrinfo` with the host's address `family`, `socktype` and `protocol`.

The host can support IPV4 and IPV6  so the requested information should include the criteria for one or both protocols. Also, address information is requested with desired socket type criteria included:

```c
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
```

If the hostname, which can be either in IP or URL formats, and the port number combination return a list of addresses from `getaddrinfo` the result(s) can be used in the `socket` and `bind`function calls:

```c
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
```

> When a socket is created with socket(2), it exists in a name space (address family) but has no address assigned to it. bind() assigns the address specified by addr to the socket referred to by the file descriptor  sockfd.   addrlen specifies the size, in bytes, of the address structure pointed to by addr. Traditionally, this operation is called  “assigning a name to a socket”.

It is best to check what each function call does in `upsocket()`. Use `man funcionname` for more information. Next call in order of importance is `listen`. With `listen` the socket can accept connections from other sockets and start reading and writing from and to them.  Not crucial but interesting to know,`setsockopt` is called to reuse the address in case the server attempts to restart before a call to `close(server)` is made.

When the socket is up, it is also ready to accept connections from other file descriptors (sockets). Once a connection is accepted sockets can communicate with read and write operations. It is important to understand that accept, as well as read, are blocking operations. For example, a call to `accept()` means the program will block subsequent operations until a connection is accepted or rejected. `read` is also a blocking operation, it allows the flow of the program past itself only when a read has something to read from a file descriptor (socket). 

Since blocking behavior is not desired, [Poll](https://man7.org/linux/man-pages/man2/poll.2.html) is used to allow the program to flow in non-blocking. [Poll](https://man7.org/linux/man-pages/man2/poll.2.html) observes a socket and tells it about events only when they are available. Events can be of type read, write, error, etc. For the purposes of this program the `POLLIN ` event is used to check when the socket has connections pending and when there are messages to read.

The first step to work with poll is to add a file descriptor to the `poll` function call, initially it will be the server socket (connected sockets will be included as they come).

```c
#define POLL_RESULT_ERROR -1
#define POLL_RESULT_TIMEOUT 0
#define POLL_RESULT_SUCCESS 1
#define POLL_TIMEOUT 0.05 * 60 * 1000

//////////////////////////////////////////////////////////////////////////////////////////
/// A struct to hold connected file descriptors.
//////////////////////////////////////////////////////////////////////////////////////////
typedef struct {
  int nconnections;
  struct pollfd *connections;
} PollData;

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

int main() {
  PollData *data = malloc(sizeof(PollData));
  data->nconnections = 0;
  data->connections = malloc(sizeof(struct pollfd));
  addfd(server, &data);
    
  while(1) {
    int nconnections = data->nconnections;
    int rpoll = poll(data->connections, data->nconnections, POLL_TIMEOUT);

    if (rpoll == POLL_RESULT_ERROR) 
      herror("Poll");

    if (rpoll == POLL_RESULT_TIMEOUT)
      continue;	

    if (rpoll == POLL_RESULT_SUCCESS) {
      for (int i = 0; i < nconnections; i++) {
        struct pollfd connection = data->connections[i];

       	if (connection.fd == server) {
          struct sockaddr_storage cli_addr; 
          socklen_t cli_addrlen = sizeof cli_addr;
          const char fd = accept(server, (struct sockaddr *) &cli_addr, &cli_addrlen);
          if (fd > 0)
            addfd(fd, &data);
        }
      }
    }	
  }
}
```

Poll keeps observing events until the specified timeout has reached. If -1 is set for timeout poll will keep observing indefinitely. Upon observing in the socket, poll indicates the type of event, the socket performs the operation that corresponds to the event and poll goes back to observation mode. In the above example, when an event is observed in the server the server performs the `accept` operation and includes the connected socket file descriptor to the connections array. Poll will now observe events in the connected socket. In the `server.c` poll observes connected sockets for incoming messages.

```c
if (connection.fd == server) {
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
```

By choice, when the server reads from a socket it sends the message to all sockets except the one that sent the message.

Note that when the number of bytes read is 0 or less  it will mean the connected socket "shutdown" and is no longer available. In this scenario the socket file descriptor is remove from poll observation.

```c
if (bytes <= 0) {
	remfd(i, &data);
}
```

When a socket disconnects the file descriptor needs to be  closed with `close` and since observations are no longer required the file descriptor is remove from the connections array.

```c
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
```

`server.c` includes a working of the concepts explained above. At this point it is better to jump to the source code, read and try to run it.

### Client

`client.c` uses the same concepts applied in `server.c`. Still, a few things are worth to be noted. 

Two file descriptors are observed: The socket itself, in case the server sends a message to the socket, and the standard user input `STDIN_FILENO` so when the client types in a message (here we assume the client is a terminal window) the socket can write a message in itself. Writing a message to itself tells the server socket that a message is available to be forwarded to the other socket connections.

```c
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
```

It is opportune to mention that a client behavior can be achieve with `telnet`.  A connection can be established with the server socket by calling telnet and passing the hostname and port number associated with the socket.

```bash
$ telnet hostname port 
```

If you read up until here, thank you.

> He who restrains his anger overcomes his greatest enemy. (Roman proverb)
