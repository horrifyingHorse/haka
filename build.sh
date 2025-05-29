#!/bin/sh

YELLOW="\033[1;33m"
RESET="\033[0m"
TOINSTALL=true
PKGMNGR=pacman
INSTALL=-Sy
TOCLEAR=false

if grep -qi 'ID=arch' /etc/os-release; then
  PKGMNGR=pacman
  INSTALL=-Sy
elif command -v apt >/dev/null 2>&1; then
  PKGMNGR=apt
  INSTALL=install
else
  TOINSTALL=false
  echo -e $YELLOW"please make sure you have 'wl-clipboard' and 'tofi' installed!\n$RESET"
fi

noDependency() {
  echo -e "cannot find dependency $YELLOW$1$RESET !\n"
}

getLibevdev() {
  if [[ ! -f ./libevdev-1.12.1.tar.xz ]]; then
    wget https://www.freedesktop.org/software/libevdev/libevdev-1.12.1.tar.xz
  fi
  tar -xvf libevdev-1.12.1.tar.xz
  cd libevdev-1.12.1
  ./configure --enable-static --disable-shared
  make -j$(nproc)
  cd ../
  TOCLEAR=true
}

systemDependencies() {
  if [ "$1" = false ]; then
    return
  fi

  if ! command -v wl-paste >/dev/null 2>&1; then
    noDependency wl-clipboard
    sudo $PKGMNGR $INSTALL wl-clipboard
  fi

  if ! command -v tofi >/dev/null 2>&1; then
    noDependency tofi
    if command -v yay >/dev/null 2>&1; then
      yay -Sy tofi
    else
      echo -e "Please install$YELLOW tofi$RESET"
    fi
  fi
}

if [[ ! -d libevdev-1.12.1 ]]; then
  noDependency libevdev
  getLibevdev
fi

systemDependencies $TOINSTALL

if [ "$TOCLEAR" = true ]; then
  echo -e "\033[2J\033[H"
fi

make
sudo setcap "cap_setgid=eip" ./haka.out
mkdir -p notes
./haka.out
