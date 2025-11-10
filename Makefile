
# Compiler
CC = gcc

# Compiler flags
# -Wall: Enable all warnings
# -g: Add debugging information
CFLAGS = -Wall -g -std=c99

# Target executable
TARGET = PingClient

# Source file
SOURCES = PingClient.c

# Default rule
all: $(TARGET)

# Rule to build the target
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

# Rule to clean up build files
clean:
	rm -f $(TARGET)