#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <arpa/inet.h>


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_address> <server_port>\n", argv[0]);
        return 1;
    }

    char *server_address_str = argv[1];
    int server_port = atoi(argv[2]);

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_address_str, &server_address.sin_addr) <= 0) {
        printf("Invalid server address: %s\n", server_address_str);
        return 1;
    }

    int client_socket;
    char buffer[1024];
    fd_set readfds;
    int bytes_received, bytes_sent;

    // Create a socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Error connecting to server");
        return 1;
    }

    // Set the socket to non-blocking mode
    fcntl(client_socket, F_SETFL, O_NONBLOCK);

    // Loop until we receive or send some data
    while (1) {
        // Clear the file descriptor set
        FD_ZERO(&readfds);

        // Add the socket and stdin to the file descriptor set
        FD_SET(client_socket, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        // Wait until either the socket or stdin is ready for reading
        select(client_socket + 1, &readfds, NULL, NULL, NULL);

        // Check if there is data available to be read from the socket
        if (FD_ISSET(client_socket, &readfds)) {
            // Receive the data
            bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

            // Check for errors or the end of the connection
            if (bytes_received <= 0) {
                break;
            }

            // Print the data
            printf("Received %d bytes: %s\n", bytes_received, buffer);
        }

        // Check if there is data available to be read from stdin
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            // Read the data from stdin
            fgets(buffer, sizeof(buffer), stdin);

            // Send the data to the server
            bytes_sent = send(client_socket, buffer, strlen(buffer), 0);

            // Check for errors or the end of the connection
            if (bytes_sent <= 0) {
                break;
            }
        }
    }

    // Close the socket
    close(client_socket);

    return 0;
}
