#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hakaBase.h"
#include "hakaEventHandler.h"

void switchFile(struct hakaStatus *haka) {
  printf("CTRL + ALT + M detected!\n");
  printf("Launching tofi\n");
  printf("tofi.cfg path: %s\n", haka->tofiCfg);

  char cmd[BUFSIZE * 2], basecmd[BUFSIZE * 2];
  snprintf(basecmd, BUFSIZE * 2, "ls %s -Ap1 | grep -v / | tofi -c %s",
           haka->notesDir, haka->tofiCfg);
  snprintf(cmd, BUFSIZE * 2,
           "%s  --prompt-text=\"  select:  \" "
           "--placeholder-text=\"%s\" --require-match=false",
           basecmd, haka->notesFileName);

  printf("Executing: %s\n", cmd);
  FILE *fp = popen(cmd, "r");
  if (fp == NULL) {
    perror("popen error.");
    exit(1);
  }

  char buf[BUFSIZE];
  bool selection = false;
  while (fgets(buf, BUFSIZE, fp)) {
    selection = true;
    buf[strcspn(buf, "\n")] = 0;
    printf("Selected: %ld %s\n", strlen(buf), buf);
    fflush(stdout);
  }

  if (selection) {
    strcpy(haka->notesFileName, buf);
    snprintf(haka->notesFile, BUFSIZE * 2, "%s/%s", haka->notesDir,
             haka->notesFileName);
    haka->fdPrevFile = open(haka->prevFile, O_TRUNC | O_CREAT | O_WRONLY, 0666);
    if (haka->fdPrevFile > 0) {
      write(haka->fdPrevFile, haka->notesFileName, strlen(haka->notesFileName));
    }
    close(haka->fdPrevFile);
  }

  pclose(fp);

  haka->served = true;
}

void writeToFile(struct hakaStatus *haka) {
  printf("CTRL + ALT + C detected!\n");
  printf("Dispatching request to get primary selection\n");

  FILE *fp = popen("wl-paste -p", "r");
  if (fp == NULL) {
    perror("popen error.");
    exit(1);
  }

  char buf[BUFSIZE];
  int notes = open(haka->notesFile, O_RDWR | O_CREAT | O_APPEND, 0666);
  if (notes < 0) {
    char errStr[BUFSIZE];
    sprintf(errStr, "Cannot open %s", haka->notesFile);
    perror(errStr);
    exit(1);
  }

  while (fgets(buf, BUFSIZE, fp)) {
    buf[strcspn(buf, "\n") + 1] = 0;
    printf("%ld %s", strlen(buf), buf);
    write(notes, buf, strlen(buf));
  }

  pclose(fp);
  close(notes);

  haka->served = true;
}
