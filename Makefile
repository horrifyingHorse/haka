# === Configurable Variables ===
CC := gcc
CFLAGS := -I./include -I./libevdev-1.12.1
LDLIBS := ./libevdev-1.12.1/libevdev/.libs/libevdev.a

SRC := $(wildcard src/*.c)
OUT := haka.out

# === Targets ===
all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(OUT)

.PHONY: all clean
