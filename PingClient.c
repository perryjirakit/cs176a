#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>

#define NUM_PINGS 10
#define TIMEOUT_SEC 1
#define PAYLOAD_SIZE 100

int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char *host = argv[1];
    int port = atoi(argv[2]);

    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;

    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "ERROR: no such host as %s\n", host);
        exit(EXIT_FAILURE);
    }

    // 1. Create UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }

    // 2. Set socket timeout
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("ERROR setting socket timeout");
        exit(EXIT_FAILURE);
    }

    //3. Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    server_addr.sin_port = htons(port);
    
    char send_buf[PAYLOAD_SIZE];
    char recv_buf[PAYLOAD_SIZE];
    struct timeval time_sent, time_recv;
    
   
    int transmitted = 0;
    int received = 0;
    double rtt_sum = 0.0;
    double rtt_min = 1e9;
    double rtt_max = 0.0;


    // loop
    for (int seq = 1; seq <= NUM_PINGS; seq++) {
        
        // Get current time for timestamp
        gettimeofday(&time_sent, NULL);
        long long time_ms = time_sent.tv_sec * 1000LL + time_sent.tv_usec / 1000;
        
        // Format the message
        sprintf(send_buf, "PING %d %lld", seq, time_ms);
        
        if (sendto(sockfd, send_buf, strlen(send_buf), 0, 
                   (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("ERROR on sendto");
            continue;
        }
        transmitted++;

        // Wait for reply
        socklen_t server_len = sizeof(server_addr);
        if (recvfrom(sockfd, recv_buf, PAYLOAD_SIZE, 0, 
                     (struct sockaddr*)&server_addr, &server_len) < 0) {
            
            // Check if it was a timeout
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("Request timeout for seq#=%d\n", seq);
            } else {
                perror("ERROR on recvfrom");
            }
        } else {
            // Packet received
            gettimeofday(&time_recv, NULL);
            received++;
            
            // Calculate RTT
            long long sent_us = time_sent.tv_sec * 1000000 + time_sent.tv_usec;
            long long recv_us = time_recv.tv_sec * 1000000 + time_recv.tv_usec;
            double rtt = (double)(recv_us - sent_us) / 1000.0;
            
            rtt_sum += rtt;
            if (rtt < rtt_min) rtt_min = rtt;
            if (rtt > rtt_max) rtt_max = rtt;

            printf("PING received from %s: seq#=%d time=%.3f ms\n", 
                   inet_ntoa(server_addr.sin_addr), seq, rtt);
        }
        sleep(1); 
    }

    close(sockfd);
    printf("--- %s ping statistics ---\n", host);

    double loss_percent = 0.0;
    if (transmitted > 0) {
        loss_percent = 100.0 * (double)(transmitted - received) / transmitted;
    }
    
    double rtt_avg = (received == 0) ? 0.0 : rtt_sum / received;
    
    // Handle case where no packets are received
    if (received == 0) {
        rtt_min = 0.0;
    }
    /*
    printf("%d packets transmitted, %d received, %.0f%% packet loss\n", 
           transmitted, received, loss_percent);
    printf("rtt min/avg/max = %.3f %.3f %.3f ms\n", 
           rtt_min, rtt_avg, rtt_max); // [cite: 347, 358]
    */
    printf("%d packets transmitted, %d received, %.0f%% packet loss", 
           transmitted, received, loss_percent);
    if (received > 0) {
        printf(" rtt min/avg/max = %.3f %.3f %.3f ms\n", 
               rtt_min, rtt_avg, rtt_max);
    } else {
        // If no packets received, just end the line.
        printf("\n");
    }

    return 0;
}
