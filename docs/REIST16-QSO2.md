# REIST16-QSO/2 — 15-Sekunden-Weak-Signal-Profil

Stand: 26. Oktober 2026 · Release Candidate 3.0.2-oe3sro.7-rc2 · build by OE3SRO

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

Beim Auswählen von REIST16 wird **Tx 1000 Hz** voreingestellt; danach bleibt
der Sendeplatz frei wählbar. Mit **Settings → Radio → Split Operation → Rig**
oder **Fake It** wird die Audioausgabe automatisch in die Filtermitte gelegt.
REIST16 verwendet dafür 250-Hz-Schritte: der unterste ausgegebene Ton liegt
bei 1500–1749 Hz, der oberste höchstens bei 1824 Hz. Alle Tonmitten liegen
damit innerhalb von 1500–2000 Hz; die gewählte HF-Frequenz bleibt erhalten.
Die Grenzen bezeichnen Tonmitten, keine harte Begrenzung des Spektrums.
Ohne Split wird die gewählte Audiofrequenz direkt ausgegeben. Tiefere Töne
sind kein allgemeiner Ausbreitungsvorteil auf KW; entscheidend ist auch der
Durchlassbereich des jeweiligen Audio-/SSB-Filters. Die Filtermitte dient
hier dazu, Audiooberwellen außerhalb des Sendefilters zu halten; siehe das
[WSJT-X-Handbuch, Split Operation](https://wsjt.sourceforge.io/wsjtx-main_en.html#RADIO).

Der QSO/2-Sender nutzt jetzt dieselbe digitale Spitzenaussteuerung wie FT8.
Der bisherige zusätzliche Faktor 0,8 entfällt. Bei gleicher nachgeschalteter
Pegelstellung erhöht dies die Audioamplitude um den Faktor 1,25 bzw. etwa
1,94 dB. Die gemeinsame Ausgangsdämpfung bleibt einstellbar. Das ist eine
Pegelkorrektur am Sender, kein Gewinn an Decoderempfindlichkeit bei gleichem
Empfangs-SNR und keine gemessene Erhöhung der HF-Leistung. Nach dem Update
den realen Senderpegel und ALC neu kontrollieren.

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
Im normalen Pfad müssen mindestens sieben von zwanzig bekannten Tönen jeweils
der stärkste der sechzehn Töne sein. Tiefe 3 ergänzt einen schwächeren Pfad
mit fünf oder sechs Treffern, der zusätzlich eine genaue GFSK-Syncreferenz,
Mindest-Kohärenzenergie und mindestens sechs verschiedene beobachtete
Nutzdatentöne verlangt. CRC, Rekodierung und Nutzdatenplausibilität bleiben
erforderlich. [Schwachsignal-Erweiterung und Grenzen](validation/reist16-multisignal/WEAK-UPGRADE.md).

Die erste Demodulation verwendet nichtkohärente Softwerte. Zusätzlich kann
der Empfänger Phase und Frequenz aus der Synchronisation schätzen und
kohärente Softwerte bilden. LDPC-Belief-Propagation wird durch eine begrenzte
Ordered-Statistics-Suche ergänzt: Tiefe 2 verwendet Ordnung 1, Tiefe 3
Ordnung 2. Tiefe 1 arbeitet ohne OSD. Die Suche prüft den besten Codewortkandidaten je Zuverlässigkeits-
basis gegen die CRC. Sie erhält weder die erwartete Nachricht noch eine
Rufzeichenliste. Zugelassen sind anschließend nur pack77-Typen 1 und 2.

Die Suche verarbeitet höchstens 180000 RX-Samples. Die produktive Ausgabe
fasst jetzt 200 Nachrichten je Slot. Erfolgreiche Stationen verbrauchen kein
Fehlversuchsbudget: Tiefe 1 prüft bis zu zwölf erfolglose Hypothesen; Tiefe 2
hat bis zu zwei Durchläufe mit je zwölf, Tiefe 3 bis zu drei mit je sechzehn.
Je Durchlauf gelten zusätzlich höchstens 256 geprüfte Kandidaten und 200
QSO/2-Ergebnisse. Die kohärente Ergänzung erlaubt über alle Durchläufe zwölf
Fehlversuche; erfolgreiche kohärente Decodes verbrauchen dieses Budget nicht.
Ein Teil der Fehlversuche bleibt für spätere, bereinigte Durchläufe reserviert.
Nach einem Durchlauf ohne neue Ergebnisse endet die Suche.
Solange noch keine Station akzeptiert wurde, darf der erste Durchlauf die
zwölf kohärenten Fehlversuche ausschöpfen. Bei Tiefe 3 werden mehrere
Frequenzhypothesen und eine genaue GFSK-Syncreferenz geprüft. CQ-AP ergänzt
ab Tiefe 2 die bekannte CQ-Struktur, ohne Sender oder Locator vorzugeben.
Es ist separat unter **Decode → Enable CQ AP (REIST16)** beziehungsweise
in der CLI mit `--no-reist16-ap` abschaltbar und standardmäßig aktiv.

Ab Tiefe 2 rekonstruiert der Empfänger vollständig geprüfte Nachrichten als
komplexe GFSK-Wellenform, verfeinert deren Startzeit und schätzt eine langsam
veränderliche komplexe Amplitude. Erkannte Signale werden sofort abgezogen,
damit schon weitere Kandidaten desselben Durchlaufs davon profitieren.
Zusätzliche Durchläufe erneuern das Suchspektrum. CRC, vollständige Rekodierung,
Synchronisationsprüfung und zugelassene Nachrichtentypen bleiben erforderlich.
Ein erfolgreicher nativer LDPC-Ersatzdurchlauf setzt jetzt auch seinen
Prüfstatus korrekt. Zeit und Frequenz werden abwechselnd verfeinert, um
Rechenarbeit zu sparen. QSO/1 und QSO/2 empfangen gleichzeitig in getrennten
Arbeitszuständen; bei fehlendem Worker ist ein serieller Rückfall möglich.

Zusätzlich muss die rekonstruierte Nutzdatenphase zur Synchronisation passen.
Die Prüfung berücksichtigt plausible Frequenz-/Driftmodelle der Synchronisation
und deren Amplitudenstreuung; bei ungenauer nichtkohärenter Startzeit wird
diese zunächst allein aus den bekannten Synchronisationsblöcken verfeinert.
So wird ein in dichter Überlagerung gefundener falscher CRC-Treffer verworfen,
ohne nur auf das formal gültige Codewort zu vertrauen. Die endlichen
Kontrollversuche sind kein allgemeiner Nachweis einer Fehlalarmrate.

Gemeinsame FFTW-Planung ist gesperrt; Daten und LDPC-Zustände sind aufruflokal.
Angezeigte SNR-Werte sind Schätzungen. Bei starken Signalen
bezieht die Rauschschätzung auch Pulsform-Leckage ein und kann zu niedrig
ausfallen; für Messreihen gilt der eingespeiste SNR2500-Wert.

## Reproduzierbare Messung

Die [Mehrsignal-Erweiterung vom 26. Oktober 2026](validation/reist16-multisignal/IMPROVEMENTS.md)
vergleicht den neuen Empfänger mit dem dokumentierten Ausgangsstand und FT8.
Mit Tiefe 3 wurden 40 Stationen bei 60 Hz, 48 bei 50 Hz und 60 bei 60 Hz
Abstand gemeinsam erkannt. Die Wellenformen überlappen teilweise spektral;
75 Hz Tonmitten-Spanne ist kein harter Mindestabstand. Das Funkformat bleibt
QSO/2-kompatibel. Diese Offlineversuche belegen keine allgemeine Gleichheit
der praktischen Bandkapazität mit FT8; Frequenzbedarf, Kanalbedingungen und
Antwortzeit bleiben zu berücksichtigen. Für dichten Betrieb Tiefe 3 wählen.

`tools/Test-Reist16Weak.ps1` erzeugt zufällige Standard-CQs und PCM16-WAVs
mit unbekannter Startzeit und gebrochener Audiofrequenz. QSO/1, QSO/2 und FT8
erhalten dieselben Nachrichten, denselben Rauschvektor und dieselbe nominelle
aktive Signalleistung. Die Dauer der Funkformate bleibt unterschiedlich.
FT8 wird durch den tatsächlichen `jt9 -8 -d 3`-Decoder geprüft, einschließlich
seiner normalen CLI-AP-Einstellung. Es wird kein passender Rufzeichenhinweis
oder erwarteter Nachrichtentext an den Decoder übergeben.

Die folgende ursprüngliche AWGN-Reihe stammt aus dem Stand vor der
CQ-AP-/Syncreferenz-Erweiterung. Der neue unabhängige CQ/QSO-Vergleich
steht im oben verlinkten Schwachsignalbericht.

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
.\tools\Test-Reist16Weak.ps1 -OutputDirectory build-custom-make\reist16-weak\new-confirmation -Trials 32 -Seed 2026102212
.\tools\Test-Reist16Weak.ps1 -OutputDirectory build-custom-make\reist16-weak\new-channel -ChannelOnly
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
Zusätzliche QSO/2-WebSDR-Empfangsaufnahmen und die getrennten Aufnahmefenster
sind in [REIST16.md](REIST16.md) und
[der Aufnahmeprüfung](../docs/validation/reist16-rx-window/README.txt) dokumentiert.
Sie ersetzen keine Abnahme eines vollständigen beidseitigen QSO/2-Hardware-QSOs.

Die Entwurfsgrundlage für kurze strukturierte Nachrichten, LDPC und
Soft-Decodierung erläutern Franke, Somerville und Taylor in
[*The FT4 and FT8 Communication Protocols*](https://wsjt.sourceforge.io/FT4_FT8_QEX.pdf).
Gemeinsamer Kanalcode macht die beiden Funkformate nicht austauschbar.

## English

The additional [weak-signal receiver](validation/reist16-multisignal/WEAK-UPGRADE.md)
tests several coherent frequency hypotheses and an exact Gaussian sync
reference at depth 3. Weaker five/six-hit sync candidates require additional
coherent-energy and observed-tone-diversity gates. An optional standard-CQ
prior, enabled by default at depth 2/3, leaves sender and locator unknown;
disable it in Decode or with `--no-reist16-ap`. The wire format is unchanged.
The historical AWGN table predates this extension. The new independent mixed
test improves −22 dB recovery from 7/16 to 11/16, without establishing a
general sensitivity threshold or universal FT8 equivalence.

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
Yaesu/Pluto screenshot demonstrates QSO/1. Later QSO/2 WebSDR recordings and
capture-window checks are documented in [REIST16.md](REIST16.md); these do
not establish a complete bidirectional QSO/2 hardware contact.
See the retained validation files for the final counts and exact conditions.

The local receiver extension dated 26 October 2026 raises the production
output capacity to 200 messages, counts failed hypotheses separately from
successful stations, and adds up to two/three search passes with signal
cancellation at depth 2/3. Complex GFSK reconstruction estimates timing and
a slowly varying complex amplitude; CRC, re-encoding, synchronization and
message-type checks remain required. A successful native LDPC fallback now
correctly updates its check status. Alternating time/frequency refinement and
concurrent independent QSO/1 and QSO/2 searches reduce latency. The coherent
fallback allows twelve failed attempts across all passes; successful coherent
decodes leave that budget intact, and some failure work is reserved for later
passes. Signals are subtracted immediately to help subsequent candidates
before the next spectral search. Failed-hypothesis limits are
12 per pass at depth 1/2 and 16 per pass at depth 3; each pass also has a
256-candidate and 200-result ceiling. A pass with no new results ends the search.
An additional payload-phase check uses plausible synchronization frequency/
drift models and measured amplitude scatter. If needed, timing is refined
from known synchronization blocks before validation. This rejects a false
CRC hit found in a dense mixture; finite controls do not establish a general
false-alarm rate.
The wire format is unchanged. Offline depth-3 mixtures recovered 40 stations
at 60 Hz spacing, 48 at 50 Hz, and 60 at 60 Hz; these finite tests do not
establish equal general RF capacity to FT8. Use depth 3 for dense reception.
See the linked [implementation and validation report](validation/reist16-multisignal/IMPROVEMENTS.md).
