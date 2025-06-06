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

#define PORT 80
#define BACKLOG 5
#define BUFFER_SIZE 1024

#define HTTP_200 "200 OK\r\n"
#define HTTP_204 "204 No Content\r\n"
#define HTTP_400 "400 Bad Request\r\n"
#define HTTP_404 "404 Not Found\r\n"
#define HTTP_411 "411 Length Required\r\n"
#define HTTP_415 "415 Unsupported Media Type\r\n"
#define HTTP_500 "500 Internal Server Error\r\n"

char *createResponse(char *);
char *handleGetRequest(char *, char *);
char *handlePostRequest(char *, char *, char *, char *);
char *createErrorResponse(char *, char *);
int safeCopy(char *, int, char *, int, int *);
void getCurrentDate(char *, int);
int intToString(unsigned int, char *, int);