# Eigene WSJT-X-Version mit PSK Reporter

Die Änderungen gegenüber dem Original und ihre Quelltextbereiche stehen in
[docs/CHANGES.md](docs/CHANGES.md). Die technische FT2-Implementierung und ihre
Prüfungen sind in [docs/FT2.md](docs/FT2.md) auf Deutsch und Englisch dokumentiert.

Start im Projektordner:

```powershell
.\Start-Custom.ps1
```

Das Startskript verwendet `dist-custom\OE3SRO-ft2\bin\wsjtx.exe --rig-name Custom`.
Eine noch laufende frühere Custom-Version vor dem Neustart normal beenden.
Die Versionsanzeige, der Fenstertitel und das About-Fenster enthalten
**build by OE3SRO**.
Die installierte Konfiguration wurde unverändert nach
`%LOCALAPPDATA%\WSJT-X - Custom\WSJT-X - Custom.ini` kopiert und per SHA256
geprüft. Alle gespeicherten Profile und Einstellungen sowie vorhandene
Dateipfade bleiben erhalten. Eine private Sicherung liegt in
`.local\installed-configuration\WSJT-X.ini`. Lokale Stationsdaten und
das Logbuch wurden ebenfalls in das Custom-Profil kopiert.

FT2 ist unter **Mode → FT2** mit 3,75-Sekunden-Perioden verfügbar.
Sender, Decoder, Auto Seq, Wasserfall und PSK-Anrufhilfen unterstützen ihn.
Die Integration adaptiert WSJT-X Improved von DG2YCB (GPLv3); Details und
Prüfungen stehen in [docs/FT2.md](docs/FT2.md). FT2 bleibt experimentell.
Bestehende Frequenzen bleiben erhalten; nur fehlende FT2-Vorgaben werden
einmalig ergänzt.

Im Menü **View → PSK Reporter — Who hears me?** öffnet sich die Stationsliste.
Sie zeigt die Empfänger deines Signals aus den letzten 30 Minuten im
aktuellen Modus zwischen Dialfrequenz und Dialfrequenz + 5 kHz. Pro Station
wird die neueste Meldung angezeigt. Die Abfragen erfolgen frühestens alle
fünf Minuten, automatisch nur bei geöffneter Ansicht.
Die Zeilenfarben entsprechen deinen WSJT-X-Hervorhebungsregeln einschließlich
ihrer Reihenfolge, aktivierten Kategorien und der Einstellung zur Trennung
nach Modus. Sie berücksichtigen das Logbuch, Rufzeichen, DXCC, Locator,
Kontinent, CQ-/ITU-Zone, aktuelles Band und LoTW-Mitgliedschaft. Die Spalte
**QSO status** zeigt zusätzlich **New callsign**, **Worked** oder **Worked on band**.
Nach einem neuen Logbucheintrag oder geänderten Hervorhebungsregeln werden die
Farben bei geöffneter Liste aktualisiert.

Rufzeichen aus dieser Liste erscheinen in **Band Activity** türkis, fett und
unterstrichen. Der Tooltip nennt die PSK-Reporter-Empfangszeit. Dies gilt auch
für bereits sichtbare Decodes. Die Markierungen verschwinden nach einem
Rufzeichen-, Modus- oder Frequenzwechsel sowie spätestens 30 Minuten nach der
zugehörigen Empfangsmeldung. Die Decodetexte bleiben unverändert.

Mit **Call station** oder Doppelklick werden Rufzeichen und Locator in die
QSO-Felder übernommen, die erste Standardnachricht ausgewählt und **Enable Tx**
aktiviert. Der Anruf wird im nächsten eingestellten TX-Zeitfenster gesendet.
Die eingestellte TX-Frequenz und TX-Sequenz werden verwendet. Die Aktion setzt
normalen QSO-Betrieb mit Tx1 voraus. Während einer laufenden Aussendung, im
Tune-Betrieb oder bei Dateiwiedergabe zeigt die Ansicht den Hinderungsgrund an.
Nach einem Wechsel von Rufzeichen, Modus oder Frequenz muss die Liste vor
der Auswahl aktualisiert werden.

Zusätzliche Anrufhilfen in **Who hears me?**:

