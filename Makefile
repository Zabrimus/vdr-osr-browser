# useful make targets
#
# make or make all  (compile all source files and creates the Release folder containing all binaries)
# make clean        (deletes all compiled files and the Release folder)
# make prepare      (Downloads the Spotify build and install all files into /opt/cef, not needed if using libcef from ppa)
# make preparejs    (prepares compilation of javascript files)
# make buildjs		(compiles javascript files and install them in Release and js folder)
# make cleanjs		(deletes all not needed files in thirdparty/HybridTvViewer)

CEF_VERSION   = 81.2.16%2Bgdacda4f%2Bchromium-81.0.4044.92
CEF_BUILD = http://opensource.spotify.com/cefbuilds/cef_binary_$(CEF_VERSION)_linux64_minimal.tar.bz2
CEF_INSTALL_DIR = /opt/cef

# Force using debian package or spotify build
# Spotify build:  0
# Debian package: 1
# PACKAGED_CEF = 0

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
CFLAGS = -g -c -O3  -Wall -std=c++11
#CFLAGS = -c -O0 -g -Wall -std=c++11
LDFLAGS = -pthread -lrt

# SOURCES = cefsimple_linux.cc simple_app.cc simple_handler.cc simple_handler_linux.cc
SOURCES = main.cpp osrhandler.cpp browserclient.cpp browsercontrol.cpp transcodeffmpeg.cpp schemehandler.cpp
OBJECTS = $(SOURCES:.cpp=.o)

SOURCES2 = osrclient.cpp
OBJECTS2 = $(SOURCES2:.cpp=.o)

SOURCES3 = osrclientvideo.cpp
OBJECTS3 = $(SOURCES3:.cpp=.o)

SOURCES4 = transcodetest.cpp transcodeffmpeg.cpp
OBJECTS4 = $(SOURCES4:.cpp=.o)

SOURCES5 = schemehandler.cpp thirdparty/cefsimple/cefsimple_linux.cpp thirdparty/cefsimple/simple_app.cpp thirdparty/cefsimple/simple_handler.cpp thirdparty/cefsimple/simple_handler_linux.cpp
OBJECTS5 = $(SOURCES5:.cpp=.o)

EXECUTABLE  = vdrosrbrowser
EXECUTABLE2  = vdrosrclient
EXECUTABLE3  = vdrosrvideo
EXECUTABLE4  = transcodetest

# Starten mit z.B. ./cefsimple --url="file://<pfad>/movie.html"
EXECUTABLE5  = cefsimple

# CEF (debian packaged or self-installed version)
ifndef PACKAGED_CEF
	ifeq (exists, $(shell test -e /usr/include/x86_64-linux-gnu/cef/cef_app.h && echo exists))
		PACKAGED_CEF = 1
		CFLAGS += -I/usr/include/x86_64-linux-gnu/cef/
		LDFLAGS += -lcef -lcef_dll_wrapper -lX11
	else
		PACKAGED_CEF = 0
		CFLAGS += $(shell pkg-config --cflags cef)
		LDFLAGS += $(shell pkg-config --libs cef)
	endif
else
	ifeq (1, PACKAGED_CEF)
		CFLAGS += -I/usr/include/x86_64-linux-gnu/cef/
		LDFLAGS += -lcef -lcef_dll_wrapper -lX11
	else
		CFLAGS += $(shell pkg-config --cflags cef)
		LDFLAGS += $(shell pkg-config --libs cef)
	endif
endif

# libcurl
CFLAGS += $(shell pkg-config --cflags libcurl)
LDFLAGS += $(shell pkg-config --libs libcurl)

# nng
NNGVERSION = 1.3.0
NNGCFLAGS  = -Ithirdparty/nng-$(NNGVERSION)/include/nng/compat
NNGLDFLAGS = thirdparty/nng-$(NNGVERSION)/build/libnng.a

# libav / ffmpeg
LIBAVCFLAGS += $(shell PKG_CONFIG_PATH=$(FFMPEG_PKG_CONFIG_PATH) pkg-config --cflags libavformat libavcodec libavfilter libavdevice libswresample libswscale libavutil)
LIBAVLDFLAGS += $(shell PKG_CONFIG_PATH=$(FFMPEG_PKG_CONFIG_PATH) pkg-config --libs libavformat libavcodec libavfilter libavdevice libswresample libswscale libavutil)

all: prepareexe emptyvideo buildnng $(SOURCES) $(EXECUTABLE) $(EXECUTABLE2) $(EXECUTABLE3) $(EXECUTABLE4) $(EXECUTABLE5)

$(EXECUTABLE): $(OBJECTS) transcodeffmpeg.h globaldefs.h main.h browser.h
	$(CC) $(OBJECTS) $(NNGCFLAGS) -o $@ $(LDFLAGS) $(LIBAVLDFLAGS) $(NNGLDFLAGS)
	mv $(EXECUTABLE) Release
	cp -r js Release

$(EXECUTABLE2): $(OBJECTS2) transcodeffmpeg.h globaldefs.h
	$(CC) $(OBJECTS2) $(NNGCFLAGS) -o $@ -pthread $(NNGLDFLAGS)
	mv $(EXECUTABLE2) Release

