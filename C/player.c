#include "structures.h"

int countPeopleInGame(Game* game, int N) {
  int result = 1;

  for (int i = 0; i < N; i++) 
    if (game->invitedById[i])
      result++;
  

  for (int i = 0; i < ALPHABET_SIZE; i++) 
    result += game->invitedByRoom[i];

  return result;
}

bool verifyGame (Game* game, Player* players, int* numberOfPlayersByRoom, int N, int M, Room* rooms) {
  int count = countPeopleInGame(game, N);
  // check if there is a room with enough space
  bool enoughSpace = false;
  for (int i = 0; i < M; i++) 
    if (rooms[i].capacity >= count && rooms[i].type == game->roomType)
      enoughSpace = true;
  
  if (!enoughSpace)
    return false;

  int guestsByRoom[ALPHABET_SIZE];

  // check if there are enough people for the game
  for (int i = 0; i < ALPHABET_SIZE; i++)
    guestsByRoom[i] = 0;

  for (int i = 0; i < N; i++) {
    if (game->invitedById[i]) {
      guestsByRoom[players[i].favouriteRoomType - 'A']++;
      if (guestsByRoom[players[i].favouriteRoomType - 'A'] > numberOfPlayersByRoom[players[i].favouriteRoomType - 'A']) {
        return false;
      }
    }
  }
 
  for (int i = 0; i < ALPHABET_SIZE; i++) {
    guestsByRoom[i] += game->invitedByRoom[i];
    if (guestsByRoom[i] > numberOfPlayersByRoom[i])
      return false;
  }

  int mainIdRoom = players[game->mainPlayerId].favouriteRoomType - 'A';
  guestsByRoom[mainIdRoom]++;
  if (guestsByRoom[mainIdRoom] > numberOfPlayersByRoom[mainIdRoom])
    return false;

  //check if all invited players exist
  for (int i = N; i < 1024; i++) 
    if (game->invitedById[i])
      return false;

  return true;
}


Game* readGame (FILE* fdIn, FILE* fdOut, int id, Player* players, int *numberOfPlayersByRoom, int N, int M, Room *rooms, int* playersWithOffers) {
  Game* game = (Game*)malloc(sizeof(Game));
  if (game == NULL)
    syserr("malloc");

  char line[5000];
  game->mainPlayerId = id;
  bool doubleInvited = false;

  for (int i = 0; i < 1024; i++) 
    game->invitedById[i] = false;
  
  for (int i = 0; i < ALPHABET_SIZE; i++) 
    game->invitedByRoom[i] = 0;

  int ch;
  ch = fgetc(fdIn);
  line[0] = ch;
  game->roomType = ch;
  int curNum = 0;
  int idx = 0;
  while (ch != '\n' && ch != EOF) {
    ch = fgetc(fdIn);
    idx++;
    line[idx] = ch;
    if (ch != ' ' && ch != '\n' && ch != EOF) {
      if (ch >= 'A')
        game->invitedByRoom[ch - 'A']++;

      else 
        curNum = curNum * 10 + ch - '0';
    }

    else if (curNum != 0) {
      if (game->invitedById[curNum - 1] == true)
        doubleInvited = true;
      game->invitedById[curNum - 1] = true;
      curNum = 0;
    }
  }

  line[idx] = '\0';

  if (ch != '\n') {
    players[id].canOfferGame = false;
    (*playersWithOffers)--;
    if (fclose(fdIn) == EOF)
      syserr("Error in close");
  }

  if (!doubleInvited && verifyGame(game, players, numberOfPlayersByRoom, N, M, rooms)) 
    return game;
  
  free(game);
  fprintf(fdOut, "%s%s%s\n","Invalid game \"", line, "\"" );
  return NULL;
}

void readGood (FILE* fdIn, FILE* fdOut, int id, Player* players, int* numberOfPlayersByRoom, int N, int M, Room* rooms, Game* offers, int* totalGamesOffered, int* playersWithOffers) { 
  Game* game = NULL;
  while (game == NULL && players[id].canOfferGame) 
    game = readGame(fdIn, fdOut, id, players, numberOfPlayersByRoom, N, M, rooms, playersWithOffers);
  if (game != NULL) {
    game->gameState = offered;
    (*totalGamesOffered)++;
    game->number = *totalGamesOffered;
    players[id].hasCurOffer = true;
    
    for (int i = 0; i < 2 * N; i++) {
      if (offers[i].gameState == finished) {
        offers[i] = *game;
        break;
      }
    }
  }
}

