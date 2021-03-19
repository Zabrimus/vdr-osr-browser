# useful make targets
#
# make or make all  (compile all source files and creates the Release folder containing all binaries)
# make clean        (deletes all compiled files and the Release folder)
# make preparejs    (prepares compilation of javascript files)
# make buildjs		(compiles javascript files and install them in Release and js folder)
# make cleanjs		(deletes all not needed files in thirdparty/HybridTvViewer)

# Included CEF version
# 89.0.12+g2b76680+chromium-89.0.4389.90
# branch 4389, based on Chromium 89

CEF_ARCHIVE = cef_binary_89.0.12+g2b76680+chromium-89.0.4389.90_linux64_minimal.tar.xz

VERSION = 0.2.0-dev

# Don't enable this!
# Needs a different CEF installation with different compile flags.
# ASAN -> 1
SANITIZER=0

# Don't enable this!
# use CEF DEBUG libs instead RELEASE
# Needs a different CEF installation with different compile flags.
USE_CEF_DEBUG=0

CC = g++

#CFLAGS = -g -c -O3  -Wall -std=c++11
CFLAGS = -c -O0 -g -Wall -std=c++11
LDFLAGS = -pthread -lrt

SOURCES = main.cpp osrhandler.cpp browserclient.cpp browsercontrol.cpp browserpaintupdater.cpp schemehandler.cpp \
          logger.cpp javascriptlogging.cpp globaldefs.cpp nativejshandler.cpp dashhandler.cpp encoder.cpp \
          sharedmemory.cpp
OBJECTS = $(SOURCES:.cpp=.o)

SOURCES5 = schemehandler.cpp logger.cpp testex/cefsimple/cefsimple_linux.cpp testex/cefsimple/simple_app.cpp \
           testex/cefsimple/simple_handler.cpp testex/cefsimple/simple_handler_linux.cpp globaldefs.cpp
OBJECTS5 = $(SOURCES5:.cpp=.o)

SOURCES4 = testex/cefosrtest/main.cpp
OBJECTS4 = $(SOURCES4:.cpp=.o)

SOURCES3 = testex/shmp/testshmp.cpp sharedmemory.cpp
OBJECTS3 = $(SOURCES3:.cpp=.o)

EXECUTABLE  = vdrosrbrowser

# Starten mit z.B. ./cefsimple --url="file://<pfad>/movie.html"
EXECUTABLE5  = cefsimple
EXECUTABLE4  = cefosrtest
EXECUTABLE3  = testshmp

# libcurl
CFLAGS += $(shell pkg-config --cflags libcurl)
LDFLAGS += $(shell pkg-config --libs libcurl)

# spdlog
LOGCFLAGS = -Ithirdparty/spdlog/buildbin/include -D SPDLOG_COMPILED_LIB
LOGLDFLAGS = thirdparty/spdlog/buildbin/lib/libspdlog.a

# ffmpeg
AVCFLAGS += $(shell pkg-config --cflags libavformat libavcodec libavfilter libavdevice libswresample libpostproc libswscale libavutil)
AVLDFLAGS += $(shell pkg-config --libs libavformat libavcodec libavfilter libavdevice libswresample libpostproc libswscale libavutil)
# ffnvcodec?

CFLAGS += $(AVCFLAGS)
LDFLAGS += $(AVLDFLAGS)

# CEF
CFLAGS += -Ithirdparty/cef/include -Ithirdparty/cef
LDFLAGS += -Lthirdparty/cef/build/libcef_dll_wrapper -Lthirdparty/cef/Release -Wl,-rpath,. -lcef -lcef_dll_wrapper -lX11

# Sanitizer ASAN
ifeq ($(SANITIZER), 1)
CC=clang++-12
ASANCFLAGS = -ggdb -fsanitize=address  -fno-omit-frame-pointer
ASANLDFLAGS = -ggdb -fsanitize=address  -fno-omit-frame-pointer /usr/lib/llvm-12/lib/libc++abi.a extlib/libclang_rt.asan_cxx-x86_64.a
endif

all:
	$(MAKE) extractcef
	$(MAKE) buildspdlog
	$(MAKE) buildcef
	$(MAKE) prepareexe
	$(MAKE) browser

browser: $(SOURCES) $(EXECUTABLE) $(EXECUTABLE5) $(EXECUTABLE4) $(EXECUTABLE3)

dist:
	tar -cJf vdr-osr-browser-$(VERSION).tar.xz Release

