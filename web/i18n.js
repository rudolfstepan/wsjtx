'use strict';
// Browser-only preferences. Access keys and station data are never persisted.
const WebI18n = (() => {
  const strings = {
    title: ['WSJT-X · Experimental web control', 'WSJT-X · Experimentelle Websteuerung'],
    edition: ['OE3SRO · Web control (experimental)', 'OE3SRO · Websteuerung (experimentell)'],
    language: ['Language', 'Sprache'], automatic: ['Browser language', 'Browsersprache'],
    disconnected: ['Not connected', 'Nicht verbunden'], connected: ['Connected', 'Verbunden'],
    offline: ['Connection lost', 'Verbindung unterbrochen'], stale: ['Status out of date', 'Status veraltet'],
    logout: ['Disconnect', 'Trennen'], loginTitle: ['Connect to the station', 'Mit der Station verbinden'],
    loginHint: ['Find the access key on the station computer under View → Web control (experimental)…',
      'Den Zugangsschlüssel findest du am Stationsrechner unter View → Web control (experimental)…'],
    accessKey: ['Access key', 'Zugangsschlüssel'], connect: ['Connect', 'Verbinden'],
    startupNotice: ['Loading web controls. If this message remains, please reload the page.',
      'Weboberfläche wird geladen. Bleibt diese Meldung sichtbar, bitte die Seite neu laden.'],
    loginWarning: ['Experimental feature. Use only on a trusted LAN or VPN. Access also permits transmitting.',
      'Experimentelle Funktion. Nur im vertrauenswürdigen LAN oder über VPN verwenden. Der Zugang erlaubt auch das Senden.'],
    mode: ['Mode', 'Betriebsart'], bandTitle: ['Band Activity', 'Bandaktivität'], rxTitle: ['Rx Frequency', 'RX-Frequenz'],
    replyHint: ['Double-click / double-tap to reply', 'Doppelklick / Doppeltippen zum Antworten'],
    headings: ['UTC    dB   DT   Freq   Message', 'UTC    dB   DT   Freq   Nachricht'],
    transport: ['Receive and transmit', 'Empfang und Senden'], cycle: ['Transmit/receive interval progress', 'Fortschritt des Sende-/Empfangsintervalls'],
    dxTitle: ['DX Station', 'DX-Station'], qsoTitle: ['QSO controls', 'QSO-Steuerung'], messagesTitle: ['Transmit messages', 'Sendetexte'],
    liveImage: ['Live image', 'Livebild'], waterfallClosed: ['Wide Graph is closed.', 'Wide Graph ist geschlossen.'],
    openWaterfall: ['Open waterfall', 'Wasserfall öffnen'], waterfallAlt: ['Live station waterfall', 'Live-Wasserfall der Station'],
    logTitle: ['Log QSO', 'QSO protokollieren'], saveLog: ['Save QSO', 'QSO speichern'], cancel: ['Cancel', 'Abbrechen'],
    footer: ['The desktop application controls the radio and audio. Disconnecting the browser does not stop an ongoing transmission.',
      'Die Desktopanwendung steuert Funkgerät und Audio. Eine getrennte Browserverbindung beendet einen laufenden Sendebetrieb nicht.'],
    updated: ['Updated: {time}', 'Aktualisiert: {time}'],
    lastTx: ['Last TX: {message}', 'Letzte Sendung: {message}'],
    dxpedition: ['Stations calling DXpedition {call}', 'Stationen rufen DXpedition {call}'],
    transmitting: ['TRANSMITTING', 'SENDEN'], receiving: ['RECEIVING', 'EMPFANG'], idle: ['IDLE', 'BEREIT'],
    dialogOpen: ['A dialog is open on the station computer: {dialog}. Close it there to continue.',
      'Am Stationsrechner ist ein Dialog geöffnet: {dialog}. Dort schließen, um weiterzuarbeiten.'],
    commandError: ['{message} — Check the current status; commands are not repeated automatically.',
      '{message} — Status prüfen; Befehle werden nicht automatisch wiederholt.'],
    networkError: ['Cannot reach the station. Check the connection.', 'Station nicht erreichbar. Verbindung prüfen.'],
    timeoutError: ['The station did not respond in time.', 'Die Station hat nicht rechtzeitig geantwortet.'],
    httpError: ['Station request failed (HTTP {status}).', 'Stationsanfrage fehlgeschlagen (HTTP {status}).'],
    dxCallEntry: ['DX Call', 'DX-Rufzeichen'], dxGridEntry: ['DX Grid', 'DX-Locator'],
    TxFreqSpinBox: ['TX · Hz', 'TX · Hz'], RxFreqSpinBox: ['RX · Hz', 'RX · Hz'], rptSpinBox: ['Report', 'Rapport'],
    bandComboBox: ['Band / frequency', 'Band / Frequenz'], outAttenuation: ['TX attenuation · dB', 'TX-Abschwächung · dB'],
    tabWidget: ['Message view', 'Nachrichtenansicht'], sbFtol: ['RX tolerance · Hz', 'RX-Toleranz · Hz'],
    respondComboBox: ['CQ response', 'CQ-Antwort'], call: ['Callsign', 'Rufzeichen'], grid: ['Grid', 'Locator'],
    name: ['Name', 'Name'], sent: ['Sent', 'Gesendet'], rcvd: ['Received', 'Empfangen'],
    txPower: ['Power', 'Leistung'], comments: ['Comment', 'Kommentar'], exchSent: ['Exchange TX', 'Exchange TX'], exchRcvd: ['Exchange RX', 'Exchange RX'],
    logQSOButton: ['Log QSO', 'QSO protokollieren'], stopButton: ['Stop', 'Stopp'], monitorButton: ['Monitor', 'Monitor'],
    EraseButton: ['Erase', 'Löschen'], DecodeButton: ['Decode', 'Dekodieren'], autoButton: ['Enable TX', 'TX aktivieren'],
    stopTxButton: ['Halt TX', 'TX stoppen'], tuneButton: ['Tune', 'Abstimmen'], lookupButton: ['Lookup', 'Nachschlagen'],
    genStdMsgsPushButton: ['Generate standard messages', 'Standardtexte erzeugen'],
    pbR2T: ['RX → TX', 'RX → TX'], pbT2R: ['TX → RX', 'TX → RX'],
    txFirstCheckBox: ['TX even / first', 'TX gerade / zuerst'], cbHoldTxFreq: ['Hold TX frequency', 'TX-Frequenz halten'],
    cbAutoSeq: ['Auto Seq', 'Auto Seq'], cbCQonly: ['CQ only', 'Nur CQ'], cbTx6: ['TX6', 'TX6'], cbRxAll: ['RX all frequencies', 'Alle Frequenzen empfangen'],
    message: ['Transmit message {number}', 'Sendetext {number}'],
    selectMessage: ['Select TX {number}', 'TX {number} wählen'], sendMessage: ['TX {number} now', 'TX {number} sofort'],
  };
  // Translate server errors without altering the control protocol or its validation.
  const serverStrings = {
    'Access key required': ['Access key required', 'Zugangsschlüssel erforderlich'],
    'Close the dialog on the station computer first': ['Close the dialog on the station computer first', 'Zuerst den Dialog am Stationsrechner schließen'],
    'Waterfall unavailable': ['Waterfall unavailable', 'Wasserfall nicht verfügbar'],
    'No QSO to log': ['No QSO to log', 'Kein QSO zum Protokollieren'],
    'Log action unavailable': ['Log action unavailable', 'Logbuchaktion nicht verfügbar'],
    'Unknown log field': ['Unknown log field', 'Unbekanntes Logbuchfeld'],
    'Finish or cancel logging first': ['Finish or cancel logging first', 'Protokollierung zuerst abschließen oder abbrechen'],
    'Mode unavailable': ['Mode unavailable', 'Betriebsart nicht verfügbar'],
    'Invalid decode': ['Invalid decode', 'Ungültige Decodierung'],
    'Decode changed; select again': ['Decode changed; select again', 'Decodierung geändert; erneut auswählen'],
    'Unknown control': ['Unknown control', 'Unbekanntes Bedienelement'],
    'Control unavailable': ['Control unavailable', 'Bedienelement nicht verfügbar'],
    'Boolean required': ['Boolean required', 'Ein-/Aus-Wert erforderlich'],
    'Invalid text': ['Invalid text', 'Ungültiger Text'],
    'Invalid value': ['Invalid value', 'Ungültiger Wert'],
    'Invalid number': ['Invalid number', 'Ungültige Zahl'],
    'Invalid tab': ['Invalid tab', 'Ungültige Registerkarte'],
    'Invalid selection': ['Invalid selection', 'Ungültige Auswahl'],
    'Unsupported control': ['Unsupported control', 'Nicht unterstütztes Bedienelement'],
    'Band Activity': ['Band Activity', 'Bandaktivität'], 'Rx Frequency': ['Rx Frequency', 'RX-Frequenz'],
    'Single-Period Decodes': ['Single-Period Decodes', 'Decodierungen pro Periode'],
    'Average Decodes': ['Average Decodes', 'Gemittelte Decodierungen'],
    'Decodes containing My Call': ['Decodes containing My Call', 'Decodierungen mit eigenem Rufzeichen'],
    'Tx Messages': ['TX Messages', 'Sendetexte'],
    'Band Hopping On': ['Band hopping on', 'Bandwechsel aktiv'],
    'Runaway Tx watchdog': ['Runaway TX watchdog', 'TX-Zeitbegrenzung ausgelöst'],
    'Receiving': ['Receiving', 'Empfang'],
    'Receiving, Filters On': ['Receiving, filters on', 'Empfang, Filter aktiv'],
    'Receiving, Filters bypassed': ['Receiving, filters bypassed', 'Empfang, Filter umgangen'],
    'CQ: None': ['CQ: None', 'CQ: Keine Antwort'], 'CQ: First': ['CQ: First', 'CQ: Erste Station'],
    'CQ: Max Dist': ['CQ: Max distance', 'CQ: Größte Entfernung'],
    'CQ: Max dB': ['CQ: Max dB', 'CQ: Höchster dB-Wert'], 'CQ: Min dB': ['CQ: Min dB', 'CQ: Niedrigster dB-Wert'],
  };
  const storageKey = 'wsjtx.web.language';
  let preference = 'auto';
  try { const saved = localStorage.getItem(storageKey); if (['auto','de','en'].includes(saved)) preference = saved; } catch {}
  function browserLanguage() {
    for (const candidate of navigator.languages || [navigator.language]) {
      const base = String(candidate).toLowerCase().split('-')[0];
      if (base === 'de' || base === 'en') return base;
    }
    return 'en';
  }
  function language() { return preference === 'auto' ? browserLanguage() : preference; }
  function value(pair) { return pair[language() === 'de' ? 1 : 0]; }
  function t(id, args = {}) {
    return (strings[id] ? value(strings[id]) : id).replace(/\{(\w+)\}/g, (match, name) => String(args[name] ?? match));
  }
  function desktop(text) {
    const last = /^Last Tx:\s*(.*)$/i.exec(String(text).trim());
    if (last) return t('lastTx', {message:last[1]});
    const dxpedition = /^Stations calling DXpedition (.+)$/.exec(String(text).trim());
    if (dxpedition) return t('dxpedition', {call:dxpedition[1]});
    const pair = Object.values(serverStrings).find(pair => pair.includes(String(text).trim()));
    return pair ? value(pair) : text;
  }
  function label(id) { return t(id); }
  function setLanguage(next) {
    preference = ['auto','de','en'].includes(next) ? next : 'auto';
    try { localStorage.setItem(storageKey, preference); } catch {}
  }
  return {t, desktop, label, language, setLanguage, preference: () => preference};
})();
