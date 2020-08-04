# useful make targets
#
# make or make all  (compile all source files and creates the Release folder containing all binaries)
# make clean        (deletes all compiled files and the Release folder)
# make prepare      (Downloads the Spotify build and install all files into /opt/cef, not needed if using libcef from ppa)
# make preparejs    (prepares compilation of javascript files)
# make buildjs		(compiles javascript files and install them in Release and js folder)
# make cleanjs		(deletes all not needed files in thirdparty/HybridTvViewer)

# previous version
# CEF_VERSION = 81.2.25%2Bg3afea62%2Bchromium-81.0.4044.113

# new default version
CEF_VERSION = 84.3.8%2Bgc8a556f%2Bchromium-84.0.4147.105

CEF_BUILD = http://opensource.spotify.com/cefbuilds/cef_binary_$(CEF_VERSION)_linux64_minimal.tar.bz2
CEF_INSTALL_DIR = /opt/cef

VERSION = 0.0.9

# Force using
#    debian package or
#    spotify build (installed in cef) or
#    spotify build (installed in thirdparty)
#
# Spotify build:  0
# Debian package: 1
# Local Cef:      2
# PACKAGED_CEF =  2

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

SOURCES = main.cpp osrhandler.cpp browserclient.cpp browsercontrol.cpp transcodeffmpeg.cpp schemehandler.cpp logger.cpp javascriptlogging.cpp globaldefs.cpp nativejshandler.cpp dashhandler.cpp
OBJECTS = $(SOURCES:.cpp=.o)

SOURCES5 = schemehandler.cpp logger.cpp thirdparty/cefsimple/cefsimple_linux.cpp thirdparty/cefsimple/simple_app.cpp thirdparty/cefsimple/simple_handler.cpp thirdparty/cefsimple/simple_handler_linux.cpp globaldefs.cpp
OBJECTS5 = $(SOURCES5:.cpp=.o)

EXECUTABLE  = vdrosrbrowser

# Starten mit z.B. ./cefsimple --url="file://<pfad>/movie.html"
EXECUTABLE5  = cefsimple

# binutils-dev
#ifeq (exists, $(shell gcc -lbfd 2>&1 | grep -q -c main && echo exists))
#   $(info binutils-dev found)
#   SOURCES += thirdparty/backward-cpp/backward.cpp
#   CFLAGS += -DBACKWARD_HAS_BFD=1
#   LDFLAGS += -lbfd -ldl
#endif
#
#CFLAGS += -I thirdparty/backward-cpp


# CEF (debian packaged or self-installed version)
ifndef PACKAGED_CEF
    $(info PACKAGED_CEF is undefined. Try to find CEF installation.)
    ifeq (exists, $(shell test -e /usr/include/x86_64-linux-gnu/cef/cef_app.h && echo exists))
        $(info Found CEF header, use debian package)

        PACKAGED_CEF = 1
        CEFCFLAGS += -I/usr/include/x86_64-linux-gnu/cef/
        CEFLDFLAGS += -lcef -lcef_dll_wrapper -lX11
    else ifeq (exists, $(shell pkg-config --exists cef && echo exists))
        $(info Use CEF found with pkg-config)

        PACKAGED_CEF = 0
        CEFCFLAGS += $(shell pkg-config --cflags cef)
        CEFLDFLAGS += $(shell pkg-config --libs cef)
    else ifeq (exists, $(shell test -e thirdparty/cef/include/cef_app.h  && echo exists))
        $(info Use locally install CEF in thirdparty directory)

        PACKAGED_CEF = 2
        CEFCFLAGS += -Ithirdparty/cef/include
        CEFLDFLAGS += -Lthirdparty/cef/build/libcef_dll_wrapper -Lthirdparty/cef/Release -lcef -lcef_dll_wrapper -lX11
    endif
else
    ifeq (1, $(PACKAGED_CEF))
        $(info CEF Debian package forced)

	CEFCFLAGS += -I/usr/include/x86_64-linux-gnu/cef/
        CEFLDFLAGS += -lcef -lcef_dll_wrapper -lX11
    else ifeq (0, $(PACKAGED_CEF))
        $(info CEF installation found by pkg-config forced)

        CEFCFLAGS += $(shell pkg-config --cflags cef)
        CEFLDFLAGS += $(shell pkg-config --libs cef)
    else ifeq (2, $(PACKAGED_CEF))
        $(info Local CEF installation in directory thirdparty forced)

        CEFCFLAGS += -Ithirdparty/cef/include -Ithirdparty/cef
        CEFLDFLAGS += -Lthirdparty/cef/build/libcef_dll_wrapper -Lthirdparty/cef/Release -Wl,-rpath,. -lcef -lcef_dll_wrapper -lX11
    endif
endif

# libcurl
CFLAGS += $(shell pkg-config --cflags libcurl)
LDFLAGS += $(shell pkg-config --libs libcurl)

# CEF
CFLAGS += $(CEFCFLAGS)
LDFLAGS += $(CEFLDFLAGS)

# nng
NNGVERSION = 1.3.0
NNGCFLAGS  = -Ithirdparty/nng-$(NNGVERSION)/include/nng/compat
NNGLDFLAGS = thirdparty/nng-$(NNGVERSION)/build/libnng.a

# spdlog
LOGCFLAGS = -Ithirdparty/spdlog/buildbin/include -DSPDLOG_COMPILED_LIB
LOGLDFLAGS = thirdparty/spdlog/buildbin/lib/libspdlog.a

all: prepareexe emptyvideo buildnng buildspdlog $(SOURCES) $(EXECUTABLE) $(EXECUTABLE2) $(EXECUTABLE3) $(EXECUTABLE5)

