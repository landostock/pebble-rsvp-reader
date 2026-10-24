# Pebble RSVP Hardwaretest

Diese Testkarte führt den ersten vollständigen lokalen Test mit Pixel 10 und Pebble Time 2 durch. Sie verändert keine Bücher auf dem Server.

## Ressourcen

- Android-App: `android/app/build/outputs/apk/debug/app-debug.apk`
- Watch-App: `watch/build/watch.pbw`
- Pixel-Vorbereitung: `scripts/prepare-pixel-test.sh`
- Watch-Installation über USB: `scripts/install-watch-over-usb.sh`
- Android-Transportprotokoll: `scripts/watch-android-logs.sh`
- Lokale Bibliothek: `http://localhost:8787`

## 1. Pixel vorbereiten

1. Auf dem Pixel `Einstellungen > Über das Telefon > Build-Nummer` öffnen und siebenmal auf die Build-Nummer tippen.
2. Unter `Einstellungen > System > Entwickleroptionen` die USB-Debugging-Funktion einschalten.
3. Pixel per USB mit dem Mac verbinden, entsperren und die Computerberechtigung auf dem Telefon bestätigen.
4. Im Projektverzeichnis ausführen:

```sh
./scripts/prepare-pixel-test.sh
```

Das Skript testet und baut die Android-App, installiert das APK, richtet `adb reverse tcp:8787 tcp:8787` ein und öffnet die lokale Bibliothek im Browser des Pixels.

Die Weiterleitung ist absichtlich nur über USB erreichbar. Auf dem Pixel zeigt `localhost:8787` dadurch zum Server auf dem Mac. Nach einem Neustart oder dem Trennen des USB-Kabels muss das Skript beziehungsweise `adb reverse tcp:8787 tcp:8787` erneut ausgeführt werden.

## 2. Android koppeln

1. Im Pixel-Browser in Pebble RSVP `Einstellungen` öffnen.
2. Unter `Android Companion` ein Geräte-Token mit dem Namen `Pixel 10` erzeugen.
3. Die einmal angezeigten Kopplungsdaten kopieren.
4. Die App `Pebble RSVP` auf dem Pixel öffnen.
5. Kopplungsdaten einfügen und `Kopplung speichern` wählen.

Erwartetes Ergebnis:

- Die Verbindung zeigt `Gekoppelt mit http://localhost:8787`.
- Der erste Sync lädt Manifest, Reader-Pakete und Cover.
- Ein zweiter Sync meldet, dass die Bibliothek bereits aktuell ist.
- Der Token wird nach dem Speichern nicht mehr angezeigt.

Offline-Test:

1. In der Android-App die aktuellen Buch-, Paket- und Coverzahlen notieren.
2. USB trennen oder den Server kurz stoppen.
3. `Jetzt synchronisieren` wählen.
4. Prüfen, dass der Fehler nur als Status erscheint und die Cache-Zahlen unverändert bleiben.
5. USB und Weiterleitung wiederherstellen.

## 3. Watch-App installieren

Voraussetzung ist die aktuelle Pebble-Mobile-App mit einer gekoppelten Pebble Time 2.

Empfohlener USB-Weg:

1. In der Pebble-App unter `Settings > Connectivity` die Option `Use LAN developer connection` aktivieren.
2. Pixel per USB angeschlossen und entsperrt lassen.
3. Im Projektverzeichnis ausführen:

```sh
./scripts/install-watch-over-usb.sh
```

Das Skript startet über den geschützten ADB-Empfänger der Pebble-App eine lokale Entwicklerverbindung, leitet Port 9000 ausschließlich über USB weiter und installiert die Watch-App. Es benötigt weder WLAN noch eine funktionierende CloudPebble-Kontozuordnung.

Alternative über Cloud Dev Connect:

1. In der Pebble-App `Devices > … > Enable Dev Connect` öffnen und mit GitHub anmelden.
2. Auf dem Mac mit demselben GitHub-Konto anmelden und installieren:

```sh
pebble login
cd watch
pebble build
pebble install --cloudpebble
```

Alternative im gleichen WLAN ohne USB:

1. In der Pebble-App Entwicklermodus und Developer Connection aktivieren.
2. Die dort angezeigte Server-IP einsetzen:

```sh
cd watch
pebble build
pebble install --phone IP_DES_PIXELS
```

Erwartetes Ergebnis: `Pebble RSVP` erscheint im App-Menü der Uhr und lässt sich öffnen.

## 4. Transport testen

In einem zweiten Terminal im Projektverzeichnis starten:

```sh
./scripts/watch-android-logs.sh
```

Dann in der Android-App:

1. `Watch-App öffnen` wählen.
2. Prüfen, dass Pebble RSVP auf der Uhr startet.
3. `Testpaket senden` wählen.
4. Prüfen, dass die Android-App eine erfolgreiche PebbleKit-Übertragung meldet. Erfolgreiche Datenpakete verwenden die native PebbleKit/AppMessage-Zustellung; ein zusätzlicher Anwendungs-ACK wird absichtlich nicht mehr gesendet.

