# WebSDR und WASAPI-Loopback

WASAPI-Loopback erfasst direkt den Windows-Wiedergabeausgang, auf dem der Browser spielt. Ein virtuelles Audiokabel und ein Stereo-Mix-Eingang sind dafür nicht erforderlich.

## Zusätzlicher Empfänger

**start-wasapi.bat** oder **start-websdr.bat** startet das getrennte Profil **WebSDR** mit `--receive-only`. Es kann neben der normalen Senderinstanz empfangen. Der Starter aktiviert keinen Sender und verändert dessen Profil nicht.

1. Im Browser die gewünschte WebSDR-Frequenz und USB einstellen.
2. Browseraudio auf den im Starter gewählten Windows-Ausgang legen.
3. Im Empfangsfenster passenden Modus und dieselbe Dialfrequenz einstellen.
4. **Monitor** einschalten. Für REIST16 **Decode → Deep** wählen.

Das Profil behält seine Modus-/Frequenzwahl. Der Standard-Gerätename des Starters passt zum ursprünglichen Arbeitsplatz; auf anderen Rechnern den exakten Ausgang übergeben:

```powershell
.\Start-Wasapi.ps1 -PlaybackDevice 'EXAKTER NAME DES WINDOWS-AUSGANGS'
```

Erfasst wird der gesamte Mix dieses Ausgangs, einschließlich anderer Programme und Systemtöne.

## Yaesu-TX mit WebSDR-RX

**start-yaesu-websdr.bat** verwendet genau **eine Custom-Instanz**: CAT/PTT und Sendeaudio bleiben beim Yaesu, nur der Empfang kommt aus dem Browser. Eine bereits laufende Custom-Instanz zuerst normal schließen. CAT stimmt den Browser nicht automatisch um.

Für einen gleichzeitig sichtbaren zusätzlichen Receive-only-Empfänger stattdessen die normale Station und das getrennte WebSDR-Profil verwenden. Dieselbe Custom-Konfiguration nicht doppelt als Sender starten.

## REIST16-Selbsttest

**start-reist16-selftest.bat** richtet getrennte TX- und RX-Testprofile ein. Der TX-Teil übernimmt beim ersten Anlegen die vorhandene Custom-Konfiguration; der RX-Teil hat keine CAT-Verbindung. Der Starter aktiviert keine Aussendung. Die normale Custom-Instanz muss geschlossen sein, damit nur der Testsender den CAT-Port nutzt.

## Verzögerung beachten

WebSDR, Browser und Netzwerk können Audio verzögern. Im reinen Empfangsmodus erfasst REIST16 bis 14,400 s des Slots; im Sende-/Empfangsbetrieb bis 13,824 s, damit Zeit für die automatische Antwort bleibt. Das ist keine Garantie für beliebig gepuffertes Browseraudio. Ausschnitte ohne vollständigen Rahmen lassen sich möglicherweise nicht dekodieren.

[Ausführliche WebSDR-Anleitung](https://github.com/rudolfstepan/wsjtx/blob/master/docs/WEBSDR-WSJTX.txt)
