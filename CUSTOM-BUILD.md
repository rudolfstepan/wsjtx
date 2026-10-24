# Eigene WSJT-X-Version mit PSK Reporter und experimentellem REIST16

Release 3.0.2-oe3sro.5 enthält REIST16-QSO/2 als experimentellen
15-s-CQ-/QSO-Modus mit 12,8 s Signalzeit, CRC14 und LDPC(174,91).
[Bedienung und Hardwareversuch](docs/REIST16.md), [Profilvertrag](docs/REIST16.txt)
und [Pluto-Bridge](docs/PLUTO-WSJTX.txt). Die Yaesu-/Pluto-Dekodierung wurde
vom Benutzer dokumentiert; eine Empfangsschwelle ist noch nicht vermessen.
Die lokale Pluto-Laufzeit lässt sich mit `tools/Setup-PlutoBridge.ps1 -Download`
aus dem per SHA256 geprüften offiziellen libiio0.26-Archiv vorbereiten.

## Abfragebegrenzung und HTTP 429

Die XML-Abfrage kann mit **Too Many Requests** abgelehnt werden, während die
Browserkarte weiterhin funktioniert. Die Karte kann andere oder bereits
zwischengespeicherte Daten verwenden; ein funktionierender Browserzugriff hebt
die Beschränkung der API-Abfrage nicht auf.

Der Fork speichert den nächsten erlaubten Abfragezeitpunkt vor jedem Abruf.
Diese Pause gilt über Neustarts sowie für Profile und Instanzen mit derselben
Konfigurationsdatei. Band-/Moduswechsel verkürzen sie nicht. Bei HTTP 429 oder
503 wartet er zunächst zehn Minuten, bei wiederholter Ablehnung 20, 40 und
maximal 60 Minuten als eigene Rückfallwartezeit. Ein längerer `Retry-After`-Wert
des Servers (Sekunden oder HTTP-Datum) hat Vorrang. Ein erfolgreicher gültiger
XML-Abruf setzt die Fehlerstufe zurück. Bestehende Beobachtungen bleiben bei
Fehlern erhalten. Der Refresh-Knopf zeigt die verbleibende Pause in Sekunden.