$(EXECUTABLE): $(OBJECTS) globaldefs.h main.h browser.h nativejshandler.h schemehandler.h javascriptlogging.h sharedmemory.h
	$(CC) $(OBJECTS) $(LOGCFLAGS) -o $@ $(ASANLDFLAGS) $(LDFLAGS) $(LOGLDFLAGS)
	mv $(EXECUTABLE) Release
	cp -r js Release
	cp thirdparty/dashjs/dash.all.debug.js Release/js
	cp thirdparty/shaka-player/shaka-player.compiled.min.js Release/js
	cp thirdparty/shaka-player/shaka-player.compiled.debug.js Release/js
	cp thirdparty/mux.js/mux.min.js Release/js
	mkdir -p Release/licenses
	cp thirdparty/License.* Release/licenses

$(EXECUTABLE5): $(OBJECTS5)
	$(CC) -O3 $(OBJECTS5) $(LOGCFLAGS) $(CEFCFLAGS) -o $@ -pthread $(ASANLDFLAGS) $(LDFLAGS) $(LOGLDFLAGS) $(CEFLDFLAGS)
	mv $(EXECUTABLE5) Release
	cp testex/cefsimple/movie.html Release

$(EXECUTABLE4): $(OBJECTS4)
	$(CC) -O3 $(OBJECTS4) $(LOGCFLAGS) $(CEFCFLAGS) -o $@ -pthread $(ASANLDFLAGS) $(LDFLAGS) $(LOGLDFLAGS) $(CEFLDFLAGS)
	mv $(EXECUTABLE4) Release

$(EXECUTABLE3): $(OBJECTS3)
	$(CC) -O3 $(OBJECTS3) -o $@ -pthread  $(LDFLAGS)
	mv $(EXECUTABLE3) Release

extractcef:
ifneq (exists, $(shell test -e thirdparty/cef/CMakeLists.txt && echo exists))
	cd thirdparty && tar -xf $(CEF_ARCHIVE)
endif

prepareexe:
	mkdir -p Release
	mkdir -p Release/cache
	mkdir -p Release/font
	mkdir -p Release/css
	mkdir -p Release/profile
	cp thirdparty/TiresiasPCfont/TiresiasPCfont.ttf Release/font
	cp thirdparty/TiresiasPCfont/TiresiasPCfont.css Release/css
	echo "resourcepath = ." > Release/vdr-osr-browser.config
	echo "localespath = ." >> Release/vdr-osr-browser.config
	echo "frameworkpath  = ." >> Release/vdr-osr-browser.config
	cp -a thirdparty/cef/Resources/* Release
	cp -a thirdparty/cef/Release/* Release
ifneq (exists, $(shell test -e Release/block_url.config && echo exists))
	cp block_url.config Release/block_url.config
endif
ifneq (exists, $(shell test -e Release/x264_encoding.settings && echo exists))
	cp x264_encoding.settings Release/x264_encoding.settings
endif

buildcef:
ifneq (exists, $(shell test -e thirdparty/cef/build/libcef_dll_wrapper/libcef_dll_wrapper.a && echo exists))
ifeq (1, $(USE_CEF_DEBUG))
	mkdir -p thirdparty/cef/build && \
	cd thirdparty/cef/build && \
	cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug .. && \
	$(MAKE)
else
	mkdir -p thirdparty/cef/build && \
	cd thirdparty/cef/build && \
	cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release .. && \
	$(MAKE)
endif
endif

buildspdlog:
ifneq (exists, $(shell test -e thirdparty/spdlog/buildbin/lib/libspdlog.a && echo exists))
	mkdir -p thirdparty/spdlog/build && \
	cd thirdparty/spdlog/build && \
	cmake -DCMAKE_INSTALL_PREFIX=../buildbin -DCMAKE_BUILD_TYPE=Release .. && \
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

# test programs (will be removed at some time)
encodetest: encodetest.o encoder.o
	$(CC) $+ -o $@ $(AVLDFLAGS)

.cpp.o:
	$(CC) $(ASANCFLAGS) -I. $(CFLAGS) $(LOGCFLAGS) -MMD $< -o $@

.c.o:
	$(CC) $(ASANCFLAGS) -I. $(CFLAGS) $(LOGCFLAGS) -MMD $< -o $@

DEPS := $(OBJECTS:.o=.d)
-include $(DEPS)

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(OBJECTS2) $(EXECUTABLE2) $(OBJECTS3) $(EXECUTABLE3) *.d tests/*.d
	rm -Rf cef_binary*
	rm -Rf Release
	rm -Rf thirdparty/spdlog/build
	rm -Rf thirdparty/spdlog/buildbin
	rm -Rf thirdparty/cef
	rm -f testex/cefsimple/*.o
	rm -f testex/cefsimple/*.d
	rm -f testex/cefosrtest/*.d
	rm -f testex/cefosrtest/*.o
	rm -f *.d

distclean: clean

debugremote: all
	cd Release && gdbserver localhost:2345  ./vdrosrbrowser --debug --autoplay --remote-debugging-port=9222 --user-data-dir=remote-profile
