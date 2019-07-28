#include "structures.h"

int main() {
  int N, M;
  if (scanf("%d", &N) == EOF)
    syserr("input reading");

  if (scanf("%d", &M) == EOF)
    syserr("input reading");

  pid_t pid;
  
  // available rooms
  Room* rooms = (Room*)createMemory("rooms", sizeof(Room) * M);
  sem_t* mutex = createSemaphore("mutex", 1);
  sem_t** semId = (sem_t**)malloc(sizeof(sem_t*) * N);
  if (semId == NULL)
    syserr("malloc");

  sem_t* forOthers = createSemaphore("forOthers", 0);
  sem_t* forManager = createSemaphore("forManager", 1);
  sem_t* forFinished = createSemaphore("forFinished", 0);
  // total number of games that have already begun
  (int*)createMemory("totalGamesPlayed", sizeof(int));
  // game that is currently collecting players
  (int*)createMemory("curGame", sizeof(int));
  // games that are offered, but nod finished yet (there are not more than 2*N of them)
  Game* offers = (Game*)createMemory("offers", sizeof(Game) * 2 * N);
  Player* players = (Player*)createMemory("players", sizeof(Player) * N);
  int* entered = (int*)createMemory("entered", sizeof(int));
  // last player that left the room;
  Player* lastPlayer = (Player*)createMemory("lastPlayer", sizeof(Player));
  int* numberOfPlayers = (int*)createMemory("numberOfPlayers", sizeof(int));
  int* numberOfRooms = (int*)createMemory("numberOfRooms", sizeof(int));
  // for each roomType remembers number of players whose favourite room it is
  int* numberOfPlayersByRoom = (int*)createMemory("numberOfPlayersByRoom", sizeof(int) * ALPHABET_SIZE);
  (int*)createMemory("idx", sizeof(int));
  int* playersWithOffers = (int*)createMemory("playersWithOffers", sizeof(int));
  int* totalGamesOffered = (int*)createMemory("totalGamesOffered", sizeof(int));

  *totalGamesOffered = 0;
  *entered = 0;
  *playersWithOffers = N;
  *numberOfPlayers = N;
  *numberOfRooms = M;

  for (int i = 0; i < N; i++) {
    semId[i] = createSemaphore(intTofileName(i + 1), 0);
    sem_close(semId[i]);
  }

  sem_close(mutex);
  sem_close(forOthers);
  sem_close(forManager);
  sem_close(forFinished);

  for (int i = 0; i < N; i++) {
    players[i].Id = i + 1;
    players[i].gamesPlayed = 0;
    players[i].lastGame = -1;
    players[i].waiting = false;
    players[i].canOfferGame = true;
    players[i].redundant = false;
    players[i].hasCurOffer = false;
  }

  for (int i = 0; i < 2 * N; i++)
    offers[i].gameState = finished; 

  for (int i = 0; i < ALPHABET_SIZE; i++) 
    numberOfPlayersByRoom[i] = 0;

  for (int i = 0; i < M; i++) {
    char type;
    int capacity;
    
    if (scanf(" %c", &type) == EOF)
      syserr("input reading");

    if (scanf("%d", &capacity) == EOF)
      syserr("input reading");
    
    rooms[i].capacity = capacity;
    rooms[i].type = type;
    rooms[i].occupied = false;
  }

  for (int i = 0; i < N; i++) {
    pid = fork();
    if (pid == -1)
      syserr("fork");

    else if (pid == 0) {
      execlp("./player", "./player", intToString(i + 1) , NULL);
      syserr("execlp");
    }
  }

  forFinished = openSemaphore("forFinished");
  forManager = openSemaphore("forManager");
  
  for (int i = 0; i < N; i++) {
    P(forFinished);
    printf("Player %d left after %d game(s)\n", lastPlayer->Id, lastPlayer->gamesPlayed);
    V(forManager);
  }
  
  wait(0);
  sem_close(mutex);
  sem_close(forOthers);
  sem_close(forManager);
  sem_close(forFinished);
  sem_unlink("mutex");
  sem_unlink("forOthers");
  sem_unlink("forManager");
  sem_unlink("forFinished");

  for (int i = 0; i < N; i++) {
    sem_close(semId[i]);
    sem_unlink(intTofileName(i + 1));
  }

  shm_unlink("rooms");
  shm_unlink("totalGamesPlayed");
  shm_unlink("playedGames");
  shm_unlink("lastPlayer");
  shm_unlink("lastGame");
  shm_unlink("curGame");
  shm_unlink("offers");
  shm_unlink("players");
  shm_unlink("entered");
  shm_unlink("numberOfPlayers");
  shm_unlink("numberOfRooms");
  shm_unlink("numberOfPlayersByRoom");
  shm_unlink("idx");
  shm_unlink("playersWithOffers");
  shm_unlink("totalGamesOffered");
  free(semId);

  return 0;
}