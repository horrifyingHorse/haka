#include <errno.h>
#include <linux/input-event-codes.h>
#include <signal.h>
#include <stdarg.h>
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

  // Signals: SIGINT and SIGTERM
  struct sigaction sa;
  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGINT, &sa, 0);
  sigaction(SIGTERM, &sa, 0);

  // clang-format off
  struct IntSet      *set    = initIntSet(2);
  struct hakaContext *haka   = initHaka();
  struct keyState    *ks     = initKeyState(SUPPORTED_KEYS);
  struct keyBindings *kbinds = initKeyBindings(2);
  // clang-format on

  // Load Key Binds (./bindings.c)
  loadBindings(kbinds, ks);

  gid_t curGrp;
  switchGrp(&curGrp, "input");

  getKbdEvents(set);
  int fds[set->size];
  struct libevdev *devs[set->size];
  openKbdDevices(set, fds, devs);

  switchGrp(&curGrp, NULL);

  while (live) {
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
      if (errno == EINTR) {
        continue;
      } else {
        perror("select");
        break;
      }
    }

    for (int i = 0; i < set->size; i++) {
      if (!FD_ISSET(fds[i], &fdSet))
        continue;

      while (libevdev_next_event(devs[i], LIBEVDEV_READ_FLAG_NORMAL, &ev) ==
             0) {
        if (ev.type != EV_KEY)
          continue;

        handleKeyEvent(ks, ev.code, ev.value);

        if (activated(ks)) {
          executeKeyBind(kbinds, ks, haka);
        }

        if (haka->childCount > 0) {
          reapChild(haka);
        }
      }
    }
  }

  Println("Clean up initiated");
  for (int i = 0; i < set->size; i++) {
    libevdev_free(devs[i]);
    close(fds[i]);
  }
  free(set);
  free(haka->config);
  free(haka);

  free(ks->keyPress);
  free(ks->activationCombo);
  free(ks);

  free(kbinds->kbind->keys);
  free(kbinds->kbind);
  free(kbinds);
  Println("Clean up complete");

  return 0;
}

struct keyState *initKeyState(int16_t size) {
  struct keyState *ks = (struct keyState *)malloc(sizeof(struct keyState));
  if (ks == 0) {
    Fprintln(stderr, "malloc failed for keymap");
    exit(EXIT_FAILURE);
  }

  ks->size = size;
  ks->activationCombo = initIntSet(2);
  ks->keyPress = (bool *)calloc(size, sizeof(bool));
  if (ks->keyPress == 0) {
    Fprintln(stderr, "calloc failed for keymap");
    exit(EXIT_FAILURE);
  }

  return ks;
}

void handleKeyEvent(struct keyState *ks, int evCode, int evVal) {
  if (ks == 0) {
    Fprintln(stderr, "keyState pointer cannot be null to handle keys");
    return;
  }
  if (evCode < 0 || evCode > ks->size) {
    Fprintln(stderr, "event code out of bounds = %d", evCode);
    return;
  }
  if (evVal >= 2 || evVal < 0) {
    // Fprintln(stderr, "event val = %d; event ignored.", evVal);
    return;
  }
  ks->keyPress[evCode] = evVal;
}

void setActivationCombo(struct keyState *ks, ...) {
  if (!resetActivationCombo(ks)) {
    Fprintln(stderr, "abort set activation combo");
    return;
  }

  va_list args;
  va_start(args, ks);
  int key;
  while ((key = va_arg(args, int)) != -1) {
    if (key < 0 || key >= ks->size) {
      Fprintln(stderr, "Invalid key to set for activation combo = %d", key);
      exit(EXIT_FAILURE);
    }
    pushIntSet(ks->activationCombo, key);
  }
}

bool resetActivationCombo(struct keyState *ks) {
  if (ks == 0) {
    Fprintln(stderr,
             "keystatus object cannot be null; activaition key reset ignored");
    return false;
  }

  if (ks->activationCombo != 0) {
    free(ks->activationCombo);
    ks->activationCombo = 0;
  }
  ks->activationCombo = initIntSet(2);
  return true;
}

bool activated(struct keyState *ks) {
  for (int i = 0; i < ks->activationCombo->size; i++) {
    if (!ks->keyPress[ks->activationCombo->set[i]]) {
      return false;
    }
  }
  return true;
}

int parseConf(struct confVars *conf, char *line) {
  if (line == NULL || conf == NULL) {
    return -1;
  }

  line = trim(line);
  if (line[0] == '#')
    return 0;

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
      char *home = getEnvVar("$HOME");
      var = var + 1;
      strcat(home, val);
      val = home;
    }
    strcpy(conf->notesDir, val);
  } else if (strcmp(var, "tofi-cfg") == 0) {
    strcpy(conf->tofiCfg, val);
  } else if (strcmp(var, "terminal") == 0) {
    strcpy(conf->terminal, val);
  }

  return 0;
}

struct confVars *initConf(struct hakaContext *haka) {
  haka->config = (struct confVars *)malloc(sizeof(struct confVars));
  struct confVars *conf = haka->config;

  strcpy(conf->editor, "/usr/bin/nvim");
  strCpyCat(conf->notesDir, haka->execDir, "/notes");
  strCpyCat(conf->tofiCfg, haka->execDir, "/tofi.cfg");

  char *term = getEnvVar("$TERM");
  if (term == NULL) {
    fprintf(stderr, "Cannot get var $TERM, recieved NULL\n");
  }
  if (strcmp(term, "") == 0) {
    fprintf(stderr, "Cannot get var $TERM, recieved '%s'\n", term);
    strcpy(conf->terminal, "alacritty");
  } else {
    strcpy(conf->terminal, term);
  }

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

  free(term);
  fclose(file);
  return conf;
}

struct hakaContext *initHaka() {
  if (checkPackage("wl-copy") || checkPackage("wl-paste")) {
    printf("please install wl-clipboard: sudo pacman -S wl-clipboard\n");
    exit(1);
  }
  if (checkPackage("tofi")) {
    printf("please install tofi: yay -S tofi\n");
    exit(1);
  }

  struct hakaContext *haka =
      (struct hakaContext *)malloc(sizeof(struct hakaContext));

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

void getExeDir(struct hakaContext *haka) {
  contextCheck(haka);

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

void getPrevFile(struct hakaContext *haka) {
  contextCheck(haka);

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

void reapChild(struct hakaContext *haka) {
  pid_t pid;
  while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
    printf("Reaped child proc %d\n", pid);
    haka->childCount--;
  }
}
