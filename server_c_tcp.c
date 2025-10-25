#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_BUF 256

int is_all_digits(const char *s) {
    for (int i = 0; s[i] != '\0'; i++) {
        if (!isdigit((unsigned char)s[i])) {
            if (s[i] == '\n' || s[i] == '\r') continue;
            return 0;
        }
    }
    return 1;
}

int sum_digits_str(const char *s) {
    int sum = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (isdigit((unsigned char)s[i])) {
            sum += (s[i] - '0');
        }
    }
    return sum;
}

int sum_digits_int(int n) {
    int sum = 0;
    if (n == 0) return 0;
    while (n > 0) {
        sum += (n % 10);
        n /= 10;
    }
    return sum;
}

void send_line(int fd, const char *msg) {
    size_t len = strlen(msg);
    send(fd, msg, len, 0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        close(listenfd);
        exit(1);
    }

    if (listen(listenfd, 5) < 0) {
        perror("listen");
        close(listenfd);
        exit(1);
    }

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        int connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd < 0) {
            continue;
        }

        char buf[MAX_BUF];
        memset(buf, 0, sizeof(buf));
        ssize_t n = recv(connfd, buf, sizeof(buf)-1, 0);
        if (n <= 0) {
            close(connfd);
            continue;
        }
        buf[n] = '\0';

        if (!is_all_digits(buf)) {
            const char *sorry = "Sorry, cannot compute!";
            send_line(connfd, sorry);
            close(connfd);
            continue;
        }

        int sum = sum_digits_str(buf);

        while (1) {
            char out[64];
            snprintf(out, sizeof(out), "%d", sum);

            send_line(connfd, out);
            send(connfd, "\n", 1, 0);

            if (sum >= 0 && sum <= 9) {
                break;
            }
            sum = sum_digits_int(sum);
        }

        close(connfd);
    }

    close(listenfd);
    return 0;
}
