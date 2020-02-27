# CEF (chrome embedded framework) OSR (offscreen browser) für VDR
Der Browser wird aktuell für den VDR Skin vdr-plugin-htmlskin verwendet. Dabei wird eine HTML-Seite für die Skin-Bestandteile
offscreen gerendert und an das VDR Plugin zur Darstellung gesendet. Verwendet wird dabei libnanomsg und das IPC Protokoll.

Der OSR Browser kann und wird über verschiedene Kommandos gesteuert. Das passiert auch mittels libnanomsg und dem REQ/PUB Protokoll.

## Warnung
Der OSR Browser und das VDR Skin sind alles andere als leichtgewichtig. Wenn man bedenkt, das das CEF praktisch fast
einer Chromium Installation entspricht, düfte dies auch klar sein.

## Voraussetzungen
- libnanomsg (https://github.com/nanomsg/nanomsg), getestet ist Version 1.1.5.
- libcurl
- eine Installation des CEF (siehe unten)

Das Makefile bietet die Möglichkeit, vorkompilierte Binaries des CEF herunterzuladen und die Entwicklungsumgebung
vorzubereiten. Die vorkompilierten Binaries finden sich auf http://opensource.spotify.com/cefbuilds/index.html.
Diese Variante hat den Nachteil, das verschiedene Codecs nicht verfügbar sind (z.B. H.264/265 und andere Audio Codes),
damit ist ein hardware-beschleunigtes Rendering nicht immer verfügbar.
Siehe z.B. auch https://github.com/Zabrimus/cef-makefile-sample
```
make prepare
```

Eine andere Variante ist das vollständige Neukompilieren des CEF mit den entsprechenden Compile-Flags. Allerdings
wird dafür ein potenter Rechner, viel Hauptspeicher und Plattenspeicher benötigt und vor allen Dingen eine schnelle 
Internetverbindung. Es werden ca. 13 GB heruntergeladen (+/- 1 GB). Ein Core i7 mit 4/8 Kernen, 32 GB Ram und einer 
1Gbit/s Netzanbindung braucht ca 4-5 Stunden für den allerersten Build.

### Docker
Im Verzeichnis docker befindet sich ein Dockerfile, das das CEF neu baut.  
```
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
```
Allerdings bricht der Container beim Paketieren ab. Ich habe Docker verworfen und nutze stattdessen LXD, bei dem 
wesentlich mehr Kontrolle über den Container möglich ist. Die Kommandos im Dockerfile habe ich exakt so im LXD Container 
ausgeführt.

## Build
Einmalig
```
make prepare
```

Ein neues Build wird gemacht mit
```
make
```

Im Unterverzeichnis Release befinden sich dann die beiden Binaries osrcef und osrclient zusammen mit 3 Softlinks in
das CEF Installationsverzeichnis. Die Softlinks sind absolut notwendig, da CEF leider verschiedene Dateien in genau
dem Verzeichnis sucht, in dem sich auch das Executable befindet und nirgends anderswo. Dies ist auch ein Grund, warum
der OSR Browser nicht als VDR Plugin implementiert werden kann ohne den VDR selbst zu patchen, was sehr wenig sinnvoll ist.


## Starten des Browsers
Die einfachste Variante zum Start ist
```
./vdrosrbrowser
```

Ein Remote-Debugging mittels Chrome kann sinnvoll sein. Dazu müssen nur folgende Parameter verwendet werden
```
/vdrosrbrowser --remote-debugging-port=9222 --user-data-dir=remote-profile
```
Und im Chrome die Seite http://localhost:9222 aufrufen. Man kann dann die Seite sehen, die gerendert wird und alle
Möglichkeiten nutzen, die Chrome bietet.

## vdrosrclient
Dies ist ein sehr einfach gehaltener Client, um Kommandos an den OSR Browser senden zu können. Hauptsächlich wird
er zum Testen verwendet, kann aber auch sonst sinnvoll sein.

### Parameter
```
--url <url>     lädt die URL im OSR Browser
--pause         Stoppt das Rendering
--resume        Startet das Rendering
--stream        Liest alle dirtyRecs (Bereiche der Seite, die sich verändert haben) und gibt eine Kurze Info aus.
--js <cmd>      Ein Javascript Kommando im OSR Browser ausführen
--connect       Ein einfacher Connect Test (Ping) an den OSR Browser
--mode          1 = HTML, 2 = HbbTV
```

## Best practice für ein noch bestehendes Problem
Aktuell gibt es beim Aufruf der allerersten Seite im VDR das Problem, das der VDR schneller startet, als der OSR Browser,
der VDR also die Seite nicht bekommt. Das kann man verhindern, indem man den OSR Browser startet und die
gewünschte Skin-HTML Seite initial lädt.
```
./vdrosrbrowser --skinurl="Pfad zur Skin Index Seite"
```
und danach erst den VDR startet.

## Historie
Probiert habe ich es zuerst mit webkitgtk+, hatte allerdings ziemliche Probleme, dieses vernünftig anzusteuern. 
JavaScript-Calls führten zum Absturz oder haben nicht funktioniert. Eine Lösung mit websocktes, statt direkten
JavaScript-Call hat zwar funktioniert, aber ich habe es nicht geschafft, den Browser-Inhalt vernünftig über das
VDR Fenster zu legen. Verschiedene Versionen von webkitgtk+ fuhren nicht vernünftig runter, sondern sind einfach
nur abgestürzt. Alle Probleme zusammen haben mich bewogen, es mit CEF zu versuchen und Ergebnisse waren viel schneller
sichtbar.
Webkit bzw. WPE hat mit Sicherheit Vorteile, aber die Probleme bzw. meine Unwissenheit der korrekten Nutzung überwiegen.
Vielleicht probiere ich es irgendwann noch einmal aus.
