#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_BUF 256

// check if server message is "Sorry, cannot compute!"
int is_error_msg(const char *s) {
    return strncmp(s, "Sorry, cannot compute!", 22) == 0;
}

int is_single_digit_number(const char *s) {
    int len = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] != '\r' && s[i] != '\n') {
            len++;
        }
    }

    int count_digits = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == '\r' || s[i] == '\n') continue;
        if (!isdigit((unsigned char)s[i])) return 0;
        count_digits++;
    }
    return (count_digits == 1 && len == 1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd;
    struct sockaddr_in servaddr;
    socklen_t serv_len = sizeof(servaddr);
    char sendbuf[MAX_BUF];
    char recvbuf[MAX_BUF];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        exit(1);
    }

    printf("Enter string: ");
    fflush(stdout);

    if (!fgets(sendbuf, sizeof(sendbuf), stdin)) {
        close(sockfd);
        return 0;
    }

    ssize_t sent = sendto(sockfd, sendbuf, strlen(sendbuf), 0,
                          (struct sockaddr *)&servaddr, serv_len);
    if (sent < 0) {
        perror("sendto");
        close(sockfd);
        exit(1);
    }
    // loop
    while (1) {
        memset(recvbuf, 0, sizeof(recvbuf));
        ssize_t n = recvfrom(sockfd, recvbuf, sizeof(recvbuf) - 1, 0,
                             NULL, NULL);
        if (n < 0) {
            break;
        }
        recvbuf[n] = '\0';

        printf("From server: %s\n", recvbuf);

        if (is_error_msg(recvbuf) || is_single_digit_number(recvbuf)) {
            break;
        }
    }

    close(sockfd);
    return 0;
}
