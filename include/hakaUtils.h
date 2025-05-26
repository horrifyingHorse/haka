#ifndef HAKA_UTILS_H
#define HAKA_UTILS_H

#include <grp.h>
#include <string.h>

#include <libevdev-1.0/libevdev/libevdev.h>

#include "hakaBase.h"

struct IntSet {
  int* set;
  int size;
  int capacity;
};
struct IntSet* initIntSet(int capacity);
int pushIntSet(struct IntSet* set, int val);
int dynamicInc(struct IntSet* set);

int checkPackage(const char* pkgName);
void forceSudo();

void switchGrp(gid_t* curGID, const char* grpnam);

int getKbdEvents(struct IntSet* set);
int openKbdDevices(struct IntSet* set, int* fds, struct libevdev* devs[]);

#endif  // !HAKA_UTILS_H
