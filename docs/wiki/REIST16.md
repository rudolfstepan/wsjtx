# REIST16-QSO/2

[Deutsch](REIST16) | [English](REIST16-EN)

REIST16 ist eine experimentelle Betriebsart von OE3SRO für kurze strukturierte Amateurfunkgespräche. **QSO/2 ist das aktuelle Funkformat.** Es ist nicht mit FT8 funkkompatibel. Beide Gegenstellen benötigen QSO/2; der aktuelle Empfänger erkennt zusätzlich das ältere QSO/1.

## Einrichten

1. Systemzeit beider Stationen synchronisieren und dieselbe USB-Frequenz vereinbaren.
2. **Mode → REIST16** wählen.
3. Für schwache oder dicht belegte Signale **Decode → Deep** einstellen.
4. **Decode → Enable CQ AP (REIST16)** schaltet den ergänzenden CQ-Strukturversuch. Er ist standardmäßig an und wirkt ab Tiefe 2.
5. Entgegengesetzte TX-Slots wählen: eine Station `:00/:30`, die andere `:15/:45`.

Unterstützt werden CQ, Rufzeichen/Locator, Rapport, R-Rapport, RR73 und 73, einschließlich Doppelklick und Auto Seq. Die sechs Sendefelder nehmen bis zu 37 Zeichen an; zusätzlich muss der Text ein unterstütztes Standardformat sein. Freitext und Contest-Sonderformate sind nicht Bestandteil des Profils.

## Was RC3 verbessert

- **200 statt 32 Ausgabeeinträge:** Erfolgreiche Stationen verbrauchen kein Fehlversuchsbudget.
- **Signalabtrennung:** Ab Tiefe 2 werden bestätigte Signale rekonstruiert und abgezogen; weitere Suchdurchläufe arbeiten auf dem bereinigten Puffer.
- **Schwache Signale:** Tiefe 3 prüft zusätzliche kohärente Frequenzhypothesen und eine genaue GFSK-Referenz der bekannten Synchronisation.
- **CQ-AP:** Verwendet nur die CQ-Struktur, keine vorgegebenen Rufzeichen, Locatoren oder Nachrichtentexte. CRC, Rekodierung und Nutzdatenprüfung bleiben erforderlich.

Die Grenze 200 ist keine Garantie für 200 empfangbare Stationen. Dichte Slots benötigen zusätzliche Rechenzeit. Ein einzelner Sender erzeugt weiterhin eine Nachricht pro Slot.

## Audio und Sendepegel

Beim Auswählen des Modus wird **Tx 1000 Hz** voreingestellt. Der Sendeplatz bleibt danach frei wählbar. Mit **Settings → Radio → Split Operation → Rig/Fake It** verschiebt das Programm den TX-VFO gegenläufig und gibt alle 16 Tonmitten innerhalb von 1500–2000 Hz aus. Der gewählte HF-Sendeplatz bleibt erhalten. Ohne Split wird die gewählte Audiofrequenz direkt ausgegeben.

Der frühere zusätzliche Amplitudenfaktor 0,8 entfällt: etwa **+1,94 dB Audiopegel** bei gleicher nachgeschalteter Dämpfung. Der digitale Spitzenpegel entspricht jetzt FT8. **Ansteuerung und ALC am Gerät nach dem Update neu kontrollieren.** Das ist kein gemessener HF-Leistungsgewinn.

## Technische Eckdaten

| Eigenschaft | QSO/2 |
| --- | --- |
| UTC-Slot / nomineller Start | 15 s / +0,5 s |
| Signalzeit | 12,8 s |
| Modulation | 16-GFSK, 200 ms je Symbol |
| Tonabstand / äußere Tonmitten | 5 Hz / 75 Hz |
| Nachricht / Fehlerschutz | 77 Bits / CRC14 + LDPC(174,91) |
| TX / RX | 48 kHz / 12 kHz |

75 Hz bezeichnet die äußeren Tonmitten, keine gemessene belegte Bandbreite. QSO/2 behält das bisherige Funkformat trotz der neuen Empfangsverfahren.

## Praxisbild

![Yaesu- und Pluto-Dialog im früheren REIST16-QSO/1](https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/docs/images/REIST16_QSO.png)

*Originalbild vom 21. Oktober 2026: Yaesu/Custom links und Pluto rechts, früheres **QSO/1**. Es dokumentiert den damaligen Dialog, keine QSO/2-Empfindlichkeitsmessung. Die im Bild sichtbare alte Tonspanne gehört nicht zu QSO/2.*

Für REIST16 werden keine PSK-Reporter-Spots gesendet; eine registrierte ADIF-Betriebsart wird nicht behauptet. Es gibt keine CQ-Mehrslot-Mittelung. [Messungen und Aussagegrenzen](Messungen-und-Grenzen).

[Vollständiger Profilvertrag](https://github.com/rudolfstepan/wsjtx/blob/master/docs/REIST16-QSO2.md) · [Bedienung im Repository](https://github.com/rudolfstepan/wsjtx/blob/master/docs/REIST16.md)
