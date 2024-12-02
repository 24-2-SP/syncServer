#include "../include/main.h"

// SIGCHLD 시그널 핸들러
void handle_sigchld(int sig)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
        // 종료된 자식 프로세스 회수
    }
}
// MIME 타입 결정 함수: 파일 이름을 기반으로 MIME 타입 반환
const char *type(const char *fname)
{
    const char *ext = strrchr(fname, '.');
    if (!ext || ext == fname)
        return "application/octet-stream";

    if (strcmp(ext, ".json") == 0)
        return "application/json";
    if (strcmp(ext, ".png") == 0)
        return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(ext, ".txt") == 0)
        return "text/plain";
    if (strcmp(ext, ".html") == 0)
        return "text/html";

    return "application/octet-stream";
}
