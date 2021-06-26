# Server-Client Chat Over C Sockets

*© Israel Pereira Tavares da Silva*

> A *socket* can be thought of as an endpoint in a two-way communication channel. Socket routines create the communication channel, and the channel is used to send data between application programs either locally or over networks. Each socket within the network has a unique name associated with it called a *socket descriptor*—a fullword integer that designates a socket and allows application programs to refer to it when needed.



* [Man](https://man7.org/linux/man-pages/man2/socket.2.html)

* [IBM](https://www.ibm.com/docs/en/zos/2.3.0?topic=services-what-is-socket)

* [Understanding and Using C Pointers: Core Techniques for Memory Management](https://www.google.com/search?q=understanding+and+using+c+pointers&source=hp&ei=qlLWYN67NvbC5OUPvYa-yAE&iflsig=AINFCbYAAAAAYNZgunrxwmx5AD1FN8M_2pWAl3UkwPQa&gs_ssp=eJzj4tFP1zc0jK9MKypKMTRg9FIqzUtJLSouScxLycxLVwBSCqXFIFayQkF-Zl4JUA4ArgsRuA&oq=understanding+and+using+c+&gs_lcp=Cgdnd3Mtd2l6EAMYADIFCC4QkwIyAggAMgIIADICCAAyBggAEBYQHjIGCAAQFhAeMgYIABAWEB4yBggAEBYQHjoCCC46CAguEMcBEKMCOgsILhDHARCjAhCTAlDjAViSNWDdSmgFcAB4AIAB5AKIAY4pkgEIMC4yNy4yLjKYAQCgAQGqAQdnd3Mtd2l6sAEA&sclient=gws-wiz)

  

## About

While reading `Understanding and Using C Pointers: Core Techniques for Memory Managemen` I wanted to practice C programing on a small application. Since the topic of sockets is of my interest, I decided to build a server-client chat application.

In summary this chat application does the following:

- Clients connect to a mutual conversation room.
- Client A sends a message that reaches Client B and C.
- Clients are able to join, leave and rejoin the chat-room.
- Chat is done on terminal tabs (testing convenience). One to run the server and others for each client. 

There are two main files to accomplish the above requirements: `server.c` and `client.c`. `server.c` is the socket that listens for connections on a local path `/tmp/socket`. `client.c` connects to the server socket. There are also `thread.c` and its header file. This is basically a file used to run a function on a separate thread.

This application was built for learning purposes and its final goal is not to be an API or an interface for another application. Use it to learn and practice while reading `man` and/or other sources of information about sockets. While there are many things to improve upon, the building blocks are here.

## Usage

Close the repository to your local folder.

```bash
$ git clone https://github.com/ismalakazel/C-Socket-Chat
```

Change the permissions of `server.sh` and `client.sh`. 

```bash
$ chmod a+rx server.sh client.sh
```

On one terminal tab run the server.

```bash
$ ./server.sh
```

Open two or more terminal tabs and on each one run the client.

```bash
./client.sh clientnicknamehere
```

Use the client tabs to send messages to other clients on the same server. Messages are sent and received via the standard input on terminal.

```bash
$ ClientA: Hi, how are you?
$ ClientB: Good thanks.
$ WriteYourMessageHere
```

> He who restrains his anger overcomes his greatest enemy. (Roman proverb)

