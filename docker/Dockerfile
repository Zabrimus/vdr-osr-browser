FROM debian:stable-backports

RUN apt update -qq && \
    apt -u -y dist-upgrade && \
    DEBIAN_FRONTEND=noninteractive apt -y install bison build-essential cdbs curl devscripts dpkg-dev elfutils fakeroot flex g++ git-core git-svn gperf libasound2-dev ffmpeg libbrlapi-dev libbz2-dev libcairo2-dev libcap-dev libcups2-dev libcurl4-gnutls-dev libdrm-dev libelf-dev libexif-dev libffi-dev libgconf2-dev libgconf-2-4 libgl1-mesa-dev libglib2.0-dev libglu1-mesa-dev libgnome-keyring-dev libgtk2.0-dev libkrb5-dev libnspr4-dev libnss3-dev libpam0g-dev libpci-dev libpulse-dev libsctp-dev libspeechd-dev libsqlite3-dev libssl-dev libudev-dev libwww-perl libxslt1-dev libxss-dev libxt-dev libxtst-dev mesa-common-dev openbox patch perl pkg-config python python-cherrypy3 python-crypto python-dev python-psutil python-numpy python-opencv python-openssl python-yaml rpm ruby subversion ttf-dejavu-core fonts-indic wdiff wget zip libgtkglext1-dev libatspi2.0-dev libatk-bridge2.0-dev openjdk-11-jdk && \
    mkdir -p /root/cef-build/cef-git && \
    mkdir -p /root/cef-build/depot && \
    mkdir -p /root/cef-build/download && \
    curl 'https://bitbucket.org/chromiumembedded/cef/raw/master/tools/automate/automate-git.py' -o /root/cef-build/automate-git.py && \
    chmod +x /root/cef-build/automate-git.py && \
    (cd /root/cef-build/depot && git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git) && \
    export PATH=/root/cef-build/depot/depot_tools:$PATH && \
    export GN_DEFINES="is_official_build=true use_sysroot=true use_allocator=none symbol_level=1 proprietary_codecs=true ffmpeg_branding=Chrome" && \
    export CEF_ARCHIVE_FORMAT=tar.bz2 && \
    (cd /root/cef-build && python automate-git.py --download-dir=/root/cef-build/download --branch=/root/cef-build/cef-git --minimal-distrib --client-distrib --force-clean --x64-build --build-target=cefsimple --branch 3626 --no-debug-build)

CMD ["/bin/bash"]
