#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <netinet/in.h>
#include "parser.c"
#include "tictactoe.h"

volatile int active = 1;

int accept_client(int serv_sock) {
   int reply_sock_fd = -1;
   socklen_t sin_size = sizeof(struct sockaddr_storage);
   struct sockaddr_storage client_addr;
   char client_printable_addr[INET6_ADDRSTRLEN];

   if ((reply_sock_fd = accept(serv_sock, 
           (struct sockaddr *)&client_addr, &sin_size)) == -1) {
      printf("socket accept error\n");
   }
   else {
    printf("server: client is connected");
   }
   return reply_sock_fd;
}

void handler(int signum)
{
    active = 0;
}

void install_handlers(void)
{
    struct sigaction act;
    act.sa_handler = handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
}

int open_listener(char *service, int queue_size)
{
    struct addrinfo hint, *info_list, *info;
    int error, sock;

    // initialize hints
    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family   = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags    = AI_PASSIVE;

    // obtain information for listening socket
    error = getaddrinfo(NULL, service, &hint, &info_list);
    if (error) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    // attempt to create socket
    for (info = info_list; info != NULL; info = info->ai_next) {
        sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

        // if we could not create the socket, try the next method
        if (sock == -1) continue;

        // bind socket to requested port
        error = bind(sock, info->ai_addr, info->ai_addrlen);
        if (error) {
            close(sock);
            continue;
        }

        // enable listening for incoming connection requests
        error = listen(sock, queue_size);
        if (error) {
            close(sock);
            continue;
        }

        // if we got this far, we have opened the socket
        break;
    }

    freeaddrinfo(info_list);

    // info will be NULL if no method succeeded
    if (info == NULL) {
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    return sock;
}

void acceptPlayer1(GameContext *game, int sockfd) {
   int player1sockfd, loc;
   while(1) {
      player1sockfd = accept_client(sockfd);
      printf("player 1 accepted\n");
      loc = acceptName(game->scoreboard, player1sockfd, game->mutex);
      
      if(loc >= 0) { break; } 
   }
   assignXGameContext(game, loc, player1sockfd);
   printf("player1 assigned\n");
}

void acceptPlayer2(GameContext *game, int sockfd) {
   int player2sockfd, loc;
   while(1) {
      player2sockfd = accept_client(sockfd);
      printf("player 2 accepted");
      loc = acceptName(game->scoreboard, player2sockfd, game->mutex);
      if(loc >= 0) { break; }
   }
   assignOGameContext(game, loc, player2sockfd);
   printf("player2 assigned\n");
}

void assignXGameContext(GameContext *game, int loc, int playersockfd) {
   game->playerXId = loc;   // Location of player X in scoreboard
   game->playerXSockfd = playersockfd;
   game->playerXName = game->scoreboard[loc].name;
}

void assignOGameContext(GameContext *game, int loc, int playersockfd) {
   game->playerOId = loc;   // Location of player X in scoreboard
   game->playerOSockfd = playersockfd;
   game->playerOName = game->scoreboard[loc].name;
}

/* Function creates thread and passes it the
   game context.
*/
void start_subserver(GameContext *game) {
   pthread_t tsubserver;
   pthread_create(&tsubserver, NULL, subserver, (void *) game);
}

/* Thread function hosts the game for both players
   and prints the final results.
*/
void *subserver(void *ptr) {
   GameContext *game = (GameContext *) ptr;
   int player1 = 1;
   int player2 = 2;
   sendBeginSignal(game);
   playGame(game);
}

/* Function sends begin signal to both 
   player 1 and player 2.
*/
void sendBeginSignal(GameContext *game) {
   printf("sending begin signal\n");
   char buffer[1024];
   // Use sprintf to format a string and store it in a buffer
   sprintf(buffer, "BEGN|X|%s", game->playerOName);
   write(game->playerXSockfd, buffer, strlen(buffer));

   // Reinitialize the buffer with new data
   memset(buffer, 0, sizeof(buffer));
   sprintf(buffer, "BEGN|O|%s", game->playerXName);
   write(game->playerOSockfd, buffer, strlen(buffer));
}

/* Function accepts names from players
   and registers them in the scoreboard if it is not full.
   Also returns the location of player on scoreboard.
*/
int acceptName(PlayerRecord *scoreboard, int playersockfd, Lock *mutex) {
   char* name;
   int size, loc, comp, result, bytes;

   while (1) {
      message_struct* msg = parse_message(playersockfd);
      if (msg == NULL) {
         write(playersockfd, "INVL|invalid message format", strlen("INVL|invalid message format"));
         continue;
      }

      if (strcmp(msg->protocol, "PLAY") == 0) {
         name = msg->message;
         break;
      } else {
         write(playersockfd, "INVL|invalid protocol", strlen("INVL|invalid protocol"));
      }
   }
   
   pthread_mutex_lock(&(mutex->lock));
   // Loop checks to see if player name is already registered
   for(loc = 0; loc < 10; loc++) {
      comp = strcmp(name, scoreboard[loc].name);
      // If name is already on scoreboard
      if(comp == 0) {
         printf("Player already on scoreboard\n\n");
         result = 0;
         write(playersockfd, "INVL", 5);
         pthread_mutex_unlock(&(mutex->lock));
         return loc;
      } 
   }
   // Loop checks to see if empty location in scoreboard exists
   for(loc = 0; loc < 10; loc++) {
      printf("loc: %d", loc); 
      // If empty space in scoreboard, place player there
      if(strcmp(scoreboard[loc].name, "") == 0) {
         strcpy(scoreboard[loc].name, name); 
         printf("Player placed on board\n\n");
         result = 0;
         write(playersockfd, "WAIT", 5);
         pthread_mutex_unlock(&(mutex->lock));
         return loc;
      }
   }
   pthread_mutex_unlock(&(mutex->lock));
   return(serverFull(playersockfd));
}

int serverFull(int playersockfd) {
   int result = -1;
   printf("Server full\n");
   send(playersockfd, &result, sizeof(int), 0);
   return -1;
}

/* Function intializes the scoreboard.
*/
void initScoreboard(PlayerRecord *scoreboard) {
  // Loop assigns each place on scoreboard as empty
   for(int i = 0; i < 10; i++) {
      strcpy(scoreboard[i].name, "");
   }
}

/* Function creates the board, handles the moves of players 
   in the correct sequence, sends updated board to players, 
   and determines if game is over in a win, loss, or draw.
*/
void playGame(GameContext *game) {
   char *board = createBoard(3,3);
   int gameStat = -1;  // Game not over while -1   
   // Game ends once a win, loss, or draw occurs
   while(1) {
       makeMove(game, game->playerXSockfd, PLAYER1, board);
       gameStat = checkWin(board, PLAYER1);     
       // If player 1 has won the game
       if(gameStat == 1) {
          sendOver(game->playerXSockfd, game->playerOSockfd, gameStat, board);
          free(board);
          break;
       }
       gameStat = checkDraw(board);      
       // If game has ended in a draw
       if(gameStat == 2) {      
          sendOver(game->playerXSockfd, game->playerOSockfd, gameStat, board);
          free(board);
          break;
       }
       // No win or draw yet, update both players
       sendMoved(game->playerXSockfd, game->playerOSockfd, PLAYER1, game->latestX, game->latestY, board); 
       makeMove(game, game->playerOSockfd, PLAYER2, board);
       gameStat = checkWin(board, PLAYER2);
       // If player 2 has won the game
       if(gameStat == 1) {
          sendOver(game->playerOSockfd, game->playerXSockfd, gameStat, board);
          free(board);
          break;
       }
       // No win or draw yet, update both players
      sendMoved(game->playerXSockfd, game->playerOSockfd, PLAYER2, game->latestX, game->latestY, board);
   }
}

/* Function send indication that game is continuing and also
   sends the updated board.
*/
void sendMoved(int p1sockfd, int p2sockfd, char psymb, int x, int y, char *board) {
   char message[100];  // create a buffer to store the message

   sprintf(message, "MOVD|%c|%d,%d|%s|", psymb, x, y, board);  // format the message string

   write(p1sockfd, message, strlen(message));
   write(p2sockfd, message, strlen(message));
}

/* Function marks the player's specified location
   on the board as long as it has not already been
   taken.
*/
void makeMove(GameContext *game, int playersockfd, char pSymb, char *board) {
   int x,y,taken;
   int bytes;
   char buffer[1024];
   // Accepts input for coordinates until 
   // untaken board location is sent
   while(1) {
      while (1) {
         message_struct* msg = parse_message(playersockfd);
         if (msg == NULL) {
            write(playersockfd, "INVL|invalid message format", strlen("INVL|invalid message format"));
            continue;
         }

         if (strcmp(msg->protocol, "MOVE") == 0) {
            x = msg->x;
            y = msg->y;
            game->latestX = x;
            game->latestY = y;
            break;
         } else {
            write(playersockfd, "INVL|invalid protocol", strlen("INVL|invalid protocol"));
         }
      }
      
      taken = isTaken(board, x, y);
      
      // If location on the board is taken
      if(taken == 0) {
         write(playersockfd, "INVL|the location is already taken", strlen("INVL|the location is already taken"));
      }

      // Location on the board is not taken, can be marked
      else { break; }
   }
   set(board, x, y, pSymb);
   send(playersockfd, &taken, sizeof(int), 0);
}

/* Function sends results of game to winning player and losing player
   or sends both players draw result depending on gameStat.
*/
void sendOver(int winner, int loser, int gameStat, char *board) {
   int lose = 0; // Sent to player that loses the game
   
   // If gameStat is 1, the game has been won by a player
   if(gameStat == 1) {
      write(winner, "OVER|W|you completed a line", sizeof("OVER|W|you completed a line"));
      write(loser, "OVER|L|opponent completed a line", sizeof("OVER|W|opponent completed a line"));
   }
   // If gameStat is 2, the game has ended in a draw
   else if(gameStat == 2) {
      write(winner, "OVER|D|the grid is full", sizeof("OVER|D|the grid is full"));
      write(loser, "OVER|D|the grid is full", sizeof("OVER|D|the grid is full"));
   }
}

/* Function gets the symbol at given
   location on the board.
*/ 
char get(char *board, int i, int j) {
   return board[(i-1) * 3 + (j-1)];
}

/* Function determines if location on board is taken.
   If it is returns a 0, if not returns a 1.
*/
int isTaken(char *board, int msgx, int msgy) {
   char loc = get(board, msgx, msgy);
   
   // Location not taken
   if(loc == EMPTY) {
      return 1;
   }
   // Location is taken
   else {
      return 0;
   } 
}   

/* Function marks location on the board with 
   specified player's symbol.
*/ 
void set(char *board, int msgx, int msgy, char playerSymbol) {
   board[(msgx-1) * 3 + (msgy-1)] = playerSymbol;
}

/* Function creates the board used for the 
   tic-tac-toe game.
*/
char *createBoard(int x, int y) {
   char *board = (char*)malloc(x*y+1);
   
   // Intialize each board location as empty
   for(int i = 0; i < x * y; i++) {
     board[i] = EMPTY;
   }
   return board;
}

/* Check to see if game has been won. Returns 1 if it has,
   and -1 if it hasn't.
*/ 
int checkWin(char *board, char playerSymbol) {
   if(board[0] == playerSymbol && board[4] == playerSymbol
     && board[8] == playerSymbol) { // Checks specific board location
      return 1;
   }
   if(board[2] == playerSymbol && board[4] == playerSymbol
     && board[6] == playerSymbol) { // Checks specific board location
      return 1;
   }
   if(board[0] == playerSymbol && board[3] == playerSymbol
     && board[6] == playerSymbol) { // Checks specific board location
      return 1;
   }
   if(board[2] == playerSymbol && board[5] == playerSymbol
     && board[8] == playerSymbol) { // Checks specific board location
      return 1; 
   }
   if(board[1] == playerSymbol && board[4] == playerSymbol
     && board[7] == playerSymbol) { // Checks specific board location
      return 1;
   }
   int row = 0;
   while(row < 7) {   // Checks row of board for win
      if(board[row] == playerSymbol && board[row+1] == playerSymbol
        && board[row+2] == playerSymbol) {
         return 1;
      }
     row = row + 3; // Add 3 to current row to get next row
   }
   return -1;
}

/* Function checks board for a draw
*/
int checkDraw(char *board) {
   int status = 2; // Status is a draw when 2
   
   // Iterates through each board location
   for(int i = 0; i < 10; i++) {
   
      // If a empty location on board exists, no draw
      if(board[i] == EMPTY) {
         status = -1;  // Game is not over
         return status;
      }
   }
   return status;
}

/* Main function which accepts player connections
   and assigns them a status as either player 1 or
   player 2. Starts the game.
*/

int main(int argc, char **argv)
{
   struct sockaddr_storage remote_host;
   char* welcome = "Welcome to Tic-Tac-Toe!";
   char** used_names;
   char buffer[1024] = {0};
   int valread;
   int reply_sock_fd;
   socklen_t remote_host_len;

   char *service = argc == 2 ? argv[1] : "15000";

	install_handlers();
	
   int listener = open_listener(service, QUEUE_SIZE);
   if (listener < 0) exit(EXIT_FAILURE);
   
   puts("Listening for incoming connections");   

   Lock *mutex = (Lock*)malloc(sizeof(Lock));
   pthread_mutex_init(&(mutex->lock), NULL);
   PlayerRecord *scoreboard = (PlayerRecord*)malloc(sizeof(PlayerRecord)*10);
   initScoreboard(scoreboard); 
   // Continue accepting players and hosting games until Ctrl + C
   while(active) {
      GameContext *game = (GameContext*)malloc(sizeof(GameContext));
      game->mutex = mutex;
      game->scoreboard = scoreboard;
      acceptPlayer1(game, listener);
      acceptPlayer2(game, listener);
      start_subserver(game);
   }

    puts("Shutting down");
    close(listener);

    return EXIT_SUCCESS;
}
