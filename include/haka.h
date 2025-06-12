#ifndef HAKA_H
#define HAKA_H

#include <grp.h>
#include <libevdev/libevdev.h>
#include <linux/types.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hakaBase.h"
#include "hakaUtils.h"

struct confVars {
  char editor[BUFSIZE];
  char notesDir[BUFSIZE];
  char terminal[BUFSIZE];
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

#define SUPPORTED_KEYS 249
struct keyStatus {
  int16_t size;
  struct IntSet* activationCombo;
  bool* keyPress;
};
struct keyStatus* initKeyStatus(int16_t size);
void handleKeyEvent(struct keyStatus* ks, int evCode, int evVal);
void setActivationCombo(struct keyStatus* ks, ...);
bool resetActivationCombo(struct keyStatus* ks);
bool activated(struct keyStatus* ks);

#define ActivationCombo(...) setActivationCombo(ks, __VA_ARGS__, -1)

void reapChild(struct hakaStatus* haka);

#endif
