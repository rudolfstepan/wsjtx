# Websteuerung im LAN

Die experimentelle Weboberfläche bildet den Hauptbedienbereich der laufenden WSJT-X-Instanz nach. Desktop und Browser verwenden denselben Decoder, dieselbe CAT-/Audio-Verbindung und dieselbe QSO-Sitzung.

## Starten

1. Am Stationsrechner **View → Web control (experimental)…** öffnen.
2. Port wählen, bei Bedarf einen eigenen **Access key** eingeben und **Start / apply key** drücken. Ein leeres Feld erzeugt einen Zufallsschlüssel.
3. Die angezeigte Adresse im Browser öffnen und mit dem Schlüssel verbinden.
4. Mit **Stop web control** den Dienst wieder abschalten.

Der Server ist nach jedem Programmstart ausgeschaltet. Für mehrere Instanzen unterschiedliche Ports verwenden. Die Browseransicht benötigt keinen zusätzlichen Webserver und kein Node.js.

![Deutsche Websteuerung mit FT8-Empfang](https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/docs/images/web-control-ft8-2026-10-25.png)

*Originalaufnahme vom 25. Oktober 2026: OE3SRO/P, JN88, FT8 auf 20 m. Die magentafarbenen Zeilen übernehmen die Desktopformatierung. Das Bild dokumentiert Empfang.*

## Bedienbereiche

| Bereich | Funktionen |
| --- | --- |
| Status und Auswahl | Rufzeichen, Locator, Betriebsart, Dialfrequenz und RX-/TX-Zustand. |
| Decodelisten | Band Activity und RX Frequency mit Desktopfarben; Doppelklick/Doppeltippen zum Auswählen. |
| Aktionen | Monitor, Stop, Decode, Erase, Enable Tx, Halt Tx und Tune. |
| DX / QSO | Rufzeichen, Locator, Rapport, RX-/TX-Audiofrequenz, Auto Seq, TX-Slot und Hold Tx Freq. |
| Sendetexte | Tx1 bis Tx6 wählen, bearbeiten und verfügbare unmittelbare Sendeaktionen auslösen. |
| Log QSO | Normale Logbestätigung im Browser bearbeiten und bestätigen oder verwerfen. |
| Wide Graph | Echtes Desktopbild mit Wasserfall und Spektrum; die im Bild sichtbaren Qt-Regler sind nicht anklickbar. |

Setup, Audio-/Geräteeinstellungen und zusätzliche Spezialdialoge bleiben am Desktop. Audio wird nicht an den Browser übertragen. RX-/TX-Werte werden über die Zahlenfelder verändert.

## Deutsch und Englisch

![Englische Websteuerung](https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/docs/images/web-control-ft8-en-2026-10-25.png)

Die Auswahl bietet **Deutsch**, **English** und **Browsersprache**. Die Sprachwahl wird im Browser gespeichert; der Zugangsschlüssel wird nicht dauerhaft im Browser gespeichert. Ein Sprachwechsel sendet keinen Steuerbefehl.

## Verbindung und Zugriff

Der Schlüssel erlaubt auch Senden. Den Dienst im vertrauenswürdigen LAN oder über VPN verwenden: Die Verbindung ist **HTTP ohne TLS**. Ein Browserabbruch beendet eine laufende Aussendung nicht. **Halt Tx** beziehungsweise die Desktopbedienung bleiben dafür maßgeblich.

[Ausführliche Bedienung](https://github.com/rudolfstepan/wsjtx/blob/master/docs/WEB-CONTROL.txt)
