#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_BUF 256

// returns 1 if all chars are digits 0-9, 0 otherwise
int is_all_digits(const char *s) {
    for (int i = 0; s[i] != '\0'; i++) {
        if (!isdigit((unsigned char)s[i])) {
            if (s[i] == '\n' || s[i] == '\r') continue;
            return 0;
        }
    }
    return 1;
}

// sum digits of str s, returns the integer sum
int sum_digits_str(const char *s) {
    int sum = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (isdigit((unsigned char)s[i])) {
            sum += (s[i] - '0');
        }
    }
    return sum;
}

// sum digits of an integer n, returns the integer sum.
int sum_digits_int(int n) {
    int sum = 0;
    if (n == 0) return 0;
    while (n > 0) {
        sum += (n % 10);
        n /= 10;
    }
    return sum;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t cli_len;
    char buf[MAX_BUF];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(1);
    }
    //loop
    for (;;) {
        cli_len = sizeof(cliaddr);
        memset(buf, 0, sizeof(buf));

        ssize_t n = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                             (struct sockaddr *)&cliaddr, &cli_len);
        if (n < 0) {
            continue;
        }
        buf[n] = '\0';
        // check if all digits
        if (!is_all_digits(buf)) {
            const char *sorry = "Sorry, cannot compute!";
            sendto(sockfd, sorry, strlen(sorry), 0,
                   (struct sockaddr *)&cliaddr, cli_len);
            continue;
        }
        // case number
        int sum = sum_digits_str(buf);
        // loop till single digit
        while (1) {
            char out[64];
            snprintf(out, sizeof(out), "%d", sum);

            sendto(sockfd, out, strlen(out), 0,
                   (struct sockaddr *)&cliaddr, cli_len);

            if (sum >= 0 && sum <= 9) {
                // single digit, done
                break;
            }
            sum = sum_digits_int(sum);
        }
    }

    close(sockfd);
    return 0;
}
