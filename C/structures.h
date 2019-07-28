#ifndef _STRUCTURES_
#define _STRUCTURES_

#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "err.h"

#define ALPHABET_SIZE 26

char string[20];

enum GameState {offered, played, finished};

typedef struct Room {
  bool occupied;
  int capacity;
  char type;
}Room;

typedef struct Game {
  int number;
  enum GameState gameState;
  char roomType;
  int mainPlayerId;
  bool invitedById[1024];
  int invitedByRoom[ALPHABET_SIZE];
  bool waitingForGame[1024];
  int room;
  int curPlaying;
}Game;

typedef struct Player {
  char favouriteRoomType;
  int Id;
  int gamesPlayed;
  int lastGame;
  int curRoom;
  int currentGame;
  bool waiting;
  bool canOfferGame;
  bool redundant;
  bool hasCurOffer;
}Player;


const char* intToString(int n) {
  sprintf(string, "%d", n);
  return string;
}

int stringToInt(const char* num) {
  int res = 0;
  int len = strlen(num);
  int mul = 1;

  for (int i = len - 1; i >= 0; i--) {
    res += mul * (num[i] - '0');
    mul *= 10;
  }

  return res;
}

const char* intTofileName(int n) {
  sprintf(string, "player-%d", n);
  return string;
}

const char* intToInFile(int n) {
  sprintf(string, "player-%d.in", n);
  return string;
}

const char* intToOutFile(int n) {
  sprintf(string, "player-%d.out", n);
  return string;
}


sem_t* createSemaphore (const char* name, int val) {
  sem_t* semaphore = sem_open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, val);
  if (semaphore == SEM_FAILED)
    syserr("sem_open");
  
  return semaphore;
}

sem_t* openSemaphore (const char* name) {
  return createSemaphore(name, 0);
}

void* createMemory (const char* name, int size) {
  void* result;
  int fdMemory = -1;
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_SHARED;
  
  fdMemory = shm_open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if(fdMemory == -1) syserr("shm_open");

  if (ftruncate(fdMemory, size) == -1)
    syserr("truncate");

  result = (void*)mmap(NULL, size, prot, flags, fdMemory, 0);
  if (result == MAP_FAILED) syserr("mmap");

  close(fdMemory);

  return result;
}

void V(sem_t* semaphore) {
  if(sem_post(semaphore))
    syserr("sem_post");
}

void P(sem_t* semaphore) {
  if(sem_wait(semaphore))
    syserr("sem_wait");
}

#endif