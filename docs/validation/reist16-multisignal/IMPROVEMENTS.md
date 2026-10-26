# REIST16: erweiterter Mehrsignalempfänger

26. Oktober 2026 · lokale Weiterentwicklung nach 3.0.2-oe3sro.6 · build by OE3SRO

Der Empfänger kann jetzt wesentlich mehr Stationen eines Slots ausgeben und
erkannte Signale abziehen, um weitere Stationen zu finden. Der bisherige
[Ausgangsstand](README.md) mit 32 Ergebnissen bleibt einschließlich seiner
Messdateien und Hashes dokumentiert. Das QSO/2-Funkformat und die
Sendewellenform sind unverändert. Für dichte Belegung **Decodiertiefe 3** wählen.

## Implementierung

Die produktiven Fortran-Ausgabepuffer und die nativen Ergebnisgrenzen fassen
jetzt 200 Nachrichten. Die C-Schnittstelle respektiert weiterhin eine kleinere
vom Aufrufer bereitgestellte Kapazität; Pufferwächter prüfen dies auch bei
Kapazität 1 und der früheren Kapazität 32. Dieselbe Nachricht auf verschiedenen
Frequenzen darf mehrfach erscheinen. 200 ist eine Ausgabegrenze, keine Zusage,
dass 200 überlagerte Stationen in einem Audiokanal empfangen werden können.

Erfolgreiche Stationen verbrauchen kein Fehlversuchsbudget. Tiefe 1 erlaubt
zwölf erfolglose Kandidaten; Tiefe 2 bis zu zwei Durchläufe mit je zwölf,
Tiefe 3 bis zu drei mit je sechzehn. Je Durchlauf gelten zusätzlich höchstens
256 geprüfte Kandidaten und 200 QSO/2-Ergebnisse. Die kohärente Ergänzung
erlaubt insgesamt zwölf Fehlversuche, von denen ein Teil für spätere Durchläufe
reserviert bleibt. Auch dort verbrauchen erfolgreich akzeptierte Stationen
kein Fehlversuchsbudget. Ein Durchlauf ohne neue Ergebnisse beendet die Suche.

Ab Tiefe 2 wird jede akzeptierte Nachricht sofort als komplexe GFSK-Wellenform
rekonstruiert. Eine weitere Zeitschätzung über bekannte Wellenformabschnitte
verbessert die Ausrichtung; zwei gleitende 200-ms-Mittelungen schätzen eine
langsam veränderliche komplexe Amplitude. Der Empfänger zieht das rekonstruierte
Signal ab, wenn die verbleibende Gesamtenergie sinkt. Weitere Kandidaten
profitieren bereits im selben Durchlauf; der nächste Durchlauf berechnet auch
das Suchspektrum neu. Rekonstruktionen mit ungültigem Aufnahmefenster oder
ohne Energiegewinn verändern den Puffer nicht.

Zusätzlich wurde ein Fehler im LDPC-Ersatzpfad behoben: Ein erfolgreicher
nativer Durchlauf übernimmt jetzt auch den erfolgreichen Prüfstatus. CRC14,
vollständige Rekodierung, mindestens sieben von zwanzig Synchronisationstönen
und die zugelassenen Nachrichtentypen bleiben erforderlich.

Die größere Suche hat während der Entwicklung einen falschen CRC-Treffer in
der 48-Stationen-Mischung mit Seed 2026102601 sichtbar gemacht. Deshalb prüft
der Empfänger zusätzlich, ob die Nutzdatenphase und -amplitude zur bekannten
Synchronisation passen. Frequenzabweichung und milde Drift werden allein aus
den Synchronisationssymbolen geschätzt. Wegen der drei getrennten Syncblöcke
werden Modelle mit mindestens 85 % der besten Syncleistung berücksichtigt;
eine ungenaue Startzeit wird bei Bedarf allein aus den bekannten Syncblöcken
verfeinert. Das verhindert, dass ein brauchbarer nichtkohärenter Decoderstart
allein wegen seiner ungenauen Phase verworfen wird.

