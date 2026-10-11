# FT2 in OE3SRO Custom / FT2 in OE3SRO Custom

**Deutsch**

FT2 ist über **Mode → FT2** verfügbar. Der experimentelle Modus verwendet
3,75-Sekunden-TX/RX-Perioden, 77-Bit-Nachrichten, 105 Symbole einschließlich
Rampen, 288 Samples pro Symbol bei 12 kHz und 41,667 Hz Tonabstand.
Die Wellenform dauert 2,52 Sekunden; die Audioausgabe startet nominell
150 ms nach dem Periodenbeginn. Decodes tragen den Marker `*`.

Die Standardnachrichten, Auto Seq, Doppelklick auf Decodes, Logbuch,
Wasserfall und PSK-Reporter-Anrufhilfen unterstützen FT2. PSK Reporter
fragt Meldungen mit `mode=FT2` ab; die Liste hängt von tatsächlich gemeldeten
FT2-Empfängen ab. Ein PSK-Empfang bestimmt weiterhin keine TX-Sequenz des
Empfängers. Beim ersten Start werden ausschließlich fehlende FT2-Vorgaben
ergänzt. Bestehende Frequenzen und selbst angelegte FT2-Einträge bleiben
erhalten; ein vollständiger Frequenz-Reset ist nicht erforderlich.
Die Vorgaben aus der Referenz sind vorläufig und nach Region gefiltert.
Unter **Settings → Frequencies** kannst du eigene FT2-Einträge ergänzen.
Fox/Hound-Betrieb wird beim Wechsel zu FT2 beendet.

Die Integration beruht auf **WSJT-X Improved 3.1.0 PLUS 260522** von
**Uwe Risse, DG2YCB**, heute [WS Suite](https://wsjt-x-improved.sourceforge.io/).
Insbesondere die Empfangsstrategie (Samples verdoppeln und mit dem FT4-Decoder
auswerten), Frequenz-/DT-Umrechnung und Timingparameter wurden daraus übernommen
oder angepasst. Die unabhängige Implementierung wird dort als kompatibel zum
FT2-Modus von IU8LMC beschrieben. Lizenz: **GNU GPLv3**, wie dieses Repository.
Der vorhandene alte experimentelle `lib/ft2`-Codec wird hierfür nicht verwendet.
Zusätzlich regeneriert der gemeinsame Wellenformgenerator seinen Gaussian-Puls
bei einem Wechsel von Symbolzeit oder Sample-Rate.

Validierung ohne Funkgerät: drei FT2-TX/RX-Roundtrips (500/1500/2900 Hz),
FT4-Regressionsfall, wiederholter FT4/FT2-Wechsel, TX-Sequenzen einschließlich
UTC-Tageswechsel, Erhalt von Frequenzeinträgen und beide unten genannten
offiziellen FT2-Aufnahmen. Sie ergeben sechs bzw. zwei Stationen.
Der echte Funkbetrieb und die Umschaltzeiten eines konkreten Transceivers
wurden nicht getestet.

```powershell
.\tools\Build-Custom.ps1
.\tools\Test-Ft2.ps1
# Optional: beide offiziellen WAVs zuvor in diesen Ordner herunterladen.
.\tools\Test-Ft2.ps1 -SamplesDirectory .local\ft2-reference\samples
```

**English**

Select **Mode → FT2**. This experimental mode uses 3.75-second TX/RX periods,
77-bit messages, 105 symbols including ramps, 288 samples per symbol at
12 kHz, and 41.667 Hz tone spacing. The waveform lasts 2.52 seconds, with
audio nominally starting 150 ms after the period boundary. Decode marker: `*`.

FT2 is supported by standard messages, Auto Seq, double-click calling,
the logbook, waterfall, and PSK Reporter calling tools. PSK Reporter requests
use `mode=FT2`; results depend on actual FT2 reception reports. Receiver
reports do not identify the receiver's TX sequence. First-start migration
adds only missing FT2 defaults, preserving existing frequencies and custom
FT2 entries. No full frequency reset is required. Reference frequencies are
provisional and region filtered; add your own under **Settings → Frequencies**.
Switching to FT2 exits Fox/Hound operation.

The integration adapts **WSJT-X Improved 3.1.0 PLUS 260522** by **Uwe Risse,
DG2YCB**, now [WS Suite](https://wsjt-x-improved.sourceforge.io/), specifically
its time-stretched FT4 receive strategy, frequency/DT conversion, and timing.
Its independently developed implementation is described as compatible with
IU8LMC's FT2. License: **GNU GPLv3**, the same as this repository. The old
experimental codec in `lib/ft2` is not used. The shared waveform generator
also rebuilds its Gaussian pulse when symbol duration or sample rate changes.

Validation without a radio includes three FT2 TX/RX round trips at 500/1500/2900
Hz, an FT4 regression case, repeated FT4/FT2 switches, reply sequences across
UTC midnight, preservation of user frequencies, and both official FT2 WAVs
below (six and two decoded stations). Live RF operation and individual
transceiver switching times have not been tested. Use the commands above.

## Sources and reproducibility

- [WS Suite / WSJT-X Improved](https://wsjt-x-improved.sourceforge.io/)
- [FT2 description in the developer's guide](https://wsjt-x-improved.sourceforge.io/wsjtx-main_en.html)
- [Source archive: wsjtx-3.1.0_improved_PLUS_260522.tgz](https://sourceforge.net/projects/wsjt-x-improved/files/WSJT-X_v3.1.0/Source%20code/wsjtx-3.1.0_improved_PLUS_260522.tgz/download)
  — SHA256 `F159CDE764ABABB2C63EE8F307F18F29DB0DB322E152622CC5B4E2C7575FB5A0`.
- [Official FT2 WAV samples](https://sourceforge.net/projects/wsjt-x-improved/files/Samples/FT2/)
  — `260301_170123.wav` and `260301_085945.wav`; pinned SHA256 values are in
  [Test-Ft2.ps1](../tools/Test-Ft2.ps1). Reference downloads stay outside Git.
