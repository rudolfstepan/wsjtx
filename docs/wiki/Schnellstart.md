# Installation und erster Empfang

[Deutsch](Schnellstart) | [English](Quick-start)

## Windows-Paket installieren

1. Das [neueste Release](https://github.com/rudolfstepan/wsjtx/releases/latest) öffnen und **windows-x64.zip** herunterladen.
2. Das **gesamte ZIP** in einen eigenen Ordner entpacken. `bin`, `share`, DLLs und Dokumentation zusammenlassen.
3. Optional die zugehörige SHA256-Datei prüfen:

   ```powershell
   Get-FileHash .\wsjtx-OE3SRO-3.0.2-oe3sro.7-rc3-windows-x64.zip -Algorithm SHA256
   ```

4. **Start-WSJT-X.bat** starten. Beim ersten Start Rufzeichen, Locator, Funkgerät und Audio in **Settings** einrichten.
5. Zunächst **Monitor** einschalten und den Empfang prüfen. Die Systemzeit muss für die gewählte Betriebsart ausreichend genau synchronisiert sein.

Die Offline-Anleitung liegt im Paket unter **docs/index.html**. Das separate **documentation.zip** enthält die Anleitung und Messberichte ohne die Anwendung.

## Welcher Starter ist richtig?

| Aufgabe | Starter |
| --- | --- |
| Normales Custom-Fenster im entpackten Download | `Start-WSJT-X.bat` |
| Normales Custom-Fenster im Quellprojekt | `start.bat` |
| Zusätzlicher reiner Browseraudio-Empfänger | `start-wasapi.bat` oder `start-websdr.bat` |
| Yaesu-TX und Browseraudio-RX in einer Instanz | `start-yaesu-websdr.bat` |
| PlutoSDR mit eigener Instanz | `start-pluto.bat` |
| HackRF mit eigener Instanz | `start-hackrf.bat` |
| HackRF-FT8-Profil, erstmals auf 17 m | `start-hackrf-ft8-17m.bat` |
| Eigene REIST16-Aussendung über getrennten WebSDR-Empfänger prüfen | `start-reist16-selftest.bat` |

Alle Starter verwenden dasselbe Programmpaket. Die Geräteeinstellungen werden durch getrennte Profile auseinandergehalten. Dieselbe CAT-Schnittstelle beziehungsweise dasselbe Funkgerät nicht gleichzeitig mit zwei Senderinstanzen öffnen.

## REIST16 ausprobieren

**Mode → REIST16** und **Decode → Deep** wählen. Der zusätzliche CQ-Strukturversuch steht unter **Decode → Enable CQ AP (REIST16)**. Die Gegenstelle muss ebenfalls QSO/2 unterstützen. Frequenz vereinbaren und entgegengesetzte TX-Slots verwenden.

Die aktuelle Version erhöht den REIST16-Audiopegel gegenüber dem früheren Stand um etwa **1,94 dB**. Die Ansteuerung und ALC am realen Gerät neu kontrollieren. [Vollständige REIST16-Anleitung](REIST16).

## Aktualisieren

Die alte Instanz normal schließen, das neue Paket vollständig in einen eigenen Ordner entpacken und von dort starten. Bestehende Custom-Profile werden weiterverwendet; ein erneuter Konfigurationsimport ist für Updates nicht erforderlich. [Profile und Audio](Funkgeraete-und-Audio).