- **Favorite** speichert die ausgewählte Station dauerhaft. Ein `*` in
  **Fav.** kennzeichnet sie; ein Doppelklick in diese Spalte schaltet den
  Favoriten um. Der Filter **Favorites** zeigt deine Favoriten, die aktuell
  in der Empfangsliste stehen.
- Suche nach Rufzeichen oder Locator, Filter **New callsigns**, **New on this
  band** und **Called before** sowie Empfangsalter 5/15/30 Minuten helfen bei
  der Auswahl. Die Neu-Filter verwenden denselben Logbuchstatus wie die Farben.
- **Call recent** ruft eine Station aus den letzten 20 Anrufen erneut auf.
  **Repeat last (Ctrl+R)** wiederholt den letzten Anruf, auch ohne neue
  Empfangsmeldung. Dafür müssen eigenes Rufzeichen, Modus und Dialfrequenz
  mit dem gespeicherten Anruf übereinstimmen.
- **Call next (Ctrl+N)** wählt unter den sichtbaren Stationen bevorzugt eine,
  die du hier noch nicht auf dieser Frequenz in diesem Modus angerufen hast.
  Es startet jeweils einen Anruf auf deinen Tastendruck oder Klick.
- **Calls** und **Last call (UTC)** zeigen Anrufversuche und den letzten
  Startzeitpunkt. Erfolgreiche QSOs ergeben sich weiterhin aus dem Logbuch.
- **Halt Tx** führt denselben Stopp wie im Hauptfenster aus. Tastenkürzel:
  **Ctrl+Shift+S** zum Stoppen, **Ctrl+Enter** zum Anrufen der ausgewählten
  Station und **Ctrl+F** für die Suche. Die Kürzel gelten im PSK-Fenster.

Favoriten und Anrufhistorie bleiben nach einem Neustart im jeweiligen
Konfigurationsprofil erhalten. Sie werden erst bei einem Favoritenwechsel
oder erfolgreich gestarteten Anruf unter `CustomPskReporter` gespeichert.
Die Stationsfilter orientieren sich an der
[GridTracker Call Roster](https://docs.gridtracker.org/latest/Making-GridTracker-Work-For-You/Using-Call-Roster.html).

Empfangsmeldungen sind verzögert. Sie belegen, dass die Station dein Signal
gehört hat, aber enthalten keine verlässliche Sendefrequenz oder TX-Sequenz
des Empfängers und bestätigen keine QSO-Bereitschaft. Die Karte ist zusätzlich
über **Open PSK Reporter map** im Browser erreichbar.

Die Abfrage verwendet die offizielle HTTPS-XML-Schnittstelle:
<https://www.pskreporter.info/pskdev.html>.

Erneut bauen und paketieren:

```powershell
.\tools\Setup-CustomBuild.ps1
.\tools\Build-Custom.ps1
.\tools\Test-PskReporter.ps1
.\tools\Test-Ft2.ps1
.\tools\Package-Custom.ps1
.\tools\Verify-CustomPackage.ps1
```

Der Windows-Build verwendet MSYS2/MinGW64 mit Qt5, Boost, FFTW, Fortran,
ActiveQt und libusb sowie das lokale Hamlib-4.7.2-SDK. Die OmniRig-Typbibliothek
wird vom offiziellen Quellprojekt geladen. MAP65 und die zusätzlichen
Simulatoren werden für dieses WSJT-X-Paket nicht gebaut.

Eine einzelne Live-Abfrage zum Prüfen der API ist über
`tools\Test-PskReporterLive.ps1` möglich. Auch zwischen manuellen Live-Abfragen
mindestens fünf Minuten warten. Die Antwort wird ausschließlich lokal unter
`.local` gespeichert. Tests verwenden einen lokalen Server und prüfen die
Abfrageparameter, die Filter, doppelte Meldungen, Stationsauswahl, das
Abfrageintervall und fehlerhafte XML-Antworten. Zusätzlich werden Favoriten,
gespeicherte Anrufe, Modus-/Frequenzprüfung bei Wiederanrufen, nächste Station,
Logbuchfilter und Halt Tx geprüft, ohne eine Funkübertragung auszulösen.

`.local`, `.tools`, `build-custom` und `dist-custom` sind von Git ausgeschlossen.
