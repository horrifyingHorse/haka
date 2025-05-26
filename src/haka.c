#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <linux/input-event-codes.h>
#include <sys/select.h>
#include <unistd.h>

#include "haka.h"
#include "hakaEventHandler.h"
#include "hakaUtils.h"

int main() {
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
                 0 &&
             ev.type == EV_KEY) {
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
        }

        if (keyCombination(ks, C) && !haka->served) {
          writeToFile(haka);
        }
      }
    }
  }

  for (int i = 0; i < set->size; i++) {
    libevdev_free(devs[i]);
    close(fds[i]);
  }

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

  return ks;
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
  strCpyCat(haka->notesDir, haka->execDir, "/notes");
  snprintf(haka->notesFile, BUFSIZE * 2, "%s/%s", haka->notesDir,
           haka->notesFileName);
  strCpyCat(haka->tofiCfg, haka->execDir, "/tofi.cfg");

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
