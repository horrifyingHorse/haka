#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "haka.h"
#include "hakaBase.h"
#include "hakaEventHandler.h"
#include "hakaUtils.h"

int main() {
  // Need to disable full buffering and switch to
  // line buffering to make sure journal catches
  // all the logs.
  setvbuf(stdout, NULL, _IOLBF, 0);
  setvbuf(stderr, NULL, _IOLBF, 0);

  struct IntSet *set = initIntSet(2);
  struct hakaStatus *haka = initHaka();
  struct keyStatus *ks = initKeyStatus();

  gid_t curGrp;
  switchGrp(&curGrp, "input");

  getKbdEvents(set);
  int fds[set->size];
  struct libevdev *devs[set->size];
  openKbdDevices(set, fds, devs);

  switchGrp(&curGrp, NULL);

  while (1) {
    fd_set fdSet;
    struct input_event ev;

    FD_ZERO(&fdSet);

    int maxFd = 0;
    for (int i = 0; i < set->size; i++) {
      FD_SET(fds[i], &fdSet);
      if (fds[i] > maxFd)
        maxFd = fds[i];
    }

    int ready = select(maxFd + 1, &fdSet, NULL, NULL, NULL);
    if (ready < 0) {
      perror("select");
      exit(1);
    }

    for (int i = 0; i < set->size; i++) {
      if (!FD_ISSET(fds[i], &fdSet))
        continue;

      while (libevdev_next_event(devs[i], LIBEVDEV_READ_FLAG_NORMAL, &ev) ==
             0) {
        if (ev.type != EV_KEY)
          continue;

        // printf("Key: %s\n", libevdev_event_code_get_name(ev.type, ev.code));
        // fflush(stdout);

        switch (ev.value) {
        case 1:
          setKeyStatus(ks, ev.code);
          break;

        case 2:
          break;

        default:
          resetKeyStatus(ks, ev.code);
          haka->served = false;
          break;
        }

        if (keyCombination(ks, M) && !haka->served) {
          switchFile(haka);

          // Issue: After Tofi launches, haka waits for it to return
          // sometimes the KeyUp events are missed by haka resulting
          // in undefined behaviour.
          //
          // Clean up after serving can fix it. Need a better way to
          // do this
          ks->M = false;
          ks->Alt = false;
          ks->Ctrl = false;
        }

        if (keyCombination(ks, C) && !haka->served) {
          writeToFile(haka);
        }

        if (keyCombination(ks, P) && !haka->served) {
          writePointToFile(haka);
        }

        if (keyCombination(ks, O) && !haka->served) {
          openFile(haka);
        }

        if (haka->childCount > 0) {
          reapChild(haka);
        }
      }
    }
  }

  for (int i = 0; i < set->size; i++) {
    libevdev_free(devs[i]);
    close(fds[i]);
  }
  free(set);
  free(haka->config);
  free(haka);
  free(ks);

  return 0;
}

struct keyStatus *initKeyStatus() {
  struct keyStatus *ks = (struct keyStatus *)malloc(sizeof(struct keyStatus));
  if (ks == NULL) {
    perror("Unable to allocate memory for keyStatus.");
    exit(1);
  }

  ks->Ctrl = false;
  ks->Alt = false;
  ks->C = false;
  ks->M = false;
  ks->O = false;
  ks->P = false;

  return ks;
}

int parseConf(struct confVars *conf, char *line) {
  if (line == NULL || conf == NULL) {
    return -1;
  }

  char *c = line;
  for (; *c != '\0' && *c != '='; c++)
    ;
  if (*c != '=') {
    return 1;
  }

  *c = '\0';
  char *var = line;
  char *val = ++c;
  var = trim(var);
  val = trim(val);

  if (strlen(val) >= 3 && val[0] == '$' && val[1] == '(' &&
      val[strlen(val) - 1] == ')') {
    val = val + 2;
    val[strlen(val) - 1] = '\0';
    FILE *sh = popen(val, "r");
    if (sh == NULL) {
      perror("Unable to popen: ");
      exit(1);
    }

    strcpy(val, "");
    char res[BUFSIZE];
    while (fgets(res, BUFSIZE, sh)) {
      res[strcspn(res, "\n")] = '\0';
      strcat(val, res);
    }

    fclose(sh);
  }

  if (strcmp(var, "editor") == 0) {
    strcpy(conf->editor, val);
  } else if (strcmp(var, "notes-dir") == 0) {
    if (val[strlen(val) - 1] == '\\' || val[strlen(val) - 1] == '/') {
      val[strlen(val) - 1] = '\0';
    }
    if (val[0] == '~' && (val[1] == '/' || val[1] == '\\')) {
      char *home = getEnvVar("HOME");
      var = var + 1;
      strcat(home, val);
      val = home;
    }
    strcpy(conf->notesDir, val);
  } else if (strcmp(var, "tofi-cfg") == 0) {
    strcpy(conf->tofiCfg, val);
  }

  return 0;
}

