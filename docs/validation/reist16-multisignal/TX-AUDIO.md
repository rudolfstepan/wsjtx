# REIST16: Sendepegel und Filtermitte

Lokale Änderung vom 26. Oktober 2026, build by OE3SRO.

Der QSO/2-Generator hatte seinen Ausgang zusätzlich mit 0,8 multipliziert.
FT8 gibt seine Wellenform mit digitaler Spitzenamplitude 1 aus; beide Modi
verwenden danach dieselbe PCM-Skalierung. Der zusätzliche Faktor entfällt
jetzt auch bei QSO/2. Bei gleicher Ausgangsdämpfung entspricht dies einem
Amplitudenfaktor von 1,25 bzw. etwa +1,94 dB. Die vorhandenen Decoderprüfungen
mit Rauschen behalten ausdrücklich denselben Signalpegel wie zuvor; eine
lautere Testwelle wird nicht als Empfindlichkeitsverbesserung gewertet.

Beim Auswählen von REIST16 wird Tx 1000 Hz voreingestellt. Mit **Rig/Fake It**
wird der TX-VFO so verschoben, dass das Audio in der Filtermitte liegt. Die
neue 250-Hz-Abstufung ergibt 1500–1749 Hz für den untersten und höchstens
1824 Hz für den obersten Ton. Die Summe aus TX-VFO und unterstem Audioton
bleibt gleich dem gewählten HF-Sendeplatz. Ohne Split wird die gewählte
Audiofrequenz direkt ausgegeben. Es werden weder Split aktiviert noch
bestehende Geräteprofile durch Dateien überschrieben.

Die Filtermitte folgt der Begründung im
[WSJT-X-Handbuch](https://wsjt.sourceforge.io/wsjtx-main_en.html#RADIO):
Audiooberwellen sollen außerhalb des Sendefilters liegen. Die Zahlen für
REIST16 bezeichnen Tonmitten; GFSK besitzt keine harte spektrale Grenze.
Eine generelle bessere KW-Ausbreitung tieferer Audiotöne wird nicht behauptet.

## Prüfung

`tools/Test-Reist16.ps1` besteht mit den Änderungen. Der Produktionspfad
Fortran-Packer → QSO/2-Encoder → Modulator → PCM16 wird ohne Audiogerät oder
CAT/PTT geprüft. Zum Vergleich durchläuft eine FT8-Wellenform denselben
Modulator. Die folgenden Effektivwerte beziehen sich auf den aktiven Teil,
jeweils ohne die ersten und letzten 100 ms; die verschiedenen Rampen werden
dadurch nicht mit einem Modulationspegel verwechselt.

| Unterster Audioton | REIST16 RMS / Vollaussteuerung | FT8 RMS / Vollaussteuerung | Differenz dB |
| --- | --- | --- | --- |
| 500 Hz | 0,707107 | 0,707102 | +0,000060 |
| 1000 Hz | 0,707107 | 0,707104 | +0,000037 |
| 1500,375 Hz | 0,707107 | 0,707106 | +0,000009 |
| 2000 Hz | 0,707107 | 0,707105 | +0,000018 |

Alle Samples sind endlich und innerhalb der digitalen Grenzen. Im aktiven
PCM-Teil stimmt jeder Samplewert bis auf höchstens einen Quantisierungsschritt
mit der Generatorwelle überein; es gibt dort keinen Überlauf oder eine
zusätzliche Begrenzung. Beide Wellenformen nutzen den digitalen Spitzenbereich.
Die Split-Abbildung wird für jeden ganzzahligen Sendeplatz 0–5000 Hz geprüft:
alle 16 Tonmitten im Bereich 1500–2000 Hz und unveränderter HF-Sendeplatz.
Saubere Decoder-Rückläufe werden zusätzlich bei 500 und 1000 Hz geprüft.

Die gesamte REIST16-Suite besteht einschließlich 48 überlagerter Stationen,
Abtrennung, schwacher Sync-Referenz, CQ/QSO-Schutzfällen sowie produktivem
Audio-/IPC-Empfang. Lokales Protokoll: `tmp/reist16-tx-level-suite.txt`.

Dies ist eine digitale Prüfung. Der reale Audio-/HF-Weg, dessen Filterkurve,
ALC und Leistung wurden nicht gemessen. Nach der Pegelerhöhung muss der
Bediener die Aussteuerung am Gerät neu einstellen. Eine Vorentzerrung wird
ohne gemessene Filterkurve nicht eingesetzt. Bestehende Rohaufnahmen und
historische Empfindlichkeits-/Build-Belege bleiben unverändert.
