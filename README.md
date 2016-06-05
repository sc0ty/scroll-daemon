# Scroll Daemon
Use any mouse device (touchpad, trackpad etc.) for scrolling only.

## But why?
In my Thinkpad there is both touchpad and trackpad installed. I am controlling cursor with trackpad. With this daemon I can now use touchpad for scrolling only.

## Other solutions
You can configure your trackpad scrolling zones to be 100% area of the device. I was using this solution, but since Google Chrome 49.0 scroll started acting funny. It was main reason to make this daemon.

## Compilation & installation
Under any Linux distribution:
```
make
make install
```
It will install daemon in "/usr/local/bin". You could change destination folder using `PREFIX` variable:
```
make install PREFIX=/my/dest/dir
```

## Usage
1. First you need to find mouse device corresponding to your device. It will be one of `/dev/input/mouseX`. Try `cat /dev/input/mouse0` and move cursor to see if it will generate something.
2. Disable this device in your window manager configuration or in `xorg.conf`.
3. Run daemon (as root): `scroll-daemon /dev/input/mouseX 5 5 -rlm`. It will register new mouse `psmouse-scroll` in your system.
4. Make sure that new mouse is enabled in your window manager.
5. Test scroll speed, change options as you wish. For more information run `scroll-daemon --help`.
6. You may wish to add this to `/etc/rc.local` (for global autostart) or to your user profile autostart. Add `&` at the end of the line to make it run in background. Note that user starting this daemon must have read rights to mouse device and write rights to `/dev/uinput` (it usually means root).
7. Consider using one of `/dev/input/by-id/*-mouse` or `/dev/input/by-path/*-mouse` devices. It will assure that your device will be identified correctly. Your devices could be numbered differently. 

