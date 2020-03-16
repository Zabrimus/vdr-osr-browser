#!/bin/sh

# ffmpeg binary
FFMPEG=/usr/bin/ffmpeg

# input video file, adapt accordingly
VIDEO_IN=somenight.mp4

# command used in browser (unoptimized, test-only, possibly broken)
$FFMPEG -y -r 25 -i $VIDEO_IN -f mpegts -q:v 10 -an -vcodec mpeg4 test1.ts
