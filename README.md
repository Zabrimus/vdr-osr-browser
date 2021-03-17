# CEF (chrome embedded framework) OSR (offscreen browser) für VDR
Der Browser wird aktuell für den VDR Plugin vdr-plugin-hbbtv verwendet. Dabei wird eine HbbTV-Seite offscreen gerendert 
und an das VDR Plugin zur Darstellung gesendet. Verwendet wird dabei libnanomsg und das IPC Protokoll.

Der OSR Browser kann und wird über verschiedene Kommandos gesteuert. Das passiert auch mittels libnanomsg und dem REQ/PUB Protokoll.

## Voraussetzungen
- libffmpeg.so muss vorhanden sein, falls man Videos sehen möchte. 
  Im beigefügten CEF wird libffmpeg.so aus rechtlichen Gründen nur dynamisch gelinkt.
- CEF verwendet nur vaapi zur Hardwarebeschleunigung. Besitzer von NVidia-Karten sollten 
  einen vaapi-vdpau Wrapper installieren. Leider habe ich keinen vaapi-nvdec Wrapper 
  oder ähnliches gefunden.

## libffmpeg.so
Da gibt es verschiedene Möglichkeiten:

### Selbst compilieren
https://github.com/wang-bin/avbuild bzw.
https://github.com/wang-bin/avbuild/wiki

### Ein Debian-Package von Ubuntu herunterladen und entpacken
Die Debian-Pakete findet man z.B. hier https://packages.ubuntu.com/de/bionic/chromium-codecs-ffmpeg-extra

Eine schöne Anleitung zum entpacken dann hier: https://vivaldi.com/blog/vivaldi-1-14-linux-delayed-autoupdate/

## Besitzer von NVidia-Karten
Sinnvoll ist auf jeden Fall einen Wrapper vaapi -> vdpau installieren. Gefunden habe diese hier

https://gitlab.com/aar642/libva-vdpau-driver und 
https://github.com/xtknight/vdpau-va-driver-vp9

## Build
Der Build wird gestartet mit

make -j 4

Im Unterverzeichnis "/Release" befinden sich nach erfolgreichem Bau die vollständige und lauffähige Umgebung des 
Browsers.    

## Starten des Browsers
Die einfachste Variante zum Start ist
```
cd Release
./vdrosrbrowser
```

Als MPEG-DASH Player kann entweder dash.js (http://cdn.dashjs.org/latest/jsdoc/index.html) oder der shaka-player (https://github.com/google/shaka-player) verwendet werden.
Beide werden bereits mitgeliefert. Standardmäßig wird dash.js verwendet. Mit folgenden Parametern wird die Auswahl gesteuert:
```
--dashplayer=dashjs
oder
--dashplayer=shaka
```

Ein Remote-Debugging mittels Chrome kann sinnvoll sein. Dazu müssen nur folgende Parameter verwendet werden
```
/vdrosrbrowser --remote-debugging-port=9222 --user-data-dir=remote-profile
```
Und im Chrome die Seite http://localhost:9222 aufrufen. Man kann dann die Seite sehen, die gerendert wird und alle
Möglichkeiten nutzen, die Chrome bietet.

Der Log-Level kann gesteuert werden mit den möglichen Parametern
```
--trace
--debug
--info
--error
--critical
```
'--trace' ist nicht wirklich für den Produktiveinsatz geeignet, da die Anzahl der Ausgaben immens ist. 


## Weitere nützliche Informationen
#### Chromium und vaapi
https://www.linuxuprising.com/2018/08/how-to-enable-hardware-accelerated.html
https://bugs.chromium.org/p/chromium/issues/detail?id=1097029#c28

Stimmt das noch? chrome://gpu zeigt Softwaredecoding, aber Hardwaredecoding funktioniert?

--use-gl=desktop --ignore-gpu-blocklist --enable-accelerated-video-decode
