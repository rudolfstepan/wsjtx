# Häufige Fragen und Fehlersuche

## REIST16 steht auf Fast oder Normal

Für die zusätzlichen Schwachsignalverfahren **Decode → Deep** wählen. CQ-AP wird getrennt unter **Enable CQ AP (REIST16)** geschaltet. Die Einstellungen gelten pro Instanz beziehungsweise Profil.

## Die Gegenstelle sieht kein REIST16

Beide Seiten müssen QSO/2 unterstützen. FT8 kann ein REIST16-Signal nicht dekodieren. Systemzeit, USB-Frequenz, Audioeingang, Filterdurchlass und vollständigen Empfang des Rahmens prüfen. [Einrichtung](REIST16).

## REIST16 wird nicht bei PSK Reporter gemeldet

Für REIST16 werden keine PSK-Reporter-Spots gesendet. Die integrierte Empfängerliste ersetzt keinen registrierten Modus im Meldesystem.

## Tx zeigt 1000 Hz, das ausgegebene Audio liegt höher

Bei Rig Split/Fake It verschiebt die Anwendung den TX-VFO und hält das Audio in der Filtermitte. Der gewählte HF-Sendeplatz bleibt gleich. Ohne Split gilt die gewählte Audiofrequenz direkt.

## Die PSK-Karte funktioniert, Refresh im Fenster wartet

Browserkarte und XML-API können unabhängig reagieren. Die Abfragepause beziehungsweise HTTP-429/503-Sperre abwarten; wiederholtes Drücken verkürzt sie nicht. Vorhandene Meldungen bleiben sichtbar.

## WebSDR bleibt stumm oder liefert keine Decodes

Den exakten Windows-Ausgang wählen, auf dem der Browser tatsächlich spielt. Browser und WSJT-X auf dieselbe USB-Dialfrequenz und denselben Modus stellen. Browserpufferung kann vollständige Frames aus dem Aufnahmefenster verschieben. [WASAPI](WebSDR-und-WASAPI).

## Die Weboberfläche kann nicht verbinden

Den Dienst am Desktop einschalten, angezeigte Adresse und Port verwenden und den aktuellen Schlüssel eingeben. Nach einem Programmneustart ist der Dienst aus. Setup und modale Desktopdialoge können Änderungen sperren. [Websteuerung](Websteuerung).

## Unterschiedliche Geräte brauchen unterschiedliche Installationen

Die Starter teilen sich ein Paket; getrennte Profile speichern die Geräteeinstellungen. Für Pluto, HackRF und WebSDR die passenden Starter verwenden. Nicht zwei Instanzen mit demselben CAT-Port betreiben.

## Was bei einer Fehlermeldung hilfreich ist

Version aus About, Starter/Profil, Modus, Betriebssystem, Audio-/Gerätepfad und genaue Meldung notieren. Bei Decoderproblemen zusätzlich angeben, ob eine vollständige WAV-Aufnahme vorhanden ist. Private Konfigurationen, Schlüssel und Rohaufnahmen nur bewusst weitergeben.
