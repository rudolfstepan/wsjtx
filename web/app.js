'use strict';
const $ = id => document.getElementById(id);
let key = '', connected = false, busy = false, polling = false, lastGood = 0, lastWaterfall = 0, imageUrl = '', session = 0;
const widgets = new Map();
const {t} = WebI18n;
let lastState = null, connectionStatus = 'disconnected', currentError = null;
const groups = {band:['bandComboBox'],transport:['logQSOButton','stopButton','monitorButton','EraseButton','DecodeButton','autoButton','stopTxButton','tuneButton'],dx:['dxCallEntry','dxGridEntry','lookupButton','genStdMsgsPushButton','outAttenuation'],qso:['TxFreqSpinBox','RxFreqSpinBox','pbR2T','pbT2R','rptSpinBox','txFirstCheckBox','cbHoldTxFreq','cbAutoSeq','cbCQonly','cbTx6','cbRxAll','respondComboBox','sbFtol','tabWidget'],messages:['txrb1','tx1','txrb2','tx2','txrb3','tx3','txrb4','tx4','txrb5','tx5','txrb6','tx6','txb1','txb2','txb3','txb4','txb5','txb6']};
function errorText(e){
  if(e.name==='TimeoutError'||e.name==='AbortError')return t('timeoutError');
  if(e instanceof TypeError)return t('networkError');
  return e.serverMessage?WebI18n.desktop(e.serverMessage):t('httpError',{status:e.status||'?'});
}
function error(e,retry=false){currentError={e,retry};renderError();$('error').hidden=false;}
function renderError(){if(currentError){const message=errorText(currentError.e);$('error').textContent=currentError.retry?t('commandError',{message}):message;}}
function setConnection(status){connectionStatus=status;$('connection').textContent=t(status);$('connection').className=status==='connected'?'online':status==='disconnected'?'':'offline';}
function applyLanguage(){
  document.documentElement.lang=WebI18n.language();$('language').value=WebI18n.preference();
  for(const node of document.querySelectorAll('[data-i18n]'))node.textContent=t(node.dataset.i18n);
  for(const attr of ['aria','alt'])for(const node of document.querySelectorAll(`[data-i18n-${attr}]`))node.setAttribute(attr==='aria'?'aria-label':'alt',t(node.dataset[attr==='aria'?'i18nAria':'i18nAlt']));
  setConnection(connectionStatus);renderError();if(lastState)render(lastState);
}
async function api(path, body){
  const response=await fetch(path,{method:body?'POST':'GET',headers:{Authorization:`Bearer ${key}`,...(body?{'Content-Type':'application/json'}:{})},body:body?JSON.stringify(body):undefined,cache:'no-store',signal:AbortSignal.timeout(4500)});
  if(!response.ok){const e=new Error();e.status=response.status;try{e.serverMessage=(await response.json()).error;}catch{}throw e;}
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
  catch(e){error(e,true);}
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
      const caption=document.createElement('span');wrapper.append(caption);
      input=document.createElement(s.kind==='select'&&!s.editable||id==='bandComboBox'?'select':'input');
      if(input.tagName==='INPUT')input.type=['number','range'].includes(s.kind)?s.kind:'text';
      input.addEventListener('change',()=>send(id,input.tagName==='SELECT'||['number','range'].includes(s.kind)?Number(input.value):input.value));
    }
    wrapper.append(input);$(parent).append(wrapper);
    w={wrapper,input,state:s};widgets.set(id,w);
  }
  w.state=s;w.blocked=blocked;w.wrapper.hidden=!s.visible;
  if(s.kind==='button'){
    const messageButton=/^tx(rb|b)[1-6]$/.test(id);
    w.input.textContent=messageButton?t(id.startsWith('txrb')?'selectMessage':'sendMessage',{number:id.slice(-1)}):WebI18n.label(id);
    w.input.setAttribute('aria-label',w.input.textContent);w.input.classList.toggle('checked',s.checked);w.input.setAttribute('aria-pressed',String(!!s.checked));
  }else{
    const messageField=/^tx[1-6]$/.test(id), label=messageField?t('message',{number:id.slice(-1)}):WebI18n.label(field);
    w.wrapper.firstElementChild.textContent=messageField?'':label;w.input.setAttribute('aria-label',label);
    if(document.activeElement===w.input)return;
    if(w.input.tagName==='SELECT'){
      const signature=WebI18n.language()+JSON.stringify(s.items);
      if(w.items!==signature){w.input.replaceChildren(...s.items.map((text,i)=>new Option(id==='respondComboBox'?WebI18n.desktop(text):text,i)));w.items=signature;}
    }
    w.input.value=w.input.tagName==='INPUT'&&s.kind==='select'?s.text:s.value;
    if(s.min!==undefined){w.input.min=s.min;w.input.max=s.max;}
  }
}
function applyTextStyle(node,style={}){
  // Only colors and text emphasis from Qt are accepted; never insert HTML/CSS from a decode.
  for(const [key,property] of [['foreground','color'],['background','backgroundColor']]){
    const color=style[key];node.style[property]=typeof color==='string'&&/^#[0-9a-f]{6}([0-9a-f]{2})?$/i.test(color)?color:'';
  }
  node.style.fontWeight=style.bold?'700':'400';node.style.fontStyle=style.italic?'italic':'normal';
  node.style.textDecoration=[style.underline?'underline':'',style.strikeout?'line-through':''].filter(Boolean).join(' ')||'none';
  node.title=style.tooltip||'';
}
function updateDecodes(id,rows,palette){
  const target=$(id);applyTextStyle(target,palette);const signature=JSON.stringify(rows);if(target.dataset.signature===signature)return;
  const bottom=target.scrollHeight-target.scrollTop-target.clientHeight<35;
  target.dataset.signature=signature;
  target.replaceChildren(...rows.map(row=>{
    const el=document.createElement('button');el.type='button';el.className='decode-row';applyTextStyle(el,{foreground:row.style?.foreground,background:row.style?.background});
    if(Array.isArray(row.runs)&&row.runs.length&&row.runs.map(run=>run.text).join('')===row.text){
      for(const run of row.runs){const span=document.createElement('span');span.textContent=run.text;applyTextStyle(span,run.style);el.append(span);}
    }else el.textContent=row.text||' ';
    const select=()=>send(id,row.line,{text:row.text});el.addEventListener('dblclick',select);
    el.addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();select();}});
    return el;
  }));
  if(bottom)target.scrollTop=target.scrollHeight;
}
function render(s){
  lastState=s;
  for(const id of ['frequency','mode','utc','callsign','grid'])$(id).textContent=s[id]||'—';
  $('rxTitle').textContent=WebI18n.desktop(s.rxTitle||'Rx Frequency');
  $('bandTitle').textContent=WebI18n.desktop(s.bandTitle||'Band Activity');
  for(const id of ['rxHeadings','bandHeadings'])$(id).textContent=s[id]?s[id].replace(/\b(Message|Nachricht)\b/g,WebI18n.language()==='de'?'Nachricht':'Message'):t('headings');
  for(const id of ['status','lastTx','watchdog'])if(s[id]!==undefined)$(id).textContent=WebI18n.desktop(s[id]);
  $('progress').max=s.progressMax||100;$('progress').value=Math.max(0,s.progress||0);
  $('radioStatus').textContent=t(s.transmitting?'transmitting':s.monitoring?'receiving':'idle');$('radioStatus').classList.toggle('tx',!!s.transmitting);
  $('notice').hidden=!s.blocked;$('notice').textContent=t('dialogOpen',{dialog:s.dialog});
  const logging=Object.keys(s.log).length>0;
  for(const [parent,ids] of Object.entries(groups))for(const id of ids)if(s.controls[id])updateWidget(id,s.controls[id],parent,s.blocked||logging);
  const select=$('modes');select.dataset.blocked=String(s.blocked||logging);
  if(document.activeElement!==select){
    select.replaceChildren(...Object.entries(s.modes).map(([id,m])=>{const option=new Option(m.text,id,false,m.checked);option.disabled=!m.enabled;return option;}));
  }
  for(const id of ['decodedTextBrowser','decodedTextBrowser2'])updateDecodes(id,s[id],s.decodePalette?.[id]);
  $('log').hidden=!logging;
  for(const [id,value] of Object.entries(s.log))updateWidget(`log.${id}`,value,'logFields',s.blocked);
  $('lastUpdate').textContent=t('updated',{time:new Date().toLocaleTimeString(WebI18n.language())});
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
    connected=true;lastGood=Date.now();setConnection('connected');
    $('station').hidden=false;$('login').hidden=true;$('logout').hidden=false;render(s);await waterfall(generation);
  }catch(e){if(generation!==session)return;connected=false;setConnection('offline');enableControls();error(e);}
  finally{polling=false;}
}
function connectStation(){
  if($('connect').disabled)return;
  $('key').value=$('key').value.trim();
  if(!$('key').reportValidity())return;
  key=$('key').value;$('error').hidden=true;poll();
}
$('connect').addEventListener('click',connectStation);
$('key').addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();connectStation();}});
$('logout').addEventListener('click',()=>{session++;key='';$('key').value='';connected=false;lastState=null;$('station').hidden=true;$('login').hidden=false;$('logout').hidden=true;$('error').hidden=true;setConnection('disconnected');if(imageUrl)URL.revokeObjectURL(imageUrl);imageUrl='';enableControls();});
$('language').addEventListener('change',()=>{WebI18n.setLanguage($('language').value);applyLanguage();});
applyLanguage();
$('openWaterfall').addEventListener('click',()=>send('waterfall.show',null));
$('modes').addEventListener('change',()=>send($('modes').value,null));
$('saveLog').addEventListener('click',()=>send('log.accept',null));$('cancelLog').addEventListener('click',()=>send('log.reject',null));
$('showWaterfall').addEventListener('change',()=>{$('waterfall').hidden=!$('showWaterfall').checked;});
document.addEventListener('visibilitychange',()=>{if(!document.hidden)poll();});
setInterval(()=>{if(connected&&Date.now()-lastGood>6000){connected=false;setConnection('stale');enableControls();}poll();},1000);
$('startupNotice').hidden=true;
$('connect').disabled=false;