Die gemeinsamen Werte liegen unter `CustomPskReporterService` außerhalb der
einzelnen Profilgruppen. Andere Programme oder Browserzugriffe lassen sich
damit nicht koordinieren. Die allgemeinen Abfragevorgaben stehen in der
[PSK-Reporter-Entwicklerdokumentation](https://www.pskreporter.info/pskdev.html).

English: HTTP 429/503 triggers increasing query backoff (10, 20, 40, then 60
minutes), honoring a longer server `Retry-After` in seconds or HTTP-date form.
The next-query deadline is shared by profiles/instances using the same settings
file and survives restarts. The browser map can remain available independently.
Other applications and browser requests are not coordinated by this local gate.

## Persönliche Ausbreitungsstatistik

Unter **View → PSK Reporter — Who hears me? → Personal DX windows** zeigt
die Anwendung empirische Zeitfenster für das eigene gesendete Signal.
Während die PSK-Liste geöffnet und ihre automatische Aktualisierung aktiv ist,
wachsen die lokalen Daten mit den regulären Abfragen im Fünfminutenabstand.
Es gibt keine zusätzlichen API-Abfragen oder historischen Rückabfragen.

Die Gruppierung erfolgt nach DXCC-Land aus der vorhandenen Rufzeichendatenbank,
Band, UTC-Stunde und Kalendermonat. Rufzeichen, Betriebsart und frei benennbares
Stations-/Antennenprofil bleiben getrennt. Der Monatfilter startet beim aktuellen
UTC-Monat; **All months** zeigt auch andere Monate. Das Suchfeld filtert Land
oder Band. Monate fassen mehrere Kalenderjahre zusammen.

**Most observed windows** ordnet nach der bisherigen Datenbasis.
**Next 24 hours (UTC)** zeigt die im passenden Kalendermonat beobachteten
Stundenfenster in zeitlicher Reihenfolge für die nächsten 24 Stunden.
Ein bereits laufendes Stundenfenster trägt **Now**. Beim Umschalten werden
automatisch alle Monate berücksichtigt, damit auch der nächste UTC-Monat am
Monatsende einbezogen wird. Diese Wiederkehr historischer Stundenfenster ist
keine Aussage über die aktuellen Ausbreitungsbedingungen.

**Minimum reception days** blendet Fenster mit weniger unterschiedlichen
Empfangstagen aus. Die Historienauswahl begrenzt die Auswertung wahlweise auf
alle Daten oder die letzten 90 bzw. 365 UTC-Kalendertage einschließlich heute.
**First observed** und **Last observed** zeigen den Zeitraum der tatsächlich
verwendeten Beobachtungen je Fenster. So bleiben alte Daten und kleine
Stichproben erkennbar; ein einzelner Empfangstag bekommt einen entsprechenden
Hinweis im Tooltip.

Die Reihenfolge bevorzugt viele unterschiedliche Empfangstage, danach viele
Empfänger-Tage. Ein Empfänger zählt pro UTC-Datum und Stunde einmal, auch bei
wiederholten oder überlappenden Abfragen. Die Spalten nennen Empfangstage,
unterschiedliche Empfänger und Empfänger-Tage. Das sind positive Beobachtungen,
keine QSO-Erfolge oder Erfolgswahrscheinlichkeiten: Eigene Sendeaktivität,
Leistung und die Aktivität der meldenden Empfänger beeinflussen die Statistik.
Fehlende Meldungen belegen keinen geschlossenen Ausbreitungsweg.

Nach Änderungen an Antenne, Standort oder Leistung einen neuen Namen unter
**Station / antenna setup** vergeben und das Eingabefeld verlassen. Alte Daten
bleiben unter ihrem bisherigen Namen verfügbar. Ab einem Profilwechsel werden
nur Meldungen ab dem Wechselzeitpunkt neu gesammelt, damit verspätete Berichte
der vorherigen Anlage nicht dem neuen Profil zugeschrieben werden.
Speicherung ausschließlich lokal im jeweiligen Konfigurationsprofil unter
`CustomPskReporter/PropagationV1`, ohne Upload. Vor der ersten Datensammlung ist
die Ansicht leer; für Schwechat werden die tatsächlich abgefragten Meldungen
des konfigurierten eigenen Rufzeichens verwendet, kein fester Standort.

English: **Personal DX windows** stores local positive reception observations
by DXCC country, band, UTC hour and calendar month. Ranking uses distinct
reception days followed by receiver-days; each receiver counts once per UTC
date/hour regardless of repeated polling. Callsigns, modes and named station
setups are separate. Collection uses the existing five-minute queries while
the receiver list is open; no historical backfill is performed. Select a new
setup name after antenna, location or power changes. Earlier observations remain
available under their original setup name. Only reports timestamped after a
setup change are added to the selected setup. These counts describe observed
windows, not QSO probabilities; transmitting and receiver activity bias them.
The **Next 24 hours (UTC)** view orders matching historical hourly windows by
their next occurrence, including UTC month/year transitions. It is not a live
propagation forecast. Filter by minimum distinct reception days and by the last
90/365 UTC calendar days (including today), or use all history. First/last
observed dates describe the samples used in each row.

## Automatischer Windows-Download

Das [aktuelle GitHub-Release](https://github.com/rudolfstepan/wsjtx/releases/latest)
enthält einen direkten Download des Windows-ZIPs und der separaten Dokumentation.
Das Windows-ZIP einmal vollständig entpacken, **Start-WSJT-X.bat** starten und
die beiliegende deutsche/englische Anleitung unter **docs/index.html** öffnen.
Release-Downloads sind öffentlich verfügbar und unterliegen nicht dem
90-Tage-Ablauf der Actions-Artefakte.

[OE3SRO Windows EXE in GitHub Actions](https://github.com/rudolfstepan/wsjtx/actions/workflows/custom-windows.yml)
baut bei jedem Push (alle Branches und Tags), Pull Request und manuellen Start
die Windows-x64-Anwendung. Im neuesten erfolgreichen Lauf führt **Artifacts**
zum versionierten Download; derselbe direkte Link steht in der Build-Zusammenfassung.
Eine GitHub-Anmeldung ist erforderlich. Die Aufbewahrungszeit beträgt 90 Tage.

Beispielname: `wsjtx-OE3SRO-3.0.2-devel-abcdef123-build12.1-windows-x64.zip`.
Die Version kommt aus `CMakeLists.txt`, ergänzt um Commit, Laufnummer und Versuch.
Das Paket enthält `bin/wsjtx.exe`, Decoder, Hamlib-Tools, Qt/OpenSSL-DLLs,
Stationsdaten, GPL-Lizenz, `build-info.json` und einen eigenen Start-Launcher.
Eine SHA256-Datei liegt neben dem ZIP. Nach dem Entpacken des Artefakts auch das
innere ZIP vollständig entpacken und **Start-WSJT-X.bat** öffnen.

Der CI-Runner lädt geprüfte Hamlib-/OmniRig-Abhängigkeiten, baut mit denselben
Skripten wie lokal, prüft PSK Reporter, FT8/FT4/FT2, REIST16 und die
Pluto-/HackRF-Bridges ohne Hardware und paketiert erst nach
erfolgreichen Tests. `Verify-CustomPackage.ps1 -PortableOnly` prüft die Laufzeit
ohne installierte Originalversion und ohne persönliche Konfiguration. Das Paket
enthält keine INI oder Logbuchdaten; beim ersten Start werden Stationsdaten und
Funkgerät eingerichtet. Ein vorhandenes Custom-Profil wird weiterverwendet.

`WSJTX_MINGW_PREFIX` erlaubt den abweichenden MSYS2-Pfad des Runners. Ohne diese
Umgebungsvariable bleibt lokal `C:\msys64\mingw64` maßgeblich.
`tools/New-WindowsArtifact.ps1 -Version VERSION` erzeugt das versionierte ZIP
aus dem geprüften Paket unter `.local\windows-artifacts`.
Es legt außerdem ein separates Dokumentations-ZIP samt SHA256 an. Die HTML-
Anleitung, technische FT2-Dokumentation und Änderungshistorie liegen in beiden
ZIPs; Links auf Quelltext verweisen auf den genauen Quellcommit.

Für ein Release zuerst `docs/releases/X.Y.Z-oe3sro.N.md` mit Release Notes
anlegen und committen. Anschließend den zugehörigen Tag auf diesen Commit setzen
und nach origin pushen, zum Beispiel:

```powershell
git tag oe3sro-v3.0.2-5
git push origin oe3sro-v3.0.2-5
```

Der Workflow prüft die CMake-Version, baut und testet den getaggten Quellstand
und veröffentlicht erst danach das Release mit Windows-ZIP, Dokumentations-ZIP
und beiden SHA256-Dateien als GitHub **Latest**. Für diesen Stand lautet
die Paketversion `3.0.2-oe3sro.5`;
die zugrunde liegende Anwendung bleibt WSJT-X 3.0.2-devel mit Custom-Branding.

## Lokaler Start und Bedienung

Die Änderungen gegenüber dem Original und ihre Quelltextbereiche stehen in
[docs/CHANGES.md](docs/CHANGES.md). Die technische FT2-Implementierung und ihre
Prüfungen sind in [docs/FT2.md](docs/FT2.md) auf Deutsch und Englisch dokumentiert.

Start im Projektordner:

Für den Start per Doppelklick die Datei **start.bat** öffnen. Sie verwendet
denselben Launcher und dasselbe Custom-Profil wie der PowerShell-Aufruf:

```powershell
.\Start-Custom.ps1
```

Das Startskript verwendet `dist-custom\OE3SRO-reist16-pluto\bin\wsjtx.exe --rig-name Custom`.
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
aktuellen Modus im aktuellen Band. Modus, Rufzeichen und Dialfrequenz werden
bei jeder Abfrage direkt aus der laufenden Anwendung gelesen. Mit **Current
frequency and mode** lässt sich die Anzeige auf Dialfrequenz bis +5 kHz
einschränken; **Current band and mode** ist die Vorgabe. Pro Station
wird die neueste Meldung angezeigt. Die Abfragen erfolgen frühestens alle
fünf Minuten, automatisch nur bei geöffneter Ansicht.
Die Refresh-Schaltfläche zeigt die verbleibende Wartezeit. Bei Abfragefehlern
bleiben vorhandene Meldungen erhalten und laufen anhand ihres Alters ab.
Antworten aus einem inzwischen geänderten Betriebskontext werden verworfen.
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
Empfänger aus anderen Arbeitsfrequenzen im Band werden angezeigt; direktes
Anrufen und Band-Activity-Markierung erfordern weiterhin den aktuellen
Arbeitsbereich (Dialfrequenz bis +5 kHz).

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
.\tools\Test-Core.ps1
.\tools\Test-TxQuality.ps1
.\tools\Test-PskReporter.ps1
.\tools\Test-Ft2.ps1
.\tools\Package-Custom.ps1
.\tools\Verify-CustomPackage.ps1
```

Der Windows-Build verwendet MSYS2/MinGW64 mit Qt5, Boost, FFTW, Fortran,
ActiveQt und libusb sowie das lokale Hamlib-4.7.2-SDK. Die OmniRig-Typbibliothek
wird mit festem Quellcommit und SHA256-Prüfung geladen. MAP65 und die zusätzlichen
Simulatoren werden für dieses WSJT-X-Paket nicht gebaut.

Auf einem Rechner ohne Originalinstallation und importiertes Profil genügt
für die Laufzeitprüfung `Verify-CustomPackage.ps1 -PortableOnly`. Das erste
[Release 3.0.2-oe3sro.1](https://github.com/rudolfstepan/wsjtx/releases/tag/oe3sro-v3.0.2-1)
wurde vom [Tag-CI-Lauf](https://github.com/rudolfstepan/wsjtx/actions/runs/38218198831)
erfolgreich gebaut, getestet und veröffentlicht. Beide öffentlichen ZIPs wurden
anschließend heruntergeladen und auf Prüfsummen, Quellcommit und Inhalt geprüft.

Eine einzelne Live-Abfrage zum Prüfen der API ist über
`tools\Test-PskReporterLive.ps1` möglich.
Das Skript ist ein FT8-Diagnosetest mit dem Rufzeichen aus der lokalen Importkopie;
der Produktionsdialog liest seinen Modus hingegen aus dem laufenden Hauptfenster.
Auch zwischen manuellen Live-Abfragen
mindestens fünf Minuten warten. Die Antwort wird ausschließlich lokal unter
`.local` gespeichert. Tests verwenden einen lokalen Server und prüfen die
Abfrageparameter, die Filter, doppelte Meldungen, Stationsauswahl, das
Abfrageintervall und fehlerhafte XML-Antworten. Zusätzlich werden Favoriten,
gespeicherte Anrufe, Modus-/Frequenzprüfung bei Wiederanrufen, nächste Station,
Logbuchfilter und Halt Tx geprüft, ohne eine Funkübertragung auszulösen.

`.local`, `.tools`, `build-custom` und `dist-custom` sind von Git ausgeschlossen.
