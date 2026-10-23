# REIST16-QSO/2 — 15-Sekunden-Weak-Signal-Profil

Stand: 22. Oktober 2026 · build by OE3SRO

Unter **Mode → REIST16** sendet dieser Stand QSO/2. Ein kompletter Rahmen
dauert 12,8 s und enthält eine Standardnachricht mit 77 Informationsbits.
Beide Gegenstellen müssen für QSO/2 aktualisiert werden. Der neue Empfänger
erkennt auch QSO/1; eine alte QSO/1-Anwendung kann QSO/2 nicht empfangen.
FT8 bleibt eine getrennte Betriebsart.

## Rahmen und Zeitbudget

| Eigenschaft | QSO/2 |
| --- | --- |
| UTC-Zyklus / nomineller Start | 15 s / +0,500 s |
| Symbolzeit / Töne | 200 ms / 16-GFSK, stetige Phase |
| Tonabstand / äußere Tonmitten | 5 Hz / 75 Hz |
| Signalzeit / Symbole | 12,8 s / 64 aktive Symbole |
| Schutz | CRC14 und LDPC(174,91) |
| Information | 77 Bits; pack77-Typen 1 und 2 |
| TX / RX | 48 kHz / 12 kHz |
| TX-Samples / RX-Samples je Rahmen | 614400 / 153600 |

75 Hz bezeichnet den Abstand der Tonmitten, keine gemessene belegte
Bandbreite. Die Gauß-Pulsform hat BT=4; die Hüllkurve wird an beiden Enden
über 25 ms ein-/ausgeblendet. Der unterste Audioton ist zwischen 100 und
4000 Hz einstellbar. Ein üblicher SSB-Audioweg muss alle Töne durchlassen.

Neue PTT-Anforderungen bleiben auf die ersten 300 ms des zugewiesenen Slots
beschränkt. Audio darf bis +1,000 s beginnen: der Rahmen endet dann bei
+13,800 s. Das Sendefenster endet bei +14,200 s. Der Empfänger startet nach
48 Halbsymbolen zu je 3456 Samples, also bei +13,824 s. Nur im expliziten
reinen Empfangsmodus (`--receive-only`) sind es 50 Halbsymbole bis +14,400 s.
Damit bleibt dem zusätzlichen WebSDR-Empfänger mehr Reserve für verzögerte
Rahmen. Eine allgemeine Verlängerung würde das Antwortbudget bis zum Ende
des nächsten PTT-Anforderungsfensters bei +15,300 s auf etwa 0,9 s verkürzen.
Im lokalen Prozess-Test benötigte Tiefe 3 etwa 1,0–1,3 s; diese Messung ist
keine Laufzeitgarantie. Deshalb behält der Sende-/Empfangsbetrieb den früheren
Start. CAT-Latenz muss zusätzlich den Audiostart bis +1,000 s erlauben.
Die Prüfungen gegen verspätetes Senden bleiben aktiv.

## Bitvertrag

Die vorhandene WSJT-X-Packung liefert 77 Bits in derselben Reihenfolge wie
bei FT8. Der native LDPC-Kern ergänzt die vorhandene CRC14 und 83 Prüfbits.
Es gelten die Tabellen in `lib/ft8/native/ft8_tables.hpp` und die
CRC14-Implementierung in `lib/crc14.cpp` (abgeschnittenes Polynom 0x2757).
Der Codewortvektor c hat 174 Bits: Information 0..76, CRC 77..90,
LDPC-Parität 91..173. Der Empfänger prüft CRC und vollständige Rekodierung.

Für die Übertragung gilt b[j] = c[(43*j) mod 174], j=0..173.
Zwei weitere Bits b[174] und b[175] sind null. Je vier aufeinanderfolgende
Bits bilden LSB zuerst eine Zahl w; der Ton ist w XOR (w >> 1).
Die 44 Datensymbole stehen an den Rahmenpositionen 7..28 und 36..57.

Die zwanzig Synchronisationssymbole liegen an diesen Positionen, ab null:

```text
Position: 0  1 2  3 4 5  6 | 29 30 31 32 33 34 35 | 58 59 60 61 62 63
Ton:      0 15 3 12 7 1 10 | 13  2 11  5 14  8  4 |  6  9  0 12  3 15
```

