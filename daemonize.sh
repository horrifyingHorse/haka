#!/bin/sh

FILE=haka.service
LINE=0

write() {
  echo $@ >>$FILE
}

checkfile() {
  if [[ -f $FILE ]]; then
    echo "$FILE already exists. use -f to force"
  fi
}

openfile() {
  rm -rf $FILE
}

if [[ "$1" != "-f" ]]; then
  checkfile
fi
openfile

write [Unit]
write Description=Haka Keyboard Listener Daemon
write After=network.target
write
write [Service]
write Type=simple
write ExecStart=$(pwd)/haka.out
write WorkingDircetory=$(pwd)
write User=$(whoami)
write Restart=on-failure
write
write [Install]
write WantedBy=deault.target
