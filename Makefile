# === Configurable Variables ===
LIBEVDEV_VERSION := 1.12.1
LIBEVDEV_DIR := libevdev-$(LIBEVDEV_VERSION)
LIBEVDEV_TAR := $(LIBEVDEV_DIR).tar.xz
LIBEVDEV_URL := https://www.freedesktop.org/software/libevdev/$(LIBEVDEV_TAR)
LDLIBS := ./$(LIBEVDEV_DIR)/libevdev/.libs/libevdev.a
CC := gcc
CFLAGS := -I./include -I./libevdev-$(LIBEVDEV_VERSION)
SRC := $(wildcard src/*.c)
OUT := haka.out

.PHONY: all clean

# === Targets ===
all: $(OUT)

$(OUT): $(SRC) $(LDLIBS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(LDLIBS): $(LIBEVDEV_TAR)
	tar -xf $(LIBEVDEV_TAR)
	cd $(LIBEVDEV_DIR) && ./configure --enable-static --disable-shared
	$(MAKE) -C $(LIBEVDEV_DIR)

$(LIBEVDEV_TAR):
	wget $(LIBEVDEV_URL)

clean:
	rm -f $(OUT)
	rm -rf prevFile.txt
	rm -rf $(LIBEVDEV_DIR)

