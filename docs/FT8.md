# FT8: C++-Kerne, SMP und TX-Diagnose / C++ kernels, SMP and TX diagnostics

**build by OE3SRO** · Stand / status: 2026-10-15

## Deutsch

Start mit `start.bat` beziehungsweise `Start-Custom.ps1` aus dem Projektordner.
Das aktuelle lokale Paket liegt in `dist-custom\OE3SRO-psk-backoff` und verwendet
weiterhin das vorhandene Profil `Custom`.

### Bedienung und FT-991A

**View → TX quality — FT8 / FT4 / FT2** zeigt die Audiofrequenz nach Split-Verschiebung,
digitale Ausgangsdämpfung, Audiogerät, zuletzt gestarteten Stream, Pufferkapazität
und übersprungene Nachrichtensamples bei verspätetem Start. Das Zeitmaß erfasst
die Softwarevorbereitung; USB-, Treiber- und Funkgerätelatenz werden nicht gemessen.

Im Dialog lassen sich der vorhandene parallele FT8-Decoder und Auto beziehungsweise
1–12 Worker auswählen. Diese Bedienelemente verwenden dieselben Einstellungen wie
das bestehende Decode-Menü. Die angeforderte Workerzahl wird durch verfügbare CPUs
und OpenMP-Laufzeitlimits begrenzt. Mehr Threads garantieren keine höhere
Empfindlichkeit oder kürzere Laufzeit bei jeder Bandbelegung.
Die Erweiterung für FT4/FT2 nutzt dieselbe Audio-Diagnose und eine eigene,
automatisch parallele Synchronisationssuche; siehe [FT4-/FT2-Stand](FT2.md).

Während TX wird ALC über Hamlib abgefragt, sofern der Backend diese Funktion
meldet. Der angezeigte Wert hat eine backendabhängige relative Skala. Bei fehlender
Unterstützung bleibt ALC unbekannt; es wird kein Nullwert vorgetäuscht. Der
FT-991A-CAT-Befehl RM unterstützt ALC, die konkrete USB/CAT-Verbindung wurde hier
nicht mit dem Gerät geprüft. ALC allein misst keine Verzerrungen.

Für normalen FT8-Betrieb: DATA-USB und die richtige USB-Audioquelle am FT-991A
verwenden. Rig split oder Fake It hält die physische Audiofrequenz nahe
1500–2000 Hz. Die tatsächliche Ansteuerung anhand des Funkgeräteinstruments und
einer externen Signalprüfung einstellen. Der Dialog verändert weder die
Funkgeräteleistung noch den USB-Pegel automatisch.

### Implementierter Umfang

- `lib/ft8/native/ft8_wave.cpp`: Gaussian-Frequenzpulse, Phase und Sinus-/Cosinus-
  Erzeugung für normale FT8-Sendung sowie Standard-/SMP-Referenzsignale in C++.
  Doppelte Phasengenauigkeit, interpolierte Sinustabelle und unveränderliche,
  begrenzt zwischengespeicherte Pulse. Arbeitsarrays gehören jedem Aufruf.
- `ft8_ldpc.cpp`: Encoder mit CRC14 und gepackten Paritätsmasken sowie Standard-
  und umgeordneten SMP-BP-Kernen. Pro Aufruf getrennte Decoderzustände; keine
  formatierte Fortran-Ein-/Ausgabe im Encoder. Tabellen stammen aus den vorhandenen
  WSJT-X-Protokolldaten; Generator: `tools/Generate-Ft8NativeTables.py`.
- Normales FT8-Audio erhält eine eigene, über die Audio-Thread-Warteschlange
  übergebene Wellenform. Änderungen des GUI-Puffers verändern diese Daten nicht.
  Das Sample hinter dem Nachrichtenende wird nicht mehr ausgegeben.
- Ungültige Audioformate verwenden keinen alten Stream weiter. Audiofehler
  stoppen TX vor dem Fehlerdialog. Reguläres Ende eines endlichen Audiopuffers
  wird nicht als Unterlauf gemeldet.
- SMP respektiert `OMP_NUM_THREADS`, `OMP_THREAD_LIMIT` und deaktivierte Teams.
  Die bestehende Architektur hat maximal zwölf Worker; dies ist keine Erweiterung
  auf 24 oder 32 Worker. Wartende Decoder-Threads verwenden standardmäßig
  `OMP_WAIT_POLICY=PASSIVE`, sofern kein Wert vorgegeben wurde.

**Noch keine vollständige C++-Portierung:** Nachrichtenpackung/-entpackung,
Synchronisation, weitere Spektralverarbeitung und der Hybrid-BP/OSD-Pfad enthalten
weiterhin Fortran. Auch Fortran wurde bereits vorher zu nativem Maschinencode
übersetzt. Die C++-Kerne ersetzen einzelne rechenintensive Routinen; die gesamte
FT8-Kette ist damit nicht vollständig C/C++.

### REIST: verwendete Idee und Grenzen

