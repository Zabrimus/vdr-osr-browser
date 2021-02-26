# useful make targets
#
# make or make all  (compile all source files and creates the Release folder containing all binaries)
# make clean        (deletes all compiled files and the Release folder)
# make preparejs    (prepares compilation of javascript files)
# make buildjs		(compiles javascript files and install them in Release and js folder)
# make cleanjs		(deletes all not needed files in thirdparty/HybridTvViewer)

# Included CEF version
# 89.0.2+g6c06fde+chromium-89.0.4389.40
# branch 4389, based on Chromium 89

VERSION = 0.2.0-dev

# Don't enable this!
# Needs a different CEF installation with different compile flags. Also clang is needed.
ASAN=0

# Alternative ffmpeg installation.
# FFMPEG_PKG_CONFIG_PATH=/usr/local/ffmpeg/lib/pkgconfig/

# ffmpeg executable.
# Will be written to the config file vdr-osr-browser.config
# and can also be changed later.
FFMPEG_EXECUTABLE = /usr/bin/ffmpeg
#FFMPEG_EXECUTABLE = /usr/local/ffmpeg/bin/ffmpeg

FFPROBE_EXECUTABLE = /usr/bin/ffprobe
#FFPROBE_EXECUTABLE = /usr/local/ffmpeg/bin/ffprobe

CC = g++

#CFLAGS = -g -c -O3  -Wall -std=c++11
CFLAGS = -c -O0 -g -Wall -std=c++11
LDFLAGS = -pthread -lrt

SOURCES = main.cpp osrhandler.cpp browserclient.cpp browsercontrol.cpp browserpaintupdater.cpp schemehandler.cpp \
          logger.cpp javascriptlogging.cpp globaldefs.cpp nativejshandler.cpp dashhandler.cpp encoder.cpp
OBJECTS = $(SOURCES:.cpp=.o)

SOURCES5 = schemehandler.cpp logger.cpp thirdparty/cefsimple/cefsimple_linux.cpp thirdparty/cefsimple/simple_app.cpp \
           thirdparty/cefsimple/simple_handler.cpp thirdparty/cefsimple/simple_handler_linux.cpp globaldefs.cpp
OBJECTS5 = $(SOURCES5:.cpp=.o)

EXECUTABLE  = vdrosrbrowser

# Starten mit z.B. ./cefsimple --url="file://<pfad>/movie.html"
EXECUTABLE5  = cefsimple

# Socket.cpp
CFLAGS += -Ithirdparty/Socket.cpp

# libcurl
CFLAGS += $(shell pkg-config --cflags libcurl)
LDFLAGS += $(shell pkg-config --libs libcurl)

# nng
NNGVERSION = 1.3.0
NNGCFLAGS  = -Ithirdparty/nng-$(NNGVERSION)/include/nng/compat
NNGLDFLAGS = thirdparty/nng-$(NNGVERSION)/build/libnng.a

# spdlog
LOGCFLAGS = -Ithirdparty/spdlog/buildbin/include -D SPDLOG_COMPILED_LIB
LOGLDFLAGS = thirdparty/spdlog/buildbin/lib/libspdlog.a

# ffmpeg
AVCFLAGS += $(shell pkg-config --cflags libavformat)
AVCFLAGS += $(shell pkg-config --cflags libavcodec)
AVCFLAGS += $(shell pkg-config --cflags libavfilter)
AVCFLAGS += $(shell pkg-config --cflags libavdevice)
AVCFLAGS += $(shell pkg-config --cflags libswresample)
AVCFLAGS += $(shell pkg-config --cflags libpostproc)
AVCFLAGS += $(shell pkg-config --cflags libswscale)
AVCFLAGS += $(shell pkg-config --cflags libavutil)

AVLDFLAGS += $(shell pkg-config --libs libcurl)
AVLDFLAGS += $(shell pkg-config --libs libavformat)
AVLDFLAGS += $(shell pkg-config --libs libavcodec)
AVLDFLAGS += $(shell pkg-config --libs libavfilter)
AVLDFLAGS += $(shell pkg-config --libs libavdevice)
AVLDFLAGS += $(shell pkg-config --libs libswresample)
AVLDFLAGS += $(shell pkg-config --libs libpostproc)
AVLDFLAGS += $(shell pkg-config --libs libswscale)
AVLDFLAGS += $(shell pkg-config --libs libavutil)
# ffnvcodec?

CFLAGS += $(AVCFLAGS)
LDFLAGS += $(AVLDFLAGS)

# CEF
CFLAGS += -Ithirdparty/cef/include -Ithirdparty/cef
LDFLAGS += -Lthirdparty/cef/build/libcef_dll_wrapper -Lthirdparty/cef/Release -Wl,-rpath,. -lcef -lcef_dll_wrapper -lX11

# Sanitizer
ifeq ($(ASAN), 1)
CC=clang++-12
ASANCFLAGS = -ggdb -fsanitize=address -fno-omit-frame-pointer
ASANLDFLAGS = -ggdb -fsanitize=address -fno-omit-frame-pointer /usr/lib/llvm-12/lib/libc++abi.a
endif

all:
	$(MAKE) buildspdlog
	$(MAKE) prepareexe
	$(MAKE) emptyvideo
	$(MAKE) buildcef
	$(MAKE) buildnng
	$(MAKE) browser

browser: $(SOURCES) $(EXECUTABLE) $(EXECUTABLE5)

dist:
	tar -cJf vdr-osr-browser-$(VERSION).tar.xz Release

