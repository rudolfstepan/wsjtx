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
    const context=await browser.newContext({viewport:{width:1440,height:1000}});
    const page=await context.newPage();const errors=[];
    page.on('pageerror',error=>errors.push(error.message));
    const origin=`http://127.0.0.1:${server.port}`;
    await page.goto(origin);
    assert(await page.locator('#login').isVisible());
    await page.locator('#key').fill(server.key);
    await page.locator('#login button').click();
    await page.locator('#station').waitFor({state:'visible'});
    await page.getByRole('button',{name:'Monitor',exact:true}).click();
    await page.waitForFunction(()=>document.querySelector('[aria-label="Monitor"]').getAttribute('aria-pressed')==='true');
    await page.getByLabel('RX · Hz',{exact:true}).fill('1800');
    await page.getByLabel('RX · Hz',{exact:true}).press('Tab');
    await page.waitForTimeout(1200);
    const state=await (await context.request.get(`${origin}/api/state`,{headers:{Authorization:`Bearer ${server.key}`}})).json();
    assert.equal(state.controls.RxFreqSpinBox.value,1800);
    await page.getByRole('button',{name:'Log QSO',exact:true}).click();
    await page.locator('#log').waitFor({state:'visible'});
    await page.getByLabel('Rufzeichen',{exact:true}).fill('TEST1');
    await page.getByLabel('Rufzeichen',{exact:true}).press('Tab');
    await page.waitForTimeout(300);
    await page.getByRole('button',{name:'QSO speichern',exact:true}).click();
    await page.locator('#log').waitFor({state:'hidden'});
    await page.locator('.decode-row').first().dblclick();
    await page.waitForTimeout(300);
    assert(await page.locator('#error').isHidden());
    const artifacts=path.resolve(process.argv[3]||'.local/webcontrol-browser');await mkdir(artifacts,{recursive:true});
    await page.screenshot({path:path.join(artifacts,'desktop.png'),fullPage:true});
    await page.setViewportSize({width:390,height:844});await page.screenshot({path:path.join(artifacts,'mobile.png'),fullPage:true});
    assert(await page.evaluate(()=>document.documentElement.scrollWidth<=window.innerWidth),'Mobile horizontal overflow');
    await context.setOffline(true);
    await page.waitForFunction(()=>document.querySelector('#connection').className==='offline');
    assert(await page.getByRole('button',{name:'Monitor',exact:true}).isDisabled());
    await context.setOffline(false);
    await page.waitForFunction(()=>document.querySelector('#connection').className==='online');
    await page.getByRole('button',{name:'Trennen',exact:true}).click();
    assert(await page.locator('#login').isVisible());assert.equal(await page.locator('#key').inputValue(),'');
    assert.deepEqual(errors,[]);
    console.log('Browser checks passed: login, monitor, frequency, logging, decode, desktop/mobile, disconnect/reconnect, logout');
  } finally {if(browser)await browser.close();fixture.kill();}
})().catch(error=>{console.error(error);process.exitCode=1;});
