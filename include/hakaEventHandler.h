#ifndef HAKA_EVENT_HANDLER_H
#define HAKA_EVENT_HANDLER_H

#include <stdio.h>

#include "haka.h"
#include "hakaBase.h"

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
