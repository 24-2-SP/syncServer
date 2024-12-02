#include "../include/main.h"

// 클라이언트 요청 처리
void handle_req(int cfd)
{
    char buf[BUFFER_SIZE];
    int bytes = read(cfd, buf, sizeof(buf) - 1);

    if (bytes <= 0)
    {
        perror("read failed");
        close(cfd);
        return;
    }
    buf[bytes] = '\0';

    // 요청 파싱
    char method[16], path[256], protocol[16];
    sscanf(buf, "%s %s %s", method, path, protocol);
    printf("Client request:\n");
    printf("Method: %s, Path: %s, Protocol: %s\n", method, path, protocol);

    // 요청 메소드 GET인 경우 고려
    if (strcmp(method, "GET") == 0)
    {
        handle_get(cfd, path + 1); // GET 요청 처리
    }
    else if (strcmp(method, "HEAD") == 0)
    {
        handle_head(cfd, path + 1); // HEAD 요청 처리
    }
    else
    {
        const char *body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        response(cfd, 405, "Method Not Allowed", "text/html", body);
    }
    close(cfd);

}

//파일 전송
void handle_get(int cfd, const char *fname)
{
    char path[256];
    snprintf(path, sizeof(path), "file/%s", fname);

    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        response(cfd, 404, "Not Found", "text/plain", "File not found");
        return;
    }

    const char *mime_type = type(path);
    if (strncmp(mime_type, "image/", 6) == 0 || strncmp(mime_type, "application/", 12) == 0)
    {
        // HTTP 응답 헤더 전송
        char header[512];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Transfer-Encoding: chunked\r\n"
                 "\r\n",
                 mime_type);
        if (write(cfd, header, strlen(header)) == -1)
        {
            perror("Failed to send header");
            fclose(fp);
            close(cfd);
            return;
        }

        // 청크 단위로 파일 읽기 및 전송
        char buffer[CHUNK_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0)
        {
            // 청크 크기 전송
            char chunk_header[32];
            int header_len = snprintf(chunk_header, sizeof(chunk_header), "%zx\r\n", bytes_read);
            if (write(cfd, chunk_header, header_len) == -1)
            {
                perror("Failed to send chunk header");
                break;
            }

            // 실제 데이터 전송
            if (write(cfd, buffer, bytes_read) == -1)
            {
                perror("Failed to send chunk data");
                break;
            }

            // 청크 종료
            if (write(cfd, "\r\n", 2) == -1)
            {
                perror("Failed to send chunk end");
                break;
            }
        }

        // 마지막 청크(종료 청크) 전송
        if (write(cfd, "0\r\n\r\n", 5) == -1)
        {
            perror("Failed to send final chunk");
        }
    }
    else
    {
        // 일반 파일 처리 (전체 전송)
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        char *buf = (char *)malloc(file_size);
        if (!buf)
        {
            perror("Memory allocation failed");
            fclose(fp);
            response(cfd, 500, "Internal Server Error", "text/plain", "Unable to allocate memory");
            return;
        }

        fread(buf, 1, file_size, fp);
        fclose(fp);

        // HTTP 헤더 전송
        char header[512];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %ld\r\n"
                 "\r\n",
                 mime_type, file_size);
        write(cfd, header, strlen(header));

        // 파일 데이터 전송
        write(cfd, buf, file_size);
        free(buf);
    }

    fclose(fp);
}

// http 응답 -> 클라이언트로 전송
void response(int cfd, int status, const char *statusM, const char *types, const char *body)
{
    char *buf = (char *)malloc(BUFFER_SIZE);
    if (!buf)
    {
        perror("malloc failed");
        close(cfd);
        return;
    }
    int needed_size = snprintf(buf, BUFFER_SIZE,
                               "HTTP/1.1 %d %s\r\n"
                               "Content-Type: %s\r\n"
                               "Content-Length: %ld\r\n"
                               "\r\n"
                               "%s",
                               status, statusM, types, strlen(body), body);

    // 반환된 크기가 현재 버퍼 크기를 초과하는 경우
    if (needed_size >= BUFFER_SIZE)
    {
        size_t size = needed_size + 1; // null-terminator 포함
        buf = (char *)realloc(buf, size);
        if (!buf)
        {
            perror("realloc failed");
            close(cfd);
            return;
        }

        // 다시 작성
        snprintf(buf, size,
                 "HTTP/1.1 %d %s\r\n"
                 "Content-Type: %s\r\n"
                 "Content-Length: %ld\r\n"
                 "\r\n"
                 "%s",
                 status, statusM, types, strlen(body), body);
    }

    // 클라이언트로 데이터 전송
    write(cfd, buf, strlen(buf));
    free(buf);
}

void handle_head(int cfd, const char *fname)
{
    char path[100];
    snprintf(path, sizeof(path), "file/%s", fname);

    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        response(cfd, 404, "Not Found", "text/plain", NULL);
        return;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fclose(fp);

    const char *mime_type = type(path);

    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "\r\n",
             mime_type, file_size);

    write(cfd, header, strlen(header));
}
