# Änderungen im OE3SRO-Fork / Changes in the OE3SRO fork

[Deutsch](../README.md) | [English](../README.en.md) | [FT2](FT2.md)

## Deutsch

### 2026-10-25 — HackRF FT8 auf 17 m und robuster USB-/TCI-Start

- Zusätzlicher Starter `start-hackrf-ft8-17m.bat` mit eigenem Profil,
  FT8/18,100 MHz beim Erststart, TCI 40004 und UDP 2243; RX und TX über USB.
- Maximale Softwarevorgaben: TX-VGA 47 dB, IQ-Pegel 1,0, interner RF-Verstärker
  nur bei TX. USB-Seitenband; bestehende Profile bleiben erhalten.
- Bereitschaft erst nach echten RX-Samples, bis 30 s Gerätewartezeit,
  RX-Neustart nach 3 s Stillstand; TCI bleibt bei RX-Erholung verbunden.
  WSJT-X wartet bis 35 s auf den TCI-Start und 5 s auf die Audioantwort.
- Lokaler Build alternativ mit MinGW Makefiles ohne Ninja; Buildverzeichnis
  für HackRF-Prüfungen und Paketierung wählbar.
- 4669 Mock-/Profilprüfungen, verzögerter USB-Start, RX-Erholung und echte
  30-s-RX-Prüfung auf 17 m bestanden. Benutzer bestätigt FT8-Empfang seines
  CQ über PSK Reporter in Finnland (−25 dB) und Spanien (−18 dB).
  Kein abgeschlossener QSO und keine kalibrierte Leistungs-/Spektrummessung.
  [Bedienung](HACKRF-WSJTX.txt), [Prüfbericht](validation/hackrf-ft8-17m/README.txt).

### 2026-10-24 — Release 3.0.2-oe3sro.5 mit REIST16-QSO/2

- Erstes Download-Release mit REIST16-QSO/2, Pluto-/HackRF-Bridges und
  WASAPI-WebSDR-Empfang. [Release Notes](releases/3.0.2-oe3sro.5.md).
- REIST16-Bedienung, Profilgrenzen und deutsche/englische Offline-Anleitung
  aktualisiert; QSO/1-Hardwarebild klar vom aktuellen QSO/2 getrennt.
- Bridge-Prüfungen finden Hamlib-DLLs auch auf frischen CI-Runnern.
  HackRF und der produktive SDR-Einstellungsdialog werden in CI geprüft.

### 2026-10-23 — WebSDR und REIST16-Aufnahmefenster

- Browserempfang per WASAPI-Loopback; Yaesu-TX und Browser-RX in einer
  Custom-Instanz, zusätzlicher reiner REIST16-Selbsttest-Empfänger.
- Reiner Empfang bis 14,400 s, Transceive weiter 13,824 s für Auto Seq.
  Verzögerte Rauschframes: 6/6 statt 0/6; kein zusätzlicher dB-Nachweis.
- CQ-/QSO-Eingabeprüfung, Nachrichtenlänge und PTT-Anzeige korrigiert.
  [Bedienung](WEBSDR-WSJTX.txt), [Aufnahmeprüfung](../docs/validation/reist16-rx-window/README.txt).

### 2026-10-22 — HackRF-TX und Pluto-Verbindungsdiagnose

- HackRF wird beim Laden, Auswählen und Speichern der Einstellungen anhand
  der registrierten TCI-Fähigkeit erkannt. Der TCI-Audioweg bleibt aktiv.
- PTT ohne gestarteten TCI-Audiostream wird abgelehnt, ohne die Bridge zu
  beenden. Ein späteres audio_start startet keine verworfene Aussendung.
- Pluto-Startfehler nennen den gewählten URI; der Starter erklärt Anschluss,
  Adresswahl und die eigene HackRF-Startdatei.
- Regression mit echtem Einstellungsdialog, CAT-Test und Audio gegen simulierte
  Bridges sowie DLL-Mocks bestanden. Keine neue HF-Abnahme.

### 2026-10-22 — Eigene HackRF-One-Instanz

- `start-hackrf.bat` / `Start-HackRF.ps1`, eigenes Profil und TCI-/UDP-Port.
- USB-IQ-Bridge mit Halbduplex-Umschaltung, begrenzten verlustfreien Queues,
  vollständiger TX-Startpufferung sowie Fehler-/PTT-Cleanup.
- Getrenntes portables Paket, DLL-/Profil-/TCI-/Mock-Prüfungen ohne HF.
- [Bedienung und Grenzen](HACKRF-WSJTX.txt); neue reale Funkabnahme offen.