// check if there is enough players and free room to play a game
bool gameReady(Game* game, Player* players, Room* rooms, int N, int M) {

  // check if host is ready
  if (!players[game->mainPlayerId].waiting)
    return false;

  bool freeRoom = false;
  for (int i = 0; i < M; i++) 
    if (!rooms[i].occupied && rooms[i].capacity >= countPeopleInGame(game, N) && rooms[i].type == game->roomType)
      freeRoom = true;

  if (!freeRoom)
    return false;

  // check if people invited by id are free
  for (int i = 0; i < N; i++) 
    if (game->invitedById[i] && !players[i].waiting)
      return false;

  // check if there are enough people with favourite room types
  int freePeopleByRoom[ALPHABET_SIZE];
  for (int i = 0; i < ALPHABET_SIZE; i++) 
    freePeopleByRoom[i] = 0;

  for (int i = 0; i < N; i++) 
    if (i != game->mainPlayerId && players[i].waiting && game->invitedById[i] == false)
      freePeopleByRoom[players[i].favouriteRoomType - 'A']++;

  for (int i = 0; i < ALPHABET_SIZE; i++) 
    if (game->invitedByRoom[i] > freePeopleByRoom[i])
      return false;

  return true;
}

// collects team and room for the game
void collectTeam(Game* offers, int gameidx, Player* players, Room* rooms, int N, int M, int* curGame, int* totalGamesPlayed, int *idx, sem_t** semId, FILE *fdOut) {
  int count = countPeopleInGame(&offers[gameidx], N);
  *curGame = gameidx;
  (*totalGamesPlayed)++;
  offers[gameidx].gameState = played;
  offers[gameidx].curPlaying = 0;
  *idx = 0;
  players[offers[gameidx].mainPlayerId].hasCurOffer = false;
  
  // take smallest room
  Room* smallestRoom = NULL;
  for (int i = 0; i < M; i++) {
    if (rooms[i].type == offers[gameidx].roomType && rooms[i].capacity >= count && !rooms[i].occupied && (smallestRoom == NULL || smallestRoom->capacity > rooms[i].capacity)) {
      smallestRoom = &rooms[i];
      offers[gameidx].room = i;
    }
  }
  smallestRoom->occupied = true; //TODO czy to działa?

  //collest host
  offers[gameidx].waitingForGame[offers[gameidx].mainPlayerId] = true;

  // collect players invited by id
  for (int i = 0; i < N; i++) 
    if (offers[gameidx].invitedById[i])
      offers[gameidx].waitingForGame[i] = true;

  // collect people invited by roomType
  for (int i = 0; i < ALPHABET_SIZE; i++) {
    int num = offers[gameidx].invitedByRoom[i];
    if (num > 0) {
      // first, we take the player that haven't played for the longest time to avoid starvation.
      Player* bestPlayer = NULL;
      int bestId = 0;
      for (int j = 0; j < N; j++) {
        if (players[j].waiting && players[j].favouriteRoomType - 'A' == i && !offers[gameidx].waitingForGame[j] && (bestPlayer == NULL || bestPlayer->lastGame > players[j].lastGame)) {
          bestPlayer = &players[j];
          bestId = j;
        }
      }
      
      offers[gameidx].waitingForGame[bestId] = true;
      num--;
      // then we collect rest players randomly
      int j = 0;
      while (num > 0) {
        if (players[j].waiting && players[j].favouriteRoomType - 'A' == i && !offers[gameidx].waitingForGame[j]) {
          num--;
          offers[gameidx].waitingForGame[j] = true;
        }
        j++;
      }
    }
  }

  fprintf(fdOut, "%s %d %s %d %s", "Game defined by", offers[gameidx].mainPlayerId + 1 ,"is going to start: room", offers[gameidx].room + 1 ,"players (");

  bool first = true;
  
  for (int i = 0; i < N; i++) {
    if (offers[gameidx].waitingForGame[i]) {
      if (first) 
        fprintf(fdOut, "%d", i + 1);
      else
        fprintf(fdOut, "%s %d", ",", i + 1);

      first = false;
    }
  }

  fprintf(fdOut, "%s\n", ")");

  while (!offers[gameidx].waitingForGame[*idx])
    (*idx)++;

  V(semId[*idx]);
}

// check which players are redundant and let them go out
void findRedundant(Game* offers, int N, Player* players, sem_t** semId) {
  bool* redundantId = (bool*)malloc(sizeof(bool) * N);
  bool* redundantRooms = (bool*)malloc(sizeof(bool) * ALPHABET_SIZE);

  for (int i = 0; i < N; i++)
    redundantId[i] = true;

  for (int i = 0; i < ALPHABET_SIZE; i++)
    redundantRooms[i] = true;

  for(int i = 0; i < 2 * N; i++) {
    if (offers[i].gameState == offered) {
      for (int j = 0; j < N; j++) 
        if (offers[i].invitedById[j])
          redundantId[j] = false;
      
      for (int j = 0; j < ALPHABET_SIZE; j++)
        if(offers[i].invitedByRoom[j] > 0)
          redundantRooms[j] = false;

      redundantId[offers[i].mainPlayerId] = false;
      redundantRooms[players[offers[i].mainPlayerId].favouriteRoomType - 'A'] = false;
    }
  }

  for (int i = 0; i < N; i++) {
    if (redundantId[i] && redundantRooms[players[i].favouriteRoomType - 'A']) {
      players[i].redundant = true;
      V(semId[i]);
    }
  }

  free(redundantId);
  free(redundantRooms);
}

