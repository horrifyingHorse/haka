#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "haka.h"
#include "hakaBase.h"
#include "hakaEventHandler.h"
#include "hakaUtils.h"

struct keyBindings *initKeyBindings(int size) {
  if (size < 0) {
    Fprintln(stderr, "keybinds size < 0?");
    exit(EXIT_FAILURE);
  }

  struct keyBindings *kbinds =
      (struct keyBindings *)malloc(sizeof(struct keyBindings));
  if (kbinds == 0) {
    Fprintln(stderr, "malloc failed for key bindings");
    exit(EXIT_FAILURE);
  }

  kbinds->size = 0;
  kbinds->capacity = size;
  kbinds->kbind = (struct keyBinding *)malloc(sizeof(struct keyBinding) * size);
  if (kbinds->kbind == 0) {
    Fprintln(stderr, "malloc failed for key binding");
    exit(EXIT_FAILURE);
  }

  return kbinds;
}

void addKeyBind(struct keyBindings *kbinds, void (*func)(struct hakaContext *),
                int keyToBind, ...) {
  if (kbinds == 0) {
    Fprintln(stderr, "keybinds are null; abort adding a keybind");
    return;
  }

  struct IntSet *keys = initIntSet(5);
  pushIntSet(keys, keyToBind);

  va_list args;
  va_start(args, keyToBind);
  int key = va_arg(args, int);
  while (key != 0) {
    pushIntSet(keys, key);
    key = va_arg(args, int);
  }
  va_end(args);

  struct keyBinding kbind = (struct keyBinding){.keys = keys, .func = func};
  pushKeyBind(kbinds, &kbind);
}

void pushKeyBind(struct keyBindings *kbinds, struct keyBinding *kbind) {
  if (kbinds == 0) {
    Fprintln(stderr, "keybinds are null; abort pushing a keybind");
    return;
  }
  if (kbind == 0) {
    Fprintln(stderr, "keybind is null; abort pushing to keybinds");
    return;
  }

  if (kbinds->size >= kbinds->capacity) {
    struct keyBinding *newKBindArr = (struct keyBinding *)malloc(
        sizeof(struct keyBinding) * kbinds->capacity * 2);
    if (newKBindArr == 0) {
      Fprintln(stderr, "malloc failed for dynamic array kbinds");
      exit(EXIT_FAILURE);
    }
    memcpy(newKBindArr, kbinds->kbind,
           sizeof(struct keyBinding) * kbinds->capacity);
    free(kbinds->kbind);
    kbinds->kbind = newKBindArr;
    kbinds->capacity *= 2;
  }

  kbinds->kbind[kbinds->size++] =
      (struct keyBinding){.keys = kbind->keys, .func = kbind->func};
}

void executeKeyBind(struct keyBindings *kbinds, struct keyState *ks,
                    struct hakaContext *haka) {
  if (kbinds == 0) {
    Fprintln(stderr, "keybinds are null; abort executing a keybind");
    return;
  }
  if (ks == 0) {
    Fprintln(stderr, "keystatus is null; abort executing a keybind");
    return;
  }

  haka->served = false;
  for (int i = 0; i < kbinds->size; i++) {
    struct keyBinding kbind = kbinds->kbind[i];
    struct IntSet *keys = kbind.keys;
    int j;
    for (j = 0; j < keys->size && ks->keyPress[keys->set[j]]; j++)
      ;
    if (j != keys->size)
      continue;

    kbind.func(haka);

    // Issue: After Tofi launches, haka waits for it to return
    // sometimes the KeyUp events are missed by haka resulting
    // in undefined behaviour.
    //
    // Clean up after serving can fix it. Need a better way to
    // do this
    haka->served = true;
    for (j = 0; j < keys->size; j++)
      ks->keyPress[keys->set[j]] = false;
    for (int i = 0; i < ks->activationCombo->size; i++)
      ks->keyPress[ks->activationCombo->set[i]] = false;
    return;
  }
}

void switchFile(struct hakaContext *haka) {
  contextCheck(haka);

  printf("CTRL + ALT + M detected!\n");
  printf("Launching tofi\n");
  printf("tofi.cfg path: %s\n", haka->config->tofiCfg);

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

void writeToFile(struct hakaContext *haka) {
  contextCheck(haka);

  printf("CTRL + ALT + C detected!\n");
  printf("Dispatching request to get primary selection\n");

  getPrimarySelection(haka);
  openNotesFile(haka);

  writeFP2FD(haka);

  eventHandlerEpilogue(haka);
}

void writePointToFile(struct hakaContext *haka) {
  contextCheck(haka);

  printf("CTRL + ALT + P detected!\n");
  printf("Dispatching request to get primary selection\n");

  getPrimarySelection(haka);
  openNotesFile(haka);

  write(haka->fdNotesFile, "- ", 2);

  writeFP2FD(haka);

  eventHandlerEpilogue(haka);
}

void openFile(struct hakaContext *haka) {
  contextCheck(haka);

  printf("CTRL + ALT + O detected!\n");
  printf("Opening current note in editor\n");

  pid_t pid = fork();
  if (pid < 0) {
    fprintf(stderr, "unable to create a fork");
    return;
  }
  if (pid == 0) {
    printf("Executing %s %s -e %s %s\n", haka->config->terminal,
           haka->config->terminal, haka->config->editor, haka->notesFile);
    execlp(haka->config->terminal, haka->config->terminal, "-e",
           haka->config->editor, haka->notesFile, NULL);
    perror("execlp failed to launch note");
    exit(1);
  }
  haka->childCount++;

  eventHandlerEpilogue(haka);
}

FILE *getPrimarySelection(struct hakaContext *haka) {
  contextCheck(haka);

  haka->fp = popen("wl-paste -p", "r");
  if (haka->fp == NULL) {
    perror("popen error.");
    exit(1);
  }
  return haka->fp;
}

int openNotesFile(struct hakaContext *haka) {
  contextCheck(haka);

  haka->fdNotesFile = open(haka->notesFile, O_RDWR | O_CREAT | O_APPEND, 0666);
  if (haka->fdNotesFile < 0) {
    char errStr[BUFSIZE];
    sprintf(errStr, "Cannot open %s", haka->notesFile);
    perror(errStr);
    exit(1);
  }
  return haka->fdNotesFile;
}

size_t writeFP2FD(struct hakaContext *haka) {
  contextCheck(haka);

  size_t bytes = 0;
  char buf[BUFSIZE];
  while (fgets(buf, BUFSIZE, haka->fp)) {
    buf[strcspn(buf, "\n")] = 0;
    bytes += strlen(buf);
    printf("%ld %s", strlen(buf), buf);
    write(haka->fdNotesFile, buf, strlen(buf));
  }
  write(haka->fdNotesFile, "\n", 1);
  return bytes;
}

FILE *triggerTofi(struct hakaContext *haka) {
  contextCheck(haka);

  char cmd[BUFSIZE * 2], basecmd[BUFSIZE * 2];
  snprintf(basecmd, BUFSIZE * 2, "ls %s -Ap1 | grep -v / | tofi -c %s",
           haka->config->notesDir, haka->config->tofiCfg);
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
