#include "../include/main.h"

// 클라이언트 요청 처리
void request(int cfd)
{
    char buf[BUFFER_SIZE];
    int bytes = read(cfd, buf, sizeof(buf) - 1);

    if (bytes <= 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            perror("read failed");
        close(cfd);
        return;
    }
    buf[bytes] = '\0'; // 문자열 종료

    // 요청 파싱
    char method[16] = {0}, path[256] = {0}, protocol[16] = {0};
    if (sscanf(buf, "%15s %255s %15s", method, path, protocol) != 3)
    {
        fprintf(stderr, "Invalid request format\n");
        const char *body = "<html><body><h1>400 Bad Request</h1></body></html>";
        response(cfd, 400, "Bad Request", "text/html", body);
        close(cfd);
        return;
    }

    printf("Client request:\nMethod: %s, Path: %s, Protocol: %s\n", method, path, protocol);

    // 요청 URL 유효성 검증
    if (strlen(path) < 2 || path[0] != '/')
    {
        fprintf(stderr, "Invalid path\n");
        const char *body = "<html><body><h1>400 Bad Request</h1></body></html>";
        response(cfd, 400, "Bad Request", "text/html", body);
        close(cfd);
        return;
    }

    // 요청 메소드 처리
    if (strcmp(method, "GET") == 0)
    {
        get(cfd, path + 1); // `get` 함수 호출 (path 앞의 '/' 제거)
    }
    else if (strcmp(method, "HEAD") == 0)
    {
        head(cfd, path + 1); // `head` 함수 호출 (path 앞의 '/' 제거)
    }
    else
    {
        fprintf(stderr, "Unsupported method: %s\n", method);
        const char *body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        response(cfd, 405, "Method Not Allowed", "text/html", body);
    }

    // 소켓 닫기
    close(cfd);
}
// 파일 전송
void get(int cfd, const char *fname)
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
     char header[BUFFER_SIZE];
    int header_len = snprintf(header, sizeof(header),
                              "HTTP/1.1 %d %s\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %ld\r\n"
                              "\r\n",
                              status, statusM, types, body ? strlen(body) : 0);

    // 헤더 전송
    if (write(cfd, header, header_len) == -1)
    {
        perror("Failed to send HTTP response header");
        close(cfd);
        return;
    }

    // 본문 전송 (HEAD 요청의 경우 본문 생략)
    if (body && write(cfd, body, strlen(body)) == -1)
    {
        perror("Failed to send HTTP response body");
    }
}

void head(int cfd, const char *fname)
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
