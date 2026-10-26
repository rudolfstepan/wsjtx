# Messungen, Praxistests und Grenzen

Die Custom-Version wird mit reproduzierbaren Softwaretests und realen Empfangsaufnahmen geprüft. Diese Nachweise haben unterschiedliche Aussagebereiche.

## REIST16

| Prüfung | Beobachtung | Aussagegrenze |
| --- | --- | --- |
| Dichte synthetische Mischungen, zwei Seeds | Tiefe 3 empfängt unter anderem 48 Stationen bei 50 Hz Abstand und 60 bei 60 Hz Abstand. | 48 × 50 Hz ergeben 2425 Hz äußere Tonmittenspanne, keinen Nachweis innerhalb eines exakt 2400-Hz-Filters. |
| Unabhängiger gemischter Test bei −22 dB SNR2500 | Vorher 7/16, jetzt 11/16; ohne CQ-AP 10/16. | Fester endlicher Testsatz; bei −20 dB bleibt ein QSO-Fall hinter FT8, bei −24/−26 dB kein erfolgreicher Decoder in dieser Reihe. |
| Dieselbe reale Aufnahme mit zusätzlichem Rauschen | Bei 8,5 dB zusätzlicher digitaler Abschwächung 7/10 statt 4/10; bei 9 dB 2/10 statt 0/10. | Keine kalibrierte HF-Leistungsänderung. |
| Vier weitere reale Aufnahmen | Drei REIST16-CQs bei geschätzten −16/−17/−18 dB; alle ohne CQ-AP. Die letzte benötigt Tiefe 3. | Kein belastbarer FT8-Vorsprung, weil eine FT8-Aussendung nicht sicher identifiziert werden konnte. |
| Sendepegelprüfung | Digitaler aktiver RMS-Pegel praktisch gleich FT8, Wellenform ohne PCM-Überlauf. | Kein Nachweis der tatsächlichen HF-Leistung oder der Linearität eines Funkgeräts. |

Die privaten Rohaufnahmen bleiben lokal. Veröffentlicht werden die aggregierten Ergebnisse, Methoden und Grenzen. 200 Ausgabeeinträge sind keine Zusage von 200 gleichzeitig dekodierbaren Stationen.

## Weitere Prüfungen

- FT8-/FT4-/FT2-Kerne gegen die ursprünglichen Implementierungen, einschließlich Encoder, LDPC, Wellenform und aufgezeichneter Samples.
- Echte Modulator-/Audio-Thread-Übergabe und IPC-Worker ohne Funkhardware.
- Logbuch-/UDP-Regressionen, PSK-Reporter-Testserver und Websteuerung.
- Pluto-/HackRF-Bridges und TCI-Protokolle im simulierten Betrieb.
- Frisch entpacktes RC3-Paket mit reinem Windows-PATH: Laufzeitbibliotheken, Branding, Ressourcen, schwacher CQ-Fall und 48 Stationen.

## Was daraus nicht folgt

Die Tests belegen keine allgemeine Überlegenheit gegenüber FT8, keine vollständige HF-Abnahme jeder SDR-Strecke und keine garantierte Echtzeitlaufzeit unter beliebiger Last. Ein schnellerer einzelner Kernel bedeutet nicht denselben Faktor für die ganze Anwendung.

## Belege

- [Mehrsignalempfang](https://github.com/rudolfstepan/wsjtx/blob/master/docs/validation/reist16-multisignal/IMPROVEMENTS.md)
- [Schwachsignalvergleich](https://github.com/rudolfstepan/wsjtx/blob/master/docs/validation/reist16-multisignal/WEAK-UPGRADE.md)
- [Digitale Sendepegelprüfung](https://github.com/rudolfstepan/wsjtx/blob/master/docs/validation/reist16-multisignal/TX-AUDIO.md)
- [Zusammenfassung realer Empfangsversuche](https://github.com/rudolfstepan/wsjtx/blob/master/docs/validation/reist16-multisignal/FIELD-TESTS.md)
- [RC3-Archiv- und Laufzeitprüfung](https://github.com/rudolfstepan/wsjtx/blob/master/docs/validation/reist16-multisignal/release-rc3-runtime.json)
