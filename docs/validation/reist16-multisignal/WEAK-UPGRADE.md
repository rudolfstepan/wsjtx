# REIST16: zusätzliche Schwachsignal-Verfahren

26. Oktober 2026 · lokale Weiterentwicklung · build by OE3SRO

Diese Erweiterung verbessert den Empfänger, ohne das QSO/2-Funkformat,
die Sendeleistung oder die 15-Sekunden-Slots zu ändern. Sie ergänzt den
[Mehrsignalempfänger](IMPROVEMENTS.md). Die nachfolgenden Zahlen gehören
zu festen synthetischen Audiopaaren; sie sind keine allgemeine FT8-Gleichwertigkeit.

## Änderungen

- Die kohärente Demodulation prüft bei Tiefe 3 bis zu drei unterschiedliche
  Frequenzhypothesen je Verfahren nahe den besten Synchronisationspeaks. Bisher wurde
  nur eine solche Hypothese demoduliert.
- Ein zusätzlicher Durchlauf verwendet eine komplexe GFSK-Referenz der
  bekannten Synchronisationsabschnitte. Die Randhälften neben unbekannten
  Nutzdaten bleiben ausgespart. Damit stehen 17 vollständige Symbolzeiten
  für diese Referenz zur Verfügung. Der Start wird über ±100 ms mit
  2,5-ms-Schritten gesucht; die Phase wird anschließend mit der tatsächlichen
  Frequenz der geprüften Hypothese neu bestimmt.
- Solange noch keine Station akzeptiert wurde, darf der erste Durchlauf
  das vorhandene Budget von zwölf kohärenten Fehlversuchen ausschöpfen.
  Es bleibt dadurch kein Budget für einen späteren Abtrennungsdurchlauf
  ungenutzt, der mangels erster Decodierung gar nicht stattfinden könnte.
- Tiefe 3 darf Kandidaten mit fünf oder sechs statt mindestens sieben
  stärksten Sync-Tönen ausschließlich über den GFSK-Referenzpfad prüfen.
  Dafür sind zusätzlich mindestens sechs verschiedene beobachtete
  Nutzdatentöne und eine kohärente Syncenergie von mindestens dem
  Sechzehnfachen der geschätzten Rauschenergie dieser Referenz erforderlich.
  Das ist eine Empfangsbedingung, keine bewiesene Fehlalarmwahrscheinlichkeit.
- Ein begrenzter CQ-AP-Versuch verwendet den standardisierten CQ-Token,
  dessen Kennbit und Nachrichtentyp 1 beziehungsweise 2. Rufzeichen,
  Locator und erwarteter Nachrichtentext werden nicht vorgegeben.
  CRC14 und vollständige Rekodierung bleiben erforderlich. AP ergänzt
  die normale Decodierung; es wird erst nach deren erfolglosen Versuchen
  eingesetzt. Es gibt keine Mehrslot-Mittelung oder Rufzeichenliste.

Die Nutzdaten-Plausibilitätsprüfung des vorherigen Empfängers bleibt aktiv.
Die native C-Schnittstelle bleibt über `reist16_decode` kompatibel;
`reist16_decode_ex` erlaubt das explizite Schalten des CQ-AP-Versuchs.
Der Shared-Memory-Vertrag behält seine Größe. Der Empfänger verwendet im
REIST16-Modus das vorhandene AP-Flag mit eigener Bedienung und Einstellung.

## Bedienung

Für die zusätzlichen Referenz-/Hypothesenversuche **Decodiertiefe 3** wählen.
Unter **Decode → Enable CQ AP (REIST16)** lässt sich der CQ-Strukturversuch
ein- und ausschalten. Er ist standardmäßig eingeschaltet, wirkt ab Tiefe 2
und wird getrennt von FT8 als `REIST16AP` gespeichert. Bestehende Stations-
und Geräteeinstellungen werden nicht importiert oder überschrieben.

