// Optional end-to-end checks: PLAYWRIGHT_MODULE points to an installed playwright package.
// node tests/webcontrol/browser_checks.cjs <webcontrol_checks.exe> <artifact-directory>
const {chromium} = require(process.env.PLAYWRIGHT_MODULE || 'playwright');
const {spawn} = require('node:child_process');
const {mkdir} = require('node:fs/promises');
const path = require('node:path');
const assert = require('node:assert/strict');

(async () => {
  const fixture = spawn(path.resolve(process.argv[2]), ['--serve'], {env:{...process.env,QT_QPA_PLATFORM:'offscreen'},windowsHide:true});
  let browser;
  try {
    const server = await new Promise((resolve,reject)=>{
      let data='';const timer=setTimeout(()=>reject(new Error('Fixture did not start')),10000);
      fixture.stdout.on('data',chunk=>{data+=chunk;const line=data.split('\n').find(line=>line.startsWith('{'));if(line){clearTimeout(timer);resolve(JSON.parse(line));}});
      fixture.on('error',reject);fixture.on('exit',code=>{clearTimeout(timer);reject(new Error(`Fixture exited: ${code}`));});
    });
    browser=await chromium.launch({channel:process.env.BROWSER_CHANNEL||'msedge',headless:true});
    const context=await browser.newContext({viewport:{width:1440,height:1000},locale:'de-AT'});
    const page=await context.newPage();const errors=[];
    page.on('pageerror',error=>errors.push(error.message));
    const origin=`http://127.0.0.1:${server.port}`;
    await page.goto(`${origin}/?`);
    const loginUrl=page.url();
    assert(await page.locator('#connect').isEnabled());
    assert(await page.locator('#startupNotice').isHidden());
    assert(await page.locator('#login').isVisible());
    assert.equal(await page.locator('html').getAttribute('lang'),'de');
    assert.equal(await page.getByRole('heading',{level:1}).textContent(),'Mit der Station verbinden');
    let commands=0;page.on('request',request=>{if(request.method()==='POST'&&request.url().endsWith('/api/command'))commands++;});
    await page.getByLabel('Sprache',{exact:true}).selectOption('en');
    assert.equal(await page.locator('html').getAttribute('lang'),'en');
    assert.equal(await page.getByRole('heading',{level:1}).textContent(),'Connect to the station');
    await page.reload();
    assert.equal(await page.locator('html').getAttribute('lang'),'en','Manual language did not survive reload');
    await page.getByLabel('Language',{exact:true}).selectOption('de');
    // Authentication errors use the same selected language as the login form.
    await page.locator('#key').fill('wrong');await page.locator('#login button').click();
    await page.waitForFunction(()=>document.querySelector('#error').textContent==='Zugangsschlüssel erforderlich');
    assert.equal(page.url(),loginUrl,'Rejected login navigated to a raw API response');
    await page.locator('#key').fill(server.key);
    await page.locator('#key').press('Enter');
    await page.locator('#station').waitFor({state:'visible'});
    assert.equal(page.url(),loginUrl,'Successful login navigated away from the web controls');
    assert.equal(await page.locator('#radioStatus').textContent(),'EMPFANG');
    const beforeLanguage=commands;
    await page.locator('#language').selectOption('en');
    assert.equal(commands,beforeLanguage,'Changing language sent a station command');
    assert.equal(await page.locator('#radioStatus').textContent(),'RECEIVING');
    assert.equal(await page.getByLabel('Transmit message 1',{exact:true}).inputValue(),'CQ TEST AA00');
    assert.equal(await page.getByLabel('CQ response',{exact:true}).locator('option').nth(1).textContent(),'CQ: First');
    const colors=await page.locator('#decodedTextBrowser').evaluate(list=>{
      const rows=list.querySelectorAll('.decode-row'),call=[...rows[0].querySelectorAll('span')].find(span=>span.textContent==='TEST');
      const style=getComputedStyle(call);return {row:getComputedStyle(rows[0]).backgroundColor,palette:getComputedStyle(list).backgroundColor,
        foreground:getComputedStyle(rows[1]).color,plain:getComputedStyle(rows[1]).backgroundColor,
        call:style.color,background:style.backgroundColor,bold:style.fontWeight,decoration:style.textDecorationLine};
    });
    assert.deepEqual(colors,{row:'rgb(53, 122, 197)',palette:'rgb(24, 37, 50)',foreground:'rgb(224, 225, 226)',plain:'rgba(0, 0, 0, 0)',call:'rgb(85, 17, 68)',background:'rgb(18, 202, 254)',bold:'700',decoration:'underline'});
    assert.deepEqual(await page.locator('.decode-row').first().locator('[title="PSK Reporter test marker"]').evaluate(marker=>({text:marker.textContent,color:getComputedStyle(marker).backgroundColor})),{text:'AA00',color:'rgb(127, 229, 223)'});
    assert.equal(await page.locator('.decode-row img').count(),0,'Decoded text was interpreted as HTML');
    assert((await page.locator('.decode-row').nth(1).textContent()).startsWith('<img src=x'));
    await page.locator('#language').selectOption('de');
    assert.equal(await page.getByLabel('Sendetext 1',{exact:true}).inputValue(),'CQ TEST AA00');
    assert.equal(await page.getByLabel('CQ-Antwort',{exact:true}).locator('option').nth(1).textContent(),'CQ: Erste Station');
    await page.getByRole('button',{name:'Monitor',exact:true}).click();
    await page.waitForFunction(()=>document.querySelector('[aria-label="Monitor"]').getAttribute('aria-pressed')==='true');
    await page.getByLabel('RX · Hz',{exact:true}).fill('1800');
    await page.getByLabel('RX · Hz',{exact:true}).press('Tab');
    await page.waitForTimeout(1200);
    const state=await (await context.request.get(`${origin}/api/state`,{headers:{Authorization:`Bearer ${server.key}`}})).json();
    assert.equal(state.controls.RxFreqSpinBox.value,1800);
    await page.getByRole('button',{name:'QSO protokollieren',exact:true}).first().click();
    await page.locator('#log').waitFor({state:'visible'});
    await page.getByLabel('Rufzeichen',{exact:true}).fill('TEST1');
    await page.getByLabel('Rufzeichen',{exact:true}).press('Tab');
    await page.waitForTimeout(300);
    const beforeLogLanguage=commands;
    await page.locator('#language').selectOption('en');
    assert.equal(await page.getByLabel('Callsign',{exact:true}).inputValue(),'TEST1');
    assert.equal(commands,beforeLogLanguage,'Language change altered the active QSO');
    await page.locator('#language').selectOption('de');
    await page.getByRole('button',{name:'QSO speichern',exact:true}).click();
    await page.locator('#log').waitFor({state:'hidden'});
    await page.locator('.decode-row').first().dblclick();
    await page.waitForTimeout(300);
    assert(await page.locator('#error').isHidden());
    const artifacts=path.resolve(process.argv[3]||'.local/webcontrol-browser');await mkdir(artifacts,{recursive:true});
    await page.screenshot({path:path.join(artifacts,'desktop.png'),fullPage:true});
    await page.setViewportSize({width:390,height:844});await page.screenshot({path:path.join(artifacts,'mobile.png'),fullPage:true});
    assert(await page.evaluate(()=>document.documentElement.scrollWidth<=window.innerWidth),'Mobile horizontal overflow');
    await page.locator('#language').selectOption('en');
    assert(await page.evaluate(()=>document.documentElement.scrollWidth<=window.innerWidth),'English mobile horizontal overflow');
    await context.setOffline(true);
    await page.waitForFunction(()=>document.querySelector('#connection').className==='offline');
    assert(await page.getByRole('button',{name:'Monitor',exact:true}).isDisabled());
    assert.equal(await page.locator('#connection').textContent(),'Connection lost');
    await page.locator('#language').selectOption('de');
    assert.equal(await page.locator('#connection').textContent(),'Verbindung unterbrochen');
    await context.setOffline(false);
    await page.waitForFunction(()=>document.querySelector('#connection').className==='online');
    await page.getByRole('button',{name:'Trennen',exact:true}).click();
    assert(await page.locator('#login').isVisible());assert.equal(await page.locator('#key').inputValue(),'');
    assert.equal(await page.evaluate(()=>localStorage.getItem('wsjtx.web.language')),'de');
    assert.equal(await page.evaluate(key=>Object.values(localStorage).some(value=>value.includes(key)),server.key),false,'Access key was persisted');
    for(const [locale,expected,denyStorage] of [['en-GB','en',false],['fr-FR','en',false],['de-DE','de',true]]){
      const fresh=await browser.newContext({locale});
      if(denyStorage)await fresh.addInitScript(()=>Object.defineProperty(window,'localStorage',{get(){throw new DOMException('Storage blocked','SecurityError');}}));
      const login=await fresh.newPage();await login.goto(origin);
      assert.equal(await login.locator('html').getAttribute('lang'),expected);
      await login.locator('#language').selectOption('de');
      assert.equal(await login.getByRole('heading',{level:1}).textContent(),'Mit der Station verbinden');
      await fresh.close();
    }
    // A failed script load must never fall back to native GET form submission (/?)
    // or show raw authentication JSON after clicking Connect or pressing Enter.
    for(const asset of ['app.js','i18n.js']){
      const broken=await browser.newContext();const login=await broken.newPage();
      await login.route(`**/${asset}`,route=>route.abort());await login.goto(origin);
      assert(await login.locator('#connect').isDisabled());
      assert(await login.locator('#startupNotice').isVisible());
      await login.locator('#key').fill('test-key');await login.locator('#key').press('Enter');
      assert.equal(new URL(login.url()).search,'','Script failure submitted a native login form');
      assert(await login.locator('#login').isVisible());await broken.close();
    }
    assert.deepEqual(errors,[]);
    console.log('Browser checks passed: DE/EN, browser-language fallback, persistence/blocked storage, translated errors, language changes without commands, QSO preservation, Qt colors/emphasis/palette, literal decode text, login, monitor, frequency, logging, decode, desktop/mobile, disconnect/reconnect, logout');
  } finally {if(browser)await browser.close();fixture.kill();}
})().catch(error=>{console.error(error);process.exitCode=1;});
