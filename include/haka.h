#ifndef HAKA_H
#define HAKA_H

#include <grp.h>
#include <libevdev/libevdev.h>
#include <linux/types.h>
#include <stdbool.h>
#include <stdlib.h>

#include "hakaBase.h"

struct confVars {
  char editor[BUFSIZE];
  char notesDir[BUFSIZE];
  char tofiCfg[BUFSIZE];
};

struct hakaStatus {
  char execDir[BUFSIZE];
  char notesFileName[BUFSIZE];
  char notesFile[BUFSIZE * 2];
  int fdNotesFile;

  int fdPrevFile;
  char prevFile[BUFSIZE];

  FILE* fp;
  struct confVars* config;

  bool served;
  int childCount;
};

#define statusCheck(haka)                                       \
  if (haka == NULL) {                                           \
    fprintf(stderr, "The hakaStatus object cannot be NULL.\n"); \
    exit(1);                                                    \
  }

#define buildAbsFilePath(haka)                                            \
  snprintf(haka->notesFile, BUFSIZE * 2, "%s/%s", haka->config->notesDir, \
           haka->notesFileName);

struct hakaStatus* initHaka();
struct confVars* initConf(struct hakaStatus* haka);
void getExeDir(struct hakaStatus* haka);
void getPrevFile(struct hakaStatus* haka);

struct keyStatus {
  bool Ctrl;
  bool Alt;

  bool C;
  bool M;
  bool O;
  bool P;
};
struct keyStatus* initKeyStatus();

#define setKeyStatus(ks, code) \
  switch (code) {              \
    case KEY_LEFTCTRL:         \
    case KEY_RIGHTCTRL:        \
      ks->Ctrl = true;         \
      break;                   \
    case KEY_LEFTALT:          \
    case KEY_RIGHTALT:         \
      ks->Alt = true;          \
      break;                   \
    case KEY_C:                \
      ks->C = true;            \
      break;                   \
    case KEY_M:                \
      ks->M = true;            \
      break;                   \
    case KEY_O:                \
      ks->O = true;            \
      break;                   \
    case KEY_P:                \
      ks->P = true;            \
      break;                   \
  }

#define resetKeyStatus(ks, code) \
  switch (code) {                \
    case KEY_LEFTCTRL:           \
    case KEY_RIGHTCTRL:          \
      ks->Ctrl = false;          \
      break;                     \
    case KEY_LEFTALT:            \
    case KEY_RIGHTALT:           \
      ks->Alt = false;           \
      break;                     \
    case KEY_C:                  \
      ks->C = false;             \
      break;                     \
    case KEY_M:                  \
      ks->M = false;             \
      break;                     \
    case KEY_O:                  \
      ks->O = false;             \
      break;                     \
    case KEY_P:                  \
      ks->P = false;             \
      break;                     \
  }

#define keyCombination(ks, KEY) ks->Ctrl && ks->Alt && ks->KEY

void reapChild(struct hakaStatus* haka);

#endif
