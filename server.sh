#!/bin/sh

rm -rf /tmp/socket && gcc -pthread -o server.out server.c thread.c && ./server.out
