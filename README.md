# CEF (chrome embedded framework) OSR (offscreen browser)
Der Browser wird aktuell für den VDR Skin vdr-plugin-htmlskin verwendet. Dabei wird eine HTML-Seite für die Skin-Bestandteile
offscreen gerendert und an das VDR Plugin zur Darstellung gesendet. Verwendet wird dabei libnanomsg und das IPC Protokoll.

Der OSR Browser kann und wird über verschiedene Kommandos gesteuert. Das passiert auch mittels libnanomsg und dem REQ/PUB Protokoll.

## Warnung
Der OSR Browser und das VDR Skin sind alles andere als leichtgewichtig. Wenn man bedenkt, das das CEF praktisch fast
einer Chrome Installation entspricht, düfte dies auch klar sein.

## Voraussetzungen
- libnanomsg (https://github.com/nanomsg/nanomsg), getestet ist Version 1.1.5.
- eine Installation des CEF (siehe unten)

Das Makefile bietet die Möglichkeit, vorkompilierte Binaries des CEF herunterzuladen und die Entwicklungsumgebung
vorzubereiten. Die vorkompilierten Binaries finden sich auf http://opensource.spotify.com/cefbuilds/index.html.
Diese Variante hat den Nachteil, das verschiedene Codecs nicht verfügbar sind (z.B. H.264/265 und andere Audio Codes),
damit ist ein hardware-beschleunigtes Rendering nicht immer verfügbar.
Siehe z.B. auch https://github.com/Zabrimus/cef-makefile-sample
'''
make prepare
'''

Eine andere Variante ist das vollständige Neukompilieren des CEF mit den entsprechenden Compile-Flags. Allerdings
wird dafür ein potenter Rechner, viel Hauptspeicher und Plattenspeicher benötigt. Diese Variante werde ich beschreiben,
sobald ich sie selber durchgeführt habe :)
'''
TODO
'''

## Build
Einmalig
'''
make prepare
'''

Ein neues Build wird gemacht mit
'''
make
'''

Im Unterverzeichnis Release befinden sich dann die beiden Binaries osrcef und osrclient zusammen mit 3 Softlinks in
das CEF Installationsverzeichnis. Die Softlinks sind absolut notwendig, da CEF leider verschiedene Dateien in genau
dem Verzeichnis sucht, in dem sich auch das Executable befindet und nirgends anderswo. Dies ist auch ein Grund, warum
der OSR Browser nicht als VDR Plugin implementiert werden kann ohne den VDR selbst zu patchen, was sehr wenig sinnvoll ist.


## Starten des Browsers
Die einfachste Variante zum Start ist
'''
./osrcef
'''

Ein Remote-Debugging mittels Chrome kann sinnvoll sein. Dazu müssen nur folgende Parameter verwendet werden
'''
/osrcef --remote-debugging-port=9222 --user-data-dir=remote-profile
'''
Und im Chrome die Seite http://localhost:9222 aufrufen. Man kann dann die Seite sehen, die gerendert wird und alle
Möglichkeiten nutzen, die Chrome bietet.

## osrclient
Dies ist ein sehr einfach gehaltener Client, um Kommandos an den OSR Browser senden zu können. Hauptsächlich wird
er zum Testen verwendet, kann aber auch sonst sinnvoll sein.

### Parameter
'''
--url <url>     lädt die URL im OSR Browser
--pause         Stoppt das Rendering
--resume        Startet das Rendering
--stream        Liest alle dirtyRecs (Bereiche der Seite, die sich verändert haben) und gibt eine Kurze Info aus.
--js <cmd>      Ein Javascript Kommando im OSR Browser ausführen
--connect       Ein einfacher Connect Test (Ping) an den OSR Browser
'''

## Best practice für ein noch bestehendes Problem
Aktuell gibt es beim Aufruf der allerersten Seite im VDR das Problem, das der VDR schneller startet, als der OSR Browser,
der VDR also die Seite nicht bekommt. Das kann man verhindern, indem man den OSR Browser startet und die
gewünschte Skin-HTML Seite initial lädt.
'''
./osrcef --skinurl=<Pfad zur Skin Index Seite>
'''
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