# `Haka`
Simply select text, press the key combination, and add it to your file, without cluttering your clipboard buffer with one-time-use text. All using ***`Haka`***, a low level global keyboard event listener for Wayland.

## build
Dependencies
- *[`libevdev`](https://gitlab.freedesktop.org/libevdev/libevdev)*
- *[`wl-clipboard`](https://github.com/bugaevc/wl-clipboard)*
- *[`tofi`](https://github.com/philj56/tofi)*

*[`build.sh`](https://github.com/horrifyingHorse/haka/blob/main/build.sh)* includes the installation of all the dependencies, but only for arch and debian based distros. For any other distribution, kindly install the aforementioned dependencies.

### For Arch / Debian based distros:
From your favourite terminal, execute the *build.sh* script for installation
```python
chmod +x build.sh
./build.sh
```

### Permissions
To access the *`input`* group and access the event devices using *libevdev*, one needs root privileges. Although running *`haka`* as the root user has many issues, the most critiacl being the current user session, including the *primary clipboard*, is not available when running as root.

To resolve this issue, we use the *[`capabilities(7)`](https://man7.org/linux/man-pages/man7/capabilities.7.html)* to grant *haka* the permission to change its *group ID* to *`input`*. Now *haka* can run as a user process and still be able to access the *evdev*. **Credit** for this workaround goes to ***[`this blog`](https://suricrasia.online/blog/turning-a-keyboard-into/#:~:text=Running%20external%20programs)*.**

> [!NOTE]
> *build.sh* handles this implicitly. If you used *build.sh* to build *haka*, this has been performed already.

```python
sudo setcap "cap_setgid=eip" ./haka.out
./haka.out     # no sudo required while running
```

## `haka.service`
If you want to use *haka* as a *[`new-style systemd`](https://www.freedesktop.org/software/systemd/man/latest/daemon.html#New-Style%20Daemons)* daemon to run in the background, use *[`daemon.sh`](https://github.com/horrifyingHorse/haka/blob/main/daemon.sh)* to generate the unit file for systemd.

*daemon.sh* creates the unit file required to run as a `--user` daemon and it creates a symlink of the unit file to `~/.config/systemd/user/`

> [!NOTE]
> Make sure to execute *daemon.sh* from the directory containing *`haka.out`*. If you renamed the executable, edit the `HAKA` variable in `daemon.sh`

```python
chmod +x daemon.sh
./daemon.sh
```

Check logs:
```python
journalctl --user -u haka.service -f
```

## Guide
- The files displayed in the selection menu is the `notes/` directory and can be found in the directory containing *haka* executable
- The file is opened by default in *[`neovim`](https://github.com/neovim/neovim)*

### Default KeyBinds
| Key Combination | Binded Task |
|-----------------|-------------|
| *`Ctrl+Alt + C`* | Paste the current selection to the current file |
| *`Ctrl+Alt + P`* | Paste the current selection as an unordered list item to the current file |
| *`Ctrl+Alt + M`* | Opens the file selection menu |
| *`Ctrl+Alt + O`* | Opens the file in neovim |

## TODO
- [ ] Switch to gtk(?): to reduce dependencies.
- [ ] Add a config file option for vars.
- [ ] Ignore newlines in selection(?)
