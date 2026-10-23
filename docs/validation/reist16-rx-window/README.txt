REIST16: laengeres Aufnahmefenster fuer reinen Empfang, 23.10.2026
build by OE3SRO

Umsetzung
---------
Die beiden REIST16-Pfade zur Festlegung des Aufnahmeendes verwenden dieselbe
Funktion capture_half_symbols(receive_only). Mit --receive-only werden50
Halbsymbole aufgenommen:172800Samples bei12kHz,also14.400s. Sende-/Empfangs-
betrieb bleibt bei48Halbsymbolen:165888Samples bzw.13.824s. Die FFT-Block-
groesse bleibt3456Samples,die Funk-Symboldauer bleibt2400Samples.

Grund fuer diese Begrenzung
--------------------------
Die vollstaendige lokale Tiefe3-Dekodierung ueber jt9/SharedMemory benoetigte
in den gemessenen Signalfaellen etwa1.0-1.3s. Bei allgemeinem Start14.400s
blieben nur900ms bis zum Ende des naechsten PTT-Anforderungsfensters.
Deshalb wird nur die explizite reine Empfangsoption verlaengert. Das betrifft
start-websdr.bat und den zusaetzlichen RX im REIST16-Selbsttest. Die kombinierte
Yaesu-/WebSDR-Instanz behaelt ihren bisherigen Start fuer automatische Antworten.
Die Messung ist keine Laufzeitgarantie; bestehende spaete TX-Sperren gelten weiter.

Pruefung
--------
tools/Test-Reist16.ps1 -SkipBuild nach Build von wsjtx,jt9 und Testprogrammen.
Vollstaendige Ausgabe:tests.txt.
- Unabhaengig erzeugte verrauschte CQ-TEST-Rahmen,Start1.1/1.3/1.5s,je2Faelle,
  -18dB bezogen auf2500Hz:altesFenster0/6,verlaengertesFenster6/6.
- Reeller IPC-Worker mit Tiefe3:normaler13.824sEmpfang,leererSlot,drei
  verzoegerte14.400sEmpfangsfaelle,danachwieder13.824sNormalfall. Exakte
  Nachricht in allen Signalfaellen,keine Nachricht im leerenSlot.
- Bestehende QSO-/Audio-/Eingabe-/ABI-/Mehrstations-/Negativtests bestanden.

Keine Funkgeraete oder echten Audiogeraete im Test geoeffnet;keine HF gesendet.
Keine Profile,Windows-Audioeinstellungen oder Originalaufnahmen veraendert.
Dies ist zusaetzliche Verzoegerungstoleranz,kein nachgewiesener dB-Gewinn.
Wiederholungs-Averaging gehoert noch nicht zu dieser Implementierung.
