# PSK Reporter — Who hears me?

[Deutsch](PSK-Reporter) | [English](PSK-Reporter-EN)

**View → PSK Reporter — Who hears me?** zeigt Stationen, die dein ausgesendetes Signal gemeldet haben. Rufzeichen, Modus und Band kommen aus der laufenden Anwendung. Die Ansicht nutzt die letzten 30 Minuten und zeigt pro Empfänger dessen neueste passende Meldung.

![Who hears me mit Favoriten, Filtern und Anrufhistorie](https://raw.githubusercontent.com/rudolfstepan/wsjtx/oe3sro-v3.0.2-7-rc3/docs/images/psk-reporter-who-hears-me.png)

*Beispielansicht mit **Testdaten**. Die tatsächlichen Farben richten sich nach den WSJT-X-Hervorhebungsregeln und dem lokalen Logbuch.*

## Anzeigen und filtern

- Suche nach Rufzeichen oder Locator.
- Zeitfilter für 5, 15 oder 30 Minuten.
- Neue Rufzeichen, neu auf Band, Favoriten und bereits angerufene Stationen filtern.
- **Current frequency and mode** begrenzt optional den Bereich auf die aktuelle Dialfrequenz bis 5 kHz darüber.
- **QSO status** unterscheidet neue Rufzeichen, bereits gearbeitet und auf diesem Band gearbeitet.

Stationen aus der Empfangsliste werden in **Band Activity** türkis, fett und unterstrichen markiert. Ein Tooltip zeigt die Empfangszeit. Favoriten und die letzten 20 Anrufe bleiben im Konfigurationsprofil gespeichert.

## Eine Station anrufen

Eine Zeile auswählen und **Call station**, **Ctrl+Enter** oder Doppelklick verwenden. Das übernimmt Rufzeichen/Locator, wählt Tx1 und aktiviert **Enable Tx** für das eingestellte Sendefenster. Unterstützt sind FT2, FT8, FT4, JT65, JT9 und Q65 im normalen QSO-Betrieb.

**Call recent**, **Repeat last** und **Call next** helfen bei weiteren einzelnen Anrufversuchen. **Halt Tx** entspricht dem Stopp im Hauptfenster. Ein Empfangsbericht ist verzögert und verrät nicht zuverlässig die Sendefrequenz, Sequenz oder aktuelle QSO-Bereitschaft des Empfängers.

| Taste | Funktion |
| --- | --- |
| Ctrl+F | Suche |
| Ctrl+Enter | Ausgewählte Station anrufen |
| Ctrl+R | Letzten Anruf wiederholen |
| Ctrl+N | Nächste passende Station anrufen |
| Ctrl+Shift+S | Halt Tx |

## Abfragepausen

Reguläre Abfragen sind frühestens alle fünf Minuten möglich. HTTP 429/503 verlängert die Pause von zehn bis zu 60 Minuten; längere Servervorgaben haben Vorrang. Der Countdown bleibt über Neustarts erhalten. Fehler löschen vorhandene Meldungen nicht. Die Browserkarte kann unabhängig von der API weiter funktionieren.

REIST16 hat keine PSK-Reporter-Integration. [Persönliche DX-Zeitfenster](DX-Zeitfenster) verwenden die beobachteten Meldungen der unterstützten Modi.
