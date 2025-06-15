#include "server.h"
#include <locale.h>

int main() {
    struct sockaddr_in server_addr;

    if ((server_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1) {
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
    server_addr.sin_addr.s_addr = INADDR_ANY;

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

    printf("Server is listening on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket;

        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                perror("accept");
            }

            continue;
        }

        int opt = 1;
        setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
        handleClient(client_socket);
    }

    close(server_socket);
    return 0;
}

void handleClient(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_read;

    bytes_read = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }

    char *response = createResponse(buffer);
    if (!response) {
        close(client_socket);
        return;
    }

    send(client_socket, response, strlen(response), 0);
    free(response);
    close(client_socket);
}

char *createResponse(char *buffer) {
    char *request = buffer;
    char method[16] = {0}, path[512] = {0}, version[16] = {0};

    if (sscanf(request, "%15s %511s %15s", method, path, version) != 3) {
        return createErrorResponse(version, HTTP_400);
    }

    if (strstr(path, "..") || strstr(path, "//")) {
        return createErrorResponse(version, HTTP_404);
    } else if (strlen("public") + strlen(path) >= 512) {
        return createErrorResponse(version, HTTP_404);
    }

    memmove(path + strlen("public"), path, strlen(path) + 1);
    memcpy(path, "public", strlen("public"));

    if (strcmp(method, "GET") == 0) {
        return handleGetRequest(path, version);
    } else if (strcmp(method, "POST") == 0) {
        char *body = strstr(buffer, "\r\n\r\n");
        return handlePostRequest(path, version, body);
    } else {
        return createErrorResponse(version, HTTP_501);
    }
}

char *handleGetRequest(char *file_name, char *version) {
    int fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        return createErrorResponse(version, HTTP_404);
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return createErrorResponse(version, HTTP_500);
    }

    size_t file_size = st.st_size;
    char *file_content = (char *)malloc(file_size);
    if (!file_content) {
        close(fd);
        return createErrorResponse(version, HTTP_500);
    }

    if (read(fd, file_content, file_size) != (ssize_t)file_size) {
        free(file_content);
        close(fd);
        return createErrorResponse(version, HTTP_500);
    }
    close(fd);

    char date[128];
    getCurrentDate(date, sizeof(date));

    char size_string[32];
    if (sizeToString(file_size, size_string, sizeof(file_size)) == -1) {
        return createErrorResponse(version, HTTP_500);
    }

    char content_type[128] = "Content-Type: ";
    getMimeType(file_name, content_type);

    char *response = (char *)malloc(strlen(version) + strlen(HTTP_200) + strlen(date) + 
                     strlen("Content-Length: ") + strlen(size_string) + strlen(content_type) +
                     file_size);
    if (!response) {
        free(file_content);
        return createErrorResponse(version, HTTP_500);
    }

    sprintf(response, "%s %s%sContent-Length: %s\r\n%s\r\n", 
            version, HTTP_200, date, size_string, content_type);
    
    memcpy(response + strlen(response), file_content, file_size);
    free(file_content);

    return response;
}

char *handlePostRequest(char *file_name, char *version, char *body) {
    if (!file_name || !version) {
        return createErrorResponse(version, HTTP_500);
    }

    int fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        return createErrorResponse(version, HTTP_500);
    }

    if (body) {
        body += 4;
    } else {
        close(fd);
        return createErrorResponse(version, HTTP_204);
    }

    size_t body_len = strlen(body);
    if (body_len > 0 && write(fd, body, body_len) != (ssize_t)body_len) {
        close(fd);
        return createErrorResponse(version, HTTP_500);
    }
    close(fd);

    char date[128];
    getCurrentDate(date, sizeof(date));

    char content_type[128] = "Content-Type: ";
    getMimeType(file_name, content_type);

    char content_length[64];
    snprintf(content_length, sizeof(content_length), "Content-Length: %zu\r\n", body_len);

    char location[512 + 16];
    snprintf(location, sizeof(location), "Location: %s\r\n", file_name);

    size_t version_len = strlen(version);
    size_t date_len = strlen(date);
    size_t content_length_len = strlen(content_length);
    size_t content_type_len = strlen(content_type);
    size_t location_len = strlen(location);
    size_t response_len = version_len + 1 + strlen(HTTP_200) +
                         date_len + content_length_len +
                         content_type_len + location_len + 2 + body_len;

    char *response = (char *)malloc(response_len + 1);
    if (!response) {
        return createErrorResponse(version, HTTP_500);
    }

    int n = snprintf(response, response_len + 1, "%s %s%s%s%s%s\r\n",
                     version, HTTP_200, date, content_length, content_type, location);

    if (body && body_len > 0) {
        memcpy(response + n, body, body_len);
        response[n + body_len] = '\0';
    } else {
        response[n] = '\0';
    }

    return response;
}

char *createErrorResponse(char *version, char *status) {
    if (!version || !status) {
        return NULL;
    }

    const char *body_template = 
        "<html><head><title>Error</title></head>"
        "<body><h1>%s</h1></body></html>";
    
    char body[512];
    int body_len = snprintf(body, sizeof(body), body_template, status);
    if (body_len < 0 || body_len >= (int)sizeof(body)) {
        return NULL;
    }

    char date[128] = {0};
    getCurrentDate(date, sizeof(date));

    size_t response_len = snprintf(NULL, 0,
        "%s %s\r\n"
        "%s"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        version, status,
        date,
        body_len,
        body);

    char *response = malloc(response_len + 1);
    if (!response) {
        return NULL;
    }

    snprintf(response, response_len + 1,
        "%s %s\r\n"
        "%s"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        version, status,
        date,
        body_len,
        body);

    return response;
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

void getMimeType(char *file_name, char *content_type) {
    if (strstr(file_name, ".html")) strcat(content_type, "text/html");
    else if (strstr(file_name, ".css")) strcat(content_type, "text/css");
    else if (strstr(file_name, ".json")) strcat(content_type, "application/json");
    else if (strstr(file_name, ".js")) strcat(content_type, "application/javascript");
    else if (strstr(file_name, ".png")) strcat(content_type, "image/png");
    else if (strstr(file_name, ".jpg")) strcat(content_type, "image/jpeg");
    else strcat(content_type, "application/octet-stream");

    strcat(content_type, "\r\n");
}

int sizeToString(size_t n, char *string, size_t string_size) {
    if (n == 0) {
        string[0] = '0';
        string[1] = '\0';

        return 0;
    }

    size_t digits = 0;
    size_t temp = n;
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