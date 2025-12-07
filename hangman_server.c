#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>

#define MAX_CLIENTS 3
#define MAX_WORDS 15
#define MAX_WORD_LENGTH 8
#define MIN_WORD_LENGTH 3
#define MAX_INCORRECT 6
#define BUFFER_SIZE 256

typedef struct {
    char word[MAX_WORD_LENGTH + 1];
    char guessed[MAX_WORD_LENGTH + 1];
    char incorrect[MAX_INCORRECT + 1];
    int num_incorrect;
    int client_socket;
    int active;
} GameState;

char words[MAX_WORDS][MAX_WORD_LENGTH + 1];
int word_count = 0;
GameState games[MAX_CLIENTS];
pthread_mutex_t games_mutex = PTHREAD_MUTEX_INITIALIZER;

void send_message(int client_socket, const char *message) {
    unsigned char buffer[BUFFER_SIZE];
    int msg_len = strlen(message);
    buffer[0] = (unsigned char)msg_len;
    memcpy(buffer + 1, message, msg_len);
    send(client_socket, buffer, msg_len + 1, 0);
}

void send_game_state(GameState *game) {
    unsigned char buffer[BUFFER_SIZE];
    int word_len = strlen(game->word);
    
    buffer[0] = 0; // msg flag = 0 for game control packet
    buffer[1] = (unsigned char)word_len;
    buffer[2] = (unsigned char)game->num_incorrect;
    
    // Add guessed word
    for (int i = 0; i < word_len; i++) {
        buffer[3 + i] = game->guessed[i];
    }
    
    // Add incorrect guesses
    for (int i = 0; i < game->num_incorrect; i++) {
        buffer[3 + word_len + i] = game->incorrect[i];
    }
    
    int total_len = 3 + word_len + game->num_incorrect;
    send(game->client_socket, buffer, total_len, 0);
}

int load_words() {
    FILE *file = fopen("hangman_words.txt", "r");
    if (!file) {
        perror("Error opening hangman_words.txt");
        return 0;
    }
    
    word_count = 0;
    char line[MAX_WORD_LENGTH + 2];
    while (fgets(line, sizeof(line), file) && word_count < MAX_WORDS) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        int len = strlen(line);
        if (len >= MIN_WORD_LENGTH && len <= MAX_WORD_LENGTH) {
            // Convert to lowercase
            for (int i = 0; i < len; i++) {
                line[i] = tolower(line[i]);
            }
            strcpy(words[word_count], line);
            word_count++;
        }
    }
    
    fclose(file);
    return word_count;
}

void init_game(GameState *game, int client_socket) {
    game->client_socket = client_socket;
    game->num_incorrect = 0;
    game->active = 1;
    
    // Choose random word
    int word_index = rand() % word_count;
    strcpy(game->word, words[word_index]);
    
    // Initialize guessed with underscores
    int word_len = strlen(game->word);
    for (int i = 0; i < word_len; i++) {
        game->guessed[i] = '_';
    }
    game->guessed[word_len] = '\0';
    
    // Clear incorrect guesses
    memset(game->incorrect, 0, sizeof(game->incorrect));
}

int is_game_won(GameState *game) {
    return strcmp(game->word, game->guessed) == 0;
}

int is_game_lost(GameState *game) {
    return game->num_incorrect >= MAX_INCORRECT;
}

void process_guess(GameState *game, char guess) {
    guess = tolower(guess);
    int word_len = strlen(game->word);
    int found = 0;
    
    // Check if letter is in word
    for (int i = 0; i < word_len; i++) {
        if (game->word[i] == guess) {
            game->guessed[i] = guess;
            found = 1;
        }
    }
    
    // If not found, add to incorrect guesses
    if (!found) {
        // Check if already guessed
        int already_guessed = 0;
        for (int i = 0; i < game->num_incorrect; i++) {
            if (game->incorrect[i] == guess) {
                already_guessed = 1;
                break;
            }
        }
        
        if (!already_guessed && game->num_incorrect < MAX_INCORRECT) {
            game->incorrect[game->num_incorrect] = guess;
            game->num_incorrect++;
            game->incorrect[game->num_incorrect] = '\0';
        }
    }
}

void *handle_client(void *arg) {
    GameState *game = (GameState *)arg;
    unsigned char buffer[BUFFER_SIZE];
    int bytes_read;
    
    // Wait for game start signal
    bytes_read = recv(game->client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_read <= 0) {
        close(game->client_socket);
        game->active = 0;
        return NULL;
    }
    
    // Send initial game state
    send_game_state(game);
    
    // Game loop
    while (game->active) {
        bytes_read = recv(game->client_socket, buffer, BUFFER_SIZE, 0);
        
        if (bytes_read <= 0) {
            break;
        }
        
        int msg_len = buffer[0];
        if (msg_len > 0 && msg_len <= bytes_read - 1) {
            char guess = buffer[1];
            process_guess(game, guess);

            // Always send the updated game state first
            int game_won = is_game_won(game);
            int game_lost = is_game_lost(game);
            
            if (game_won || game_lost) {
                // Show the complete word
                for (int i = 0; i < strlen(game->word); i++) {
                    game->guessed[i] = game->word[i];
                }
            }
            
            // Send current game state (with full word if game ended)
            send_game_state(game);
            if (game_won) {
                send_message(game->client_socket, "You Win!");
                send_message(game->client_socket, "Game Over!");
                break;  // End the game loop
            } else if (game_lost) {
                send_message(game->client_socket, "You Lose.");
                send_message(game->client_socket, "Game Over!");
                break;  // End the game loop
            }
        }
    }
    
    close(game->client_socket);
    game->active = 0;
    
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    
    int port = atoi(argv[1]);
    
    // Seed random number generator
    srand(time(NULL));
    
    // Load words
    if (load_words() == 0) {
        fprintf(stderr, "Failed to load words from hangman_words.txt\n");
        exit(1);
    }
    
    // Initialize games
    for (int i = 0; i < MAX_CLIENTS; i++) {
        games[i].active = 0;
    }
    
    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(1);
    }
    
    // Set socket options
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(1);
    }
    
    // Listen
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(1);
    }
    
    printf("Server listening on port %d\n", port);
    
    // Accept connections
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_socket < 0) {
            perror("Accept failed");
            continue;
        }
        
        // Find available game slot
        pthread_mutex_lock(&games_mutex);
        int slot = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!games[i].active) {
                slot = i;
                break;
            }
        }
        
        if (slot == -1) {
            // Server overloaded
            send_message(client_socket, "Server Overloaded");
            close(client_socket);
            pthread_mutex_unlock(&games_mutex);
        } else {
            init_game(&games[slot], client_socket);
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, &games[slot]);
            pthread_detach(thread);
            pthread_mutex_unlock(&games_mutex);
        }
    }
    
    close(server_socket);
    return 0;
}