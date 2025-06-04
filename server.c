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
    char method[16] = {0}, path[512] = {0}, version[16] = {0};

    if (!rest) {
        fprintf(stderr, "Invalid request format.\n");
        return NULL;
    }

    if (sscanf(request, "%15s %511s %15s", method, path, version) != 3) {
        fprintf(stderr, "Failed to parse request line.\n");
        return NULL;
    }

    if (strncmp(method, "GET", strlen("GET")) != 0 && strncmp(method, "POST", strlen("POST")) != 0) {
        return createErrorResponse(version, HTTP_500);
    }

    if (strstr(path, "..") || strstr(path, "//")) {
        return createErrorResponse(version, HTTP_404);
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

    //Handle the body if present
    char *body = strstr(buffer, "\r\n\r\n");
    if (body) {
        body += 4; // Move past the \r\n\r\n
        printf("Body: %s\n\n", body);
    } else {
        printf("No body in request.\n\n");
    }

    char *dir = "public";
    int dir_len = strlen(dir);
    int path_len = strlen(path);

    if (dir_len + path_len >= 512) {
        return createErrorResponse(version, HTTP_404);
    }

    char file_name[512] = {0};
    int offset = 0;
    safeCopy(file_name, sizeof(file_name), dir, dir_len, &offset);
    safeCopy(file_name, sizeof(file_name), path, path_len, &offset);

    if (strncmp(method, "GET", strlen("GET")) == 0) {
        return handleGetRequest(file_name, version);
    }
    // else if (strncmp(method, "POST", strlen("POST"))) {
    //     char *file_data;
    //     handlePostRequest(file_name, file_data, version, &response);
    // }

    return createErrorResponse(version, HTTP_500);
}

char *handleGetRequest(char *file_name, char *version) {
    printf("Looking for file: %s\n\n", file_name);

    FILE *file = fopen(file_name, "rb");
    if (!file) {
        fprintf(stderr, "Not able to open file.\n");

        return createErrorResponse(version, HTTP_404);
    }

    struct stat st;
    if (fstat(fileno(file), &st) == -1) {
        perror("fstat");
        fclose(file);

        return createErrorResponse(version, HTTP_500);
    }

    char date[128] = {0};
    getCurrentDate(date, sizeof(date));

    char file_size[32];
    if (intToString(st.st_size, file_size, sizeof(file_size)) == -1) {
        fclose(file);

        return createErrorResponse(version, HTTP_500);
    }

    char content_type[128] = {0};
    getContentType(file_name, content_type, sizeof(content_type));

    int version_len = strlen(version);
    int date_len = strlen(date);
    int file_size_len = strlen(file_size);
    int content_type_len = strlen(content_type);
    int ws_len = strlen("\r\n");
    int headers_len = version_len + strlen(" ") + strlen(HTTP_200) + 
                      date_len + strlen("Content-Length: ") + file_size_len +
                      ws_len + strlen("Content-Type: ") + content_type_len + ws_len * 2;
    int response_len = headers_len + st.st_size;

    char *response = (char *)calloc(response_len, sizeof(char));
    if (!response) {
        perror("calloc");
        fclose(file);

        return createErrorResponse(version, HTTP_500);
    }

    int offset = 0;
    safeCopy(response, response_len, version, version_len, &offset);
    safeCopy(response, response_len, " ", strlen(" "), &offset);
    safeCopy(response, response_len, HTTP_200, strlen(HTTP_200), &offset);
    safeCopy(response, response_len, date, date_len, &offset);
    safeCopy(response, response_len, "Content-Length: ", strlen("Content-Length: "), &offset);
    safeCopy(response, response_len, file_size, file_size_len, &offset);
    safeCopy(response, response_len, "\r\n", ws_len, &offset);
    safeCopy(response, response_len, content_type, content_type_len, &offset);

    printf("%s\n", response);

    if (fread(response + offset, 1, st.st_size, file) != (size_t)st.st_size) {
        fclose(file);
        free(response);

        return createErrorResponse(version, HTTP_500);
    }

    fclose(file);
    
    return response;
}

