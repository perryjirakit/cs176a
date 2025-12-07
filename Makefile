CC = gcc
CFLAGS = -Wall -pthread -g
TARGETS = hangman_server hangman_client

all: $(TARGETS)

hangman_server: hangman_server.c
	$(CC) $(CFLAGS) -o hangman_server hangman_server.c

hangman_client: hangman_client.c
	$(CC) $(CFLAGS) -o hangman_client hangman_client.c

clean:
	rm -f $(TARGETS)

.PHONY: all clean