release: prepare_release
	$(MAKE) PACKAGED_CEF=2

dist:
	tar -cJf vdr-osr-browser-$(VERSION).tar.xz Release

$(EXECUTABLE): $(OBJECTS) transcodeffmpeg.h globaldefs.h main.h browser.h nativejshandler.h schemehandler.h javascriptlogging.h
	$(CC) $(OBJECTS) $(NNGCFLAGS) $(LOGCFLAGS) -o $@ $(LDFLAGS) $(NNGLDFLAGS) $(LOGLDFLAGS)
	mv $(EXECUTABLE) Release
	cp -r js Release

$(EXECUTABLE5): $(OBJECTS5)
	$(CC) -O3 $(OBJECTS5) $(LOGCFLAGS) $(CEFCFLAGS) -o $@ -pthread $(LDFLAGS) $(LOGLDFLAGS) $(CEFLDFLAGS)
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
ifeq ($(PACKAGED_CEF),1)
	cd Release && \
	echo "resourcepath = /usr/share/cef/Resources" > vdr-osr-browser.config && \
	echo "localespath = /usr/share/cef/Resources/locales" >> vdr-osr-browser.config
else ifeq ($(PACKAGED_CEF),0)
	cd Release && \
	echo "resourcepath = $(CEF_INSTALL_DIR)/lib" > vdr-osr-browser.config && \
	echo "localespath = $(CEF_INSTALL_DIR)/lib/locales" >> vdr-osr-browser.config && \
	echo "frameworkpath  = $(CEF_INSTALL_DIR)/lib" >> vdr-osr-browser.config && \
	rm -f icudtl.dat natives_blob.bin v8_context_snapshot.bin && \
	ln -s $(CEF_INSTALL_DIR)/lib/icudtl.dat && \
	ln -s $(CEF_INSTALL_DIR)/lib/natives_blob.bin && \
	ln -s $(CEF_INSTALL_DIR)/lib/v8_context_snapshot.bin
else ifeq ($(PACKAGED_CEF),2)
	echo "resourcepath = ." > Release/vdr-osr-browser.config && \
	echo "localespath = ." >> Release/vdr-osr-browser.config && \
	echo "frameworkpath  = ." >> Release/vdr-osr-browser.config && \
	cp -a thirdparty/cef/Resources/* Release && \
	cp -a thirdparty/cef/Release/* Release && \
	strip thirdparty/cef/Release/*.so && \
	strip thirdparty/cef/Release/swiftshader/*.so
endif
ifneq (exists, $(shell test -e Release/vdr-osr-ffmpeg.config && echo exists))
	sed -e "s#FFMPEG_EXECUTABLE#$(FFMPEG_EXECUTABLE)#" -e "s#FFPROBE_EXECUTABLE#$(FFPROBE_EXECUTABLE)#" vdr-osr-ffmpeg.config.sample > Release/vdr-osr-ffmpeg.config
endif
ifneq (exists, $(shell test -e Release/block_url.config && echo exists))
	cp block_url.config Release/block_url.config
endif

buildnng:
ifneq (exists, $(shell test -e thirdparty/nng-$(NNGVERSION)/build/libnng.a && echo exists))
	mkdir -p thirdparty/nng-$(NNGVERSION)/build
	cd thirdparty/nng-$(NNGVERSION)/build && cmake ..
	$(MAKE) -C thirdparty/nng-$(NNGVERSION)/build -j 6
endif

buildspdlog:
ifneq (exists, $(shell test -e thirdparty/spdlog/buildbin/lib/libspdlog.a && echo exists))
	mkdir -p thirdparty/spdlog/build
	cd thirdparty/spdlog/build && cmake -DCMAKE_INSTALL_PREFIX=../buildbin ..
	$(MAKE) -C thirdparty/spdlog/build -j 6 install
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
emptyvideo: prepareexe
	mkdir -p Release/movie
	cp -r movie/* Release/movie/
ifneq (exists, $(shell test -e movie/transparent-full.webm && echo exists))
	$(FFMPEG_EXECUTABLE) -y -loop 1 -i movie/transparent-16x16.png -t 21600 -r 1 -c:v libvpx -auto-alt-ref 0 movie/transparent-full.webm
endif

.cpp.o:
	$(CC) -I. $(CFLAGS) $(NNGCFLAGS) $(LOGCFLAGS) -MMD $< -o $@

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
	rm -f *.d

# download and install cef binary in directory /opt/cef
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

# download and install cef binary in directory thirdparty/cef
prepare_release:
ifneq (exists, $(shell test -e thirdparty/cef && echo exists))
	cd thirdparty && \
	curl -L $(CEF_BUILD)  -o - | tar -xjf -
	mv thirdparty/cef_binary* thirdparty/cef
	cd thirdparty/cef/include && ln -s ../include cef
endif
ifneq (exists, $(shell test -e thirdparty/cef/build/libcef_dll_wrapper/libcef_dll_wrapper.a && echo exists))
	mkdir -p thirdparty/cef/build && \
	cd thirdparty/cef/build && \
	cmake .. && \
	make -j 6
endif

prepare_debian:
ifneq (exists, $(shell test -e thirdparty/cef && echo exists))
	cd thirdparty && \
	curl -L $(CEF_BUILD)  -o - | tar -xjf -
	mv thirdparty/cef_binary* thirdparty/cef
	cd thirdparty/cef/include && ln -s ../include cef
endif

debugremote: all
	cd Release && gdbserver localhost:2345  ./vdrosrbrowser --debug --autoplay --remote-debugging-port=9222 --user-data-dir=remote-profile
