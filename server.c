#include "server.h"

int main() {
    struct sockaddr_in server_addr;
    int server_socket;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_socket);
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "0.0.0.0", &server_addr.sin_addr);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, BACKLOG) == -1) {
        perror("listen");
        close(server_socket);
        return 1;
    }

    printf("Server is listening on port %d...\n\n", PORT);

    while (1) { // Infinite loop to keep accepting new connections
        struct sockaddr_in client_addr;
        int client_socket;

        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_len = sizeof(client_addr);
        
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
            perror("accept");
            continue; // Try to accept the next connection
        }

        char buffer[BUFFER_SIZE] = {0};
        if (recv(client_socket, buffer, sizeof(buffer), 0) == -1) {
            perror("recv");
            close(client_socket);
            continue;
        }

        char *response = createResponse(buffer);
        if (!response) {
            fprintf(stderr, "Error in creating response.\n");
            close(client_socket);
            continue;
        }

        if (send(client_socket, response, strlen(response), 0) == -1) {
            perror("send");
            close(client_socket);
            free(response);
            continue;
        }
        
        printf("Response sent to client.\n");

        close(client_socket);
        free(response);
    }

    close(server_socket);
    return 0;
}

char *createResponse(char *buffer) {
    char *request = buffer;
    char *rest = strstr(request, "\r\n");
    char method[16] = {0}, path[128] = {0}, version[16] = {0};

    if (!rest) {
        fprintf(stderr, "Invalid request format.\n");
        return NULL;
    }

    if (sscanf(request, "%15s %255s %15s", method, path, version) != 3) {
        fprintf(stderr, "Failed to parse request line.\n");
        return NULL;
    }

    char *header = rest + 2; // Move past the \r\n
    while (header && *header != '\0' && *header != '\r') {
        char *next_line = strstr(header, "\r\n");

        if (next_line) {
            *next_line = '\0'; // Null-terminate the header line
        }

        printf("%s\n", header);

        if (next_line) {
            header = next_line + 2; // Move past the \r\n
        } else {
            break; // No more headers
        }
    }

    // Handle the body if present
    char *body = strstr(buffer, "\r\n\r\n");
    if (body) {
        body += 4; // Move past the \r\n\r\n
        printf("Body: %s\n\n", body);
    } else {
        printf("No body in request.\n\n");
    }

    char *response = (char *)calloc(BUFFER_SIZE, sizeof(char));
    if (!response) {
        perror("calloc");
        return NULL;
    }

    char file_name[256] = {0};
    snprintf(file_name, sizeof(file_name), "public%s", path);

    if (strncmp(method, "GET", strlen("GET")) == 0) {
        handleGetRequest(file_name, version, &response);
    } else if (strncmp(method, "POST", strlen("POST"))) {
        char *file_data;
        handlePostRequest(file_name, file_data, version, &response);
    }

    return response;
}

void handleGetRequest(char *file_name, char *version, char **response) {
    printf("Looking for file: %s\n\n", file_name);

    char status[128] = {0};
    char date[128] = {0};
    FILE *file = fopen(file_name, "r");
    if (!file) {
        fprintf(stderr, "Not able to open file.\n");
        snprintf(status, sizeof(status), "%s 404 Not Found\r\n", version);
        getCurrentDate(date);
        snprintf(*response, BUFFER_SIZE, "%s%s\r\n", status, date);
    }

    snprintf(status, sizeof(status), "%s 200 OK\r\n", version);
    getCurrentDate(date);
    
    char content_length[128] = {0};
    fseek(file, 0, SEEK_END);
    snprintf(content_length, sizeof(content_length), "Content-Length: %ld\r\n", ftell(file));
    fseek(file, 0, SEEK_SET);

    char file_type[128] = {0};
    magic_t magic = magic_open(MAGIC_MIME_TYPE);
    if (strstr(file_name, ".css")) {
        snprintf(file_type, sizeof(file_type), "Content-Type: text/css\r\n\r\n");
    } else {
        magic_load(magic, NULL);
        snprintf(file_type, sizeof(file_type), "Content-Type: %s\r\n\r\n", magic_file(magic, file_name));
    }
    magic_close(magic);

    snprintf(*response, BUFFER_SIZE, "%s%s%s%s", status, date, content_length, file_type);
    printf("%s", *response);

    char text[256] = {0};
    int curr_size = BUFFER_SIZE;
    while (fgets(text, sizeof(text), file)) {
        int response_len = strlen(*response);
        int text_len = strlen(text);

        if (response_len + text_len + 1 > curr_size) {
            while (response_len + text_len + 1 > curr_size) {
                curr_size *= 2;
            }

            *response = (char *)realloc(*response, curr_size);
            if (!(*response)) {
                perror("realloc");
                free(*response);
                exit(1);
            }
        }

        strncat(*response, text, strlen(*response));
    }

    fclose(file);
}

void handlePostRequest(char *file_name, char *file_data, char *version, char **response) {
    printf("Uploading file: %s\n\n", file_name);

    char status[128] = {0};
    char date[128] = {0};
    FILE *file = fopen(file_name, "w");
    if (!file) {
        fprintf(stderr, "Not able to open file.\n");
        snprintf(status, sizeof(status), "%s 500 Internal Server Error\r\n", version);
        getCurrentDate(date);
        snprintf(*response, BUFFER_SIZE, "%s%s\r\n", status, date);
    }

    snprintf(status, sizeof(status), "%s 200 OK\r\n", version);
    getCurrentDate(date);
}

void getCurrentDate(char *date) {
    time_t current_date = time(NULL);
    struct tm *utc = gmtime(&current_date);
    int year = utc -> tm_year + 1900;
    int month = utc -> tm_mon + 1;
    int day = utc -> tm_mday;
    int hour = utc -> tm_hour;
    int min = utc -> tm_min;
    int sec = utc -> tm_sec;

    snprintf(date, 128, "Date: %04d-%02d-%02d %02d:%02d:%02d UTC\r\n", year, month, day, hour, min, sec);
}
