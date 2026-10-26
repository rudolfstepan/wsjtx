# WSJT-X OE3SRO Custom

**build by OE3SRO** · Digitale Betriebsarten, eigene Empfangswerkzeuge und SDR-Anbindung.

Diese Version ergänzt WSJT-X um **REIST16**, optimierte FT8-/FT4-/FT2-Kerne, eine integrierte PSK-Reporter-Empfängerliste, persönliche DX-Zeitfenster und eine Weboberfläche für den Stationsbetrieb im LAN.

**[Windows herunterladen](https://github.com/rudolfstepan/wsjtx/releases/latest)** · [Schnellstart](Schnellstart) · [Alle Funktionen](Funktionen) · [English](English)

Aktueller Download: **3.0.2-oe3sro.7-rc3**, Basis WSJT-X **3.0.2-devel**. RC3 ist auf Wunsch des Projektbetreibers als GitHub-**Latest** veröffentlicht; die Versionsbezeichnung bleibt RC3. REIST16 und die Websteuerung bleiben experimentelle Erweiterungen. Der aktuelle Download wurde lokal gebaut und geprüft; GitHub Actions ist derzeit deaktiviert.

![Deutsche Weboberfläche mit FT8-Empfang und Live-Wasserfall](https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/docs/images/web-control-ft8-2026-10-25.png)

*Originalaufnahme vom 25. Oktober 2026: FT8-Empfang auf 20 m. Desktop und Browser bedienen dieselbe laufende Station. Das Bild zeigt keinen abgeschlossenen QSO.*

## Die wichtigsten Erweiterungen

| Bereich | Was die Custom-Version bietet |
| --- | --- |
| [REIST16](REIST16) | Eigener CQ-/QSO-Modus, Mehrsignalempfang, Signalabtrennung und zusätzliche Schwachsignalverfahren. |
| [FT8 / FT4 / FT2](FT8-FT4-FT2) | Native C++-Kerne, parallele Verarbeitung und gemeinsame TX-Diagnose. |
| [PSK Reporter](PSK-Reporter) | „Who hears me?“ mit Logbuchfarben, Filtern, Favoriten und direkten Anrufhilfen. |
| [DX-Zeitfenster](DX-Zeitfenster) | Lokal gesammelte Empfangshistorie nach DXCC, Band, UTC-Stunde und Stationsaufbau. |
| [Websteuerung](Websteuerung) | Hauptbedienbereich und Live-Wasserfall im Browser, auf Deutsch und Englisch. |
| [Funkgeräte und Audio](Funkgeraete-und-Audio) | Klassische CAT-/USB-Audio-Verbindung sowie eigene Pluto- und HackRF-Bridges. |
| [WebSDR / WASAPI](WebSDR-und-WASAPI) | Browseraudio direkt dekodieren, auch mit zusätzlichem Empfangsfenster. |

## Orientierung

- **Neu hier:** [Installation und erster Empfang](Schnellstart).
- **Die Oberfläche ansehen:** [Screenshots mit Erläuterungen](Screenshots).
- **Ergebnisse beurteilen:** [Messungen, Praxistests und Grenzen](Messungen-und-Grenzen).
- **Selbst bauen:** [Build und Entwicklung](Build-und-Entwicklung).
- **Probleme lösen:** [FAQ](FAQ).

Die Custom-Version baut auf der Arbeit des WSJT Development Team auf. FT2 wurde aus der GPLv3-Implementierung von **DG2YCB / WSJT-X Improved** übernommen und angepasst. [Quellcode und Lizenz](https://github.com/rudolfstepan/wsjtx) · [Änderungshistorie](https://github.com/rudolfstepan/wsjtx/blob/master/docs/CHANGES.md).
