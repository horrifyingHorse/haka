#!/bin/sh

YELLOW="\033[1;33m"
RESET="\033[0m"

getLibevdev() {
  echo -e "cannot find dependency$YELLOW libevdev$RESET !\n"
  wget https://www.freedesktop.org/software/libevdev/libevdev-1.12.1.tar.xz
  tar -xvf libevdev-1.12.1.tar.xz
  cd libevdev-1.12.1
  ./configure --enable-static --disable-shared
  make -j$(nproc)
  cd ../
}

if [[ ! -d libevdev-1.12.1 ]]; then
  getLibevdev
fi

make
sudo setcap "cap_setgid=eip" ./haka.out
mkdir -p notes
./haka.out
