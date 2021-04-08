#!/bin/sh

# launch lxc container, push the build script and starts it
lxc launch images:debian/11 build-osr-browser
lxc file push ./build-browser-debian.sh build-osr-browser/root/

# wait some seconds. The container needs a network interface
sleep 10

lxc exec build-osr-browser -- bash -c "cd /root; ./build-browser-debian.sh"

# pull archive file (which hopefully has been created)
lxc file pull build-osr-browser/root/vdr-osr-browser-encoding.tar.bz2 .
