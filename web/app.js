'use strict';
const $ = id => document.getElementById(id);
let key = '', connected = false, busy = false, polling = false, lastGood = 0, lastWaterfall = 0, imageUrl = '', session = 0;
const widgets = new Map();
const labels = {dxCallEntry:'DX Call',dxGridEntry:'DX Grid',TxFreqSpinBox:'TX · Hz',RxFreqSpinBox:'RX · Hz',rptSpinBox:'Rapport',bandComboBox:'Band / Frequenz',outAttenuation:'TX-Abschwächung · dB',tabWidget:'Nachrichtenansicht',sbFtol:'RX-Toleranz · Hz',respondComboBox:'CQ-Antwort',tx1:'',tx2:'',tx3:'',tx4:'',tx5:'',tx6:'',call:'Rufzeichen',grid:'Locator',name:'Name',sent:'Gesendet',rcvd:'Empfangen',txPower:'Leistung',comments:'Kommentar',exchSent:'Exchange TX',exchRcvd:'Exchange RX'};
const groups = {band:['bandComboBox'],transport:['logQSOButton','stopButton','monitorButton','EraseButton','DecodeButton','autoButton','stopTxButton','tuneButton'],dx:['dxCallEntry','dxGridEntry','lookupButton','genStdMsgsPushButton','outAttenuation'],qso:['TxFreqSpinBox','RxFreqSpinBox','pbR2T','pbT2R','rptSpinBox','txFirstCheckBox','cbHoldTxFreq','cbAutoSeq','cbCQonly','cbTx6','cbRxAll','respondComboBox','sbFtol','tabWidget'],messages:['txrb1','tx1','txrb2','tx2','txrb3','tx3','txrb4','tx4','txrb5','tx5','txrb6','tx6','txb1','txb2','txb3','txb4','txb5','txb6']};
function error(message){$('error').textContent=message;$('error').hidden=false;}
async function api(path, body){
  const response=await fetch(path,{method:body?'POST':'GET',headers:{Authorization:`Bearer ${key}`,...(body?{'Content-Type':'application/json'}:{})},body:body?JSON.stringify(body):undefined,cache:'no-store',signal:AbortSignal.timeout(4500)});
  if(!response.ok){let message=`HTTP ${response.status}`;try{message=(await response.json()).error||message;}catch{}throw new Error(message);}
  return response;
}
function enableControls(){
  for(const [id,w] of widgets){w.input.disabled=!connected||busy||!w.state?.enabled||(w.blocked&&id!=='stopTxButton');}
  $('modes').disabled=!connected||busy||$('modes').dataset.blocked==='true';
  $('saveLog').disabled=$('cancelLog').disabled=!connected||busy;
  for(const row of document.querySelectorAll('.decode-row'))row.disabled=!connected||busy||$('modes').dataset.blocked==='true';
}
async function send(id,value,extra={}){
  if(!connected||(busy&&id!=='stopTxButton'))return;
  busy=true;enableControls();$('error').hidden=true;
  try{await api('/api/command',{id,value,...extra});}
  catch(e){error(`${e.message} — Status prüfen; Befehle werden nicht automatisch wiederholt.`);}
  finally{busy=false;enableControls();await poll();}
}
function updateWidget(id,s,parent,blocked){
  let w=widgets.get(id);const field=id.replace(/^log\./,'');
  if(!w){
    const wrapper=document.createElement('label');wrapper.className='field';
    let input;
    if(s.kind==='button'){
      input=document.createElement('button');input.type='button';wrapper.classList.add('button');
      input.addEventListener('click',()=>send(id,s.checkable?!widgets.get(id).state.checked:null));
      if(['autoButton','stopTxButton','tuneButton'].includes(id))input.classList.add('danger');
    }else{
      const caption=document.createElement('span');caption.textContent=labels[field]??field;wrapper.append(caption);
      input=document.createElement(s.kind==='select'&&!s.editable||id==='bandComboBox'?'select':'input');
      if(input.tagName==='INPUT')input.type=['number','range'].includes(s.kind)?s.kind:'text';
      input.addEventListener('change',()=>send(id,input.tagName==='SELECT'||['number','range'].includes(s.kind)?Number(input.value):input.value));
    }
    input.setAttribute('aria-label',labels[field]||field);wrapper.append(input);$(parent).append(wrapper);
    w={wrapper,input,state:s};widgets.set(id,w);
  }
  w.state=s;w.blocked=blocked;w.wrapper.hidden=!s.visible;
  if(s.kind==='button'){
    w.input.textContent=s.text||(/^txrb[1-6]$/.test(id)?`Tx ${id.slice(-1)}`:id);w.input.setAttribute('aria-label',w.input.textContent);w.input.classList.toggle('checked',s.checked);w.input.setAttribute('aria-pressed',String(!!s.checked));
  }else if(document.activeElement!==w.input){
    if(w.input.tagName==='SELECT'){
      const signature=JSON.stringify(s.items);
      if(w.items!==signature){w.input.replaceChildren(...s.items.map((text,i)=>new Option(text,i)));w.items=signature;}
    }
    w.input.value=w.input.tagName==='INPUT'&&s.kind==='select'?s.text:s.value;
    if(s.min!==undefined){w.input.min=s.min;w.input.max=s.max;}
  }
}
function updateDecodes(id,rows,call){
  const target=$(id),signature=JSON.stringify(rows);if(target.dataset.signature===signature)return;
  const bottom=target.scrollHeight-target.scrollTop-target.clientHeight<35;
  target.dataset.signature=signature;
  target.replaceChildren(...rows.map(row=>{
    const el=document.createElement('button');el.type='button';el.className='decode-row';el.textContent=row.text||' ';
    if(/\bCQ\b/.test(row.text))el.classList.add('cq');
    if(call&&row.text.includes(call))el.classList.add('own');
    if(/\bTx\b/.test(row.text))el.classList.add('tx');
    const select=()=>send(id,row.line,{text:row.text});el.addEventListener('dblclick',select);
    el.addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();select();}});
    return el;
  }));
  if(bottom)target.scrollTop=target.scrollHeight;
}
function render(s){
  for(const id of ['frequency','mode','utc','callsign','grid'])$(id).textContent=s[id]||'—';
  $('rxTitle').textContent=s.rxTitle||'Rx Frequency';  $('bandTitle').textContent=s.bandTitle||'Band Activity';  for(const id of ['rxHeadings','bandHeadings','status','lastTx','watchdog'])if(s[id]!==undefined)$(id).textContent=s[id];  $('progress').max=s.progressMax||100;$('progress').value=Math.max(0,s.progress||0);
  $('radioStatus').textContent=s.transmitting?'TRANSMITTING':s.monitoring?'RECEIVING':'IDLE';$('radioStatus').classList.toggle('tx',!!s.transmitting);
  $('notice').hidden=!s.blocked;$('notice').textContent=`Am Stationsrechner ist ein Dialog geöffnet: ${s.dialog}. Dort schließen, um weiterzuarbeiten.`;
  const logging=Object.keys(s.log).length>0;
  for(const [parent,ids] of Object.entries(groups))for(const id of ids)if(s.controls[id])updateWidget(id,s.controls[id],parent,s.blocked||logging);
  const select=$('modes');select.dataset.blocked=String(s.blocked||logging);
  if(document.activeElement!==select){
    select.replaceChildren(...Object.entries(s.modes).map(([id,m])=>{const option=new Option(m.text,id,false,m.checked);option.disabled=!m.enabled;return option;}));
  }
  for(const id of ['decodedTextBrowser','decodedTextBrowser2'])updateDecodes(id,s[id],s.callsign);
  $('log').hidden=!logging;
  for(const [id,value] of Object.entries(s.log))updateWidget(`log.${id}`,value,'logFields',s.blocked);
  $('lastUpdate').textContent=`Aktualisiert: ${new Date().toLocaleTimeString()}`;
  enableControls();
}
async function waterfall(generation){
  if(!$('showWaterfall').checked||Date.now()-lastWaterfall<1800)return;lastWaterfall=Date.now();
  try{
    const r=await api('/api/waterfall');const blob=await r.blob();if(generation!==session)return;
    if(imageUrl)URL.revokeObjectURL(imageUrl);imageUrl=URL.createObjectURL(blob);$('waterfall').src=imageUrl;$('waterfall').hidden=false;$('waterfallHint').hidden=true;
  }catch{if(generation!==session)return;$('waterfall').hidden=true;$('waterfallHint').hidden=false;}
}
async function poll(){
  if(!key||polling||document.hidden)return;polling=true;const generation=session;
  try{
    const s=await(await api('/api/state')).json();if(generation!==session)return;
    connected=true;lastGood=Date.now();$('connection').textContent='Verbunden';$('connection').className='online';
    $('station').hidden=false;$('login').hidden=true;$('logout').hidden=false;render(s);await waterfall(generation);
  }catch(e){if(generation!==session)return;connected=false;$('connection').textContent='Verbindung unterbrochen';$('connection').className='offline';enableControls();error(e.message);}
  finally{polling=false;}
}
$('login').addEventListener('submit',e=>{e.preventDefault();key=$('key').value.trim();$('error').hidden=true;poll();});
$('logout').addEventListener('click',()=>{session++;key='';$('key').value='';connected=false;$('station').hidden=true;$('login').hidden=false;$('logout').hidden=true;$('error').hidden=true;$('connection').textContent='Nicht verbunden';$('connection').className='';if(imageUrl)URL.revokeObjectURL(imageUrl);imageUrl='';enableControls();});
$('openWaterfall').addEventListener('click',()=>send('waterfall.show',null));
$('modes').addEventListener('change',()=>send($('modes').value,null));
$('saveLog').addEventListener('click',()=>send('log.accept',null));$('cancelLog').addEventListener('click',()=>send('log.reject',null));
$('showWaterfall').addEventListener('change',()=>{$('waterfall').hidden=!$('showWaterfall').checked;});
document.addEventListener('visibilitychange',()=>{if(!document.hidden)poll();});
setInterval(()=>{if(connected&&Date.now()-lastGood>6000){connected=false;$('connection').textContent='Status veraltet';$('connection').className='offline';enableControls();}poll();},1000);
