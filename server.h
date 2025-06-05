#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <magic.h>

#define PORT 8080
#define BACKLOG 5
#define BUFFER_SIZE 1024

#define HTTP_200 "200 OK\r\n"
#define HTTP_404 "404 Not Found\r\n"
#define HTTP_500 "500 Internal Server Error\r\n"

char *createResponse(char *);
char *handleGetRequest(char *, char *);
void handlePostRequest(char *, char *, char *, char **);
char *createErrorResponse(char *, char *);
void safeCopy(char *, int, char *, int, int *);
void getCurrentDate(char *, int);
void getContentType(char *, char *, int);
int intToString(unsigned int, char *, int);