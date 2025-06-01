#!/bin/sh

FILE=haka.service
HAKA=haka.out

write() {
  echo $@ >>$FILE
}

checkfile() {
  if [[ -f $FILE ]]; then
    echo "$FILE already exists. use -f to force"
    exit 1
  fi
}

openfile() {
  rm -rf $FILE
}

silent() {
  $@ >/dev/null 2>&1
}

if [[ "$1" != "-f" ]]; then
  checkfile
fi
openfile

write [Unit]
write Description=Haka Keyboard Listener Daemon
write
write [Service]
write Type=simple
write ExecStart=$(pwd)/$HAKA
write WorkingDirectory=$(pwd)
# Do not need the User field
# it is only needed when the daemon is running as a root
# daemon. For a --user daemon, specifying User field
# causes it to break severely
# write User=$(whoami)
write Restart=on-failure
write StandardOutput=journal
write StandardError=journal
write
write [Install]
write WantedBy=default.target

silent systemctl --user status $FILE
silent systemctl --user stop $FILE
silent systemctl --user disable $FILE
ln -s $(pwd)/$FILE ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now $FILE
systemctl --user status $FILE

# To check logs:
# $ journalctl --user -u $FILE -f