$(EXECUTABLE3): $(OBJECTS3) transcodeffmpeg.h globaldefs.h
	$(CC) $(OBJECTS3) $(NNGCFLAGS) -o $@ -pthread $(NNGLDFLAGS)
	mv $(EXECUTABLE3) Release

$(EXECUTABLE4): $(OBJECTS4) transcodeffmpeg.h globaldefs.h
	$(CC) -O3 $(OBJECTS4) $(NNGCFLAGS) $(LIBAVCFLAGS) -o $@ -pthread $(LIBAVLDFLAGS) $(NNGLDFLAGS)
	mv $(EXECUTABLE4) Release

$(EXECUTABLE5): $(OBJECTS5)
	$(CC) -O3 $(OBJECTS5) -o $@ -pthread $(LDFLAGS)
	mv $(EXECUTABLE5) Release
	cp thirdparty/cefsimple/movie.html Release

prepareexe:
	mkdir -p Release
ifeq ($(PACKAGED_CEF),1)
	cd Release && \
	echo "resourcepath = /usr/share/cef/Resources" > vdr-osr-browser.config && \
	echo "localespath = /usr/share/cef/Resources/locales" >> vdr-osr-browser.config && \
	echo "ffmpeg_executable = $(FFMPEG_EXECUTABLE)" >> vdr-osr-browser.config && \
	echo "ffprobe_executable = $(FFPROBE_EXECUTABLE)" >> vdr-osr-browser.config
else
	cd Release && \
	echo "resourcepath = $(CEF_INSTALL_DIR)/lib" > vdr-osr-browser.config && \
	echo "localespath = $(CEF_INSTALL_DIR)/lib/locales" >> vdr-osr-browser.config && \
	echo "frameworkpath  = $(CEF_INSTALL_DIR)/lib" >> vdr-osr-browser.config && \
	echo "ffmpeg_executable = $(FFMPEG_EXECUTABLE)" >> vdr-osr-browser.config && \
	echo "ffprobe_executable = $(FFPROBE_EXECUTABLE)" >> vdr-osr-browser.config && \
	rm -f icudtl.dat natives_blob.bin v8_context_snapshot.bin && \
	ln -s $(CEF_INSTALL_DIR)/lib/icudtl.dat && \
	ln -s $(CEF_INSTALL_DIR)/lib/natives_blob.bin && \
	ln -s $(CEF_INSTALL_DIR)/lib/v8_context_snapshot.bin
endif

buildnng:
ifneq (exists, $(shell test -e thirdparty/nng-$(NNGVERSION)/build/libnng.a && echo exists))
	mkdir -p thirdparty/nng-$(NNGVERSION)/build
	cd thirdparty/nng-$(NNGVERSION)/build && cmake ..
	$(MAKE) -C thirdparty/nng-$(NNGVERSION)/build -j 6
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
emptyvideo: prepareexe
	mkdir -p Release/movie
	cp -r movie/* Release/movie/
ifneq (exists, $(shell test -e Release/movie/transparent-full.webm && echo exists))
	$(FFMPEG_EXECUTABLE) -y -loop 1 -i Release/movie/transparent-16x16.png -t 21600 -r 1 -c:v libvpx -auto-alt-ref 0 Release/movie/transparent-full.webm
endif

.cpp.o:
	$(CC) -I. $(CFLAGS) $(LIBAVCFLAGS) $(NNGCFLAGS) -MMD $< -o $@

DEPS := $(OBJECTS:.o=.d)
-include $(DEPS)

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(OBJECTS2) $(EXECUTABLE2) $(OBJECTS3) $(EXECUTABLE3) $(OBJECTS4) $(EXECUTABLE4) *.d tests/*.d
	rm -Rf cef_binary*
	rm -Rf Release
	rm -Rf thirdparty/nng-$(NNGVERSION)/build

# download and install cef binary
prepare:
	mkdir -p $(CEF_INSTALL_DIR)/lib
	mkdir -p /usr/local/lib/pkgconfig
	curl -L $(CEF_BUILD)  -o - | tar -xjf -
	cd cef_binary* && \
	cmake . && \
    make -j 6 && \
	cp -r include $(CEF_INSTALL_DIR) && \
	cp -r Release/* $(CEF_INSTALL_DIR)/lib && \
	cp -r Resources/* $(CEF_INSTALL_DIR)/lib && \
	cp libcef_dll_wrapper/libcef_dll_wrapper.a $(CEF_INSTALL_DIR)/lib
	sed "s:CEF_INSTALL_DIR:$(CEF_INSTALL_DIR):g" < cef.pc.template > cef.pc
	mv cef.pc /usr/local/lib/pkgconfig
	echo "$(CEF_INSTALL_DIR)/lib" > /etc/ld.so.conf.d/cef.conf
	ldconfig

debugremote: all
	cd Release && gdbserver localhost:2345  ./vdrosrbrowser --debug --autoplay --remote-debugging-port=9222 --user-data-dir=remote-profile

