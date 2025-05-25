#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libevdev-1.0/libevdev/libevdev.h>
#include <linux/input-event-codes.h>
#include <sys/select.h>

#include "haka.h"

int main() {
  init();

  struct libevdev *dev = NULL;
  struct IntSet *set = NULL;

  getKbdEvents(&set);
  printf("------\n");

  char kbd[BUFSIZE];
  int fds[set->size], fd;
  struct libevdev *devs[set->size];
  for (int i = 0; i < set->size; i++) {
    snprintf(kbd, BUFSIZE, "/dev/input/event%d", set->set[i]);
    printf("Opening: %s\n", kbd);

    fd = open(kbd, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
      perror("Failed to open device");
      return 1;
    }

    libevdev_new_from_fd(fd, &dev);
    printf("Device: %s\n", libevdev_get_name(dev));
    printf("Listening for key events...\n");
    fds[i] = fd;
    devs[i] = dev;
  }

  struct keyStatus *ks;
  initKeyStatus(&ks);
  struct input_event ev;
  fd_set fdSet;
  int maxFd = 0;
  while (1) {
    FD_ZERO(&fdSet);

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
        printf("Key: %s", libevdev_event_code_get_name(ev.type, ev.code));
        switch (ev.value) {
        case 1:
          printf(" key down\n");
          setKeyStatus(ks, ev.code);
          break;

        case 2:
          printf(" pressed\n");
          break;

        default:
          printf(" key up %d\n", ev.value);
          resetKeyStatus(ks, ev.code);
        }
        if (ks->Ctrl && ks->Alt && ks->C) {
          printf("CTRL + ALT + C detected!\n");
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

int getKbdEvents(struct IntSet **set) {

  DIR *dir = opendir("/dev/input/by-path/");
  if (dir == NULL) {
    perror("Failed to open directory");
    exit(1);
  }
  printf("ref for /dev/input/by-path/ created @ %p\n", dir);

  *set = NULL;
  initSet(set, 1);
  struct dirent *entry = NULL;

  char symlinkTo[BUFSIZE], absPath[BUFSIZE];

  while ((entry = readdir(dir)) != NULL) {
    size_t dNameLen = strlen(entry->d_name);
    if (dNameLen < 9) {
      continue;
    }

    char compVal[9];
    strncpy(compVal, entry->d_name + dNameLen - 9, 9);
    if (strcmp("event-kbd", compVal) != 0) {
      continue;
    }

    strcpy(absPath, "/dev/input/by-path/");
    strcat(absPath, entry->d_name);
    ssize_t len = readlink(absPath, symlinkTo, BUFSIZE);
    if (len == -1) {
      continue;
    }
    symlinkTo[len] = '\0';

    printf("Entry: /dev/input/by-path/%s\t\tis a symlink to -> %s\n",
           entry->d_name, symlinkTo);
    pushSet(*set, atoi((symlinkTo + strlen(symlinkTo) - 1)));
  }

  closedir(dir);

  int size = (*set)->size;
  printf("eventX is a keyboard Event | X =  ");
  while (size-- > 0) {
    printf("%d, ", (*set)->set[size]);
  }
  printf("\b\b;\n");

  return 0;
}

int initSet(struct IntSet **set, int capacity) {
  *set = (struct IntSet *)malloc(sizeof(struct IntSet));
  if (set == NULL) {
    return 1;
  }
  (*set)->size = 0;
  (*set)->capacity = capacity;
  (*set)->set = (int *)malloc(sizeof(int) * capacity);

  return 0;
}

int pushSet(struct IntSet *set, int val) {
  if (set == NULL) {
    exit(1);
  }
  int size = set->size;
  while (size-- > 0) {
    if (val == set->set[size]) {
      return 1;
    }
  }

  if (set->size >= set->capacity) {
    dynamicInc(set);
  }
  set->set[set->size++] = val;

  return 0;
}

int dynamicInc(struct IntSet *set) {
  int newCapacity = set->capacity * 2;
  int *newArr = (int *)malloc(sizeof(int) * newCapacity);
  if (newArr == NULL) {
    return 1;
  }

  int size = set->size;
  while (size-- >= 0) {
    newArr[size] = set->set[size];
  }

  set->capacity = newCapacity;
  free(set->set);
  set->set = newArr;
  return 0;
}

void forceSudo() {
  if (!getuid()) {
    return;
  }
  printf("proc no root\nForcing root...\n");

  char buf[100];
  int rl = readlink("/proc/self/exe", buf, 100);
  if (rl < 0) {
    perror("readlink error:");
    exit(1);
  }
  buf[rl] = '\0';
  int ex = execlp("sudo", "sudo", buf, NULL);
  printf("Failed to restart application `%s`\n, error: %d", buf, ex);
  perror("exec failed: ");
  exit(1);
}

int initKeyStatus(struct keyStatus **ks) {
  *ks = (struct keyStatus *)malloc(sizeof(struct keyStatus));
  if (*ks == NULL) {
    return 1;
  }
  (*ks)->Ctrl = false;
  (*ks)->Alt = false;
  (*ks)->C = false;
  return 0;
}

int checkPackage(const char *pkgName) {
  char cmd[512];
  sprintf(cmd, "which %s > /dev/null 2>&1", pkgName);

  int retVal = system(cmd);
  if (retVal == -1) {
    printf("system() failed to execute.");
    perror("system err: ");
    exit(1);
  }
  if (WEXITSTATUS(retVal) != 0) {
    printf("Cannot find %s in PATH\n", pkgName);
    printf("which %s returned %d\n", pkgName, WEXITSTATUS(retVal));
    return 1;
  }
  return 0;
}

void init() {
  if (checkPackage("wl-copy") || checkPackage("wl-paste")) {
    printf("please install wl-clipboard: sudo pacman -S wl-clipboard\n");
    exit(1);
  }
  forceSudo();
}