$(EXECUTABLE): $(OBJECTS) transcodeffmpeg.h globaldefs.h main.h browser.h nativejshandler.h schemehandler.h javascriptlogging.h
	$(CC) $(OBJECTS) $(NNGCFLAGS) $(LOGCFLAGS) -o $@ $(ASANLDFLAGS) $(LDFLAGS) $(NNGLDFLAGS) $(LOGLDFLAGS)
	mv $(EXECUTABLE) Release
	cp -r js Release

$(EXECUTABLE5): $(OBJECTS5)
	$(CC) -O3 $(OBJECTS5) $(LOGCFLAGS) $(CEFCFLAGS) -o $@ -pthread $(ASANLDFLAGS) $(LDFLAGS) $(LOGLDFLAGS) $(CEFLDFLAGS)
	mv $(EXECUTABLE5) Release
	cp thirdparty/cefsimple/movie.html Release

prepareexe:
	mkdir -p Release
	mkdir -p Release/cache
	mkdir -p Release/font
	mkdir -p Release/css
	mkdir -p Release/profile
	cp thirdparty/TiresiasPCfont/TiresiasPCfont.ttf Release/font
	cp thirdparty/TiresiasPCfont/TiresiasPCfont.css Release/css
	echo "resourcepath = ." > Release/vdr-osr-browser.config && \
	echo "localespath = ." >> Release/vdr-osr-browser.config && \
	echo "frameworkpath  = ." >> Release/vdr-osr-browser.config && \
	cp -a thirdparty/cef/Resources/* Release && \
	cp -a thirdparty/cef/Release/* Release
	#strip thirdparty/cef/Release/*.so && \
	#strip thirdparty/cef/Release/swiftshader/*.so
ifneq (exists, $(shell test -e Release/vdr-osr-ffmpeg.config && echo exists))
	sed -e "s#FFMPEG_EXECUTABLE#$(FFMPEG_EXECUTABLE)#" -e "s#FFPROBE_EXECUTABLE#$(FFPROBE_EXECUTABLE)#" vdr-osr-ffmpeg.config.sample > Release/vdr-osr-ffmpeg.config
endif
ifneq (exists, $(shell test -e Release/block_url.config && echo exists))
	cp block_url.config Release/block_url.config
endif

buildcef:
ifneq (exists, $(shell test -e thirdparty/cef/build/libcef_dll_wrapper/libcef_dll_wrapper.a && echo exists))
	mkdir -p thirdparty/cef/build && \
	cd thirdparty/cef/build && \
	cmake .. && \
	$(MAKE)
endif

buildnng:
ifneq (exists, $(shell test -e thirdparty/nng-$(NNGVERSION)/build/libnng.a && echo exists))
	mkdir -p thirdparty/nng-$(NNGVERSION)/build && \
	cd thirdparty/nng-$(NNGVERSION)/build && cmake .. && \
	$(MAKE)
endif

buildspdlog:
ifneq (exists, $(shell test -e thirdparty/spdlog/buildbin/lib/libspdlog.a && echo exists))
	mkdir -p thirdparty/spdlog/build && \
	cd thirdparty/spdlog/build && \
	cmake -DCMAKE_INSTALL_PREFIX=../buildbin .. && \
	$(MAKE) install
	cd thirdparty/spdlog/buildbin/ && if [ -d lib64 ]; then ln -s lib64 lib; fi
endif

preparejs:
	cd thirdparty/HybridTvViewer && npm i

buildjs:
	cd thirdparty/HybridTvViewer && npm run build
	cp thirdparty/HybridTvViewer/build/* js
ifeq (exists, $(shell test -e Release/js/ && echo exists))
	cp thirdparty/HybridTvViewer/build/* Release/js
endif

cleanjs:
	rm -Rf thirdparty/HybridTvViewer/build
	rm -Rf thirdparty/HybridTvViewer/node_modules
	rm thirdparty/HybridTvViewer/package-lock.json

# create a 6 hours video containing... nothing
emptyvideo:
	mkdir -p Release/movie
	cp -r movie/* Release/movie/
ifneq (exists, $(shell test -e movie/transparent-full.webm && echo exists))
	$(FFMPEG_EXECUTABLE) -y -loop 1 -i movie/transparent-16x16.png -t 21600 -r 1 -c:v libvpx -auto-alt-ref 0 movie/transparent-full.webm
endif

# test programs (will be removed at some time)
encodetest: encodetest.o encoder.o
	$(CC) $+ -o $@ $(AVLDFLAGS)

.cpp.o:
	$(CC) $(ASANCFLAGS) -I. $(CFLAGS) $(NNGCFLAGS) $(LOGCFLAGS) -MMD $< -o $@

.c.o:
	$(CC) $(ASANCFLAGS) -I. $(CFLAGS) $(NNGCFLAGS) $(LOGCFLAGS) -MMD $< -o $@

DEPS := $(OBJECTS:.o=.d)
-include $(DEPS)

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(OBJECTS2) $(EXECUTABLE2) $(OBJECTS3) $(EXECUTABLE3) *.d tests/*.d
	rm -Rf cef_binary*
	rm -Rf Release
	rm -Rf thirdparty/nng-$(NNGVERSION)/build
	rm -Rf thirdparty/spdlog/build
	rm -Rf thirdparty/spdlog/buildbin
	rm -Rf thirdparty/cef/build
	rm -f thirdparty/cefsimple/*.o
	rm -f thirdparty/cefsimple/*.d
	rm -f *.d

distclean: clean

debugremote: all
	cd Release && gdbserver localhost:2345  ./vdrosrbrowser --debug --autoplay --remote-debugging-port=9222 --user-data-dir=remote-profile
