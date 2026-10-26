# FT8, FT4 und FT2

Die Custom-Version behält die vorhandenen Funkformate bei und ersetzt bestimmte rechenintensive Routinen durch native C++-Kerne. Nachrichtenpackung, Teile der Synchronisation und weitere Decoderstufen verwenden weiterhin Fortran. Es ist keine vollständige C++-Portierung.

| Modus | Zyklus | Custom-Erweiterungen |
| --- | --- | --- |
| FT8 | 15 s | Native Wellenform-/LDPC-Kerne, vorhandener paralleler Decoder und TX-Diagnose. |
| FT4 | 7,5 s | Native Wellenform und parallele Synchronisationssuche. |
| FT2 | 3,75 s | Experimentelle Integration mit Sender, Decoder, Auto Seq, Wasserfall und PSK-Anrufhilfen. |

FT2 stammt aus **WSJT-X Improved / WS Suite von DG2YCB**, übernommen und angepasst unter GPLv3.

## TX-Diagnose und Threads

**View → TX quality — FT8 / FT4 / FT2** zeigt Audiogerät, digitale Dämpfung, tatsächliche Audiofrequenz nach Split, Software-Startdaten und Pufferinformationen. ALC wird angezeigt, wenn der Hamlib-Backend es unterstützt. Ein unbekannter ALC-Wert wird nicht als null dargestellt.

Für den parallelen FT8-Decoder stehen Auto beziehungsweise bis zu zwölf Worker zur Auswahl. CPU- und OpenMP-Grenzen können die tatsächliche Zahl reduzieren. Mehr Threads garantieren weder mehr Empfindlichkeit noch eine kürzere Laufzeit bei jedem Signalbild. Die FT4-/FT2-Synchronisationssuche parallelisiert passende Suchraster automatisch.

ALC und Softwarezeitstempel sind keine vollständige Messung von Verzerrung, USB-Latenz oder HF-Leistung.

## Dokumentierter FT2-Funkbetrieb

![FT2-QSO OE3SRO–OG5O auf 20 m](https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/docs/images/ft2-qso-oe3sro-og5o-2026-10-16.png)

*Originalaufnahme vom 16. Oktober 2026: OE3SRO mit OG5O/KP20 in Finnland, 14,084 MHz Dialfrequenz. Sichtbar sind Rapport, empfangenes RR73 und abschließendes gesendetes 73. Das ist ein benutzerdokumentierter FT2-QSO-Nachweis.*

Die nativen und ursprünglichen Kerne wurden mit deterministischen Eingaben und aufgezeichneten FT8-/FT4-/FT2-Samples verglichen. Einzelne Kernelbenchmarks sind keine Geschwindigkeitsfaktoren für die gesamte Anwendung. [Prüfungen](Messungen-und-Grenzen).

[FT8-Details](https://github.com/rudolfstepan/wsjtx/blob/master/docs/FT8.md) · [FT2-/FT4-Details und Herkunft](https://github.com/rudolfstepan/wsjtx/blob/master/docs/FT2.md)
