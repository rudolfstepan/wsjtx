# Funkgeräte, Audio und Profile

[Deutsch](Funkgeraete-und-Audio) | [English](Radio-and-audio)

Alle lokalen Starter verwenden dasselbe Paket unter **dist-custom/bin**. Unterschiedliche Geräte oder Aufgaben werden durch eigene WSJT-X-Profile getrennt, nicht durch Kopien der EXE.

| Aufgabe | Profil | Verbindung |
| --- | --- | --- |
| Normale Station, etwa Yaesu | Custom | Bestehende CAT-/PTT- und Soundkarten-/USB-Audio-Einstellungen. |
| Browseraudio als zusätzlicher Empfänger | WebSDR | WASAPI-Loopback, explizit Receive-only. |
| PlutoSDR | Pluto | Lokale TCI-Bridge, normalerweise Port 40002. |
| HackRF | HackRF | Lokale TCI-Bridge, normalerweise Port 40003. |
| HackRF-FT8 auf 17 m | HackRF-FT8-17m | Eigener Erststart mit FT8/18,100 MHz, TCI 40004. |

Die Profile liegen unter `%LOCALAPPDATA%`. Bestehende Einstellungen bleiben erhalten. Ein erneuter Import bei jedem Update ist nicht erforderlich. Gleichzeitige Senderinstanzen dürfen nicht um denselben CAT-Port oder dasselbe SDR konkurrieren.

## Klassischer Transceiver

Funkgerät, CAT/PTT und Audio in **Settings** einrichten. Bei USB-Audio den richtigen Ein- und Ausgang wählen. Rig Split oder Fake It kann den ausgegebenen Audiobereich in der Filtermitte halten; die Dialverschiebung wird dabei kompensiert. Die digitale Dämpfung ist keine gemessene HF-Leistungsanzeige.

## PlutoSDR

`start-pluto.bat` startet Bridge und eigene Pluto-Instanz. Die Bridge überträgt Frequenz, PTT und Audio gemeinsam über TCI; ein virtuelles Audiokabel wird nicht benötigt. Geräteadresse und Frequenz lassen sich im PowerShell-Starter angeben. Beim Schließen der Instanz beendet der Starter die Bridge geordnet.

Beim ersten Anlegen werden nur Rufzeichen und Locator aus Custom übernommen. CAT-/Audiokonfiguration und Logbuch bleiben getrennt. [Pluto-Einrichtung und Grenzen](https://github.com/rudolfstepan/wsjtx/blob/master/docs/PLUTO-WSJTX.txt).

## HackRF One

`start-hackrf.bat` startet eine eigene HackRF-Instanz. `start-hackrf-ft8-17m.bat` verwendet ein getrenntes Profil mit FT8/18,100 MHz als Erstvorgabe. Spätere Frequenz-/Moduswahlen bleiben erhalten. HackRF arbeitet halbduplex; RX wird für TX umgeschaltet.

Die 17-m-Option hat absichtlich hohe TX-Vorgaben: TX-VGA 47 dB, IQ-Pegel 1,0 und interner RF-Verstärker während TX. Das ist keine kalibrierte Ausgangsleistung. Parameter und **Enable Tx** vor dem Einsatz prüfen.

Das lokal erstellte **RC3-Paket enthält** die Pluto- und HackRF-Laufzeitdateien unter `bin/pluto` und `bin/hackrf`. Bei selbst erstellten oder anderen Paketen deren Inhalt prüfen. Windows-USB-Treiber werden dadurch nicht automatisch installiert. Bei mehreren HackRF-Geräten die Seriennummer auswählen.

Die Software meldet USB-/Audioausfälle und verwirft verspätete TX-Anforderungen, statt sie später nachzuholen. Simulationsprüfungen ersetzen keine vollständige HF-Abnahme. [HackRF-Anleitung](https://github.com/rudolfstepan/wsjtx/blob/master/docs/HACKRF-WSJTX.txt).
