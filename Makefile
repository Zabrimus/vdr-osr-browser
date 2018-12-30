CEF_VERSION_1   = 3.3578.1860.g36610bd
CEF_VERSION_2   = Chromium 71.0.3578.80
CEF_INSTALL_DIR = /opt/cef

# 64 bit
CEF_BUILD = http://opensource.spotify.com/cefbuilds/cef_binary_$(CEF_VERSION_1)_linux64_minimal.tar.bz2

# 32 bit
# CEF_BUILD = http://opensource.spotify.com/cefbuilds/cef_binary_$(CEF_VERSION_1)_linux32_minimal.tar.bz2

CC = g++
CFLAGS = -c -g -O0 -Wall -std=c++11
LDFLAGS = -pthread

# SOURCES = cefsimple_linux.cc simple_app.cc simple_handler.cc simple_handler_linux.cc
SOURCES = main.cpp osrhandler.cpp browserclient.cpp browsercontrol.cpp
OBJECTS = $(SOURCES:.cpp=.o)

SOURCES2 = osrclient.cpp
OBJECTS2 = osrclient.cpp

EXECUTABLE  = osrcef
EXECUTABLE2  = osrclient

# CEF
CFLAGS += `pkg-config --cflags cef`
LDFLAGS += `pkg-config --libs cef`

# nanomsg
CFLAGS += `pkg-config --cflags nanomsg`
LDFLAGS += `pkg-config --libs nanomsg`

all: prepareexe $(SOURCES) $(EXECUTABLE) $(EXECUTABLE2)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	mv $(EXECUTABLE) Release

$(EXECUTABLE2): $(OBJECTS2)
	$(CC) $(SOURCES2) -o $@ `pkg-config --cflags nanomsg` `pkg-config --libs nanomsg`
	mv $(EXECUTABLE2) Release

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