### 2026-10-22 — REIST16-QSO/2

- 12,8 s Signalzeit in 15-s-Slots, 16-GFSK mit 5 Hz Tonabstand,
  CRC14 und LDPC(174,91). QSO/1 bleibt empfangbar; beide Sender aktualisieren.
- Nichtkohärente und kohärente Demodulation mit begrenzter OSD-Nachsuche,
  unabhängiger Synchronisations- und QSO-Typprüfung.
- PCM/TCI-Puffer, Zeitfenster und Wasserfallmarker für das neue Format.
- Unabhängiger Rauschvergleich: bei −22 dB SNR2500 QSO/2 20/32, FT8 15/32;
  keine allgemeine FT8-Gleichwertigkeit oder neue RF-Abnahme behauptet.
- [Profil und vollständige Messbedingungen](REIST16-QSO2.md).


### 2026-10-21 — REIST16 und PlutoSDR (experimentell)

- Neue eigene Betriebsart REIST16: 15-s-UTC-Slots, 16-FSK, 77-Bit-
  Standardnachrichten, K7-Fehlerschutz, Interleaver, CRC32 und Auto Seq.
- Native TX-/RX-Kette und eigene Moduskennung; keine FT8-Funkkompatibilität.
- Pluto als Funkgerät einer zweiten Instanz über lokale TCI-Bridge, mit
  Frequenzsteuerung, PTT und Audio. TX: vier 40-ms-Puffer; RX: 10 ms.
- Vollrufzeichen mit /P oder /R bleiben bei Adressierung und DX-Auswahl getrennt.
- Benutzer-Versuch vom 21.10.: Yaesu OE3SRO und Pluto OE3SRO/P auf 432,500 MHz,
  beidseitige Decodes, Auto Seq bis RR73 und danach 73-TX. Keine gemessene
  Empfangsschwelle; letzter 73-Empfang nicht im Screenshot sichtbar.
- [REIST16-Dokumentation](REIST16.md), [Profilvertrag](REIST16.txt),
  [Pluto](PLUTO-WSJTX.txt); Original-Screenshot, native und Bridge-Tests in CI.
- REIST16 gehört zum aktuellen Quellstand, nicht rückwirkend zum Release 3.0.2-oe3sro.4.


### 2026-10-16 — Release 3.0.2-oe3sro.4: GitHub-Build mit UCRT64

Die Installation des entfallenen MinGW64-libusb-Pakets verhinderte den Build
von 3.0.2-oe3sro.3 vor der Kompilierung. Keine ZIPs dieser Version veröffentlicht.
Die GitHub-CI verwendet jetzt MSYS2 UCRT64; lokale MinGW64-Builds bleiben möglich.
Tag **oe3sro-v3.0.2-4**, [Release Notes](releases/3.0.2-oe3sro.4.md).

