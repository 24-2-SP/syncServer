
#include "../include/main.h"

// 서버 초기화 함수
int init()
{
    int sfd;
    struct sockaddr_in sin;

    // 소켓 생성 SOCK_STREAM : tcp 타입
    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket failed");
        exit(1);
    }

    // 소켓 옵션 설정
    int opt = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt failed");
        exit(1);
    }

    // 서버 구조체 초기화
    memset((char *)&sin, '\0', sizeof(sin));
    sin.sin_family = AF_INET;                   // IPv4 사용
    sin.sin_port = htons(PORT);                 // 포트 번호 설정
    sin.sin_addr.s_addr = inet_addr("0.0.0.0"); // 모든 네트워크에서 연결 허용

    // 소켓을 주소와 바인딩
    if (bind(sfd, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    {
        perror("bind failed");
        exit(1);
    }

    // 클라이언트 요청 대기 (최대 10000개) (대기열 크기)
    if (listen(sfd, 10000) == -1)
    {
        perror("listen failed");
        close(sfd);
        exit(1);
    }

    printf("server on port %d is running\n", PORT);
    return sfd;
}
