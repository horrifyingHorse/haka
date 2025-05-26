#ifndef HAKA_H
#define HAKA_H

#include <grp.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <linux/types.h>
#include <stdbool.h>
#include <stdlib.h>

const size_t BUFSIZE = 1024;

void init(char* execDir);
void switchGrp(gid_t* curGID, const char* grpnam);

struct keyStatus {
  bool Ctrl;
  bool Alt;
  bool C;
  bool M;
};
int initKeyStatus(struct keyStatus** ks);

#define setKeyStatus(ks, code) \
  switch (code) {              \
    case KEY_LEFTCTRL:         \
    case KEY_RIGHTCTRL:        \
      ks->Ctrl = true;         \
      break;                   \
    case KEY_LEFTALT:          \
    case KEY_RIGHTALT:         \
      ks->Alt = true;          \
      break;                   \
    case KEY_C:                \
      ks->C = true;            \
      break;                   \
    case KEY_M:                \
      ks->M = true;            \
      break;                   \
  }

#define resetKeyStatus(ks, code) \
  switch (code) {                \
    case KEY_LEFTCTRL:           \
    case KEY_RIGHTCTRL:          \
      ks->Ctrl = false;          \
      break;                     \
    case KEY_LEFTALT:            \
    case KEY_RIGHTALT:           \
      ks->Alt = false;           \
      break;                     \
    case KEY_C:                  \
      ks->C = false;             \
      break;                     \
    case KEY_M:                  \
      ks->M = false;             \
      break;                     \
  }

int checkPackage(const char* pkgName);

struct IntSet {
  int* set;
  int size;
  int capacity;
};
int initSet(struct IntSet** set, int capacity);
int pushSet(struct IntSet* set, int val);
int dynamicInc(struct IntSet* set);

void forceSudo();
int getKbdEvents(struct IntSet** set);

#endif
