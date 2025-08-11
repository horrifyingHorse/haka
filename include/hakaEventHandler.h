#ifndef HAKA_EVENT_HANDLER_H
#define HAKA_EVENT_HANDLER_H

#include <stdio.h>

#include "haka.h"
#include "hakaBase.h"
#include "hakaUtils.h"

struct keyBinding {
  struct IntSet* keys;
  void (*func)(struct hakaContext*);
};

struct keyBindings {
  int size;
  int capacity;
  struct keyBinding* kbind;
};

struct keyBindings* initKeyBindings(int size);
void addKeyBind(struct keyBindings* kbinds,
                void (*func)(struct hakaContext*),
                int keyToBind,
                ...);
void pushKeyBind(struct keyBindings* kbinds, struct keyBinding* kbind);
void executeKeyBind(struct keyBindings* kbinds,
                    struct keyState* ks,
                    struct hakaContext* haka);
void loadBindings(struct keyBindings* kbinds, struct keyState* ks);

// Event Handler Declarations
void switchFile(struct hakaContext* haka);
void writeToFile(struct hakaContext* haka);
void writePointToFile(struct hakaContext* haka);
void openFile(struct hakaContext* haka);

// Event Handler Helper Functions
FILE* getPrimarySelection(struct hakaContext* haka);
int openNotesFile(struct hakaContext* haka);
size_t writeFP2FD(struct hakaContext* haka);
FILE* triggerTofi(struct hakaContext* haka);

#define Bind(func, ...) addKeyBind(kbinds, func, __VA_ARGS__, 0)

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