Das Paper von Rudolf Stepan, *REIST-Division – Eine implementierungsorientierte
Formulierung zentrierter Restarithmetik* (2026,
[DOI 10.5281/zenodo.21206471](https://doi.org/10.5281/zenodo.21206471)), beschreibt
persistente zentrierte Restzustände mit begrenzten Updates. Übertragen auf den
Phasenakkumulator bleibt die Phase in `[-π, π)`; ein zuvor begrenzter Schritt
erfordert höchstens eine Korrektur um `2π`. Ungewöhnlich große Frequenzschritte
werden vorher mit `remainder` reduziert.

Dies ist eine Anwendung des Zustandsprinzips auf Gleitkommaphasen. Die
Ganzzahlbeweise und Benchmarkfaktoren des Papers werden nicht ungeprüft auf FT8
übertragen. Der Wellenformgewinn umfasst auch Sinustabelle, Interpolation und
weitere Änderungen. Der zusätzliche Phasen-Mikrobenchmark isoliert eine
Beispielschleife mit allgemeiner Restberechnung beziehungsweise begrenzter
Korrektur; er misst nicht den Anteil an einer kompletten Decodierung.
LDPC rechnet bereits effizient in GF(2) mit XOR/Parität. Dafür ist hier kein
zusätzlicher REIST-Vorteil nachgewiesen.

### Reproduzierbare Prüfung

```powershell
.\tools\Build-Custom.ps1
.\tools\Test-TxQuality.ps1
.\tools\Test-Ft2.ps1
```

`Test-TxQuality.ps1` prüft den echten Modulator mit einem Speicherausgang,
Thread-Übergabe, Ende und Moduswechsel sowie ALC-Zustände und Dialogsteuerung.
Keine Soundkarte und kein Funkgerät werden geöffnet. Der Vergleich mit den
ursprünglichen Fortran-Kernen umfasst 1.000 Nachrichten je Encoder, 2.000
deterministische verrauschte BP-Eingaben mit und ohne AP-Maske, komplexe und
reelle Wellenformen, eine unabhängige analytische Trägerprüfung und zwölf
gleichzeitig arbeitende Wellenformaufrufe mit wechselnden Pulsen.

Elf synthetische WAV-Fälle testen Standard- und SMP-Decodierung einschließlich
eingeschränkter und deaktivierter OpenMP-Teams. Ein zusätzlicher Referenzdecoder
wird nur für Tests gebaut. Bei `samples/FT8/210703_133430.wav` lieferten die
Originalkerne und die neuen Kerne dieselben Nachrichten: 20 im Standardpfad,
19 im SMP-Pfad. Das belegt keine allgemeine Empfindlichkeitsverbesserung.

Lokale Größenordnung auf i9-14900K, MinGW GCC 15.2, Release-Build:

| Kernel | Original | C++ | Messung |
| --- | ---: | ---: | --- |
| Wellenform, 79 × 7680 Samples | etwa 21 ms | etwa 2–3 ms | 30 Wiederholungen, aufgewärmter Cache |
| LDPC-Encoder | etwa 8,3 µs | etwa 0,25 µs | 10.000 Wiederholungen |
| BP-Kerne | 1,0 × | etwa 1,2 × schneller | 2.000 gemischte Eingaben |

Werte hängen von CPU, Compiler und Auslastung ab. Sie sind Kernelmessungen,
keine Beschleunigungsfaktoren der gesamten Anwendung. Keine RF-Aussendung,
USB-Latenzmessung, ALC-Geräteprüfung oder nachgewiesene Verbesserung des
Empfangs bei Gegenstationen war Bestandteil dieser Tests.

## English

Open **View → TX quality — FT8 / FT4 / FT2** for physical split-adjusted audio frequency,
digital attenuation, audio stream/buffer information, skipped message samples,
optional CAT ALC and the existing parallel-decoder controls (Auto or 1–12).
Timing measures software preparation only; ALC uses the backend's relative scale.

Native C++ kernels now generate FT8 waveforms, encode LDPC messages and execute
the standard/reordered BP algorithms. The waveform uses double-precision centered
phase, an interpolated lookup table and immutable shared pulse data. Mutable
scratch arrays belong to each call. OpenMP team limits are respected to prevent
section/barrier deadlocks. Audio errors stop TX, and normal FT8 uses a private
waveform snapshot delivered through the audio thread's queue.

The centered phase update applies the bounded-state idea discussed in Stepan's
REIST paper cited above. Its integer speedups are not FT8 claims. Waveform speedup
also includes table lookup and other changes. No additional REIST benefit for
GF(2) LDPC arithmetic has been established here.

The full FT8 chain has **not** been ported to C++: message packing, synchronization,
other spectral processing and hybrid BP/OSD still contain Fortran. The previous
Fortran implementation was already compiled to native machine code.

Run the scripts above for hardware-free regression and differential tests.
The recorded FT8 sample produced the same 20 standard / 19 SMP messages with
both kernel implementations. Kernel timings in the table are local measurements,
not full-decoder speedups or proof of improved on-air reception. No radio test
or private configuration import was performed.

## Quellen / sources

- Existing WSJT-X FT8 sources in `lib/ft8` and `lib/ft8var` (GPLv3).
- [WSJT-X User Guide](https://wsjt.sourceforge.io/wsjtx-doc/wsjtx-main-2.6.1.pdf):
  audio configuration and split operation.
- [Yaesu FT-991A CAT manual](https://www.yaesu.com/Files/4CB893D7-1018-01AF-FA97E9E9AD48B50C/FT-991A_CAT_OM_ENG_1711-D.pdf): RM meter command.
- [GNU libgomp OMP_WAIT_POLICY](https://gcc.gnu.org/onlinedocs/gcc-7.1.0/libgomp/OMP_005fWAIT_005fPOLICY.html).
