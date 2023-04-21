#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct message_struct {
    char* protocol;
    int message_length;
    char* message;
    int x; // if protocol is MOVE
    int y; // if protocol is MOVE
} message_struct;

message_struct* parse_message(int sock_fd) {
    message_struct* msg = malloc(sizeof(message_struct));
    msg->protocol = NULL;
    msg->message_length = -1;
    msg->message = NULL;

    char buf[1024];
    memset(buf, 0, sizeof(buf));

    int bytes_received = 0;
    int total_bytes_received = 0;
    while (total_bytes_received < sizeof(buf)) {
        bytes_received = recv(sock_fd, buf + total_bytes_received, sizeof(buf) - total_bytes_received, 0);
        if (bytes_received < 0) {
            perror("recv failed");
            return NULL;
        }
        if (bytes_received == 0) {
            break;
        }
        total_bytes_received += bytes_received;

        // Check if we have received a complete message
        char* token = strtok(buf, "|");
        if (token == NULL) {
            continue;
        }
        if (strcmp(token, "PLAY") != 0 && strcmp(token, "MOVE") != 0 &&
            strcmp(token, "RSGN") != 0 && strcmp(token, "DRAW") != 0) {
            fprintf(stderr, "Invalid protocol\n");
            return NULL;
        }
        msg->protocol = strdup(token);

        token = strtok(NULL, "|");
        if (token == NULL) {
            continue;
        }
        int message_length = atoi(token);
        if (message_length <= 0 || message_length > 1024) {
            fprintf(stderr, "Invalid message length\n");
            return NULL;
        }
        msg->message_length = message_length;

        if (total_bytes_received < message_length + strlen(msg->protocol) + strlen(token) + 2) {
            continue;
        }


        msg->message = malloc(message_length);
        memcpy(msg->message, buf + strlen(msg->protocol) + strlen(token) + 2, message_length - 1);
        msg->message[message_length] = '\0';

        if (strcmp(msg->protocol, "MOVE") == 0) {
            int x = 0;
            int y = 0;
            char symb;
            if (sscanf(msg->message, "%c|%d,%d|", &symb, &x, &y) != 3 || x < 1 || x > 9 || y < 1 || y > 9) {
                fprintf(stderr, "Invalid MOVE message\n");
                return NULL;
            }
            msg->x = x;
            msg->y = y;
        } 

        break;
    }

    return msg;
}

// int main(int argc, char* argv[]) {
//     if (argc != 2) {
//         fprintf(stderr, "Usage: %s port\n", argv[0]);
//         exit(EXIT_FAILURE);
//     }
//     int port = atoi(argv[1]);

//     int sock_fd;
//     struct sockaddr_in serv_addr;
//     sock_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (sock_fd < 0) {
//         perror("socket creation failed");
//         exit(EXIT_FAILURE);
//     }
//     memset(&serv_addr, 0, sizeof(serv_addr));
//     serv_addr.sin_family = AF_INET;
//     serv_addr.sin_port = htons(port);
//     serv_addr.sin_addr.s_addr = INADDR_ANY;
//     if (bind(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
//         perror("bind failed");
//         exit(EXIT_FAILURE);
//     }
//     if (listen(sock_fd, 5) < 0) {
//         perror("listen failed");
//         exit(EXIT_FAILURE);
//     }
//     printf("Listening on port %d...\n", port);
//     int client_fd = accept(sock_fd, NULL, NULL);
//     if (client_fd < 0) {
//         perror("accept failed");
//         exit(EXIT_FAILURE);
//     }

//     while (1) {
//         message_struct* msg = parse_message(client_fd);
//         if (msg == NULL) {
//             fprintf(stderr, "Failed to parse message\n");
//             exit(EXIT_FAILURE);
//         }
//         printf("Protocol: %s\n", msg->protocol);
//         printf("Message length: %d\n", msg->message_length);
//         printf("Message: %s\n", msg->message);
//     }


// }
   