int main(int argc, char** argv) {
	assert(argc == 2);
  int id = stringToInt(argv[1]) - 1;

  int* numberOfPlayers = (int*)createMemory("numberOfPlayers", sizeof(int));
  int* numberOfRooms = (int*)createMemory("numberOfRooms", sizeof(int));
  int N = *numberOfPlayers;
  int M = *numberOfRooms;
  
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
  int* totalGamesPlayed = (int*)createMemory("totalGamesPlayed", sizeof(int));
  // game that is currently collecting players
  int* curGame = (int*)createMemory("curGame", sizeof(int));
  // games that are offered, but nod played yet (there are not more than 2*N of them)
  Game* offers = (Game*)createMemory("offers", sizeof(Game) * 2 * N);
  Player* players = (Player*)createMemory("players", sizeof(Player) * N);
  int* entered = (int*)createMemory("entered", sizeof(int));
  // last player that left the room;
  Player* lastPlayer = (Player*)createMemory("lastPlayer", sizeof(Player));
  // for each room remembers number of players whose favourite room it is
  int* numberOfPlayersByRoom = (int*)createMemory("numberOfPlayersByRoom", sizeof(int) * ALPHABET_SIZE);
  int* idx = (int*)createMemory("idx", sizeof(int));
  int* playersWithOffers = (int*)createMemory("playersWithOffers", sizeof(int));
  int* totalGamesOffered = (int*)createMemory("totalGamesOffered", sizeof(int));

  for (int i = 0; i < N; i++) 
    semId[i] = createSemaphore(intTofileName(i + 1), 0);

  FILE *fdIn, *fdOut;
  int ch;
  char favouriteRoomType;
  fdIn = fopen(intToInFile(id + 1), "r");
  fdOut = fopen(intToOutFile(id + 1), "w");

  if(fdIn == NULL || fdOut == NULL)
    syserr("Error in open");
  ch = fgetc(fdIn);
  favouriteRoomType = ch;
  players[id].favouriteRoomType = favouriteRoomType;

  if (fgetc(fdIn) == EOF) {
    if(fclose(fdIn) == EOF)
      syserr("Error in close");
    players[id].canOfferGame = false;
    (*playersWithOffers)--;
  }
  numberOfPlayersByRoom[favouriteRoomType - 'A']++;

  P(mutex);
  (*entered)++;

  if (*entered < N) {
    V(mutex);
    P(forOthers);
  }
  else
    V(mutex);

  V(forOthers);
  while (true) {
    P(mutex);
    players[id].waiting = true;

    if (!players[id].hasCurOffer)
      readGood(fdIn, fdOut, id, players, numberOfPlayersByRoom, N, M, rooms, offers, totalGamesOffered, playersWithOffers);

    if (*playersWithOffers == 0)
      findRedundant(offers, N, players, semId);

    int goodGame = -1;
    for (int i = 0; i < 2 * N; i++) 
      if (offers[i].gameState == offered && gameReady(&offers[i], players, rooms, N, M) && (goodGame == -1 || offers[i].number < offers[goodGame].number))
        goodGame = i;  

    if (goodGame != -1)
      collectTeam(offers, goodGame, players, rooms, N, M, curGame, totalGamesPlayed, idx, semId, fdOut);
    else 
      V(mutex);

    P(semId[id]);

    if (players[id].redundant)
      break;

    players[id].gamesPlayed++;
    players[id].lastGame = *totalGamesPlayed;
    players[id].waiting = false;
    offers[*curGame].curPlaying++;
    (*idx)++;

    fprintf(fdOut, "%s %d%s %d %s", "Entered room", offers[*curGame].room + 1, ", game defined by", offers[*curGame].mainPlayerId + 1, "waiting for players (" );

    bool first = true;
    for (int i = *idx; i < N; i++) {
      if (offers[*curGame].waitingForGame[i]) {
        if (first)
          fprintf(fdOut, "%d", i + 1);
        else
          fprintf(fdOut, "%s %d", ",", i + 1);

        first = false;
      }
    }

    fprintf(fdOut, "%s\n", ")");

    players[id].curRoom = offers[*curGame].room;
    players[id].currentGame = *curGame;

    while (*idx < N && !offers[*curGame].waitingForGame[*idx])
      (*idx)++;

    if ((*idx) < N) 
      V(semId[*idx]);

    else 
      V(mutex);
    

    ////////////////////////////////////////////////////////////
                      // ENJOY THE GAME! //
    ////////////////////////////////////////////////////////////
    
    P(mutex);
    offers[players[id].currentGame].curPlaying--;
    fprintf(fdOut, "%s %d\n", "Left room", players[id].curRoom + 1);

    if (offers[players[id].currentGame].curPlaying == 0) {
      offers[players[id].currentGame].gameState = finished;
      rooms[players[id].curRoom].occupied = false;
    }

    V(mutex);
  }  

  P(forManager);
  *lastPlayer = players[id];
  V(forFinished);

  if (fclose(fdOut) == EOF)
    syserr("Error in close");

  free(semId);

  return 0;

}