Die CLI verwendet CQ-AP standardmäßig. Zum Vergleich:

```powershell
jt9 --reist16 -d 3 datei_000000.wav
jt9 --reist16 --no-reist16-ap -d 3 datei_000000.wav
```

## Unabhängiger Vergleich

Der vorab für die abschließende Prüfung gewählte Seed **2026102629** erzeugt
64 Nachrichten: je acht CQ- und acht QSO-Nachrichten bei −20, −22, −24 und
−26 dB SNR2500. Sender/Empfänger, Phase, Frequenz und Start sind zufällig.
Die QSO-Nachrichten durchlaufen Locator, Rapport, R-Rapport, RR73 und 73;
ein Teil der Rufzeichen enthält `/P`. Vier der acht QSO-Fälle pro Pegel
haben zusätzlich +0,02 Hz/s Drift, die CQ-Fälle keine Drift. Die Gruppen
sind deshalb nur jeweils zwischen den Decodern, nicht als gleich schwere
CQ-/QSO-Kanäle zu vergleichen.

REIST16 und FT8 erhalten dieselbe nominelle aktive Leistung, denselben
Rauschvektor, Start, Phase, Frequenz und Drift. Unterschiedliche Rahmen-
dauern und Pulsformen bedeuten dennoch unterschiedliche Energie. Die
Aufnahmen sind PCM16/12 kHz mit voller 15-s-Aufnahme, ohne Clipping,
SSB-Filterung, Fading oder Mehrweg. Kein Decoder bekommt den erwarteten
Text oder die generierten Rufzeichen als Hinweis. FT8 verwendet den
normalen produktiven CLI-Pfad samt seiner normalen AP-Voreinstellung,
ohne Mehrthread-FT8-Option. Alle Aufrufe verwenden Tiefe 3 und 100..4000 Hz.

| SNR2500 | Nachrichten | Vorher | Jetzt, CQ-AP aus | Jetzt, CQ-AP ein | FT8 |
| --- | --- | --- | --- | --- | --- |
| −20 dB | 8 CQ | 8/8 | 8/8 | 8/8 | 8/8 |
| −20 dB | 8 QSO | 7/8 | 7/8 | 7/8 | 8/8 |
| −22 dB | 8 CQ | 5/8 | 7/8 | 8/8 | 3/8 |
| −22 dB | 8 QSO | 2/8 | 3/8 | 3/8 | 0/8 |
| −24 dB | 8 CQ + 8 QSO | 0/16 | 0/16 | 0/16 | 0/16 |
| −26 dB | 8 CQ + 8 QSO | 0/16 | 0/16 | 0/16 | 0/16 |

Bei −22 dB steigt das Ergebnis in diesen Fällen von 7/16 auf 11/16.
Auch ohne CQ-AP verbessert sich der Wert auf 10/16; der Gewinn ist also
nicht nur auf das zusätzliche CQ-Vorwissen beschränkt. Es gab keine
fremden Nachrichten in den 256 Decoderprozessen der vier Varianten.
Bei −20 dB bleibt in einem QSO-Fall eine Lücke gegenüber FT8. Bei −24
und −26 dB liefert diese Reihe für keinen Decoder einen Nachweis einer
erfolgreichen Decodierung. Es wird daher weder eine neue allgemeine
Empfindlichkeitsschwelle noch ein Gleichstand in allen Kanälen behauptet.

Die Entwicklung verwendete zuvor die getrennten Seeds 2026102627/28;
deren Ergebnisse werden nicht in die obige Prüfmenge eingerechnet.
Zusätzliche reale Aufnahmeversuche werden lokal separat gehalten.

## Regression und Wiederholung

