#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_EVENTS 10000
#define PORT 12345
#define BUFFER_SIZE 512
#define CHUNK_SIZE 65536

// 서버 초기화
int init();

// HTTP 요청을 분석하고 처리
void request(int cfd);

// GET 요청 처리 함수
void get(int cfd, const char *fname);
// HEAD 요청 처리 함수
void head(int cfd, const char *fname);

// HTTP 응답 전송 함수: 상태 코드, 상태 메시지, 컨텐츠 타입, 본문 데이터를 클라이언트로 전송
void response(int cfd, int status, const char *statusM, const char *types, const char *body);

// MIME 타입 결정 함수: 파일 이름을 기반으로 MIME 타입 반환
const char *type(const char *fname);
// SIGCHLD 시그널 핸들러: 종료된 자식 프로세스 회수
void sigchld(int sig);

#endif
