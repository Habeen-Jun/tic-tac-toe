#define PLAYER1 'X'
#define PLAYER2 'O'
#define BUFSIZE 256
#define HOSTSIZE 100
#define PORTSIZE 10
#define EMPTY '.'
#define QUEUE_SIZE 8

typedef struct score_board {
   char name[20];
} PlayerRecord;

typedef struct LOCK {
   pthread_mutex_t lock; // Used to protect scoreboard
}  Lock;

typedef struct messege_struct {
    char* protocol;
    int messege_length;
    char* messege;
} messege_struct;

typedef struct game_context {
   int playerXId;            // id of player X
   int playerOId;            // id of player O
   int playerXSockfd;        // sockfd for player X
   int playerOSockfd;        // sockfd for player O
   char *playerOName;
   char *playerXName;
   int latestX; // latest x value of MOVE
   int latestY; // latest y value of MOVE
   Lock *mutex;
   PlayerRecord *scoreboard;
} GameContext;


void playGame(GameContext *game);
void makeMove(GameContext *game, int playersockfd, char pSymb, char *board);
char *createBoard(int x, int y);
int isTaken(char *board, int msgx, int msgy);
void set(char *board, int msgx, int msgy, char playerSymbol);
int checkWin(char *board, char playerSymbol);
int checkDraw(char *board);
void sendOver(int winner, int loser, int gameStat, char *board);
void sendMoved(int p1sockfd, int p2sockfd, char psymb, int x, int y, char *board);
int acceptName(PlayerRecord *scoreboard, int playersockfd, Lock *mutex);
void initScoreboard(PlayerRecord *scoreboard);
void start_subserver(GameContext *game);
void *subserver(void *ptr);
void sendBeginSignal(GameContext *game);
void assignXGameContext(GameContext *game, int loc, int playersockfd);
void assignOGameContext(GameContext *game, int loc, int playersockfd);
int serverFull(int playersockfd);
void acceptPlayer1(GameContext *game, int sockfd);
void acceptPlayer2(GameContext *game, int sockfd);