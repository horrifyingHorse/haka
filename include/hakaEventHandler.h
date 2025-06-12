#ifndef HAKA_EVENT_HANDLER_H
#define HAKA_EVENT_HANDLER_H

#include <stdio.h>

#include "haka.h"
#include "hakaBase.h"
#include "hakaUtils.h"

struct keyBinding {
  struct IntSet* keys;
  void (*func)(struct hakaStatus*);
};

struct keyBindings {
  int size;
  int capacity;
  struct keyBinding* kbind;
};
struct keyBindings* initKeyBindings(int size);
void addKeyBind(struct keyBindings* kbinds,
                void (*func)(struct hakaStatus*),
                int keyToBind,
                ...);
void pushKeyBind(struct keyBindings* kbinds, struct keyBinding* kbind);
void executeKeyBind(struct keyBindings* kbinds,
                    struct keyStatus* ks,
                    struct hakaStatus* haka);
#define Bind(func, ...) addKeyBind(kbinds, func, __VA_ARGS__, 0)

void switchFile(struct hakaStatus* haka);
void writeToFile(struct hakaStatus* haka);
void writePointToFile(struct hakaStatus* haka);
void openFile(struct hakaStatus* haka);

FILE* getPrimarySelection(struct hakaStatus* haka);
int openNotesFile(struct hakaStatus* haka);
size_t writeFP2FD(struct hakaStatus* haka);
FILE* triggerTofi(struct hakaStatus* haka);

#define updatePrevFile(haka)                                                   \
  haka->fdPrevFile = open(haka->prevFile, O_TRUNC | O_CREAT | O_WRONLY, 0666); \
  if (haka->fdPrevFile > 0) {                                                  \
    write(haka->fdPrevFile, haka->notesFileName, strlen(haka->notesFileName)); \
  }                                                                            \
  close(haka->fdPrevFile);

#define eventHandlerEpilogue(haka) \
  if (haka != NULL) {              \
    if (haka->fp != NULL) {        \
      pclose(haka->fp);            \
      haka->fp = NULL;             \
    }                              \
    if (haka->fdNotesFile > 0)     \
      close(haka->fdNotesFile);    \
    haka->served = true;           \
  }

#endif  // !HAKA_EVENT_HANDLER_H
