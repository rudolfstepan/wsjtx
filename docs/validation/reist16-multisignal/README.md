# Mehrsignalprüfung REIST16 gegenüber FT8, 26. Oktober 2026

Dieses Dokument hält den Ausgangsstand vor der anschließenden Erweiterung
fest. Er konnte mehrere gleichzeitige Aussendungen an verschiedenen
Audiofrequenzen empfangen, erreichte jedoch noch nicht dieselbe Stationskapazität
wie FT8. Die damalige Untersuchung veränderte keinen Decoder, keine
Konfiguration und keine laufende Funkinstanz. Den inzwischen verbesserten
Stand beschreibt [IMPROVEMENTS.md](IMPROVEMENTS.md); die ursprünglichen
Ergebnisse und Hashes bleiben hier unverändert erhalten.

## Quellcodebefund

- `lib/reist16/qso2.cpp`: Suche über `low..high`, auf 100..4000 Hz begrenzt;
  Kandidaten nach Synchronisationswert sortiert, anschließend einzeln geprüft.
  Die Tiefe begrenzt die Versuche auf 12/24/48 Kandidaten. Das sind keine
  garantierten Stationszahlen: erfolglose Hypothesen verbrauchen das Budget.
  Der QSO/2-Kern beendet die Suche bei 32 Ergebnissen. Die zusätzliche
  kohärente Demodulation gilt nur für die ersten zwölf geprüften Kandidaten.
- `lib/reist16/reist16.f90`: produktive Ausgabe mit 32 Einträgen, auch bei
  gemischtem QSO/1- und QSO/2-Empfang. `lib/reist16/reist16.cpp` vereinigt
  beide Profile, sortiert nach Frequenz und schneidet auf diese Kapazität ab.
  Dadurch können bei Überfüllung höhere Frequenzen aus der Ausgabe fallen.
- REIST16 besitzt weder Signalabtrennung noch wiederholte Suche auf einem
  von bereits erkannten Signalen bereinigten Empfangspuffer.
- `lib/ft8_decode.f90`: bis zu 1000 Kandidaten pro Suchdurchlauf und
  `MAX_EARLY=200` gespeicherte verschiedene Nachrichten je Decoderinstanz;
  bei Tiefe 3 drei Durchläufe mit Signalabtrennung durch `subtractft8`.
  Diese Grenzen sind keine Empfangsgarantie. Der optionale FT8-Mehrthreadpfad
  in `lib/decoder.f90` kann zusätzlich Frequenzbereiche aufteilen;
  REIST16 verwendet dort einen einzigen Decoderaufruf.
- `widgets/mainwindow.cpp`, REIST16-Sendezweig: eine Standardnachricht,
  eine frei gewählte TX-Audiofrequenz und eine Wellenform pro Instanz.
  Mehrere Stationen können unabhängig verschiedene Audiofrequenzen verwenden.
  Ein einzelner REIST16-Sender erzeugt derzeit keinen Mehrträgerbetrieb
  mit mehreren Nachrichten wie der besondere FT8-Fox-Sendezweig.

## Offlineversuch

`probe.cpp` erzeugt neun gepaarte Mischungen mit unterschiedlichen Standard-CQs,
Seed 2026102601, zufälliger Phase und Starts zwischen +0,4 und +0,6 s.
Jedes normale Signal hat nominell -18 dB SNR bezogen auf 2500 Hz,
ungerampte Leistung 0,32 mal Verstärkung zum Quadrat. Ein Sonderfall hebt
die erste Station um 24 dB an. Alle Stationen eines Falls teilen denselben
Rauschpuffer; auch FT8 und REIST16 erhalten denselben Rauschvektor,
dieselben Startzeiten, Phasen, Frequenzen und nominellen Leistungen.
Die Formate haben unterschiedliche Dauer und damit unterschiedliche Energie.
Es gibt kein Fading, keine Drift, keine HF- oder Audiogerätefilterung.
PCM16, 12 kHz, 15 Sekunden, kein Clipping. Niedrigster Ton
200,375 Hz plus Stationsindex mal Abstand; Suche 100..4000 Hz.

Die Produktionsprüfung verwendet für jede Datei einen eigenen Prozess:
`jt9 --reist16 -d 3 -L 100 -H 4000 DATEI.wav` bzw.
`jt9 -8 -d 3 -L 100 -H 4000 DATEI.wav`. Normale FT8-AP-Voreinstellung,
keine erwarteten Texte oder Rufzeichen als Decoderhinweis, kein `-M`.
Die CLI bekommt die vollständigen 180000 Samples.

| Stationen | Abstand unterster Töne | REIST16, Tiefe 3 | FT8, Tiefe 3 |
| --- | --- | --- | --- |
| 3 | 100 Hz | 3/3 | 3/3 |
| 12 | 100 Hz | 12/12 | 12/12 |
| 24 | 100 Hz | 24/24 | 24/24 |
| 32 | 100 Hz | 32/32 | 32/32 |
| 36 | 100 Hz | 32/36 | 36/36 |
| 40 | 60 Hz | 32/40 | 40/40 |
| 2 | 50 Hz | 2/2 | 2/2 |
| 2 | 80 Hz | 2/2 | 2/2 |
| 2, erste +24 dB | 100 Hz | 1/2 | 2/2 |