Dieser Fork von [WSJTX/wsjtx](https://github.com/WSJTX/wsjtx) basiert für die
hier dokumentierten Erweiterungen auf WSJT-X 3.0.2-devel, Commit `967c85a61`.
Die Einträge beschreiben den implementierten Stand vom 16. Oktober 2026;
sie sind keine Veröffentlichung einer neuen offiziellen WSJT-X-Version.

### 2026-10-16 — Windows-Release 3.0.2-oe3sro.3

[Release Notes](releases/3.0.2-oe3sro.3.md): persönliche DX-Zeitfenster und
persistente PSK-Reporter-Abfragepause. Tag **oe3sro-v3.0.2-3** baut die Anwendung
und veröffentlicht Windows-/Dokumentations-ZIP mit SHA256 nach erfolgreicher CI.

### 2026-10-16 — Persönliche empirische DX-Zeitfenster

- HTTP-429-/503-Behandlung: steigende Wartezeit, `Retry-After` beachten und
  Abfragepause über Neustarts/Profilwechsel mit derselben Konfiguration erhalten.
  Spezifische Statusmeldung statt langer Fehler-URL. Aktuelles lokales Paket
  mit unverändertem Custom-Profil: `dist-custom/OE3SRO-psk-backoff`.

- **Personal DX windows** im PSK-Reporter-Fenster: lokale Aggregation nach
  DXCC-Land, Band, UTC-Stunde und Kalendermonat mit Monats-/Land-/Bandfilter.
- Rangfolge nach unterschiedlichen Empfangstagen, danach Empfänger-Tagen;
  wiederholte Abfragen zählen denselben Empfänger pro Datum/Stunde einmal.
  Rufzeichen, Modus und benennbare Stations-/Antennenprofile bleiben getrennt.
- Ansicht **Next 24 hours (UTC)** mit zeitlicher Reihenfolge und **Now** für
  laufende historische Stundenfenster. Filter nach mindestens N Empfangstagen
  und den letzten 90/365 UTC-Tagen; erste/letzte Beobachtung je Fenster sichtbar.
- Unveränderte Beobachtungen lösen keinen erneuten Schreibvorgang aus;
  ungültige gespeicherte Stunden werden beim Laden verworfen.
- Positive Beobachtungen sind keine QSO-Wahrscheinlichkeiten. Datensammlung
  mit den bestehenden Abfragen; keine historischen Downloads. Details unter
  [CUSTOM-BUILD](../CUSTOM-BUILD.md#persönliche-ausbreitungsstatistik).
- PSK-/Statistiktests, visueller Dialogcheck, vollständiger Windows-Build und
  Paketlaufzeitprüfung bestanden. Neues lokales Paket: `dist-custom/OE3SRO-propagation`.

### 2026-10-15 — Windows-Release 3.0.2-oe3sro.2

Die [zweite Custom-Paketversion](https://github.com/rudolfstepan/wsjtx/releases/tag/oe3sro-v3.0.2-2) fasst die
nachfolgenden FT8-/FT4-/FT2-Verbesserungen und Logbuch-/UDP-Korrekturen zusammen.
Tag `oe3sro-v3.0.2-2` baut Windows-Anwendung und DE/EN-Dokumentation in GitHub
Actions; beide ZIPs werden nach den Tests mit SHA256-Dateien veröffentlicht.

### 2026-10-15 — Erweiterung auf FT4 und FT2

- Native C++-GFSK-Wellenformen einschließlich modusspezifischer Rampen,
  doppelter zentrierter Phase und interpolierter Sinustabelle. Der bereits
  native LDPC-Encoder wird von FT8/FT4/FT2 gemeinsam verwendet.
- Native Synchronisationsberechnung und parallele Frequenz-/Zeitsuche mit bis
  zu acht Workern. CPU-/OpenMP-Limits beachtet, kleine Feinsuchen seriell,
  ursprüngliche Reihenfolge bei gleichen Maxima erhalten.
- Private Audio-Snapshots und gemeinsamer Dialog
  **View → TX quality — FT8 / FT4 / FT2**; FT8-Threadregler nur in FT8 aktiv.
- Wellenform-, Grob-/Feinsuch- und Parallelvergleiche gegen Originale bestanden.
  Aufnahmevergleiche: jeweils 19 FT4-, 6/2 FT2-Nachrichten auch unter Teamlimits.
  Kernelmessungen: FT4-Welle ca. 8,6 → 1,0 ms; FT2 ca. 4,2 → 0,4 ms. Keine generellen
  Gesamtdecoder-Speedups oder RF-Empfangsverbesserungen daraus ableiten.
- Neues Paket `dist-custom\OE3SRO-ftx-native`; vorhandenes Custom-Profil erhalten.
  Hybrid-BP/OSD und weitere Decoderteile enthalten weiterhin Fortran.

### 2026-10-15 — FT8: C++-Kerne, SMP und TX-Diagnose

- Normale FT8-Wellenform sowie LDPC-Encoder und Standard-/SMP-BP-Kerne auf C++
  portiert. Zentrierte Phase nach dem Zustandsprinzip des REIST-Papers,
  doppelte Phasengenauigkeit, interpolierte Sinustabelle, gepackte Parität und
  getrennte Decoder-Arbeitsdaten. Vollständige FT8-Kette noch nicht C++:
  Nachrichtenpackung, Synchronisation und Hybrid-BP/OSD enthalten weiter Fortran.
- Fehler bei OpenMP-Limits behoben: Die tatsächliche Teamgrenze begrenzt die
  angeforderte Zahl der maximal zwölf Worker; deaktivierte Teams werden beachtet.
- Normales FT8 nutzt einen eigenen Audiopuffer. Nachrichtenende ohne zusätzliches
  Sample, kein Weiterverwenden ungültiger Audioformate und TX-Stopp vor Audiofehlerdialog.
- **View → TX quality — FT8** ergänzt Audio-/Start-/optionale CAT-ALC-Anzeige sowie
  die vorhandenen SMP-Bedienelemente. ALC hat eine backendabhängige relative Skala.
- Differenztests gegen Originalkerne und WAV-Aufnahme ohne Decode-Verlust im
  geprüften Material. Lokale Kernelmessungen: Wellenform etwa 21 → 2–3 ms,
  Encoder etwa 8,3 → 0,25 µs, BP etwa Faktor 1,2. Keine Aussage über allgemeine
  Empfindlichkeit oder bessere RF-Empfangbarkeit; kein Geräte-/Funk-Sendetest.
- Paketpfad `dist-custom\OE3SRO-ft8-native`, Start über `start.bat` mit bestehendem
  Custom-Profil. [Umfang, Paper-Einordnung und Tests](FT8.md).

### 2026-10-15 — Fehlerprüfung: Logbuch und UDP

- Beim Einlesen des ADIF-Logbuchs werden vollständige, noch gepufferte QSOs
  auch nach Erreichen des Dateiendes verarbeitet. Zuvor konnten einzeilige
  Dateien oder mehrere QSOs in der letzten Zeile unvollständig eingelesen
  werden; dadurch konnte die Anzeige „bereits gearbeitet“ falsch sein.
  Die Logbuchdatei selbst wurde dabei nicht verändert.
- UDP-Befehle werden nur bei passender Ziel-ID der WSJT-X-Instanz verarbeitet.
  Zuvor wurden auch Befehle für andere Instanzen angenommen.
- Ein UDP-Serverwechsel verwirft eine noch laufende DNS-Abfrage. Eine verspätete
  Antwort kann die neue Adresse oder einen abgeschalteten Server nicht ersetzen.
- Lokale Regressionstests unter `tests/core`, Aufruf `tools/Test-Core.ps1`;
  auch in den Windows-CI-Workflow aufgenommen.
- Neues lokales Paket `dist-custom\OE3SRO-bugfix`, da die bisherige Version
  während der Korrektur lief. Start über `start.bat` oder `Start-Custom.ps1`.
- Lokal bestanden: neue Regressionstests, Gesamtbuild, PSK-Tests, FT2/FT4
  einschließlich beider Referenz-WAVs und Paketlaufzeitprüfung. Build- und
  Paket-EXE sind hashgleich. Keine RF-Aussendung für diese Prüfungen.

### 2026-10-12 — Automatische Windows-Binaries

- Erstes öffentliches Custom-Release **3.0.2-oe3sro.1**, Tag `oe3sro-v3.0.2-1`:
  Windows-ZIP mit EXE und deutscher/englischer HTML-Anleitung sowie separates
  Dokumentations-ZIP und SHA256-Dateien. Release-Links stehen in beiden READMEs.
- Der [Release-Tag-Build](https://github.com/rudolfstepan/wsjtx/actions/runs/38218198831)
  bestand Build, PSK-/FT2-/FT4-Tests, Paketprüfung und Veröffentlichung. Die vier
  veröffentlichten Dateien wurden ohne Anmeldung heruntergeladen; ZIP-/EXE-
  Prüfsummen, Quellcommit und Paketinhalt stimmen.
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

### 2026-10-25 — HackRF FT8 on 17 m and robust USB/TCI startup

- Additional `start-hackrf-ft8-17m.bat` launcher with an independent profile,
  initially FT8/18.100 MHz, TCI 40004 and UDP 2243; USB RX and TX.
- Maximum software defaults: TX VGA 47 dB, IQ amplitude 1.0, internal RF
  amplifier during TX only. Upper sideband; existing profiles are preserved.
- Readiness requires actual RX samples; up to 30 s device wait, RX restart
  after 3 s without data, TCI connection preserved during RX recovery.
  WSJT-X allows 35 s TCI startup and 5 s audio acknowledgement.
- Optional MinGW Makefiles build without Ninja; selectable build directory
  for HackRF checks and packaging.
- 4669 mock/profile checks, delayed USB startup, RX recovery and a real 30 s
  receive-only test on 17 m passed. User reports reception of their FT8 CQ
  through PSK Reporter in Finland (−25 dB) and Spain (−18 dB).
  No completed QSO or calibrated RF power/spectrum measurement is claimed.
  [Setup](HACKRF-WSJTX.txt), [Validation](validation/hackrf-ft8-17m/README.txt).

### 2026-10-24 — Release 3.0.2-oe3sro.5 with REIST16-QSO/2

- First download release with REIST16-QSO/2, Pluto/HackRF bridges and WASAPI
  WebSDR reception. [Release notes](releases/3.0.2-oe3sro.5.md).
- Updated REIST16 operation, profile limits and bilingual offline guide;
  historical QSO/1 RF evidence is distinguished from current QSO/2.
- Bridge checks resolve Hamlib DLLs on clean CI runners. CI also verifies
  the HackRF bridge and production SDR settings dialog.

### 2026-10-23 — WebSDR and REIST16 capture windows

- WASAPI browser reception, Yaesu TX/browser RX in one Custom instance,
  and a separate receive-only REIST16 self-test receiver.
- Receive-only capture to 14.400 s; transceive retains 13.824 s for Auto Seq.
  Delayed noisy-frame test: 6/6 instead of 0/6, without an extra dB claim.
- Corrected CQ/QSO preflight, message length and PTT-aware indication.
  [Operation](WEBSDR-WSJTX.txt).

### 2026-10-22 — HackRF integration and TX recovery

- Separate HackRF profile and TCI bridge; settings preserve TCI audio when
  loading, switching rigs and saving. PTT without audio is rejected without
  stopping the bridge or replaying a discarded transmission later.
- Improved Pluto connection diagnostics. Software/mock validation passed;
  HackRF RF acceptance is pending. [Setup](HACKRF-WSJTX.txt).

### 2026-10-22 — REIST16-QSO/2

- 12.8 s frames in 15-second slots, 16-GFSK with 5 Hz spacing,
  CRC14 and LDPC(174,91). RX also accepts QSO/1; update both endpoints.
- Noncoherent/coherent metrics, bounded OSD, independent sync/type checks,
  updated PCM/TCI timing and waterfall span.
- Independent AWGN comparison at −22 dB SNR2500: QSO/2 20/32, FT8 15/32.
  No general FT8-equivalence or new RF validation claimed.
- [Protocol and measurement conditions](REIST16-QSO2.md).


### 2026-10-21 — REIST16 and PlutoSDR (experimental)

- New independent 15-second UTC CQ/QSO mode: 16-FSK, 77-bit standard messages,
  K7 coding, interleaving, CRC32 and Auto Seq; incompatible with FT8 on air.
- Local Pluto TCI radio bridge with frequency control, PTT and audio;
  four 40 ms TX buffers, 10 ms RX blocks, complete /P and /R station identities.
- User-operated 432.500 MHz Yaesu/Pluto exchange confirms two-way decoding
  through RR73, followed by a transmitted 73; its reception is not visible.
  Sensitivity and general reliability are not measured by this example.
- [REIST16](REIST16.md), profile contract, original screenshot and CI tests
  included. The source feature does not change the older stable release.


### 2026-10-16 — Release 3.0.2-oe3sro.4: UCRT64 GitHub build

The unavailable MinGW64 libusb package stopped 3.0.2-oe3sro.3 before compilation;
no ZIPs were published. GitHub CI now uses MSYS2 UCRT64; local MinGW64 builds
remain possible. Tag **oe3sro-v3.0.2-4**, [release notes](releases/3.0.2-oe3sro.4.md).

### 2026-10-16 — Windows release 3.0.2-oe3sro.3

[Release notes](releases/3.0.2-oe3sro.3.md): personal DX windows and persistent
PSK Reporter query cooldown. Tag **oe3sro-v3.0.2-3** builds the application and
publishes Windows/documentation ZIPs with SHA256 after successful CI checks.

### 2026-10-16 — Personal empirical DX windows

- HTTP 429/503 now triggers increasing backoff, honors `Retry-After`, and shares
  a persistent query cooldown across restarts/profiles using the same settings
  file. Specific status message replaces the long error URL. Current local
  package: `dist-custom/OE3SRO-psk-backoff`, preserving the Custom profile.

- **Personal DX windows** in the PSK Reporter dialog groups local observations
  by DXCC country, band, UTC hour and calendar month, with month/country/band filters.
- Ranking uses distinct reception days, followed by receiver-days. Repeated
  queries count a receiver once per UTC date/hour. Callsigns, modes and named
  station/antenna setups remain separate. These are positive observations,
  not QSO probabilities. Collection uses existing queries without historical backfill.
- **Next 24 hours (UTC)** orders recurring historical hourly windows by time,
  marking an ongoing window **Now**. Filter by minimum reception days and the
  last 90/365 UTC days; first/last observed dates are shown for each window.
- Unchanged observations avoid redundant storage writes; malformed saved hours
  are rejected when loading statistics.
- Statistics and PSK tests, visual inspection, the full Windows build and
  package runtime checks passed. Local package: `dist-custom/OE3SRO-propagation`.

This fork of [WSJTX/wsjtx](https://github.com/WSJTX/wsjtx) uses WSJT-X
3.0.2-devel, commit `967c85a61`, as the base for these additions. The entries
describe the implemented state on October 16, 2026; they do not announce a
new official WSJT-X release.

### 2026-10-15 — Windows release 3.0.2-oe3sro.2

The [second custom package version](https://github.com/rudolfstepan/wsjtx/releases/tag/oe3sro-v3.0.2-2) combines the
FT8/FT4/FT2 improvements and logbook/UDP fixes below. Tag `oe3sro-v3.0.2-2`
builds the Windows application and German/English documentation in GitHub
Actions; both ZIPs are published with SHA256 files after the checks pass.

### 2026-10-15 — Extension to FT4 and FT2

- Native C++ GFSK waveforms with mode-specific ramps, double centered phase
  and interpolated sine lookup. FT8/FT4/FT2 share the native LDPC encoder.
- Native synchronization and parallel frequency/time search, up to eight workers
  within CPU/OpenMP limits. Small fine grids run serially; peak ties retain the
  original search order.
- Private audio snapshots and shared **View → TX quality — FT8 / FT4 / FT2**.
  Recorded comparisons preserve 19 FT4 and 6/2 FT2 messages under all tested team
  limits. Waveform kernel timings: FT4 about 8.6 → 1.0 ms, FT2 about 4.2 → 0.4 ms;
  these are not general full-decoder or on-air improvements.
- New local package `dist-custom\OE3SRO-ftx-native`, preserving the Custom profile.
  Hybrid BP/OSD and other decoder stages still contain Fortran.

### 2026-10-15 — FT8: C++ kernels, SMP and TX diagnostics

- Ported normal FT8 waveform generation, LDPC encoding and standard/reordered BP
  kernels to C++. Centered phase applies the REIST paper's bounded-state idea;
  double precision, interpolated lookup, packed parity and private decoder scratch.
  Message packing, synchronization and hybrid BP/OSD still contain Fortran.
- Corrected SMP worker selection under restricted or disabled OpenMP teams;
  the existing architecture still supports at most twelve workers.
- Private normal-FT8 audio snapshot, exact message end, invalid-format handling
  and TX stop before audio error dialogs. Added **View → TX quality — FT8** with
  audio/start/optional CAT ALC readings and existing parallel-decoder controls.
- Differential kernel tests and the recorded FT8 sample show no lost messages in
  tested material. Local kernel timings: waveform about 21 → 2–3 ms, encoding
  about 8.3 → 0.25 µs, BP about 1.2× faster. These are not full-decoder speedups
  or evidence of improved on-air reception. No radio test was performed.
- Local package: `dist-custom\OE3SRO-ft8-native`, using the existing Custom profile.
  [Scope, paper interpretation and validation](FT8.md).

### 2026-10-15 — Bug review: logbook and UDP

- The ADIF log reader processes complete buffered QSOs even after reaching
  EOF. Previously, single-line files or multiple QSOs on the final line could
  be incompletely loaded, causing incorrect worked-before status. The log file
  itself was not modified by this bug.
- UDP commands are processed only when their target ID matches this WSJT-X
  instance. Previously, commands for other instances were also accepted.
- Changing the UDP server cancels a pending DNS lookup. A delayed result can
  no longer overwrite the new address or restore a disabled server.
- Local regression tests in `tests/core`, run with `tools/Test-Core.ps1`,
  also included in the Windows CI workflow.
- New local package `dist-custom\OE3SRO-bugfix` because the previous version
  was still running. Start with `start.bat` or `Start-Custom.ps1`.
- Passed locally: new regression tests, full build, PSK tests, FT2/FT4
  including both reference WAVs, and package runtime verification. Build and
  package executables have matching hashes. These checks did not transmit RF.

### 2026-10-12 — Automatic Windows binaries

- First public custom release **3.0.2-oe3sro.1**, tag `oe3sro-v3.0.2-1`:
  Windows ZIP with EXE and German/English HTML guide, plus a separate
  documentation ZIP and SHA256 files. Release links appear in both READMEs.
- The [release tag build](https://github.com/rudolfstepan/wsjtx/actions/runs/38218198831)
  passed the build, PSK/FT2/FT4 tests, runtime checks, and publication. All four
  public assets were downloaded without authentication; ZIP/EXE checksums,
  source commit, and package contents matched.
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
