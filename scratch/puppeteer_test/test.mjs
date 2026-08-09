import puppeteer from 'puppeteer';

(async () => {
  const browser = await puppeteer.launch({ 
    args: ['--enable-unsafe-webgpu', '--no-sandbox'] 
  });
  const page = await browser.newPage();
  
  page.on('console', msg => console.log('LOG:', msg.text()));
  page.on('pageerror', err => console.log('ERROR:', err.toString()));
  
  // Also expose a function to capture JS errors
  await page.evaluateOnNewDocument(() => {
    window.addEventListener('unhandledrejection', event => {
      console.log('UNHANDLED REJECTION:', event.reason);
    });
    window.addEventListener('error', event => {
      console.log('WINDOW ERROR:', event.message);
    });
  });

  try {
    await page.goto('http://localhost:8000');
    await new Promise(r => setTimeout(r, 4000));
  } catch(e) {
    console.error("Navigation error:", e);
  }
  
  await browser.close();
})();
