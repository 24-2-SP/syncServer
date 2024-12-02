#include "./include/main.h"

int main()
{
    // SIGCHLD 시그널 핸들러 설정
    struct sigaction sa;
    sa.sa_handler = sigchld;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    // 서버 초기화
    int sfd = init();
    struct sockaddr_in cli;
    socklen_t clen = sizeof(cli);

    while (1)
    {
        // 클라이언트 연결 수락
        int cfd = accept(sfd, (struct sockaddr *)&cli, &clen);
        if (cfd == -1)
        {
            perror("accept failed");
            continue;
        }
        printf("new client : %d\n", cfd);

        // 자식 프로세스 생성
        pid_t pid = fork();
        if (pid == 0)
        {
            // 자식 프로세스에서 클라이언트 요청 처리
            close(sfd);   // 부모 서버 소켓 닫기
            request(cfd); // 클라이언트 요청 처리
            exit(0);
        }
        else if (pid > 0)
        {
            // 부모 프로세스
            close(cfd); // 클라이언트 소켓 닫기
        }
    }
    close(sfd);
    return 0;
}
