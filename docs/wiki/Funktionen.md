# Funktionsübersicht

[Deutsch](Funktionen) | [English](Features)

Die Custom-Erweiterungen ergänzen die vorhandenen WSJT-X-Betriebsarten. Diese Übersicht unterscheidet die zusätzlichen Funktionen vom experimentellen Entwicklungsstand.

## Betriebsarten und Signalverarbeitung

| Funktion | Umfang | Anleitung |
| --- | --- | --- |
| REIST16-QSO/2 | 77-Bit-Standardnachrichten, CRC14 + LDPC, 16-GFSK, 15-s-Slots, Auto Seq und normale QSO-Folge. | [REIST16](REIST16) |
| REIST16-Mehrsignalempfang | Ausgabegrenze 200, ab Tiefe 2 Signalabtrennung und erneute Suche. Ein Sender sendet weiterhin eine Nachricht pro Slot. | [Messungen](Messungen-und-Grenzen) |
| REIST16-Schwachsignalverfahren | GFSK-Syncreferenz, mehrere kohärente Hypothesen und separat schaltbares CQ-AP. | [REIST16](REIST16) |
| FT8 / FT4 / FT2 | Native Wellenform-/LDPC-Kerne und parallele Verarbeitung bestimmter Decoderteile. | [FT8, FT4, FT2](FT8-FT4-FT2) |
| FT2 | Experimentelle 3,75-s-Betriebsart, Auto Seq, Wasserfall und Anrufhilfen; Herkunft DG2YCB. | [FT2-Praxisnachweis](FT8-FT4-FT2) |
| TX-Diagnose | Audiogerät, digitale Dämpfung, Audiofrequenz nach Split, Start- und Pufferinformationen sowie verfügbare ALC-Daten. | [FT8, FT4, FT2](FT8-FT4-FT2) |

## Empfangsmeldungen und Anrufhilfen

| Funktion | Nutzen |
| --- | --- |
| Who hears me? | Empfänger des eigenen Signals mit Rufzeichen, Locator, SNR, Zeit, Frequenz und Modus. |
| Logbuchfarben / QSO-Status | Vorhandene WSJT-X-Hervorhebungen, „Worked“, „Worked on band“ und neue Rufzeichen. |
| Band-Activity-Markierungen | Empfangende Stationen türkis, fett und unterstrichen; Tooltip mit Empfangszeit. |
| Direkter Anruf | Rufzeichen/Locator übernehmen, Tx1 wählen und Enable Tx aktivieren. |
| Filter | Rufzeichen/Locator, Zeit, Favoriten, neue Rufzeichen, neu auf Band und Anrufstatus. |
| Favoriten und Historie | Favoriten speichern, letzte 20 Anrufe auswählen, letzten Anruf wiederholen. |
| Call next | Eine passende sichtbare Station auswählen und einen einzelnen Anruf starten. |
| Halt Tx | Aussendung direkt aus dem Empfangsfenster stoppen. |
| Personal DX windows | Beobachtete DXCC-/Band-/Monats-/UTC-Fenster getrennt nach eigenem Rufzeichen, Modus und Stationsaufbau. |

[PSK-Reporter-Bedienung](PSK-Reporter) · [DX-Zeitfenster](DX-Zeitfenster)

## Geräte, Bedienung und Bereitstellung

- **Websteuerung:** Hauptbedienbereich im Browser, DE/EN, echte Desktopfarben, QSO-Logbestätigung und Live-Wasserfall. [Anleitung](Websteuerung).
- **Pluto / HackRF:** Eigene Instanzen mit TCI für Frequenz, PTT und Audio; getrennte Profile. [Anleitung](Funkgeraete-und-Audio).
- **WASAPI:** Browseraudio ohne virtuelles Audiokabel dekodieren; separater Receive-only-Empfänger oder kombinierter Yaesu-TX/WebSDR-RX-Betrieb. [Anleitung](WebSDR-und-WASAPI).
- **Konfiguration:** Ein optionaler Erstimport im Quellprojekt legt ein separates Custom-Profil an; vorhandene Profile werden nicht überschrieben.
- **Ein Paket, ein Build:** `dist-custom` für alle Starter und `build-custom-make` für den lokalen Build. Downloads enthalten Dokumentation, Laufzeitbibliotheken und Prüfsummen.

REIST16 und die Websteuerung sind experimentell. Eine vollständige C++-Portierung, allgemeine FT8-Überlegenheit oder ein Mehrträger-REIST16-Sender werden nicht behauptet. [Prüfstand und Grenzen](Messungen-und-Grenzen).
