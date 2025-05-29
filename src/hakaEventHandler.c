#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "haka.h"
#include "hakaBase.h"
#include "hakaEventHandler.h"
#include "hakaUtils.h"

void switchFile(struct hakaStatus *haka) {
  statusCheck(haka);

  printf("CTRL + ALT + M detected!\n");
  printf("Launching tofi\n");
  printf("tofi.cfg path: %s\n", haka->tofiCfg);

  triggerTofi(haka);

  char buf[BUFSIZE];
  bool selection = false;
  while (fgets(buf, BUFSIZE, haka->fp)) {
    selection = true;
    buf[strcspn(buf, "\n")] = 0;
    printf("Selected: %ld %s\n", strlen(buf), buf);
    fflush(stdout);
  }

  if (selection) {
    strcpy(haka->notesFileName, buf);
    buildAbsFilePath(haka);
    updatePrevFile(haka);
  }

  eventHandlerEpilogue(haka);
}

void writeToFile(struct hakaStatus *haka) {
  statusCheck(haka);

  printf("CTRL + ALT + C detected!\n");
  printf("Dispatching request to get primary selection\n");

  getPrimarySelection(haka);
  openNotesFile(haka);

  writeFP2FD(haka);

  eventHandlerEpilogue(haka);
}

void writePointToFile(struct hakaStatus *haka) {
  statusCheck(haka);

  printf("CTRL + ALT + P detected!\n");
  printf("Dispatching request to get primary selection\n");

  getPrimarySelection(haka);
  openNotesFile(haka);

  write(haka->fdNotesFile, "- ", 2);

  writeFP2FD(haka);

  eventHandlerEpilogue(haka);
}

void openFile(struct hakaStatus *haka) {
  statusCheck(haka);

  printf("CTRL + ALT + O detected!\n");
  printf("Opening current note in editor\n");

  char *term = getEnvVar("$TERM");
  if (term == NULL) {
    fprintf(stderr, "Cannot get var $TERM, recieved NULL\n");
    return;
  }
  if (!strcmp(term, "")) {
    fprintf(stderr, "Cannot get var $TERM, recieved '%s'\n", term);
    free(term);
    return;
  }

  pid_t pid = fork();
  if (pid < 0) {
    fprintf(stderr, "unable to create a fork");
    return;
  }
  if (pid == 0) {
    execlp(term, term, "-e", "nvim", haka->notesFile, NULL);
    perror("execlp failed to launch note");
    exit(1);
  }
  haka->childCount++;

  free(term);
  eventHandlerEpilogue(haka);
}

FILE *getPrimarySelection(struct hakaStatus *haka) {
  statusCheck(haka);

  haka->fp = popen("wl-paste -p", "r");
  if (haka->fp == NULL) {
    perror("popen error.");
    exit(1);
  }
  return haka->fp;
}

int openNotesFile(struct hakaStatus *haka) {
  statusCheck(haka);

  haka->fdNotesFile = open(haka->notesFile, O_RDWR | O_CREAT | O_APPEND, 0666);
  if (haka->fdNotesFile < 0) {
    char errStr[BUFSIZE];
    sprintf(errStr, "Cannot open %s", haka->notesFile);
    perror(errStr);
    exit(1);
  }
  return haka->fdNotesFile;
}

size_t writeFP2FD(struct hakaStatus *haka) {
  statusCheck(haka);

  size_t bytes = 0;
  char buf[BUFSIZE];
  while (fgets(buf, BUFSIZE, haka->fp)) {
    buf[strcspn(buf, "\n") + 1] = 0;
    bytes += strlen(buf);
    printf("%ld %s", strlen(buf), buf);
    write(haka->fdNotesFile, buf, strlen(buf));
  }
  return bytes;
}

FILE *triggerTofi(struct hakaStatus *haka) {
  statusCheck(haka);

  char cmd[BUFSIZE * 2], basecmd[BUFSIZE * 2];
  snprintf(basecmd, BUFSIZE * 2, "ls %s -Ap1 | grep -v / | tofi -c %s",
           haka->notesDir, haka->tofiCfg);
  snprintf(cmd, BUFSIZE * 2,
           "%s  --prompt-text=\"  select:  \" "
           "--placeholder-text=\"%s\" --require-match=false",
           basecmd, haka->notesFileName);
  printf("Executing: %s\n", cmd);

  haka->fp = popen(cmd, "r");
  if (haka->fp == NULL) {
    perror("popen error.");
    exit(1);
  }

  return haka->fp;
}
