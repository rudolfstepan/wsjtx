# Änderungen im OE3SRO-Fork / Changes in the OE3SRO fork

[Deutsch](../README.md) | [English](../README.en.md) | [FT2](FT2.md)

## Deutsch

Dieser Fork von [WSJTX/wsjtx](https://github.com/WSJTX/wsjtx) basiert für die
hier dokumentierten Erweiterungen auf WSJT-X 3.0.2-devel, Commit `967c85a61`.
Die Einträge beschreiben den implementierten Stand vom 11. Oktober 2026;
sie sind keine Veröffentlichung einer neuen offiziellen WSJT-X-Version.

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
describe the implemented state on October 11, 2026; they do not announce a
new official WSJT-X release.

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