Reader-Fenster testen:

1. In der Android-App unter `Gecachte Bücher` eines der synchronisierten Bücher wählen.
2. Die App öffnet Pebble RSVP automatisch und überträgt das erste Reader-Fenster.
3. Auf der Uhr müssen Titel und echter Buchtext erscheinen; mit `Select` startet oder pausiert RSVP.
4. Mit langem `Select` direkt zwischen RSVP und Volltext wechseln. Die Anzeige wechselt seitenweise zwischen den geladenen Fenstern und setzt keine eckigen Markierungszeichen um das aktuelle Wort.
5. Mit langem `Up` das Lesemenü öffnen. Es enthält `Kapitel`, `Bibliothek`, `Einstellungen`, `Anzeige` und `Statistik`; zwischen RSVP und Volltext wird direkt mit langem `Select` gewechselt.
6. Unter `Anzeige > Beleuchtung` die Modi `Intelligent`, `Immer` und `System` prüfen. `Intelligent` leuchtet während RSVP und Volltext, gibt die Beleuchtung bei Pause, Bibliothek und Menüs aber an PebbleOS zurück.

Die Statuszeile der Android-App meldet die Anzahl der übertragenen Wörter und Pakete. Im Logcat der Watch-App erscheint bei erfolgreicher Rekonstruktion `Reader window loaded`. Beim Erreichen des Fensters sendet die Watch automatisch die nächste Fensteranfrage; im Android-Log erscheint dann `Sent requested reader window`.

Watch-Galerie testen:

1. Die Watch-App öffnen und warten, bis die Bibliotheksliste übertragen wurde.
2. Im Lesemenü mit langem `Up` öffnen und `Bibliothek` wählen. Die Liste zeigt Titel, Autor, den höchsten Lesefortschritt mit einer Nachkommastelle und, sofern vorhanden, ein farbreduziertes Cover.
3. Mit `Up` und `Down` blättern und mit `Select` ein Buch auswählen.
4. Android überträgt anschließend nur das ausgewählte Reader-Fenster.
5. Im Volltextmodus mit `Up` und `Down` fensterweise vor- und zurückblättern. Am Abschnittsrand lädt die Uhr das vorherige oder nächste Fenster, ohne den Textmodus zu verlassen.
6. Im RSVP-Modus mit `Back` zum Satzanfang und mit erneutem `Back` zum vorherigen Satz springen.
7. Im Lesemenü `Einstellungen` Sprache, Sortierung, Tempo, Wiedereinstieg, lange Wörter und Touch testen. Unter `Anzeige` Fokus, Informativ und Custom prüfen. Bei Fokus verschwinden Titel und Fußzeile nur während des aktiven Lesens; in Pause sind alle Informationen sichtbar. Die Detailoptionen werden nur bei Custom eingeblendet. Eine WPM-Änderung wird kurz nahe der Wortposition eingeblendet.
8. Im Reader `Down` lange drücken und zwischen Prozent, aktueller Position/Gesamtwörtern und verbleibenden Minuten wechseln.
9. Unter `Statistik` beide Seiten prüfen: Fortschritt, Position, verbleibende Wörter, Restzeit, Sitzungswörter und Durchschnittstempo sowie heutige/Gesamtwörter, gesamte und durchschnittliche Lesezeit und aktuelle/längste Serie.

Danach Bluetooth am Pixel kurz ausschalten, erneut senden und prüfen, dass die Android-App nicht abstürzt. Bluetooth wieder einschalten und den erfolgreichen Test wiederholen.

## 5. Ergebnisse festhalten

Für den nächsten Implementierungsschritt werden benötigt:

- Android-Version des Pixels;
- Version der Pebble-Mobile-App und Watch-Firmware;
- Erfolg oder Fehler bei `Watch-App öffnen`;
- Erfolg oder Fehler der nativen PebbleKit-Übertragung;
- relevante Ausgabe aus `watch-android-logs.sh`;
- Verhalten nach Bluetooth-Unterbrechung.

Die maximale stabile Paketgröße wird erst nach erfolgreichem Basistest gemessen. Der aktuelle Payload bleibt bis dahin konservativ bei 160 Byte.

## Verifizierter Basistest vom 30. August 2026

- Pixel 10 mit Android 17 (API 37).
- Pebble-Mobile-App 1.11.0.3.
- Pebble Time 2 mit PebbleOS 4.36.2.
- Android-Begleitapp installiert und mit vier Büchern synchronisiert.
- Vier Reader-Pakete und vier Cover im Offline-Cache.
- Watch-App über `scripts/install-watch-over-usb.sh` installiert.
- Watch-Startbefehl erfolgreich.
- Testpaket erfolgreich übertragen.
- Native PebbleKit-Übertragung und rollendes Prefetch verifiziert; der neue Vierfensterpuffer und die intelligente Beleuchtung benötigen den nächsten Regressionstest.
