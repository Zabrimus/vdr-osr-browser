CEF_VERSION_1   = 79.1.36%2Bg90301bd%2Bchromium-79.0.3945.130
CEF_VERSION_2   = Chromium 79.1.36+g90301bd
CEF_INSTALL_DIR = /opt/cef

# 64 bit
CEF_BUILD = http://opensource.spotify.com/cefbuilds/cef_binary_$(CEF_VERSION_1)_linux64_minimal.tar.bz2

# 32 bit
# CEF_BUILD = http://opensource.spotify.com/cefbuilds/cef_binary_$(CEF_VERSION_1)_linux32_minimal.tar.bz2

CC = g++
#CFLAGS = -c -O3  -Wall -std=c++11
CFLAGS = -c -O0 -g -Wall -std=c++11
LDFLAGS = -pthread

# SOURCES = cefsimple_linux.cc simple_app.cc simple_handler.cc simple_handler_linux.cc
SOURCES = main.cpp osrhandler.cpp browserclient.cpp browsercontrol.cpp
OBJECTS = $(SOURCES:.cpp=.o)

SOURCES2 = osrclient.cpp
OBJECTS2 = osrclient.cpp

EXECUTABLE  = osrcef
EXECUTABLE2  = osrclient

# CEF
CFLAGS += $(shell pkg-config --cflags cef)
LDFLAGS += $(shell pkg-config --libs cef)

# nanomsg
ifeq ($(shell pkg-config --exists libnanomsg && echo 1),1)
NANOMSG_CFLAGS = $(shell pkg-config --cflags libnanomsg)
NANOMSG_LDFLAGS = $(shell pkg-config --libs libnanomsg)
else
NANOMSG_CFLAGS = $(shell pkg-config --cflags nanomsg)
NANOMSG_LDFLAGS = $(shell pkg-config --libs nanomsg)
endif

CFLAGS += $(NANOMSG_CFLAGS)
LDFLAGS += $(NANOMSG_LDFLAGS)

# libcurl
CFLAGS += $(shell pkg-config --cflags libcurl)
LDFLAGS += $(shell pkg-config --libs libcurl)

all: prepareexe $(SOURCES) $(EXECUTABLE) $(EXECUTABLE2)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	mv $(EXECUTABLE) Release

$(EXECUTABLE2): $(OBJECTS2)
	$(CC) $(SOURCES2) -o $@ $(NANOMSG_CFLAGS) $(NANOMSG_LDFLAGS)
	mv $(EXECUTABLE2) Release
	cp -r js Release

prepareexe:
	mkdir -p Release && \
	cd Release && \
	echo "resourcepath = $(CEF_INSTALL_DIR)/lib" > cef-osr-browser.config && \
	echo "localespath = $(CEF_INSTALL_DIR)/lib/locales" >> cef-osr-browser.config && \
	echo "frameworkpath  = $(CEF_INSTALL_DIR)/lib" >> cef-osr-browser.config && \
	rm -f icudtl.dat natives_blob.bin v8_context_snapshot.bin && \
	ln -s $(CEF_INSTALL_DIR)/lib/icudtl.dat && \
	ln -s $(CEF_INSTALL_DIR)/lib/natives_blob.bin && \
	ln -s $(CEF_INSTALL_DIR)/lib/v8_context_snapshot.bin

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
	rm -Rf cef_binary*
	rm -Rf Release

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