Die Prüfung beanstandet einen Nutzdaten-Amplitudenquotienten unter 0,7 nur
dann, wenn auch der Quotient plus dreifacher geschätzter Streuung unter 0,9
bleibt. Die Streuung stammt aus den Synchronisationssymbolen. Dies ist eine
empirisch geprüfte Empfangsplausibilität, keine statistisch bewiesene
Fehlalarmgrenze. Die Frequenz-/Driftsuche berücksichtigt ±0,5 Hz Restfehler
und ±0,05 Hz/s Drift. Sie erweitert nicht automatisch die Driftfestigkeit
des vorgeschalteten Demodulators.

Zeit und Frequenz werden abwechselnd verfeinert statt über ihr vollständiges
Produktgitter. QSO/1 und QSO/2 suchen gleichzeitig mit unabhängigen Daten und
Decoderzuständen; die gemeinsame FFTW-Planung bleibt gesperrt. Kann der
Legacy-Worker nicht gestartet werden, erfolgt die Suche seriell. Der native
QSO/2-Kern ruft den nativen LDPC-Kern direkt auf. Diese Änderungen verkürzen
auf diesem Mehrkernrechner die Zeit bis zur gemeinsamen Ergebnisliste.

## Vergleich mit FT8

Die endgültige Prüfung verwendet die Seeds **2026102601** und **2026102602**
mit je 14 Fällen. `probe.cpp` erzeugt unterschiedliche Standard-CQs, zufällige
Phasen und Startzeiten von +0,4 bis +0,6 s. Jedes normale Signal hat nominell
−18 dB SNR2500. Je Formatpaar sind Rauschvektor, nominelle Leistung, Frequenz,
Phase und Startzeit gleich; unterschiedliche Rahmendauern bedeuten dennoch
unterschiedliche Energie. PCM16, 12 kHz, 15 s, kein Clipping, keine Drift,
kein Fading und keine Audiogeräte-/SSB-Filterung in dieser Mehrsignalreihe.
Der erste unterste Ton liegt bei 200,375 Hz, weitere Stationen folgen im
angegebenen Abstand. Die Suche reicht von 100 bis 4000 Hz.

Die Produktionsprüfung startet für jede Datei einen eigenen `jt9`-Prozess
mit `--reist16` beziehungsweise `-8`, Tiefe 3 und voller 15-s-Aufnahme.
FT8 verwendet seine normale AP-Voreinstellung, ohne passende Rufzeichen-
oder Texthinweise und ohne `-M`. Die native Prüfung verwendet dagegen das
kurze Live-Transceive-Fenster von 165888 Samples und prüft die vollständigen
77 Informationsbits. Beide Aufnahmefenster liefern bei Tiefe 3 die folgenden
endgültigen Ergebnisse:

| Mischung | REIST16 vorher, Seed 1 | REIST16 jetzt, Seed 1 / 2 | FT8 jetzt, Seed 1 / 2 |
| --- | --- | --- | --- |
| 36 Stationen, 100 Hz | 32/36 | 36/36 · 36/36 | 36/36 · 36/36 |
| 40 Stationen, 60 Hz | 32/40 | 40/40 · 40/40 | 40/40 · 40/40 |
| 48 Stationen, 50 Hz | nicht gemessen | 48/48 · 48/48 | 48/48 · 48/48 |
| 60 Stationen, 60 Hz | nicht gemessen | 60/60 · 60/60 | 60/60 · 60/60 |
| 2 Stationen, 100 Hz, erste +24 dB | 1/2 in CLI | 2/2 · 2/2 | 2/2 · 2/2 |

Auch die Fälle mit 3, 12, 24 und 32 Stationen bei 100 Hz sowie zwei Stationen
bei 50 oder 80 Hz werden vollständig empfangen. Mit +24 dB für die erste
Station werden zusätzlich 25, 50 und 80 Hz Abstand geprüft; jeweils 2/2 bei
beiden Seeds. Keine fremden Texte in den 56 endgültigen Produktionsläufen
und keine fremden Bits in den 56 nativen Aufrufen beider Decodiertiefen.