`reist16_weak_checks --unit` prüft an unabhängigen Nutzdaten, dass die
verwendeten GFSK-Syncabschnitte mit einer nur aus bekannten Sync-Tönen
erzeugten Referenz übereinstimmen. Der Test ist in `Test-Reist16.ps1`
integriert. Der reale IPC-Worker wird dort mit wechselndem AP-Flag geprüft.
Die Mehrsignal-/Puffer-/Rekonstruktionsregression und die Kanalreihe mit
128 Rausch-/Störträger-Slots bestehen weiterhin ohne fremde Nutzlasten.
Zusätzliche Guard-Frames prüfen CQ TEST/RST, portable CQs und QSO-Antworten
bei zwei Pegeln; die CQ-Annahme darf deren tatsächlichen Inhalt nicht ersetzen.

Alle neun Produktionsprogramme und alle 17 vorhandenen lokalen Pakete
wurden aktualisiert und auf gleiche Programmhashes geprüft. Zusätzlich
besteht jedes fertige Paket mit reinem Windows-PATH die Branding-, Manifest-
und Webressourcenprüfung, einen neu gewonnenen schwachen CQ-Fall und einen
48-Stationen-Decode. Der AP-Schalter wurde am selben schwachen Fall kontrolliert:
eingeschaltet korrekt, ausgeschaltet ohne Decode. `weak-upgrade-runtime.json`
hält die tatsächlich ausgelieferten Binärhashes und diese Einzelprüfungen fest.

Die zusätzlichen Verfahren kosten Rechenzeit. Insbesondere bei dichter
Belegung kann die Zeitreserve für automatische Antworten knapp werden;
die Paketprüfungen erfassen auch Prozessstartzeit und sind keine Garantie
für jede Rechnerlast. Die bestehenden Schutzprüfungen gegen verspätetes
Senden bleiben aktiv. Die normalen IPC-Testslots lagen hier ungefähr bei
0,5 s; die dichteren CLI-Prüfungen benötigen mehr Zeit.

```powershell
.\tools\Test-Reist16.ps1 -BuildDirectory build-custom-make
python .\tools\Test-Reist16WeakUpgrade.py --output build-custom-make\new-weak-comparison --seed 2026102629
```

Der Python-Helfer benötigt nur die Standardbibliothek. Er baut den
Generator und Decoder, erzeugt neue Audiopaare und prüft den aktuellen
REIST16-Decoder mit/ohne CQ-AP sowie FT8. Mit `--reference-decoder` lässt
sich zusätzlich eine separat erhaltene ältere `jt9.exe` vergleichen.
Die aktuelle Ausgabe wird nie über ein vorhandenes Ergebnisverzeichnis
geschrieben. Gleichzeitige Decoderprozesse dienen der Funktionsprüfung;
ihre Laufzeiten sind keine isolierten Benchmarks.

Die hier gespeicherten Dateien `weak-upgrade-{summary,results,sources}.json`
halten Ausgänge und Hashes der Messung fest. Vollständige synthetische WAVs
und Logs liegen lokal unter `tmp/reist16-weak-holdout-seed29` und
`tmp/reist16-weak-final-comparison`. Die bestehende QSO/2-Kompatibilität
erspart ein Update des Senders; zum Nutzen der neuen Empfangsverfahren
muss der Empfänger neu gestartet werden.

## English

The receiver now tests additional coherent frequency hypotheses, an exact
Gaussian-FSK synchronization reference, and a bounded standard-CQ prior.
No callsign, locator or expected payload is supplied. The CQ prior can be
disabled in the Decode menu or with `--no-reist16-ap`; depth 3 enables the
additional weak synchronization path. Existing CRC, re-encoding and payload
plausibility checks remain active. The QSO/2 wire format and 15-second slots
are unchanged.

An independent 64-message paired test recovered 11/16 messages at −22 dB,
versus 7/16 previously and 10/16 with the CQ prior disabled. There were no
foreign messages in the 256 production decoder processes. The table also
shows remaining limits: one QSO case at −20 dB was recovered only by FT8,
and neither format decoded these −24/−26 dB cases. These finite tests do not
establish universal FT8 equivalence or a general sensitivity threshold.