Keine fremden Nachrichtentexte in diesen 18 Produktionsläufen. Die
40 REIST16-Signale überlappen spektral; ihr Empfang belegt deshalb auch,
dass 75 Hz Tonmitten-Spanne keine harte Mindestabstandsgrenze ist.

Zusätzlich prüft der Helfer die produktive REIST16-C-Schnittstelle mit dem
Live-Transceive-Aufnahmefenster von 165888 Samples (+13,824 s). Dort ergeben
sich bei 100 Hz Abstand für 3/12/24/32/36 Stationen mit Tiefe 1 jeweils
3/11/12/12/12, mit Tiefe 3 jeweils 3/12/24/32/32 korrekte Ergebnisse.
Der +24-dB-Sonderfall liefert im kürzeren Fenster bei Tiefe 3 sogar 2/2,
gegenüber 1/2 mit vollem CLI-Puffer. Der Unterschied zeigt die Abhängigkeit
der Kandidatenreihung vom Aufnahmefenster; er rechtfertigt keine allgemeine
Aussage über einen garantierten Pegelabstand. Alle nativen Ergebnisse
enthielten ausschließlich erwartete Bits.

Native Gesamtlaufzeiten einschließlich beider REIST16-Empfangsprofile:
Tiefe 1 etwa 0,25..0,34 s, Tiefe 3 etwa 0,88..1,12 s.
Produktions-CLI einschließlich Prozessstart: REIST16 etwa 0,95..1,22 s,
FT8 etwa 0,98..1,89 s. Ein Lauf je Fall auf diesem Windows-Rechner;
keine statistische Laufzeit- oder Echtzeitgarantie.

## Frequenzraum und Schlussfolgerung

QSO/2 hat 16 Töne mit 5 Hz Abstand, also 75 Hz zwischen den äußeren
Tonmitten. FT8 hat acht Töne mit 6,25 Hz Abstand, also 43,75 Hz.
Beides ist keine gemessene belegte Bandbreite. Unter der vereinfachten
Annahme von etwa 80 bzw. 50 Hz breiten Zuweisungen passen in 2400 Hz
ungefähr 30 REIST16- oder 48 FT8-Zuweisungen. Diese Rechnung illustriert
den Platzbedarf; sie ist kein gemessener Mindestabstand und keine
Kapazitätsgrenze für überlappende Signale.

Das Ziel „nicht weniger gleichzeitige Benutzer als FT8 im selben
Empfangsbereich“ war im Ausgangsstand somit nicht erfüllt. Die Mehrsignalarchitektur
ist vorhanden, aber Ausgabegrenze, Suchbudget, fehlende Signalabtrennung
und größerer Frequenzbedarf begrenzen sie. Sinnvolle nächste Arbeiten sind:

1. Die 32er-Grenze durchgängig über Kern, C-Schnittstelle und Fortran anheben;
   gleichzeitig ein zeitlich begrenztes, über Frequenzbereiche fair verteiltes
   Suchbudget einführen. Nur die Ergebnisarrays zu vergrößern genügt nicht.
2. Erkannte Signale rekonstruieren und abziehen, erneut suchen und dabei
   Pegelunterschiede, enge Nachbarsignale, Fehlalarme und Antwortzeit prüfen.
3. Für vergleichbare Stationsdichte entweder belastbare Überlappungsfähigkeit
   nachweisen oder ein schmaleres Profil entwerfen. Ein schmalerer Tonabstand
   verändert Symbolzeit, Orthogonalität und Empfangsverfahren; er lässt sich
   nicht ohne neue Protokoll- und Empfindlichkeitsprüfung einsetzen.

Die endlichen synthetischen Versuche sind kein allgemeiner Nachweis gleicher
Empfindlichkeit oder praktischer Bandkapazität. Insbesondere ist das
Heraufsetzen von Limits allein kein Nachweis von FT8-Gleichstand.

## Belege und Wiederholung

`native.tsv`, `production.csv` und `sources.json` halten Ergebnisse und Hashes
fest. WAVs, Einzeldecoderlogs und das Helferprogramm liegen lokal unter
`tmp/reist16-multisignal-20261026/` und werden nicht versioniert.
Der Helfer linkt die vorhandenen Produktionsarchive. Seine aktuelle Fassung
verwendet die erweiterte Empfangskapazität, unterstützt weitere Fälle und
einen wählbaren Seed. Eine Wiederholung mit dem aktuellen Decoder prüft
somit den neuen Stand; die obigen Ausgangszahlen gehören zu `sources.json`.

```powershell
.\docs\validation\reist16-multisignal\Run-Probe.ps1
```

Voraussetzung sind ein konfigurierter MinGW-Make-Build, die Produktionsarchive,
`jt9.exe` und die CMake-Linkdatei des vorhandenen QSO/2-Testtargets.
Der Ablauf ersetzt keine installierten Programme und sendet keine HF.
