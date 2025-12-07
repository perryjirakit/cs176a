#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define BUFFER_SIZE 256

void print_game_state(unsigned char *buffer) {
    int word_len = buffer[1];
    int num_incorrect = buffer[2];
    
    // Print word with spaces
    printf(">>>");
    for (int i = 0; i < word_len; i++) {
        printf("%c", buffer[3 + i]);
        if (i < word_len - 1) {
            printf(" ");
        }
    }
    printf("\n");
    
    // Print incorrect guesses
    printf(">>>Incorrect Guesses:");
    if (num_incorrect > 0) {
        printf(" ");
        for (int i = 0; i < num_incorrect; i++) {
            printf("%c", buffer[3 + word_len + i]);
            if (i < num_incorrect - 1) {
                printf(",");
            }
        }
    }
    printf("\n");
    printf(">>>\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }
    
    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    
    // Create socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Connect to server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(client_socket);
        exit(1);
    }
    
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        exit(1);
    }
    
    // Ask if ready to start
    char ready[10];
    printf(">>>Ready to start game? (y/n): ");
    fflush(stdout);
    
    if (fgets(ready, sizeof(ready), stdin) == NULL) {
        close(client_socket);
        exit(1);
    }
    
    // Remove newline
    ready[strcspn(ready, "\n")] = 0;
    
    if (ready[0] != 'y' && ready[0] != 'Y') {
        close(client_socket);
        exit(0);
    }
    
    // Send empty message to signal game start
    unsigned char start_msg[1] = {0};
    send(client_socket, start_msg, 1, 0);
    
    // Game loop
    unsigned char buffer[BUFFER_SIZE];
    int game_over = 0;
    
    while (!game_over) {
        int bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);
        
        if (bytes_read <= 0) {
            break;
        }
        
        unsigned char msg_flag = buffer[0];
        
        if (msg_flag > 0) {
            // Message packet
            char message[BUFFER_SIZE];
            memcpy(message, buffer + 1, msg_flag);
            message[msg_flag] = '\0';
            printf(">>>%s\n", message);
            
            // Check if game over
            if (strstr(message, "Game Over") != NULL) {
                game_over = 1;
            }
        } else {
            // Game control packet
            print_game_state(buffer);
            fd_set readfds;
            struct timeval tv;
            FD_ZERO(&readfds);
            FD_SET(client_socket, &readfds);
            tv.tv_sec = 0;
            tv.tv_usec = 50000; // 50ms timeout
            
            int ready = select(client_socket + 1, &readfds, NULL, NULL, &tv);
            
            if (ready > 0) {
                // Server has more data (likely win/lose messages), loop to receive them
                continue;
            }
            
            // No more immediate messages, so ask for guess
            if (!game_over) {
                // Get guess from user
                char guess[10];
                int valid = 0;
                
                while (!valid) {
                    printf(">>>Letter to guess: ");
                    fflush(stdout);
                    
                    if (fgets(guess, sizeof(guess), stdin) == NULL) {
                        close(client_socket);
                        exit(1);
                    }
                    
                    // Remove newline
                    guess[strcspn(guess, "\n")] = 0;
                    
                    // Validate input
                    if (strlen(guess) != 1 || !isalpha(guess[0])) {
                        printf(">>>Error! Please guess one letter.\n");
                    } else {
                        valid = 1;
                    }
                }
                
                // Send guess to server
                unsigned char msg[2];
                msg[0] = 1; // msg length
                msg[1] = tolower(guess[0]);
                send(client_socket, msg, 2, 0);
            }
        }
    }
    
    close(client_socket);
    return 0;
}