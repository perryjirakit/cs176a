#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_BUF 256

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("socket");
        exit(1);
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0){
        perror("inet_pton");
        close(sockfd);
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        perror("connect");
        close(sockfd);
        exit(1);
    }
    // client input
    char sendbuf[MAX_BUF];
    printf("Enter string: ");
    fflush(stdout);
    if (!fgets(sendbuf, sizeof(sendbuf), stdin)) {
        close(sockfd);
        return 0;
    }

    ssize_t sent = send(sockfd, sendbuf, strlen(sendbuf), 0);
    if (sent < 0) {
        perror("send");
        close(sockfd);
        exit(1);
    }

    // read everything server sends
    char recvbuf[MAX_BUF];
    ssize_t n;
    char linebuf[MAX_BUF * 2];
    size_t line_len = 0;
    memset(linebuf, 0, sizeof(linebuf));

    while ((n = recv(sockfd, recvbuf, sizeof(recvbuf)-1, 0)) > 0) {
        recvbuf[n] = '\0';
        for (ssize_t i = 0; i < n; i++) {
            char c = recvbuf[i];
            if (c == '\n') {
                linebuf[line_len] = '\0';
                if (line_len > 0) {
                    printf("From server: %s\n", linebuf);
                } else {
                    // empty line -> ignore(maybe lets test)
                }
                line_len = 0;
                memset(linebuf, 0, sizeof(linebuf));
            } else {
                if (line_len < sizeof(linebuf)-1) {
                    linebuf[line_len++] = c;
                }
            }
        }
    }

    if (line_len > 0) {
        linebuf[line_len] = '\0';
        printf("From server: %s\n", linebuf);
    }

    close(sockfd);
    return 0;
}