struct confVars *initConf(struct hakaStatus *haka) {
  haka->config = (struct confVars *)malloc(sizeof(struct confVars));
  struct confVars *conf = haka->config;

  strcpy(conf->editor, "nvim");
  strCpyCat(conf->notesDir, haka->execDir, "/notes");
  strCpyCat(conf->tofiCfg, haka->execDir, "/tofi.cfg");

  char configFile[BUFSIZE];
  strCpyCat(configFile, haka->execDir, "/haka.cfg");
  FILE *file = fopen(configFile, "r");
  if (file == NULL) {
    printf("No config file haka.cfg found in execDir: %s\n", haka->execDir);
    return conf;
  }

  char line[BUFSIZE];
  while (fgets(line, BUFSIZE, file)) {
    line[strcspn(line, "\n")] = '\0';
    parseConf(conf, line);
  }

  fclose(file);
  return conf;
}

struct hakaStatus *initHaka() {
  if (checkPackage("wl-copy") || checkPackage("wl-paste")) {
    printf("please install wl-clipboard: sudo pacman -S wl-clipboard\n");
    exit(1);
  }
  if (checkPackage("tofi")) {
    printf("please install tofi: yay -S tofi\n");
    exit(1);
  }

  struct hakaStatus *haka =
      (struct hakaStatus *)malloc(sizeof(struct hakaStatus));

  getExeDir(haka);
  getPrevFile(haka);
  haka->config = initConf(haka);
  buildAbsFilePath(haka);
  haka->fdNotesFile = -1;
  haka->fp = NULL;
  haka->childCount = 0;

  printf("Notes File: %ld %s\n", strlen(haka->notesFile), haka->notesFile);

  // forceSudo();
  //
  // Cannot be the root user for the entire time of execution
  // Reason? wl-clipboard. The dependency, even the clipboard
  // is user dependent. It needs user space vars that are not
  // inhereted when ran as a root.
  //
  // Workaround?
  // We only need root to access the input devices. This, on
  // most linux devices, is already available in a group
  // named "input", (can find by using
  // `$ ls -l /dev/input/event*` ->
  // `crw-rw---- 1 root *input* 13, 64....`)
  // if the proc is in the `input`  grp,  it can  access the
  // input device.
  //
  // refs:
  // https://suricrasia.online/blog/turning-a-keyboard-into/
  // capabilities(7)
  // user_namespaces(7)

  return haka;
}

void getExeDir(struct hakaStatus *haka) {
  statusCheck(haka);

  ssize_t nbytes = readlink("/proc/self/exe", haka->execDir, BUFSIZE);
  if (nbytes < 0) {
    perror("Cannot readlink /proc/self/exe");
    exit(1);
  }
  haka->execDir[nbytes] = '\0';
  printf("Readlink: %s\n", haka->execDir);

  // Strip off the file name
  char *p = &haka->execDir[nbytes];
  while (*p != '/') {
    p--;
  }

  size_t len = p - &haka->execDir[0];
  if (len <= 0) {
    printf("Invalid path to binary %s", haka->execDir);
    exit(1);
  }
  haka->execDir[len] = '\0';

  printf("Dir Path: %s\n", haka->execDir);
}

void getPrevFile(struct hakaStatus *haka) {
  statusCheck(haka);

  strcpy(haka->notesFileName, "notes.txt\0");
  size_t bytes = strlen(haka->notesFileName);
  sprintf(haka->prevFile, "%s/prevFile.txt", haka->execDir);

  haka->fdPrevFile = open(haka->prevFile, O_RDWR, 0666);
  if (haka->fdPrevFile > 0) {
    bytes = read(haka->fdPrevFile, haka->notesFileName, BUFSIZE);
  }

  if (bytes > 0) {
    haka->notesFileName[bytes] = '\0';
  }

  close(haka->fdPrevFile);
}

void reapChild(struct hakaStatus *haka) {
  pid_t pid;
  while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
    printf("Reaped child proc %d\n", pid);
    haka->childCount--;
  }
}
