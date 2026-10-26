# Reale Empfangsversuche: Zusammenfassung

26. Oktober 2026 · build by OE3SRO

Zusätzlich zu den reproduzierbaren synthetischen Tests wurden private
WebSDR-Aufnahmen mit dem tatsächlich ausgelieferten Decoder geprüft. Die
Originaldateien, die lokalen Einzelanalysen und Hörproben werden nicht mit
dem Release veröffentlicht. Diese Zusammenfassung hält deshalb nur die
Ergebnisse und ihre Aussagegrenzen fest.

## Aufnahme mit zusätzlichem Rauschen

Eine reale REIST16-Aufnahme wurde digital abgeschwächt und mit unabhängigem,
an den vorhandenen Rauschhintergrund angepasstem Rauschen ergänzt. Der gesamte
Originalinhalt einschließlich seines Rauschens wurde skaliert; das Nutzsignal
wurde nicht aus bekanntem Nachrichtentext neu erzeugt. Der neue und der
vorherige Decoder erhielten dieselben 118 gespeicherten Eingaben.

| Zusätzliche digitale Abschwächung | Vorher | Zusätzliche Schwachsignalverfahren |
| --- | --- | --- |
| 0 / 2 / 4 dB | jeweils 3/3 | jeweils 3/3 |
| 6 / 6,5 dB | jeweils 10/10 | jeweils 10/10 |
| 7 dB | 9/10 | 10/10 |
| 7,5 dB | 9/10 | 9/10 |
| 8 dB | 6/10 | 9/10 |
| 8,5 dB | 4/10 | 7/10 |
| 9 dB | 0/10 | 2/10 |
| 9,5 / 10 dB | jeweils 0/10 | jeweils 0/10 |
| 12 / 14 / 16 dB | jeweils 0/3 | jeweils 0/3 |
| Reines Rauschen | 0/10 | 0/10 |

Es erschien in diesem endlichen Satz keine fremde Nachricht. Der Gewinn
stammt aus der Decoderänderung: die gespeicherten Empfangseingaben bleiben
gleich. Die zusätzliche digitale Abschwächung ist keine kalibrierte HF-
Leistungsänderung; auch die ausgegebenen SNR-Werte sind Schätzungen.

## Vier neue Originalaufnahmen

Vier weitere reale WAVs wurden ohne Pegeländerung von 8 auf 12 kHz umgesetzt.
Beide Decoder wurden mit verschiedenen 15-s-Zeitfenstern geprüft. FT8 lief
zusätzlich über den Mehrfachdecoder, REIST16 auch mit Tiefe 1 und CQ-AP aus.
Insgesamt wurden 63 produktive CLI-Läufe ausgeführt.

Drei Aufnahmen liefern dieselbe Standard-CQ-Nachricht im REIST16-Modus,
mit geschätzten −16, −17 und −18 dB SNR. Alle drei gelingen auch ohne CQ-AP.
Bei der letzten gelingt Tiefe 3, Tiefe 1 bleibt erfolglos. Die vierte Aufnahme
liefert in keinem der beiden Modi eine bestätigte Teststationsnachricht.
Die WAVs zeigen kein digitales Clipping; daraus folgt kein Nachweis eines
linearen HF-Senders.

FT8 liefert keine bestätigte Nachricht der Teststation. Ein einzelner,
nicht bestätigter anderer Ausgang mit nicht aufgelösten Rufzeichen wird
weder als Teststationsempfang noch als bewiesener Fehlalarm gezählt.
Da sich eine ausgesendete FT8-Nachricht in den Dateien nicht sicher
identifizieren lässt, darf man daraus keine Decodierquote „REIST16 gegen
FT8“ oder einen Vorsprung in dB berechnen. Insbesondere ist das Ausbleiben
eines FT8-Decodes auf einem REIST16-Signal kein Vergleichsverlust für FT8.

Die Benutzerbeobachtung gleicher eingestellter Sendeleistung und zumindest
konkurrenzfähigen Empfangs motiviert weitere Tests. Ein belastbarer Vergleich
braucht abwechselnde dokumentierte Modi, gleiche gemessene HF-Leistung und
HF-Frequenz, feste Empfangseinstellungen sowie mehrere Wiederholungen pro
Leistungsstufe. Die realen Aufnahmen belegen bereits den praktischen Nutzen
der höheren REIST16-Decodiertiefe und der neuen Schwachsignalverfahren.