QSO/2 überträgt keinen CFP2-Kopf, keine Sequenznummer und keine CRC32.
Die Synchronisationsfolge kennzeichnet das Profil. Der bisherige
Sequenzparameter der Generator-Schnittstelle wird für QSO/2 ignoriert.
Das bisherige QSO/1-Kanalformat bleibt für den Empfang erhalten.

## Empfänger

Die Suche verwendet die über den Rahmen verteilte Synchronisation.
Mindestens sieben von zwanzig bekannten Tönen müssen jeweils der stärkste
der sechzehn Töne sein. Dies verhindert, dass eine zufällige CRC-Übereinstimmung
unter stationären Störträgern allein als Signal akzeptiert wird.

Die erste Demodulation verwendet nichtkohärente Softwerte. Zusätzlich kann
der Empfänger Phase und Frequenz aus der Synchronisation schätzen und
kohärente Softwerte bilden. LDPC-Belief-Propagation wird durch eine begrenzte
Ordered-Statistics-Suche ergänzt: Tiefe 2 verwendet Ordnung 1, Tiefe 3
Ordnung 2. Tiefe 1 arbeitet ohne OSD. Die Suche prüft den besten Codewortkandidaten je Zuverlässigkeits-
basis gegen die CRC. Sie erhält weder die erwartete Nachricht noch eine
Rufzeichenliste. Zugelassen sind anschließend nur pack77-Typen 1 und 2.

Die Suche verarbeitet höchstens 180000 RX-Samples, 48 Kandidaten bei Tiefe 3
und 32 Ergebnisse je Profil. Die kohärente Ergänzung ist auf zwölf Kandidaten
begrenzt. Gemeinsame FFTW-Planung ist gesperrt; Daten und LDPC-Zustände sind
aufruflokal. Angezeigte SNR-Werte sind Schätzungen. Bei starken Signalen
bezieht die Rauschschätzung auch Pulsform-Leckage ein und kann zu niedrig
ausfallen; für Messreihen gilt der eingespeiste SNR2500-Wert.

## Reproduzierbare Messung

`tools/Test-Reist16Weak.ps1` erzeugt zufällige Standard-CQs und PCM16-WAVs
mit unbekannter Startzeit und gebrochener Audiofrequenz. QSO/1, QSO/2 und FT8
erhalten dieselben Nachrichten, denselben Rauschvektor und dieselbe nominelle
aktive Signalleistung. Die Dauer der Funkformate bleibt unterschiedlich.
FT8 wird durch den tatsächlichen `jt9 -8 -d 3`-Decoder geprüft, einschließlich
seiner normalen CLI-AP-Einstellung. Es wird kein passender Rufzeichenhinweis
oder erwarteter Nachrichtentext an den Decoder übergeben.

Bei 12 kHz Abtastrate belegt reelles weißes Rauschen 6000 Hz. Mit
Signalverstärkung g und ungerampter Leistung 0,32*g² wird die Rauschvarianz
als 0,32*g² * 6000/2500 / 10^(SNR2500/10) festgelegt. Das ist ein nomineller
SNR während des aktiven Signals; die Energie pro Nachricht ist wegen der
unterschiedlichen Signaldauer nicht gleich. Die Wellenformen werden vor
dem Vergleich auf dieselben PCM16-Abtastwerte quantisiert.

| SNR2500 | QSO/1 | QSO/2 | FT8 |
| --- | --- | --- | --- |
| −18 dB | 0/32 | 32/32 | 32/32 |
| −20 dB | 0/32 | 32/32 | 32/32 |
| −22 dB | 0/32 | 20/32 | 15/32 |
| −24 dB | 0/32 | 0/32 | 0/32 |

Es gab keine falschen Nutzlasten in diesen 128 Versuchen. Alle nativen
Ergebnisse wurden nach der rein technischen Trennung der Fortran-ABI-
Dateien mit den endgültigen Binärdateien identisch reproduziert.
32 Fälle pro Pegel liefern noch keine allgemeine Empfindlichkeitsschwelle.

Die endgültigen Zahlen und Quellhashes stehen in
`docs/validation/reist16-qso2/`. Lokale WAVs und vollständige Messausgaben
liegen unter `build-custom/reist16-weak/confirmation-03`.
Dieser Bestätigungslauf verwendet Seed 2026102211 und 32 Nachrichten je
Pegel. Er folgt auf die Ergänzung der unabhängigen Synchronisationsprüfung.
Frühere Entwicklungsläufe bleiben unter ihren eigenen Verzeichnissen erhalten.

