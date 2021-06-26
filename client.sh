#!/bin/sh

gcc -pthread -o client.out client.c thread.c && ./client.out $1
