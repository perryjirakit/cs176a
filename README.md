# CS 176A - Homework 3: UDP Ping Client

by Perry Thampiratwong



## Program Documentation



### Implementation Details

The program's logic is structured as follows:

1.  **Initialization:**
    * Parses command-line arguments (`host`, `port`), resolves the hostname to an IP address using `gethostbyname()`, and creates a UDP socket (`SOCK_DGRAM`)

2.  **Socket Timeout:**
    * Sets one second timeout (`SO_RCVTIMEO`) on the socket using `setsockopt()`. This prevents `recvfrom()` from blocking indefinitely and is used to detect packet loss.

3.  **Main Ping Loop:**
    Loops 10 times (once per sequence number). In each loop, the client:
    * Formats a `PING <seq_num> <timestamp>` message.
    * Records the start time and sends the packet with `sendto()`.
    * Calls `recvfrom()` to wait for a reply.
    * **On Timeout:** Prints a "Request timeout" message.
    * **On Success:** Records the end time, calculates the RTT, updates statistics (min, max, avg), and prints the "PING received..." message.
    * Calls `sleep(1)` to wait before sending the next ping


---

## How to Compile and Run

1.  **Compile:**
    Run the `make` command in the terminal.
    ```bash
    make
    ```

2.  **Run the Server:**
    In one terminal window, start the Python server.
    ```bash
    python UDPPingerServer.py
    ```

3.  **Run the Client:**
    In a second terminal window, run the compiled client, pointing it at the server's host (e.g., `localhost`) and port (12000).
    ```bash
    ./PingClient localhost 12000
    ```