#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 8080
#define BACKLOG 100
#define BUFFER_SIZE 1024

#define HTTP_200 "200 OK\r\n"
#define HTTP_204 "204 No Content\r\n"
#define HTTP_400 "400 Bad Request\r\n"
#define HTTP_404 "404 Not Found\r\n"
#define HTTP_500 "500 Internal Server Error\r\n"
#define HTTP_501 "501 Not Implemented\r\n"

static int server_socket;

char *createResponse(char *);
void handleClient(int client_socket);
char *handleGetRequest(char *, char *);
char *handlePostRequest(char *, char *, char *);
char *createErrorResponse(char *, char *);
void getCurrentDate(char *, int);
void getMimeType(char *, char *);
int sizeToString(size_t, char *, size_t);