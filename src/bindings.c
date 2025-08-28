#include "haka.h"
#include "hakaEventHandler.h"
#include "hakaUtils.h"
#include <linux/input-event-codes.h>
#include <stdlib.h>

// clang-format off
void loadBindings(struct keyBindings *kbinds, struct keyState* ks) {
  if (kbinds == 0 || ks == 0) {
    Fprintln(stderr, "cannot bind keys to null");
    exit(EXIT_FAILURE);
  }

  // Activation Combo
  ActivationCombo(KEY_LEFTCTRL, KEY_LEFTALT);

  // Key Binds
  Bind(writeToFile,       KEY_C);
  Bind(switchFile,        KEY_M);
  Bind(openFile,          KEY_O);
  Bind(writePointToFile,  KEY_DOT);
  Bind(sendNewlineToFile, KEY_N);
}
