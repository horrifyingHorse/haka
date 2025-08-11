#ifndef HAKA_H
#define HAKA_H

#include <grp.h>
#include <libevdev/libevdev.h>
#include <linux/types.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "hakaBase.h"
#include "hakaUtils.h"

static volatile sig_atomic_t live = true;
static void handler(int signum) {
  live = false;
}

struct confVars {
  char editor[BUFSIZE];
  char notesDir[BUFSIZE];
  char terminal[BUFSIZE];
  char tofiCfg[BUFSIZE];
};

struct hakaContext {
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

struct keyState {
  int16_t size;
  struct IntSet* activationCombo;
  bool* keyPress;
};

struct hakaContext* initHaka();
struct confVars* initConf(struct hakaContext* haka);
void getExeDir(struct hakaContext* haka);
void getPrevFile(struct hakaContext* haka);

struct keyState* initKeyState(int16_t size);
void handleKeyEvent(struct keyState* ks, int evCode, int evVal);
void setActivationCombo(struct keyState* ks, ...);
bool resetActivationCombo(struct keyState* ks);
bool activated(struct keyState* ks);

void reapChild(struct hakaContext* haka);

#define SUPPORTED_KEYS 249
#define ActivationCombo(...) setActivationCombo(ks, __VA_ARGS__, -1)

#define contextCheck(haka)                                       \
  if (haka == NULL) {                                            \
    fprintf(stderr, "The hakaContext object cannot be NULL.\n"); \
    exit(1);                                                     \
  }

#define buildAbsFilePath(haka)                                            \
  snprintf(haka->notesFile, BUFSIZE * 2, "%s/%s", haka->config->notesDir, \
           haka->notesFileName);

#endif
