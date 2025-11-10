/*
 * PingClient.c
 *
 * This program implements a UDP Ping client as specified in CS 176A Homework #3.
 * It sends 10 ping messages to a server and calculates RTT and packet loss.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>      // For gethostbyname
#include <sys/time.h>   // For gettimeofday, struct timeval
#include <errno.h>      // For errno

#define NUM_PINGS 10    // Number of pings to send [cite: 306]
#define TIMEOUT_SEC 1   // Timeout value in seconds [cite: 308]
#define PAYLOAD_SIZE 100

int main(int argc, char *argv[]) {
    
    // Check command line arguments [cite: 311]
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char *host = argv[1];
    int port = atoi(argv[2]);

    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;

    // --- 1. Resolve host name ---
    server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "ERROR: no such host as %s\n", host);
        exit(EXIT_FAILURE);
    }

    // --- 2. Create UDP socket ---
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }

    // --- 3. Set socket timeout --- [cite: 316]
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("ERROR setting socket timeout");
        exit(EXIT_FAILURE);
    }

    // --- 4. Set up server address struct ---
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    server_addr.sin_port = htons(port);
    
    char send_buf[PAYLOAD_SIZE];
    char recv_buf[PAYLOAD_SIZE];
    struct timeval time_sent, time_recv;
    
    // Statistics variables
    int transmitted = 0;
    int received = 0;
    double rtt_sum = 0.0;
    double rtt_min = 1e9; // Init min to a large number
    double rtt_max = 0.0;

    printf("Pinging %s (IP: %s) on port %d:\n", 
           host, inet_ntoa(server_addr.sin_addr), port);

    // --- 5. Ping loop ---
    for (int seq = 1; seq <= NUM_PINGS; seq++) {
        
        // Get current time for timestamp [cite: 325]
        gettimeofday(&time_sent, NULL);
        long long time_ms = time_sent.tv_sec * 1000LL + time_sent.tv_usec / 1000;
        
        // Format the message [cite: 324]
        sprintf(send_buf, "PING %d %lld", seq, time_ms);
        
        // Send PING
        if (sendto(sockfd, send_buf, strlen(send_buf), 0, 
                   (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("ERROR on sendto");
            continue; // Don't count this as a transmitted packet
        }
        transmitted++;

        // Wait for reply
        socklen_t server_len = sizeof(server_addr);
        if (recvfrom(sockfd, recv_buf, PAYLOAD_SIZE, 0, 
                     (struct sockaddr*)&server_addr, &server_len) < 0) {
            
            // Check if it was a timeout
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                printf("Request timeout for seq#=%d\n", seq); // [cite: 342, 349]
            } else {
                perror("ERROR on recvfrom");
            }
        } else {
            // Packet received
            gettimeofday(&time_recv, NULL);
            received++;
            
            // Calculate RTT in milliseconds
            long long sent_us = time_sent.tv_sec * 1000000 + time_sent.tv_usec;
            long long recv_us = time_recv.tv_sec * 1000000 + time_recv.tv_usec;
            double rtt = (double)(recv_us - sent_us) / 1000.0;
            
            // Update stats
            rtt_sum += rtt;
            if (rtt < rtt_min) rtt_min = rtt;
            if (rtt > rtt_max) rtt_max = rtt;

            // Print success message [cite: 340, 350]
            printf("PING received from %s: seq#=%d time=%.3f ms\n", 
                   inet_ntoa(server_addr.sin_addr), seq, rtt);
        }

        // Wait ~1 second before next ping [cite: 306]
        // This makes the pings "separated by approximately one second"
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

    printf("%d packets transmitted, %d received, %.0f%% packet loss\n", 
           transmitted, received, loss_percent);
    printf("rtt min/avg/max = %.3f %.3f %.3f ms\n", 
           rtt_min, rtt_avg, rtt_max); // [cite: 347, 358]

    return 0;
}
