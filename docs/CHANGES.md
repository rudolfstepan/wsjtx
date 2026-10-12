# Änderungen im OE3SRO-Fork / Changes in the OE3SRO fork

[Deutsch](../README.md) | [English](../README.en.md) | [FT2](FT2.md)

## Deutsch

Dieser Fork von [WSJTX/wsjtx](https://github.com/WSJTX/wsjtx) basiert für die
hier dokumentierten Erweiterungen auf WSJT-X 3.0.2-devel, Commit `967c85a61`.
Die Einträge beschreiben den implementierten Stand vom 12. Oktober 2026;
sie sind keine Veröffentlichung einer neuen offiziellen WSJT-X-Version.

### 2026-10-12 — Automatische Windows-Binaries

- Erstes öffentliches Custom-Release **3.0.2-oe3sro.1**, Tag `oe3sro-v3.0.2-1`:
  Windows-ZIP mit EXE und deutscher/englischer HTML-Anleitung sowie separates
  Dokumentations-ZIP und SHA256-Dateien. Release-Links stehen in beiden READMEs.
- Custom-Tags `oe3sro-vX.Y.Z-N` bauen und testen den getaggten Stand und
  veröffentlichen nach erfolgreicher Paketprüfung automatisch ein GitHub-Release.

- Neuer Workflow **OE3SRO Windows EXE** bei allen Pushes, Tags, Pull Requests
  und manuellem Start; nutzt den lokal bewährten MinGW64-/Qt5-Buildweg.
- Versioniertes Windows-x64-ZIP mit EXE, Decodern, DLLs, Daten, Start-Launcher,
  Buildmanifest und SHA256. Downloadlinks in beiden READMEs und im Actions-Lauf;
  Aufbewahrung 90 Tage, GitHub-Anmeldung für Artefaktdownloads erforderlich.
- CI prüft PSK Reporter, FT2/FT4 und die eigenständige Paketlaufzeit.
  Runnerpfade sind konfigurierbar; Paketprüfung benötigt im CI keine installierte
  Originalversion oder private Konfiguration. OmniRig-Quelle und Hash festgelegt.
- Der [erste GitHub-Build](https://github.com/rudolfstepan/wsjtx/actions/runs/38216578996)
  bestand alle Schritte. Das heruntergeladene Artefakt wurde zusätzlich auf
  ZIP-/EXE-Prüfsummen, Quellcommit und vollständigen Paketinhalt geprüft.

### 2026-10-12 — PSK-Reporter-Liste und Aktualisierung

- Die Abfrage verwendet den Modus, das Rufzeichen und die Dialfrequenz direkt
  aus dem laufenden Hauptfenster. Der aktuell gewählte Modus bleibt maßgeblich.
- **Current band and mode** zeigt standardmäßig Meldungen im gesamten aktuellen
  Band. Der bisherige Bereich Dialfrequenz bis +5 kHz konnte Meldungen ausblenden,
  die auf der Website sichtbar waren. **Current frequency and mode** schränkt
  die Anzeige optional wieder auf diesen Bereich ein, ohne zusätzliche Abfrage.
- Die Refresh-Schaltfläche zeigt die verbleibende Wartezeit bis zur nächsten
  erlaubten Abfrage. Mindestintervall weiterhin fünf Minuten.
- Netzwerk- oder XML-Fehler erhalten vorhandene Meldungen bis zu deren Ablauf.
  Kontextwechsel entfernt alte sichtbare Zeilen; Antworten aus einem inzwischen
  geänderten Modus, Rufzeichen oder Dialfrequenz werden verworfen.
- Direkte Anrufe, **Call next** und Band-Activity-Markierungen berücksichtigen
  weiterhin nur Meldungen im aktuellen Arbeitsbereich Dialfrequenz bis +5 kHz.
- Neuer Paketordner `dist-custom\OE3SRO-psk-fix`; Launcher, Paketierung,
  Paketprüfung und deutsche/englische Bedienungsanleitung angepasst.
- PSK-Funktionstests einschließlich Bandfilter und Moduswechsel, vollständiger
  Build sowie Paket- und Laufzeitprüfungen bestanden. Echte HTTPS-Abfrage mit
  der paketierten Qt-Laufzeit erfolgreich. Der Benutzer bestätigte anschließend,
  dass die Liste im laufenden Programm funktioniert.

### 2026-10-11 — FT2

Implementierungscommit: [4c7c48d53](https://github.com/rudolfstepan/wsjtx/commit/4c7c48d538fcdc58863d3fd38efabcf6c1fa351a).

- Experimenteller Modus **Mode → FT2** mit 3,75-Sekunden-Perioden, Sender,
  Decoder, Standardnachrichten, Auto Seq, Doppelklick-Anrufen und Wasserfall.
- Adaption der GPLv3-Implementierung aus WSJT-X Improved 3.1.0 PLUS 260522
  von Uwe Risse, DG2YCB. Quellen und Parameter stehen in [FT2.md](FT2.md).
- FT4-Decodierung mit zeitlich gestreckten FT2-Eingaben und Rückrechnung
  von Audiofrequenz und DT; CLI-Auswahl `jt9 --ft2` bzw. `jt9 -2`.
- Gemeinsamer Wellenformgenerator regeneriert seinen Gaussian-Puls bei
  geänderter Symbolzeit oder Sample-Rate, auch nach wiederholten Moduswechseln.
- FT2-Sequenzzuordnung berücksichtigt die auf ganze Sekunden gekürzten
  Decode-Zeitstempel. Soundkarten- und TCI-Pfad verwenden nominell 150 ms
  Audio-Startversatz. ADIF-Einträge enthalten `MODE=MFSK`, `SUBMODE=FT2`.
- Gespeicherte Modus-IDs bleiben erhalten. FT2-Frequenzvorgaben werden einmalig
  ergänzt, sofern noch keine FT2-Einträge existieren; andere Frequenzen und
  eigene FT2-Einträge bleiben erhalten. Spätere Löschungen werden respektiert.
- PSK-Reporter-Abfragen, Markierungen und Anrufhilfen unterstützen FT2.
  About nennt zusätzlich DG2YCB und verlinkt WS Suite.
- Neuer Paketordner `dist-custom\OE3SRO-ft2`; Launcher und Paketprüfung angepasst.
- FT2-WAV-Roundtrips, FT4-Regressionsfall, Moduswechsel, Sequenzzuordnung,
  Frequenzmigration und zwei offizielle FT2-Aufnahmen erfolgreich geprüft.
  Vollständiger Windows-Build und Paketprüfung bestanden. Echter Funkbetrieb
  und konkrete Transceiver-Umschaltzeiten wurden bisher nicht getestet.

### 2026-10-11 — PSK Reporter, Anrufhilfen und Branding

Implementierungscommit: [b203637e6](https://github.com/rudolfstepan/wsjtx/commit/b203637e6e2d33acb48bb85b0d78f2a3c086ccb1).

- **View → PSK Reporter — Who hears me?** integriert die offizielle HTTPS-
  XML-Abfrage. Anzeige der neuesten Meldung je Empfänger für eigenes Rufzeichen,
  aktuellen Modus und Dialfrequenz bis +5 kHz aus den letzten 30 Minuten.
- Abfragen frühestens alle fünf Minuten, Automatik nur bei geöffneter Ansicht;
  Timeout, Größenlimit und Prüfung von XML, Rufzeichen, Modus, Zeit und Frequenz.
- **Call station** setzt Rufzeichen und Locator, wählt Tx1 und aktiviert
  **Enable Tx** für den nächsten eingestellten TX-Zeitraum. Eigene TX-Frequenz
  und TX-Sequenz bleiben maßgeblich. Bei unpassendem Betrieb erscheint der Grund.
- Stationen, die das eigene Signal gemeldet haben, werden in **Band Activity**
  türkis, fett und unterstrichen markiert. Markierungen verfallen nach
  30 Minuten oder bei Kontextwechsel.
- Die Empfängerliste verwendet WSJT-X-Logbuchstatus und Hervorhebungsregeln.
- Persistente Favoriten, Suche, Neu-/Band-/Historien-/Altersfilter, die letzten
  20 Anrufe, **Repeat last**, **Call next**, Anrufzähler und **Halt Tx**.
- Credit **build by OE3SRO** in Version, Fenstertitel, About und Splashscreen;
  die interne Versionsnummer bleibt für Schnittstellen unverändert.
- Erstimport der installierten Konfiguration in ein separates Custom-Profil
  mit bytegleicher INI-Kopie und lokalen Stations-/Logbuchdaten. Bestehende
  Custom-Einstellungen werden durch spätere Starts und Paketprüfungen erhalten.
- Windows-Skripte für Abhängigkeiten, Build, Tests, Paketierung und Laufzeitprüfung.
  Private Konfiguration, Werkzeuge, Referenzdaten und Buildausgaben bleiben lokal.

Die deutsche und englische README wurden in
[8e987dcf1](https://github.com/rudolfstepan/wsjtx/commit/8e987dcf15e5b233cee1f9ec3aa66d0c42a99aba)
ergänzt. [8bf344fde](https://github.com/rudolfstepan/wsjtx/commit/8bf344fde)
speichert den FT2-Fortsetzungspunkt; dieser Commit ändert keine Programmfunktion.

## English

This fork of [WSJTX/wsjtx](https://github.com/WSJTX/wsjtx) uses WSJT-X
3.0.2-devel, commit `967c85a61`, as the base for these additions. The entries
describe the implemented state on October 12, 2026; they do not announce a
new official WSJT-X release.

### 2026-10-12 — Automatic Windows binaries

- First public custom release **3.0.2-oe3sro.1**, tag `oe3sro-v3.0.2-1`:
  Windows ZIP with EXE and German/English HTML guide, plus a separate
  documentation ZIP and SHA256 files. Release links appear in both READMEs.
- Custom tags `oe3sro-vX.Y.Z-N` build and test the tagged source and
  automatically publish a GitHub release after successful package verification.

- New **OE3SRO Windows EXE** workflow for all pushes, tags, pull requests,
  and manual runs, using the established local MinGW64/Qt5 build scripts.
- Versioned Windows x64 ZIP with EXE, decoders, DLLs, data, launcher, build
  manifest, and SHA256. Download links in both READMEs and the Actions summary;
  90-day retention and GitHub login required for artifact downloads.
- CI verifies PSK Reporter, FT2/FT4, and the standalone package runtime.
  Runner paths are configurable; CI package checks need no installed original
  version or private settings. OmniRig source revision and checksum are pinned.
- The [first GitHub build](https://github.com/rudolfstepan/wsjtx/actions/runs/38216578996)
  passed every step. The downloaded artifact was also checked for ZIP/EXE
  checksums, source commit, and complete package contents.

### 2026-10-12 — PSK Reporter list and refresh

- Queries read mode, callsign, and dial frequency directly from the running
  main window. The currently selected mode remains the query mode.
- **Current band and mode** now shows reports across the current band by
  default. The previous dial-frequency-to-+5-kHz range could hide reports shown
  on the website. **Current frequency and mode** optionally restores that
  narrower display without another API request.
- The Refresh button shows the remaining wait until the next allowed request.
  The minimum request interval remains five minutes.
- Network or XML errors preserve existing reports until they expire. Context
  changes clear old visible rows; replies from an outdated mode, callsign,
  or dial-frequency context are discarded.
- Direct calls, **Call next**, and Band Activity markers still use only reports
  within the current working range from dial frequency to +5 kHz.
- New package directory `dist-custom\OE3SRO-psk-fix`; launcher, packaging,
  package checks, and German/English usage documentation updated.
- PSK tests including band filtering and mode changes, the full build, and
  package/runtime checks passed. A real HTTPS query using the packaged Qt runtime
  succeeded. The user subsequently confirmed the list works in the running app.

### 2026-10-11 — FT2

Implementation commit: [4c7c48d53](https://github.com/rudolfstepan/wsjtx/commit/4c7c48d538fcdc58863d3fd38efabcf6c1fa351a).

- Experimental **Mode → FT2** with 3.75-second periods, transmit and decode,
  standard messages, Auto Seq, double-click calling, and waterfall display.
- Adapted from the GPLv3 implementation in WSJT-X Improved 3.1.0 PLUS 260522
  by Uwe Risse, DG2YCB. See [FT2.md](FT2.md) for sources and parameters.
- FT4 decoding of time-stretched FT2 samples, converting audio frequency
  and DT back to FT2; CLI selection through `jt9 --ft2` or `jt9 -2`.
- Shared waveform generator rebuilds its Gaussian pulse when symbol duration
  or sample rate changes, including repeated switches between modes.
- FT2 sequence assignment handles decode timestamps truncated to whole seconds.
  Soundcard and TCI paths use a nominal 150 ms audio start offset.
  ADIF entries use `MODE=MFSK`, `SUBMODE=FT2`.
- Existing stored mode IDs are preserved. FT2 frequency defaults are added
  once if there are no FT2 entries; other frequencies and custom FT2 entries
  are preserved. Later deletions are respected.
- PSK Reporter queries, markers, and calling tools support FT2. About also
  credits DG2YCB and links to WS Suite.
- New package directory `dist-custom\OE3SRO-ft2`, with launcher and package
  verification updated accordingly.
- FT2 WAV round trips, FT4 regression, mode switching, sequence assignment,
  frequency migration, and two official FT2 recordings passed. The full
  Windows build and package checks passed. Live RF operation and individual
  transceiver switching times have not yet been tested.

### 2026-10-11 — PSK Reporter, calling tools, and branding

Implementation commit: [b203637e6](https://github.com/rudolfstepan/wsjtx/commit/b203637e6e2d33acb48bb85b0d78f2a3c086ccb1).

- **View → PSK Reporter — Who hears me?** integrates the official HTTPS XML
  query. Shows each receiver's latest report for your callsign, current mode,
  dial frequency to +5 kHz, and the past 30 minutes.
- At least five minutes between queries; automatic refresh while the view
  is open. Timeout, response size limit, and XML/callsign/mode/time/frequency checks.
- **Call station** fills in callsign and locator, selects Tx1, and enables
  transmission in the next configured TX period. Your TX frequency and
  sequence remain in use. An unsuitable operating state shows the reason.
- Receiver callsigns in **Band Activity** are turquoise, bold, and underlined.
  Markers expire after 30 minutes or a context change.
- Receiver rows use WSJT-X logbook status and highlighting rules.
- Saved favorites, search, new/band/history/age filters, the last 20 calls,
  **Repeat last**, **Call next**, call counters, and **Halt Tx**.
- **build by OE3SRO** in the version display, window title, About, and splash
  screen; the internal version number remains unchanged for interfaces.
- Initial import of the installed configuration into a separate custom
  profile, with a byte-identical INI copy and station/logbook data. Subsequent
  starts and package checks preserve the existing custom settings.
- Windows scripts for dependencies, building, testing, packaging, and runtime
  verification. Private configuration, tools, reference data, and build outputs
  remain local.

German and English READMEs were added in
[8e987dcf1](https://github.com/rudolfstepan/wsjtx/commit/8e987dcf15e5b233cee1f9ec3aa66d0c42a99aba).
[8bf344fde](https://github.com/rudolfstepan/wsjtx/commit/8bf344fde) records the
FT2 resume checkpoint without changing program behavior.

## Quelltextübersicht / Source map

| Bereich / Area | Dateien / Files |
| --- | --- |
| PSK-Abfrage, Tabelle, Historie / PSK query, table, history | [PskReporterDialog.hpp](../widgets/PskReporterDialog.hpp) |
| Band-Activity-Markierung / Band Activity markers | [PskReporterHighlight.hpp](../widgets/PskReporterHighlight.hpp) |
| WSJT-X-Farben / WSJT-X colors | [displaytext.cpp](../widgets/displaytext.cpp), [displaytext.h](../widgets/displaytext.h) |
| Anrufsteuerung und FT2-Modus / Calling and FT2 mode | [mainwindow.cpp](../widgets/mainwindow.cpp), [mainwindow.ui](../widgets/mainwindow.ui) |
| Versionsanzeige / Version display | [revision_utils.cpp](../revision_utils.cpp), [revision_utils_wsjtx.cpp](../revision_utils_wsjtx.cpp), [AppVersion.cpp](../AppVersion/AppVersion.cpp) |
| About und Splash / About and splash | [about.cpp](../widgets/about.cpp), [SplashScreen.cpp](../widgets/SplashScreen.cpp) |
| FT2-Signalverarbeitung / FT2 signal processing | [FT2.md](FT2.md#technical-implementation) |
| Build und Tests / Build and tests | [tools](../tools), [tests/ft2](../tests/ft2), [tests/pskreporter](../tests/pskreporter) |

Bedienung / Operation: [README.md](../README.md), [README.en.md](../README.en.md).
Windows-Paket / Windows package: [CUSTOM-BUILD.md](../CUSTOM-BUILD.md).
