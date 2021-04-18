#!/bin/sh

apt update
apt install -y build-essential git cmake libavcodec-dev libavutil-dev libavformat-dev libavfilter-dev libavdevice-dev \
               libsdl2-dev libcurl4-openssl-dev libnss3 libcups2 libatk-bridge2.0-0 libatk1.0-0 libxcomposite1 \
               curl

# prepare libffmpeg.so
curl http://security.ubuntu.com/ubuntu/pool/universe/c/chromium-browser/chromium-codecs-ffmpeg-extra_89.0.4389.90-0ubuntu0.18.04.2_amd64.deb -o codecs.deb
dpkg -i codecs.deb
cp /usr/lib/chromium-browser/libffmpeg.so /usr/local/lib
ldconfig

# clone repository and build
git clone https://github.com/Zabrimus/vdr-osr-browser.git
cd vdr-osr-browser/
git checkout encoding

# starts build
make -j 4

# create archive
mv Release vdr-osr-browser-encoding
tar -cJf ../vdr-osr-browser-encoding.tar.bz2 vdr-osr-browser-encoding


