CEF_VERSION   = 80.0.4+g74f7b0c+chromium-80.0.3987.122
CEF_INSTALL_DIR = /opt/cef

# 64 bit
CEF_BUILD = http://opensource.spotify.com/cefbuilds/cef_binary_$(CEF_VERSION)_linux64_minimal.tar.bz2

CC = g++
#CFLAGS = -c -O3  -Wall -std=c++11
CFLAGS = -c -O0 -g -Wall -std=c++11
LDFLAGS = -pthread

# SOURCES = cefsimple_linux.cc simple_app.cc simple_handler.cc simple_handler_linux.cc
SOURCES = main.cpp osrhandler.cpp browserclient.cpp browsercontrol.cpp
OBJECTS = $(SOURCES:.cpp=.o)

SOURCES2 = osrclient.cpp
OBJECTS2 = osrclient.cpp

EXECUTABLE  = vdrosrbrowser
EXECUTABLE2  = vdrosrclient

# CEF (debian packaged or self-installed version)
ifeq (exists, $(shell test -e /usr/include/x86_64-linux-gnu/cef/cef_app.h && echo exists))
	PACKAGED_CEF = 1
	CFLAGS += -I/usr/include/x86_64-linux-gnu/cef/
	LDFLAGS += -lcef -lcef_dll_wrapper -lX11
else
	PACKAGED_CEF = 0
	CFLAGS += $(shell pkg-config --cflags cef)
	LDFLAGS += $(shell pkg-config --libs cef)
endif

# libcurl
CFLAGS += $(shell pkg-config --cflags libcurl)
LDFLAGS += $(shell pkg-config --libs libcurl)

# nng
NNGCFLAGS  = -Ithirdparty/nng-1.2.6/include/nng/compat
NNGLDFLAGS = thirdparty/nng-1.2.6/build/libnng.a

all: prepareexe buildnng $(SOURCES) $(EXECUTABLE) $(EXECUTABLE2)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS) $(NNGLDFLAGS)
	mv $(EXECUTABLE) Release

$(EXECUTABLE2): $(OBJECTS2)
	$(CC) $(SOURCES2) $(NNGCFLAGS) -o $@ -pthread $(NNGLDFLAGS)
	mv $(EXECUTABLE2) Release
	cp -r js Release

prepareexe:
	mkdir -p Release
ifeq ($(PACKAGED_CEF),1)
	cd Release && \
	echo "resourcepath = /usr/share/cef/Resources" > vdr-osr-browser.config && \
	echo "localespath = /usr/share/cef/Resources/locales" >> vdr-osr-browser.config
else
	cd Release && \
	echo "resourcepath = $(CEF_INSTALL_DIR)/lib" > vdr-osr-browser.config && \
	echo "localespath = $(CEF_INSTALL_DIR)/lib/locales" >> vdr-osr-browser.config && \
	echo "frameworkpath  = $(CEF_INSTALL_DIR)/lib" >> vdr-osr-browser.config && \
	rm -f icudtl.dat natives_blob.bin v8_context_snapshot.bin && \
	ln -s $(CEF_INSTALL_DIR)/lib/icudtl.dat && \
	ln -s $(CEF_INSTALL_DIR)/lib/natives_blob.bin && \
	ln -s $(CEF_INSTALL_DIR)/lib/v8_context_snapshot.bin
endif

buildnng:
	mkdir -p thirdparty/nng-1.2.6/build
	cd thirdparty/nng-1.2.6/build && cmake ..
	$(MAKE) -C thirdparty/nng-1.2.6/build -j 6

.cpp.o:
	$(CC) $(CFLAGS) $(NNGCFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
	rm -Rf cef_binary*
	rm -Rf Release
	rm -Rf thirdparty/nng-1.2.6/build

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
	cd Release && ./cefsimple --remote-debugging-port=9222 --user-data-dir=remote-profile