Tiefe 1 bleibt für dichte Überlagerung weniger vollständig: Bei 40 Stationen
und 60 Hz Abstand ergeben sich 36/40 beziehungsweise 32/40, bei 48 Stationen
und 50 Hz 35/48 beziehungsweise 40/48. Die getrennten 36 Stationen bei 100 Hz
werden bereits mit Tiefe 1 vollständig erkannt. Das frühere feste
Zwölf-Kandidaten-Limit ließ dort nur zwölf Stationen zu.

Die nativen Aufrufe einschließlich beider Profile benötigen in dieser Reihe
bei Tiefe 3 etwa 0,43 bis 1,20 s. Die Produktions-CLI einschließlich Prozessstart
benötigt etwa 0,49 bis 1,30 s. Die dichtesten Fälle passen auf diesem Rechner
damit in das nominelle Zeitbudget von 1,476 s zwischen Aufnahmeende +13,824 s
und Ende des nächsten PTT-Anforderungsfensters +15,300 s. Das ist keine
Echtzeitgarantie: IPC, GUI, CAT-Latenz, weitere Programme und andere Rechner
brauchen ebenfalls Zeit. Die zusätzliche CPU-Arbeit des parallelen
Legacy-Empfängers ist in diesen Aufrufen enthalten.

## Regression und Grenzen

Die Tests halten insbesondere beide 48-Stationen-Mischungen als Regression
fest: Der zweite Seed deckte eine zu frühe Erschöpfung des kohärenten Budgets
auf, der erste den falschen CRC-Treffer. Die vollständigen Nutzdaten werden
im kurzen und im vollen Aufnahmefenster verglichen. Weitere native Tests
prüfen neun Rekonstruktionen mit beliebiger Phase, absichtlichen Zeit- und
Frequenzfehlern und mindestens 15 dB Signaldämpfung, unveränderte Null-/Randfälle,
40 Stationen bei Tiefe 1, kleinere ABI-Puffer und vier gleichzeitige Empfänger.

Die endgültige Schwachsignalreihe mit Seed 2026102211 reproduziert alle 128
einzelnen bisherigen Ergebnisse: 32/32, 32/32, 20/32 und 0/32 bei −18, −20,
−22 und −24 dB SNR2500. Auch die sechzehn negativen Kontrollslots bleiben
ohne Ausgabe. Die gesonderte Kanalreihe liefert weiterhin alle vier
Zufallsphase-/Driftfälle ohne Fading und einen von vier Fadingfällen;
128 Rausch-/Störträger-Slots bleiben ohne Ausgabe. Drei gemischte QSO/1-/QSO/2-
Stationen werden korrekt über die produktive C-Schnittstelle empfangen.

Die vollständige REIST16-Suite einschließlich sieben Produktions-PCM-
Roundtrips und echtem Shared-Memory-Worker besteht. Normale, leere und
verspätete Aufnahmen bis +1,5 s werden geprüft. Kompakte Ausgaben stehen in
`regression.txt`, die Kanalresultate in `final-channel.jsonl` und die
Schwachsignalsummen in `final-weak-summary.json`. Diese Regressionsläufe
erfolgen zur Funktionsprüfung teilweise gleichzeitig; ihre Laufzeiten sind
keine zusätzlichen Benchmarks. Es wird keine Funkhardware geöffnet und
keine HF ausgesendet.

Diese endlichen Versuche belegen deutliche Fortschritte gegenüber dem
Ausgangsstand und denselben Erfolg wie FT8 in den geprüften Mischungen.
Sie beweisen keine allgemeine Gleichheit von Empfindlichkeit, Bandkapazität,
Pegelabstand, Fehlalarmrate oder Fadingfestigkeit. Die größere Tonmitten-Spanne
von QSO/2 bleibt bestehen: 75 Hz gegenüber 43,75 Hz bei FT8. Insbesondere
benötigt die 48er-Mischung zwischen erster und letzter äußerer Tonmitte
2425 Hz; ein schmaler SSB-Filter wurde nicht mitgeprüft. Die verschiedenen
Frequenzraster sind keine garantierten Mindestabstände.

