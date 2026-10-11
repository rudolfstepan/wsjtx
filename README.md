# OE3SRO Custom

**build by OE3SRO**

Dieser [Fork](https://github.com/rudolfstepan/wsjtx) von
[WSJTX/wsjtx](https://github.com/WSJTX/wsjtx) ergänzt WSJT-X um eine integrierte
PSK-Reporter-Empfängerliste und praktische Anrufhilfen. Du siehst direkt in der
App, welche Stationen dein Signal gemeldet haben, erkennst ihren Logbuchstatus
und kannst sie auswählen oder erneut anrufen.

Die Erweiterungen stammen von **OE3SRO**. Die Versionsanzeige, der Fenstertitel,
das About-Fenster und der Splashscreen tragen den Zusatz **build by OE3SRO**.
Der Custom-Build basiert derzeit auf **WSJT-X 3.0.2-devel**; die hier
beschriebenen Build-Skripte und Paketprüfungen sind für Windows ausgelegt.

## Neue Funktionen von OE3SRO

| Funktion | Nutzen |
| --- | --- |
| **PSK Reporter: Who hears me?** | Empfänger des eigenen Signals direkt in WSJT-X anzeigen, mit Rufzeichen, Locator, SNR, Empfangszeit und Modus. |
| **Station direkt anrufen** | Rufzeichen und Locator übernehmen, die erste Standardnachricht wählen und **Enable Tx** aktivieren. |
| **Empfangsmarkierung in Band Activity** | Rufzeichen aus der Empfangsliste erscheinen türkis, fett und unterstrichen; ein Tooltip zeigt die Empfangszeit. |
| **Bestehende WSJT-X-Farben** | Die Liste verwendet deine Hervorhebungsregeln und das Logbuch, einschließlich Band, Modus, DXCC, Locator und LoTW. |
| **Gespeicherte Favoriten** | Häufig gewünschte Stationen markieren und die Empfangsliste auf Favoriten filtern. |
| **Letzte Anrufe und Wiederanruf** | Die letzten 20 Einträge wieder auswählen oder die zuletzt angerufene Station mit einem Tastenkürzel erneut rufen. |
| **Nächste Station** | Unter den sichtbaren Stationen bevorzugt eine auswählen und anrufen, die im aktuellen Frequenz-/Moduskontext hier noch nicht angerufen wurde. |
| **Suche und Filter** | Nach Rufzeichen oder Locator suchen; neue Rufzeichen, neu auf diesem Band, Favoriten, bereits angerufene Stationen oder Meldungen der letzten 5/15/30 Minuten anzeigen. |
| **Anrufübersicht und Halt Tx** | Anrufversuche und letzte Anrufzeit sehen; die Aussendung direkt aus der Liste stoppen. |
| **Konfiguration übernehmen** | Die installierte WSJT-X-Konfiguration bytegenau in ein eigenes Custom-Profil kopieren, mit lokalen Stationsdaten und Logbuch. |

![Who hears me? mit Favoriten, Logbuchfarben, Anrufhistorie und Anrufhilfen](docs/images/psk-reporter-who-hears-me.png)

*Beispielansicht mit Testdaten. Die tatsächlichen Zeilenfarben richten sich
nach deinen WSJT-X-Einstellungen.*

## Bedienung

Öffne **View → PSK Reporter — Who hears me?**. Die Ansicht fragt
Empfangsmeldungen für dein konfiguriertes Rufzeichen im aktuellen Modus ab.
Sie berücksichtigt die letzten 30 Minuten und den Bereich von der
Dialfrequenz bis 5 kHz darüber. Pro Empfänger erscheint die neueste Meldung.
Eine neue Abfrage ist frühestens nach fünf Minuten möglich; die automatische
Aktualisierung arbeitet bei geöffneter Ansicht.

Die Spalte **QSO status** zeigt **New callsign**, **Worked** oder
**Worked on band**. Farben und Logbuchfilter aktualisieren sich auch nach
einem neuen Logbucheintrag oder geänderten Hervorhebungsregeln.

### Eine Station anrufen

1. Wähle eine Station in der Liste aus.
2. Klicke **Call station**, drücke **Ctrl+Enter** oder doppelklicke die Zeile
   außerhalb der Favoritenspalte.
3. Die App übernimmt Rufzeichen und Locator, wählt **Tx1** und aktiviert
   **Enable Tx**. Der Anruf wird im nächsten eingestellten TX-Zeitfenster
   gesendet, mit deiner eingestellten TX-Frequenz und TX-Sequenz.

Das direkte Anrufen unterstützt **FT8, FT4, JT65, JT9 und Q65** im normalen
QSO-Betrieb mit aktivem Tx1. Während einer laufenden Aussendung, bei Tune,
Dateiwiedergabe oder einer speziellen Betriebsaktivität zeigt die Ansicht
den Hinderungsgrund. Nach einem Wechsel des eigenen Rufzeichens, des Modus
oder der Dialfrequenz muss die Empfangsliste vor einem Anruf aktualisiert werden.

Empfangsmeldungen sind verzögert: Sie zeigen, dass eine Station dein Signal
empfangen hat. Sie enthalten keine verlässliche Sendefrequenz oder TX-Sequenz
des Empfängers und bestätigen keine aktuelle QSO-Bereitschaft.
**Open PSK Reporter map** öffnet zusätzlich die
[PSK-Reporter-Karte](https://www.pskreporter.info/pskmap.html) im Browser.

### Häufiger und gezielter anrufen

- **Favorite** markiert die ausgewählte Station dauerhaft. Ein `*` in
  **Fav.** kennzeichnet sie. Ein Doppelklick in diese Spalte schaltet die
  Markierung um. Der Filter **Favorites** zeigt gespeicherte Favoriten, die
  in der aktuellen Empfangsliste stehen.
- **Call recent** ruft den ausgewählten Eintrag aus der Anrufhistorie erneut
  auf. **Repeat last** wiederholt den letzten Anruf, auch ohne aktuelle
  Empfangsmeldung. Eigenes Rufzeichen, Modus und Dialfrequenz müssen dabei
  zum gespeicherten Anruf passen.
- **Call next** berücksichtigt die aktuellen Filter und die Sortierung.
  Bevorzugt wird eine im aktuellen Kontext noch nicht angerufene Station.
  Jeder Klick startet einen einzelnen Anruf.
- **Calls** zählt hier gestartete Anrufversuche; **Last call (UTC)** zeigt
  deren letzten Startzeitpunkt. Erfolgreiche QSOs werden über den
  Logbuchstatus angezeigt.
- **Halt Tx** führt denselben Stopp wie im Hauptfenster aus.

Favoriten und Anrufhistorie bleiben im jeweiligen Konfigurationsprofil nach
einem Neustart erhalten. Empfangsmarkierungen in **Band Activity** verschwinden
nach einem Rufzeichen-, Modus- oder Frequenzwechsel sowie spätestens 30 Minuten
nach der zugehörigen Meldung.

### Tastenkürzel im PSK-Fenster

| Tastenkürzel | Aktion |
| --- | --- |
| **Ctrl+F** | Suche öffnen/fokussieren |
| **Ctrl+Enter** | Ausgewählte Station anrufen |
| **Ctrl+R** | Letzte Station erneut anrufen |
| **Ctrl+N** | Nächste passende Station anrufen |
| **Ctrl+Shift+S** | Halt Tx |

## Unter Windows bauen

Die Skripte erwarten **MSYS2/MinGW64 unter `C:\msys64\mingw64`** mit
CMake, Ninja, GCC/G++, Fortran, Qt5, Boost, FFTW und libusb. Qt muss die
Komponenten Widgets, Network, Multimedia, PrintSupport, SerialPort, Sql,
WebSockets, ActiveQt, SVG und die Qt-Werkzeuge bereitstellen.

`Setup-CustomBuild.ps1` lädt das **Hamlib-4.7.2-SDK** mit SHA256-Prüfung sowie
die OmniRig-Typbibliothek in das lokale Verzeichnis `.tools`. MSYS2 und seine
Pakete müssen bereits installiert sein.

Führe im Repository-Verzeichnis aus:

```powershell
.\tools\Setup-CustomBuild.ps1
.\tools\Build-Custom.ps1
.\tools\Test-PskReporter.ps1
.\tools\Package-Custom.ps1
```

Der Release-Build liegt unter `build-custom`; das Paket mit Laufzeitbibliotheken,
Decodern und Stationsdaten unter `dist-custom\OE3SRO-calling`.
Die Skripte bauen WSJT-X mit den zugehörigen Hilfsprogrammen; MAP65 und die
zusätzlichen Simulatoren werden für dieses Paket nicht gebaut.

### Bestehende Konfiguration übernehmen und starten

Vor dem **ersten** Start kannst du deine installierte WSJT-X-Konfiguration
in das Custom-Profil übernehmen:

```powershell
.\tools\Copy-InstalledConfiguration.ps1
.\Start-Custom.ps1
```

Der Import liest `%LOCALAPPDATA%\WSJT-X\WSJT-X.ini` und erstellt
`%LOCALAPPDATA%\WSJT-X - Custom\WSJT-X - Custom.ini`. Die INI wird bytegenau
kopiert und per SHA256 geprüft. Gespeicherte Konfigurationen, Einstellungen
und Dateipfade bleiben erhalten; vorhandene Stationsdaten und Logbücher
werden ebenfalls kopiert. Eine private Sicherung liegt unter `.local`.
Existiert das Custom-Profil bereits, bricht der Import ab und bewahrt es.

`Start-Custom.ps1` startet das Paket mit `--rig-name Custom`. Bei späteren
Starts genügt dieses Skript. Beende eine noch laufende ältere Custom-Version
normal, bevor du das neue Paket startest.

Weitere Einzelheiten stehen in [CUSTOM-BUILD.md](CUSTOM-BUILD.md).
Lokale Konfigurationen, API-Antworten, Werkzeuge und Build-Ausgaben unter
`.local`, `.tools`, `build-custom` und `dist-custom` werden nicht in Git übernommen.

## Prüfungen

`Test-PskReporter.ps1` verwendet einen lokalen HTTP-Testserver und Qt im
Offscreen-Modus. Geprüft werden unter anderem Empfangsfilter, neueste Meldung
je Station, Abfrageintervall, fehlerhafte XML-Antworten, Farben und Logbuchstatus,
Band-Activity-Markierungen, Favoriten und gespeicherte Anrufe, Wiederanruf im
passenden Kontext, nächste Station und der Halt-Tx-Callback. Dabei wird keine
Funkübertragung ausgelöst.

Nach dem Konfigurationsimport lässt sich das Paket zusätzlich prüfen:

```powershell
.\tools\Verify-CustomPackage.ps1
```

Diese lokale Prüfung kontrolliert die Paketlaufzeit, OpenSSL und den Erhalt
der Custom-Konfiguration. Für den Versionsvergleich erwartet das Skript die
Originalinstallation unter `C:\WSJT\wsjtx\bin\wsjtx.exe`.

## Grundlage und Danksagung

Die Erweiterungen bauen auf der Arbeit des **WSJT Development Team** und
aller Mitwirkenden an WSJT-X auf. Empfangsdaten kommen von
[PSK Reporter](https://www.pskreporter.info/pskdev.html). Die Stationsfilter
orientieren sich an der
[GridTracker Call Roster](https://docs.gridtracker.org/latest/Making-GridTracker-Work-For-You/Using-Call-Roster.html).

Die folgende ursprüngliche Projektbeschreibung sowie die Lizenz- und
Markenhinweise bleiben erhalten.

---

## Weak Signal Communication Software — Upstream

### Programs
*WSJT-X*, *MAP65*, and *QMAP* are open-source, multi-platform programs designed for weak-signal digital communication by amateur radio. *WSJT-X* works with a standard SSB transceiver, while *MAP65* and *QMAP* use wideband SDR-style hardware. The programs are open source, free of charge, and licensed under the GNU General Public License.

*WSJT-X* implements communication protocols or "modes" called **FST4**, **FST4W**, **FT4**, **FT8**, **JT4**, **JT9**, **JT65**, **MSK144**, **Q65**, **WSPR**, and **Echo**. The first nine modes were designed for making reliable, confirmed QSOs in a wide variety of weak-signal propagation circumstances. These modes use timed Transmit/Receive sequences of specific lengths, synchronized with UTC. **WSPR** mode is for probing potential propagation paths with low-power transmissions, and **Echo** mode is for detecting and measuring reflections of your own signals from the Moon.

*MAP65* implements a wideband receiver for **JT65** and **Q65** signals, optimized for EME on the VHF/UHF bands. It can be used together with *Linrad* (by SM5BSZ) or with direct input from a soundcard, FUNcube Dongle, or similar hardware. The program decodes all **JT65** or **Q65** signals in a passband up to 90 kHz wide, producing a sorted band map of decoded callsigns. In a dual-polarization system, *MAP65* optimally matches the linear polarization angle of each signal, thereby eliminating problems with Faraday rotation and spatial polarization offsets. *MAP65* also handles T/R switching and generates suitable messages and audio waveforms for the selected mode.

*QMAP* is similar to *MAP65* in providing wideband reception of signals over a full EME sub-band. It works cooperatively with *WSJT-X*, supporting the **Q65** mode in both 30-second and 60-second submodes. The *QMAP* + *WSJT-X* combination provides full rig control and Doppler compensation for the EME (Earth-Moon-Earth) path.


### Mode Details
**JT4**, **JT9**, and **JT65** use nearly identical message structure, efficient compression of messages for minimal QSOs, and 60-second T/R sequences. **JT4** and **JT65** were designed for EME on the VHF, UHF, and microwave bands, while **JT9** is optimized for the MF and HF bands. **JT9** is about 2 dB more sensitive than **JT65** while using less than 10% of the bandwidth.

**FT4** and **FT8** use T/R cycles of only 7.5 and 15 s, respectively. They have become extremely popular for world-wide DXing on the HF bands. **MSK144** is designed for Meteor Scatter on the VHF bands. **Q65** offers submodes with T/R sequence lengths from 15 seconds to 5 minutes, and a wide range of tone spacings. Particular Q65 submodes are highly recommended for EME, ionospheric scatter, and other weak signal work on VHF, UHF, and microwave bands. These modes include message formats explicitly supporting nonstandard callsigns and some popular radio contests.

**FST4** and **FST4W** are designed particularly for the LF and MF bands. On these bands their fundamental sensitivities are better than other *WSJT-X* modes with the same sequence lengths, approaching the theoretical limits for their rates of information throughput. **FST4** is optimized for two-way QSOs, while **FST4W** is for quasi-beacon transmissions of **WSPR**-style messages. **FST4** and **FST4W** do not require the strict, independent time synchronization and phase locking of modes like EbNaut.


### License
Licensed under the GNU General Public License v3.0 — see [COPYING](COPYING).

### Trademark
"WSJT-X," "MAP65," and "QMAP" are trademarks of the WSJT Development Team — see [TRADEMARK.md](TRADEMARK.md) for our policy on use of these names.
