# Synthetischer REIST16-QSO/2- und FT8-Schwellenvergleich

Sechs feste Nachrichten, AWGN-Seed 2026102301, unterster Ton 1500,375 Hz,
12 kHz mono und 15-s-Slots. SNR bezieht sich auf 2500 Hz. Die aktive
Leistung wird pro Modus gemessen; die aktive Signaldauer unterscheidet sich.

Bei dieser Realisierung dekodieren alle sechs Nachrichten in REIST16-QSO/2
bei -21 dB, in FT8 bei -20,5 dB. Das einzelne FT8-CQ dekodiert mit normaler
AP-Unterstützung bei -22,5 dB und bei -23 dB nicht mehr. Tiefe 3,
Suchbereich 100–4000 Hz, keine vorgegebenen Payload-/Rufzeichenhinweise.
Dies sind Übergänge für feste Testdaten, keine allgemeine Empfindlichkeits-
oder statistische Gleichwertigkeitsmessung. Keine HF-Hardware benutzt.

Die Float32-Varianten senken Signal und Rauschen gemeinsam ab: nominal
89 dB gegenüber dem nativen Generator bzw. weitere 49 dB gegenüber den
PCM16-Dateien mit Gain 0,01. Der Rauschabstand bleibt erhalten.

Erzeugen: `python tools/Generate-Reist16Threshold.py`,
`python tools/Generate-Ft8Threshold.py`, danach
`python tools/Finish-Ft8CqThreshold.py`.
Benötigt vorhandene WSJT-X-Bibliotheken und jt9 in `build-custom` sowie
MinGW. Die Ausgabeordner müssen neu sein. Generierte WAVs und ZIPs
liegen lokal unter `build-custom` und werden nicht als Buildprodukte versioniert.
Die JSON-Dateien hier archivieren die tatsächlich beobachteten Ergebnisse.
