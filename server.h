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

char *createResponse(char *);
char *handleGetRequest(char *, char *);
void handlePostRequest(char *, char *, char *, char **);
void getCurrentDate(char *);