# CEF (chrome embedded framework) OSR (offscreen browser) für VDR
Der Browser wird aktuell für den VDR Plugin vdr-plugin-hbbtv verwendet. Dabei wird eine HbbTV-Seite offscreen gerendert 
und an das VDR Plugin zur Darstellung gesendet. Verwendet wird dabei libnanomsg und das IPC Protokoll.

Der OSR Browser kann und wird über verschiedene Kommandos gesteuert. Das passiert auch mittels libnanomsg und dem REQ/PUB Protokoll.

## Warnung
Der OSR Browser ist alles andere als leichtgewichtig. Wenn man bedenkt, das das CEF praktisch fast
einer Chromium Installation entspricht, düfte dies auch klar sein.

## Voraussetzungen
- ffmpeg / ffprobe
- eine Installation des CEF (siehe unten)

## Build

### Installation eines automatischen CEF-Builds nur für den vdr-osr-browser
Dies ist die bevorzugte Variante, da keine Dateien im System installiert werden und 
auch die einfache Möglichkeit besteht, den Browser schnell und einfach wieder zu löschen.
Dabei wird der Browser und CEF in ein einziges Verzeichnis Release installiert.
```
make prepare_release
make release
```
Das Release Verzeichnis enthält alles um den Browser zu starten.

### Installation eines automatischen CEF-Builds nach /opt/cef  
Das Makefile bietet die Möglichkeit, vorkompilierte Binaries des CEF herunterzuladen und in /opt/cef zu installieren
und die Entwicklungsumgebung vorzubereiten. 
Die vorkompilierten Binaries finden sich auf http://opensource.spotify.com/cefbuilds/index.html.
```
make prepare
make
```

### Compilieren von CEF aus den Sourcen
Eine andere Variante ist das vollständige Neukompilieren des CEF mit den entsprechenden Compile-Flags. Allerdings
wird dafür ein potenter Rechner, viel Hauptspeicher und Plattenspeicher benötigt und vor allen Dingen eine schnelle 
Internetverbindung. Es werden ca. 20 GB heruntergeladen (+/- 1 GB). Ein Core i7 mit 4/8 Kernen, 32 GB Ram und einer 
1Gbit/s Netzanbindung braucht ca 4-5 Stunden für den allerersten Build.

### Debian/Ubuntu-Packages
Im Ubuntu PPA sind Debian Packages verfügbar. Die Packages befinden sich hier:
https://launchpad.net/~zabrimus/+archive/ubuntu/vdr-cef
Nach der Installation reicht ein 
```
make
```

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

Der Log-Level kann gesteuert werden mit den möglichen Parametern
```
--trace
--debug
--info
--error
--critical
```
'--trace' ist nicht wirklich für den Produktiveinsatz geeignet, da die Anzahl der Ausgaben immens ist. 

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

