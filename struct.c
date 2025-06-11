#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define Fprintln(buf, ...) \
  fprintf(buf, __VA_ARGS__);\
  fprintf(buf, "\n")

#define Println(...) \
  printf(__VA_ARGS__);\
  printf("\n")

struct keyStatus {
  int16_t size;
  bool* keyPress; 
};

struct keyStatus* initKeyStatus(int16_t size) {
  struct keyStatus* ks = (struct keyStatus*)malloc(sizeof(struct keyStatus));
  if (ks == 0) {
    Fprintln(stderr, "malloc failed for keymap");
    exit(EXIT_FAILURE);
  }
  ks->size = size;
  ks->keyPress = (bool*)calloc(size, sizeof(bool));
  if (ks->keyPress == 0) {
    Fprintln(stderr, "calloc failed for keymap");
    exit(EXIT_FAILURE);
  }
  return ks;
}

void handleKeyEvent(struct keyStatus* ks, int evCode, int evVal) {
  if (ks == 0) {
    Fprintln(stderr, "keyStatus pointer cannot be null to handle keys");
    return;
  }
  if (evCode < 0 || evCode > ks->size) {
    Fprintln(stderr, "event code out of bounds = %d", evCode);
    return;
  }
  if (evVal >= 2 || evVal < 0) {
    Fprintln(stderr, "event val = %d; event ignored.", evVal);
    return;
  } 
  ks->keyPress[evCode] = evVal;
}

int main() {
  struct keyStatus* ks = initKeyStatus(249);
  Println("a=%d", ks->keyPress[30]);
  handleKeyEvent(ks, 30, 1);
  Println("a=%d", ks->keyPress[30]);
  return 0;
}