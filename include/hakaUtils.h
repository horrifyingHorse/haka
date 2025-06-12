#ifndef HAKA_UTILS_H
#define HAKA_UTILS_H

#include <grp.h>
#include <stdio.h>
#include <string.h>

#include <libevdev/libevdev.h>

#include "hakaBase.h"

#define Fprintln(buf, ...)   \
  fprintf(buf, __VA_ARGS__); \
  fprintf(buf, "\n")

#define Println(...)   \
  printf(__VA_ARGS__); \
  printf("\n")

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
char* getEnvVar(const char* var);

void switchGrp(gid_t* curGID, const char* grpnam);

int getKbdEvents(struct IntSet* set);
int openKbdDevices(struct IntSet* set, int* fds, struct libevdev* devs[]);

char* ltrim(char* s);
char* rtrim(char* s);
char* trim(char* s);

#endif  // !HAKA_UTILS_H
