#pragma once

#include <Arduino.h>

static const char SetupPortalPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Biztola Smart Fountain Setup</title>
<style>
body{font-family:Arial,sans-serif;background:#f3f4f6;margin:0;padding:20px;color:#111827}
.card{max-width:460px;margin:24px auto;background:white;padding:24px;border-radius:14px;box-shadow:0 4px 18px rgba(0,0,0,.08)}
h1{font-size:24px;margin:0 0 8px}p{line-height:1.45;color:#4b5563}
label{display:block;font-weight:700;margin-top:16px}
select,input,button{width:100%;box-sizing:border-box;font-size:16px;border-radius:9px}
select,input{padding:12px;margin-top:6px;border:1px solid #d1d5db}
button{padding:12px;margin-top:20px;border:0;background:#2563eb;color:white;font-weight:700}
button:disabled{background:#9ca3af}.secondary{background:#e5e7eb;color:#111827;margin-top:10px}
.password-row{display:flex;gap:8px}.password-row input{flex:1}.password-row button{width:auto;margin-top:6px}
.message{display:none;padding:12px;border-radius:9px;margin:14px 0}.info{background:#dbeafe;color:#1e3a8a}.error{background:#fee2e2;color:#991b1b}.success{background:#dcfce7;color:#166534}
.small{font-size:13px}.hidden{display:none}
</style>
</head>
<body>
<div class="card">
<h1>Biztola Smart Fountain Setup</h1>
<p>Choose your Wi-Fi network or enter a hidden SSID manually.</p>
<div id="message" class="message"></div>
<form id="wifi-form">
<label for="ssid">Wi-Fi network</label>
<select id="ssid"><option value="">Loading nearby networks...</option></select>
<button class="secondary" id="refresh" type="button">Refresh network list</button>
<label for="manual-ssid">Manual SSID / hidden network</label>
<input id="manual-ssid" maxlength="32" autocomplete="off">
<label for="password">Wi-Fi password</label>
<div class="password-row">
<input id="password" type="password" maxlength="63" autocomplete="new-password">
<button class="secondary" id="toggle-password" type="button">Show</button>
</div>
<button id="submit" type="submit">Test, Save and Restart</button>
</form>
<p class="small">The fountain's local controls and water safety continue working during setup.</p>
<p class="small">Setup hotspot: Fountain-Setup<br>Setup password: fountain123<br>Setup page: http://192.168.4.1</p>
</div>
<script>
const form=document.getElementById('wifi-form');
const ssidSelect=document.getElementById('ssid');
const manualSsid=document.getElementById('manual-ssid');
const password=document.getElementById('password');
const submit=document.getElementById('submit');
const message=document.getElementById('message');
const refresh=document.getElementById('refresh');
const toggle=document.getElementById('toggle-password');
function show(text,type){message.textContent=text;message.className='message '+type;message.style.display='block'}
async function loadNetworks(force=false){
  ssidSelect.innerHTML='<option value="">Scanning nearby networks...</option>';
  try{
    const response=await fetch('/networks'+(force?'?refresh=1':''));
    const data=await response.json();
    if(response.status===202||data.status==='scanning'){
      setTimeout(()=>loadNetworks(false),900);return;
    }
    ssidSelect.innerHTML='<option value="">Select a network or use manual SSID</option>';
    (data.networks||[]).forEach(network=>{
      const option=document.createElement('option');
      option.value=network.ssid;
      option.textContent=network.ssid+' ('+network.rssi+' dBm)';
      ssidSelect.appendChild(option);
    });
    if(!(data.networks||[]).length){show('No networks found. Enter the SSID manually.','info')}
  }catch(error){show('Network scan unavailable. Enter the SSID manually.','info')}
}
async function pollStatus(){
  try{
    const response=await fetch('/setup-status');
    const data=await response.json();
    if(data.status==='connecting'){show('Testing Wi-Fi credentials...','info');setTimeout(pollStatus,900);return}
    if(data.status==='success'){show(data.message||'Wi-Fi saved. Restarting...','success');form.classList.add('hidden');return}
    if(data.status==='failed'){show(data.message||'Connection failed. Check the password and try again.','error');submit.disabled=false;password.focus();return}
    setTimeout(pollStatus,900);
  }catch(error){setTimeout(pollStatus,1200)}
}
form.addEventListener('submit',async event=>{
  event.preventDefault();
  const selected=ssidSelect.value.trim();
  const manual=manualSsid.value.trim();
  const ssid=manual||selected;
  if(!ssid){show('Select a network or enter the SSID manually.','error');return}
  submit.disabled=true;show('Starting Wi-Fi test...','info');
  const body=new URLSearchParams({ssid:ssid,password:password.value});
  try{
    const response=await fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body.toString()});
    const data=await response.json();
    if(!response.ok){show(data.message||'Could not start Wi-Fi test.','error');submit.disabled=false;return}
    pollStatus();
  }catch(error){show('Could not contact the setup portal.','error');submit.disabled=false}
});
refresh.addEventListener('click',()=>loadNetworks(true));
toggle.addEventListener('click',()=>{const visible=password.type==='text';password.type=visible?'password':'text';toggle.textContent=visible?'Show':'Hide'});
window.addEventListener('load',()=>setTimeout(()=>loadNetworks(false),250));
</script>
</body>
</html>
)rawliteral";
