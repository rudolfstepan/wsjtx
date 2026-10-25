HackRF FT8 17 m / maximale TX-Einstellungen, 25.10.2026
build by OE3SRO

Startoption start-hackrf-ft8-17m.bat: eigenes Profil HackRF-FT8-17m,
FT8 / 18100000 Hz beim Erststart, TCI 40004, UDP 2243, AutoTx false.
TX-VGA 47 dB, digitaler IQ-Pegel 1,0, interner RF-Verstaerker nur bei TX.
Bestehende Profile/Frequenzen/Modi bleiben erhalten; keine Profilimporte.

Pruefungen ohne echte Hardware:
- hackrf_bridge_checks: 4669 ABI-/IQ8-/Halbduplex-/Profil-/CLI-Pruefungen.
  Neuer Fall bei 18,1 MHz: TX Gain 47, IQ8 +127/-127 bei Pegel 1,0,
  Amp beim Empfang aus, nach PTT an, nach PTT aus wieder aus.
  Cleanup schaltet Amp auch aus einer ausstehenden TX-Anforderung aus.
  1,4 s verzoegerte USB-Daten ueberstehen den frueheren 1-s-Timeout;
  ein laengerer RX-Stillstand startet den Stream neu und kehrt zu RX zurueck.
- Echter wsjtx.exe-Profil-CLI-Aufruf mit FT8/17m/eigenen Ports und
  Erhalt spaeterer FT4-/20m-Wahl; AutoTx beim Anlegen aus.
- Produktions-Konfigurationsdialog fuer HackRF/Pluto mit TCI-Audio,
  CAT-Test und Einstellungen-OK gegen simulierte Funkgeraete.
- HackRF-TCI-Protokoll: fehlendes Audio, spaeter Audiostart,
  Watchdog, Disconnect, Wiederanlauf, geordnetes Beenden.
- Gemeinsamer USB-DSP/TCI-Treiber mit PCM-Snapshot, Modem-Roundtrip,
  CAT/PTT/RX gegen simulierte Bridge; Pluto-Regression.
- Test-HackrfFt8Launcher.ps1: echte Profilvorbereitung in temporaerem
  LOCALAPPDATA, maximaler TX-Default, reduzierte Leistungsoption,
  Quellprofil unveraendert, getrennte Ports, konkurrierende Instanz gesperrt.
- Portable Qt-/OpenSSL-Laufzeit ohne Entwicklungs-PATH geprueft.
- Verzoegerter HackRF-Mock mit 2,5 s bis zu ersten Samples: produktiver
  TCI-Treiber wartet ueber den bisherigen 2-s-Handshake hinaus; nach
  temporaerem RX-Aussetzer bleibt er online und empfaengt wieder Audio.

Die Softwarepruefungen verwenden keine echte Hardware. Zusaetzlich wurde
der echte HackRF separat auf 18,1 MHz ausschliesslich fuer RX geoeffnet:
erster USB-Datenblock und Bereitschaft bestaetigt, PTT false. Anschliessend
echte Bridge als separater Prozess mit produktivem WSJT-X-TCI-CAT-Thread:
30 s Empfang auf 18,1 MHz, Frequenzreadback und Audio ohne Rig-Control-Fehler.
Der Hardwareteil fordert keine PTT an; die Sendetests davor sind Simulation.
Der Agent hat keine HF-Aussendung ausgeloest; kein abgeschlossener FT8-QSO.
Ausgangsleistung/Spektrum/Oszillatorgenauigkeit sind nicht kalibriert.
Maximale Softwareeinstellung ist keine gemessene Leistungsangabe.

Benutzerbetrieb danach: CQ OE3SRO/P auf 17 m. Der Benutzer meldet am
25.10.2026 PSK-Reporter-Empfang in Finnland -25 dB und Spanien -18 dB.
Damit liegt Benutzerbestaetigung fuer entfernte FT8-Dekodierung vor.
Kein eigener PSK-Reporter-Rohdatenexport; keine gemessene Leistung,
kein umfassender Nachweis der spektralen Reinheit und kein QSO-Abschluss.

Wegen gemeldeter Ninja-Malware-Erkennung lokaler Neubau mit
Build-Custom.ps1 -Generator 'MinGW Makefiles' in build-custom-make.
Keine Antivirus-Ausnahmen; temporaer geladene Ninja-EXE und ZIP entfernt.
Build-Custom behaelt den bisherigen Ninja-Standard fuer CI-Kompatibilitaet.
Test-HackrfBridge und Package-Custom akzeptieren -BuildDirectory.