## Wiederholung und Belege

Der anschließend ausdrücklich beauftragte vollständige Clean-Build mit
MinGW Makefiles ist abgeschlossen. Alle neun Produktionsprogramme wurden
neu gebaut und in alle 17 vorhandenen lokalen Pakete übernommen; 153
Programmhashes wurden geprüft. Alle 17 fertigen Pakete bestehen zusätzlich
Versions-/Branding-, Manifest- und Webressourcenprüfung sowie einen eigenen
48-Stationen-Decode mit reinem Windows-PATH. Die Nachweise des fertigen
Clean-Builds stehen in `clean-build-runtime.json`; die Vergleichsmessungen
oben behalten die Hashes ihrer zuvor gemessenen Binärdateien.

In diesem ersten Testbuild war die zusätzliche Schwachsignalstufe noch nicht
enthalten. Die anschließende [Schwachsignal-Erweiterung](WEAK-UPGRADE.md)
ist inzwischen implementiert und separat geprüft. Eine allgemeine FT8-
Gleichwertigkeit bleibt unbewiesen. Profile und laufende Instanzen wurden
nicht umkonfiguriert; neue Programme werden beim nächsten Start wirksam.

```powershell
.\tools\Test-Reist16.ps1 -BuildDirectory build-custom-make
.\docs\validation\reist16-multisignal\Run-Probe.ps1 -Seed 2026102601
.\docs\validation\reist16-multisignal\Run-Probe.ps1 -Seed 2026102602
.\tools\Test-Reist16Weak.ps1 -OutputDirectory build-custom-make\new-weak-run -Trials 32 -Seed 2026102211 -SkipReference
```

Die Helfer werden im Repository ausgeführt. Sie verwenden lokal MinGW
Makefiles und erhalten den vorhandenen CI-Buildpfad. `Test-Reist16.ps1`
integriert das neue Target `reist16_multisignal_checks`. Der Kanaltest heißt
`reist16_qso2_channel_checks`. WAVs und vollständige lokale Logs verbleiben
in `tmp/reist16-validated-seed1-20261026/`,
`tmp/reist16-validated-seed2-20261026/` und den zugehörigen Regressionsordnern.
Die Dateien `final-seed*-production.csv`, `final-seed*-native.tsv` und
`final-seed*-sources.json` enthalten die endgültigen Ergebnisse und Hashes.
Die ursprünglichen Dateien ohne `final-` gehören weiter zum Ausgangsstand.

## English

The local receiver extension raises production output capacity from 32 to
200 messages, preserves search budgets for successful stations, and adds
immediate GFSK reconstruction/subtraction and repeated search at depth 2/3.
A payload-phase guard checks plausible synchronization frequency/drift
models and measured amplitude scatter; sync-only timing refinement avoids
rejecting a valid noncoherent timing estimate. CRC, re-encoding and existing
synchronization/type checks remain required. Independent QSO/1 and QSO/2
searches run concurrently. The QSO/2 wire format is unchanged.

Both fixed seeds recovered all 36 stations at 100 Hz spacing, 40 at 60 Hz,
48 at 50 Hz and 60 at 60 Hz at depth 3, with the production decoder and the
short live capture. FT8 recovered the same stations. No foreign payloads
occurred in the final 56 production processes or 56 native calls. Tests also
cover 24-dB near/far pairs, buffer boundaries, reconstruction and concurrent
receivers. Use depth 3 for dense reception. These finite unfiltered offline
mixtures do not establish general RF equivalence, false-alarm probability,
minimum spacing or real-time guarantees. QSO/2 still has a larger tone span;
the 48-station mixture spans 2425 Hz between its outermost tone centres.