// void handlePostRequest(char *file_name, char *file_data, char *version, char **response) {
//     printf("Uploading file: %s\n\n", file_name);

//     char status[128] = {0};
//     char date[128] = {0};
//     FILE *file = fopen(file_name, "w");
//     if (!file) {
//         fprintf(stderr, "Not able to open file.\n");
//         snprintf(status, sizeof(status), "%s 500 Internal Server Error\r\n", version);
//         getCurrentDate(date);
//         snprintf(*response, BUFFER_SIZE, "%s%s\r\n", status, date);
//     }

//     snprintf(status, sizeof(status), "%s 200 OK\r\n", version);
//     getCurrentDate(date);
// }

char *createErrorResponse(char *version, char *status) {
    char date[128] = {0};
    getCurrentDate(date, sizeof(date));

    int version_len = strlen(version);
    int status_len = strlen(status);
    int date_len = strlen(date);
    int response_len = version_len + status_len + date_len;

    char *response = (char *)calloc(response_len + 1, sizeof(char));
    if (!response) {
        perror("calloc");
        return NULL;
    }

    int offset = 0;
    safeCopy(response, response_len, version, version_len, &offset);
    safeCopy(response, response_len, " ", strlen(" "), &offset);
    safeCopy(response, response_len, status, status_len, &offset);
    safeCopy(response, response_len, date, date_len, &offset);
    safeCopy(response, response_len, "\r\n", strlen("\r\n"), &offset);

    return response;
}

void safeCopy(char *dest, int dest_len, char *src, int src_len, int *offset) {
    if (*offset + src_len >= dest_len) {
        dest = NULL;
    }

    memcpy(dest + *offset, src, src_len);
    *offset += src_len;
}

void getCurrentDate(char *date, int date_size) {
    time_t current_date = time(NULL);
    struct tm *utc = gmtime(&current_date);
    int year = utc -> tm_year + 1900;
    int month = utc -> tm_mon + 1;
    int day = utc -> tm_mday;
    int hour = utc -> tm_hour;
    int min = utc -> tm_min;
    int sec = utc -> tm_sec;

    snprintf(date, date_size, "Date: %04d-%02d-%02d %02d:%02d:%02d UTC\r\n",
        year, month, day, hour, min, sec);
}

void getContentType(char *file_name, char *content_type, int content_type_size) {
    if (strstr(file_name, ".css")) {
        int offset = 0;
        safeCopy(content_type, content_type_size, "Content-Type: text/css\r\n\r\n",
            strlen("Content-Type: text/css\r\n\r\n"), &offset);
    } else {
        magic_t magic = magic_open(MAGIC_MIME_TYPE);
        magic_load(magic, NULL);

        char *mime_type = (char *)magic_file(magic, file_name);
        int offset = 0;
        safeCopy(content_type, content_type_size, "Content-Type: ", strlen("Content-Type: "), &offset);
        safeCopy(content_type, content_type_size, mime_type, strlen(mime_type), &offset);
        safeCopy(content_type, content_type_size, "\r\n\r\n", strlen("\r\n\r\n"), &offset);

        magic_close(magic);
    }
}

int intToString(unsigned int n, char *string, int string_size) {
    if (n == 0) {
        string[0] = '0';
        string[1] = '\0';

        return 0;
    }

    int digits = 0;
    unsigned int temp = n;
    while (temp > 0) {
        temp /= 10;
        digits++;
    }

    if (digits >= string_size) {
        return -1;
    }

    int i = 0;
    while (n > 0) {
        string[i++] = n % 10 + '0';
        n /= 10;
    }

    string[i] = '\0';

    for (int j = 0, k = i - 1; j < k; j++, k--) {
        char temp = string[j];
        string[j] = string[k];
        string[k] = temp;
    }

    return 0;
}