```powershell
.\tools\Test-Reist16Weak.ps1 -OutputDirectory build-custom\reist16-weak\new-confirmation -Trials 32 -Seed 2026102212
.\tools\Test-Reist16Weak.ps1 -OutputDirectory build-custom\reist16-weak\new-channel -ChannelOnly
.\tools\Test-Reist16.ps1
.\tools\Test-PlutoBridge.ps1
.\tools\Test-TxQuality.ps1
```

Die Kanalreihe enthält zufällige Phasen, ±0,02 Hz/s Drift und Fading.
Bei nominell −16 dB vor Fading dekodierten alle vier Fälle ohne Fading,
jedoch nur einer von vier Fading-Fällen. Die Fading-Hüllkurve schwankt
zwischen 0,30 und 1,00 und senkt dabei auch die mittlere Signalleistung.
128 Slots mit Rauschen und bis zu drei stationären oder getasteten
Störträgern blieben ohne Ausgabe. Drei getrennte QSO/1-/QSO/2-Stationen
wurden gemeinsam über die Produktionsschnittstelle empfangen.

Diese endlichen Versuche belegen keine allgemeine Fehlalarmrate und keine
Empfindlichkeit unter beliebiger Drift, Mehrweg oder Fading. Das
Yaesu-/Pluto-QSO-Bild vom 21. Oktober gehört zum bisherigen QSO/1-Profil.
Eine neue QSO/2-Funkabnahme liegt noch nicht vor.

Die Entwurfsgrundlage für kurze strukturierte Nachrichten, LDPC und
Soft-Decodierung erläutern Franke, Somerville und Taylor in
[*The FT4 and FT8 Communication Protocols*](https://wsjt.sourceforge.io/FT4_FT8_QEX.pdf).
Gemeinsamer Kanalcode macht die beiden Funkformate nicht austauschbar.

## English

This build transmits **REIST16-QSO/2** in the existing 15-second UTC mode:
77 information bits, CRC14, LDPC(174,91), sixteen continuous-phase Gaussian
FSK tones, 200 ms per symbol, 5 Hz spacing and 64 symbols (12.8 s).
BT is 4; the outer tone-centre span is 75 Hz, not a measured occupied bandwidth.
TX starts nominally at +0.5 s and may start no later than +1.0 s.
Transceive capture ends at +13.824 s. Explicit receive-only mode (`--receive-only`)
extends capture to +14.400 s to retain delayed WebSDR frames. Extending all
receivers would leave only about 0.9 s for decoding and GUI processing before
the next PTT-request window closes. A local depth-3 worker test took about
1.0–1.3 s, so transceive operation retains the earlier start. This measurement
is not a runtime guarantee. CAT acknowledgement must still permit audio to
start by +1.0 s; existing late-transmit guards remain active.

Both endpoints need this update to exchange QSO/2. The new receiver also
accepts QSO/1, including mixed slots; old receivers cannot decode QSO/2.
Standard pack77 types 1 and 2 are supported. FT8 remains a separate mode.
Noise measurements use random standard CQs, identical nominal active power,
paired white-noise vectors and PCM16 samples. Message durations and therefore
energies differ. The comparison uses the normal jt9 FT8 CLI decoder,
including its default AP setting, without the expected text or callsign hints.
The final independent seed is 2026102211, with 32 messages per SNR2500 level.

At −22 dB SNR2500, QSO/2 decoded 20/32, FT8 15/32 and QSO/1 0/32.
At −20 and −18 dB both QSO/2 and FT8 decoded 32/32; at −24 dB both decoded 0/32.
All native outcomes were reproduced with the final binaries.

All four random-phase/mild-drift cases without fading decoded at −16 dB;
only one of the four faded cases did. Fading also reduced average signal power.
There were no outputs in 128 noise/interference slots. These finite tests do
not establish general false-alarm, fading or RF performance. The earlier
Yaesu/Pluto screenshot demonstrates QSO/1; QSO/2 has not yet had a new RF test.
See the retained validation files for the final counts and exact conditions.
