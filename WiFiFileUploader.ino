#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <LittleFS.h>

#define APSSID "ESPboy"
#define APHOST "espboy"
#define APPSK  "87654321"

const char *ssid = APSSID;
const char *password = APPSK;
const char *host = APHOST;

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
File fsUploadFile;

const char index_html[] PROGMEM = R"=====(
<!DOCTYPE html><html lang="en"><head><meta name="viewport"content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no"><title>SD Editor</title><style type="text/css"media="screen">#i
{background:#33691e;border-radius:5px;bottom:1em;box-shadow:0 1px 6px 0 rgba(0,0,0,0.44),0 3px 8px 0 rgba(0,0,0,0.68);color:white;font-size:2em;left:-150%;margin-left:2%;opacity:0.9;padding:0.5em;position:fixed;text-align:center;transition:0.2s ease-out;webkit-transition:0.3s ease-out;width:90%;}#loader
{background:rgba(0,0,0,0.4);display:none;height:100%;left:0;position:fixed;top:0;width:100%;}#p-bar
{color:black;height:2em;line-height:2em;position:relative;text-align:center;width:200px;}#p-bar span
{background-color:rgba(200,200,200,0.6);left:0;position:absolute;top:0;width:100%;}#p-bar span:last-child
{background-color:rgba(0,200,0,0.6);clip:rect(0 0px 2em 0);color:white;}#uploader
{background-color:#00A;background-image:linear-gradient(to right,#0060b3 50%,#999 100%);color:#FFF;padding-left:10px;}.close
{height:1em;opacity:0.7;position:absolute;right:1em;width:1em;}.close:after
{transform:rotate(-45deg);}.close:before
{transform:rotate(45deg);}.close:before,.close:after
{background-color:#333;content:' ';height:33px;left:15px;position:absolute;width:2px;}.close:hover
{opacity:1;}.container
{background:#fff;margin:5rem auto;max-width:40rem;position:relative;width:100%;}.container::before
{bottom:0;box-shadow:0 8px 10px 1px rgba(0,0,0,0.14),0 3px 14px 2px rgba(0,0,0,0.12),0 5px 5px-3px rgba(0,0,0,0.2);content:'';left:0;position:absolute;right:0;top:0;}.m-l
{animation-delay:-0.6s;font-size:0.1rem;margin:6rem auto 8.5rem auto;position:relative;transform:translateZ(0);}.m-l:after
{left:4rem;}.m-l:before
{animation-delay:-1.2s;left:-4rem;}.m-l:before,.m-l:after
{content:'';position:absolute;top:0;}.m-l:before,.m-l:after,.m-l
{animation:m-l 1.8s infinite ease-in-out;animation-fill-mode:both;border-radius:40%;height:2.5rem;width:2.5rem;}.sz
{font-size:70%;margin:0.4em;position:absolute;right:5em;}@keyframes fade
{0%{opacity:1;transform:scale(0);}
100%{opacity:0;}
50%{opacity:1;transform:scale(1);}}@keyframes m-l
{0%,80%,100%{box-shadow:0 2.5rem 0-1.3em white;}
40%{box-shadow:0 2.5rem 0 0 white;}}
body{background-color:#f2f2f2;}
button{background-color:transparent;border:1px solid white;border-radius:2px;color:white;cursor:pointer;display:inline-block;font-family:sans-serif;font-size:12px;letter-spacing:1px;margin:5px;overflow:hidden;padding:0.6em 1.2em;position:relative;text-align:center;transition:0.2s ease-out;webkit-transition:0.2s ease-out;}
button:active{box-shadow:0 1px 3px 0 rgba(0,0,0,0.44),0 1px 1px 0 rgba(0,0,0,0.68);}
button:focus{outline:none;}
button:focus::before{animation-duration:1s;animation-name:fade;animation-timing-function:ease-out;background:#AAA;border-radius:1000px;content:"";display:block;height:150px;left:-30px;opacity:0;position:absolute;top:-58px;width:150px;z-index:-2;}
button:hover{background-color:rgba(255,255,255,0.6);box-shadow:0 3px 6px 0 rgba(0,0,0,0.22),0 4px 4px 0 rgba(0,0,0,0.35);color:#222;text-decoration:none;}
h1{font-size:2rem;margin:0 0 2em;text-align:center;}
input{border:none;border-radius:2px;padding:0.55em;width:9em;}
li{cursor:pointer;border-bottom:1px solid rgba(0,0,0,.12);display:block;font-size:1.5rem;height:40px;margin:5px;padding-left:16px;padding-top:16px;text-align:left;}
ul{list-style-type:decimal;}</style><script>var treeRoot;function createFileUploader(b){var d;var f=document.getElementById('u_h');var c=document.getElementById('b_u');function h(){if(d.readyState==4){document.getElementById('loader').style.display='none';if(d.status!=200){alert('ERROR['+d.status+']: '+d.responseText)}else{setTimeout(function(){ok();httpGet(treeRoot,'/');},300);}}}
c.onclick=function(k){if(f.files.length===0){return}
document.getElementById('loader').style.display='block';d=new XMLHttpRequest();d.onreadystatechange=h;var j=new FormData();j.append('data',f.files[0],f.value.replace(/^.*[\\\/]/,''));d.open('POST','/e');d.send(j);};}
function del(n){if(confirm("Delite "+n+"?")){xmlHttp=new XMLHttpRequest();xmlHttp.onreadystatechange=setTimeout(function(){ok();httpGet(treeRoot,'/');},300);var formData=new FormData();formData.append("path",'/'+n);xmlHttp.open("DELETE","/e");xmlHttp.send(formData);}}
function addList(d,g,a){var c='<ul>';viewSize(a[0]);for(var b=1;b<a.length;b++){var f=a[b].split(':');if(f[0].length>0);c+='<li>'+f[0]+'<b class=\u0022sz\u0022>'+(Math.floor(f[1]/102.4)/10)+'KB</b><i class=\u0022close\u0022 onClick=\u0022del(\u0027'+f[0]+'\u0027)\u0022></i></li>';}
c+='</ul>';d.innerHTML=c}
function getCb(a,b){return function(){if(xmlHttp.readyState==4){if(xmlHttp.status==200){addList(a,b,xmlHttp.responseText.split('/'));}}}}
function httpGet(a,b){if(b!='/'){a.onclick=function(){}}
xmlHttp=new XMLHttpRequest(a,b);xmlHttp.onreadystatechange=getCb(a,b);xmlHttp.open('GET','/l',true);xmlHttp.send(null)}
function onBodyLoad(){createFileUploader('uploader');treeRoot=document.getElementById('tree');httpGet(treeRoot,'/');};function ok(){var i=document.getElementById('i');i.style.left="0";setTimeout(function(){document.getElementById('i').style.left="-150%";},3000);}
function viewSize(s){var s=s.split(':');var p=Math.floor(s[0]/s[1]*100);document.getElementById('pr1').innerHTML=Math.floor(s[0]/1024)+'KB/'+Math.floor(s[1]/1024)+'KB';document.getElementById('pr2').innerHTML=Math.floor(s[0]/1024)+'KB/'+Math.floor(s[1]/1024)+'KB';document.getElementById('pr2').style.clip="rect(0 "+p*2+"px 2em 0)";}
function luf(){var nw=window.open('/update','Update firmware','width=380,height=200,left=200,top=200');nw.onload=function(){var div=nw.document.createElement('div');var b=nw.document.body;div.innerHTML='Select a file with the .bin extension to upgrade the firmware.'
b.insertBefore(div,b.firstChild);}}</script></head><body onload='onBodyLoad()'><div class='container'><div id='uploader'><input type="file"id="u_h"
style="position: absolute; display: block; overflow: hidden; width: 0; height: 0; border: 0; padding: 0;"
onchange="document.getElementById('u_v').value = this.value.replace(/^.*[\\\/]/, '');"/><input type="text"readonly="1"id="u_v"
onclick="document.getElementById('u_h').click();"/><button onclick="document.getElementById('u_h').click();">Browse</button><button id="b_u">Upload</button><button id="p-bar"><span id="pr1">0/0</span><span id="pr2">0/0</span></button><button onclick="luf()">Update</button></div><div id='tree'style='top:1em'></div></div><div id="i">The operation was successful</div><div id="loader"><div class="m-l"></div></div></body></html>
)=====";

void startServer(){
  WiFi.forceSleepWake();
  serverSetup();

  myESPboy.tft.fillScreen(0x0000);
  myESPboy.tft.setTextSize(1);
  myESPboy.tft.setTextColor(0xffff);
  myESPboy.tft.setCursor(0,10);
  myESPboy.tft.print(F("\n\n SSID "));
  myESPboy.tft.print(F(APSSID));
  myESPboy.tft.print(F("\n Password "));
  myESPboy.tft.print(F(APPSK));
  myESPboy.tft.print(F("\n\n Go to \n http://192.168.4.1"));
  myESPboy.tft.print(F("\n in a web browser"));
  myESPboy.tft.print(F("\n\n Press any button to\n reboot"));

  Serial.print(F("FreeHeap:"));
  Serial.println(ESP.getFreeHeap());
  while(1){
    serverLoop();
    check_key();
    if(pad_state_t) ESP.reset();
    delay(100);
  }
}

void handleFileList() {
  Dir dir = LittleFS.openDir("/");
  FSInfo fs_info;
  String output;
  LittleFS.info(fs_info);
  Serial.print(fs_info.usedBytes);
  Serial.print('/');
  Serial.println(fs_info.totalBytes);
  output += fs_info.usedBytes;
  output += ':';
  output += fs_info.totalBytes;
  while (dir.next()) {
    Serial.print(dir.fileName());
    Serial.print(' ');
    Serial.println(dir.fileSize());
    if (dir.fileName()[0] != '/')
      output += '/';
    output += String(dir.fileName());
    output += ':';
    output += String(dir.fileSize());
  }
  
  server.send(200, "text/json", output);
  output = String();
  Serial.print(F("Free "));
  Serial.println(ESP.getFreeHeap());
}

void handleFileUpload() {
  if (server.uri() != "/e") {
    return;
  }
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    Serial.print(F("handleFileUpload Name: ")); Serial.println(filename);
    fsUploadFile = LittleFS.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
    }
    Serial.print(F("handleFileUpload Size: ")); Serial.println(upload.totalSize);
  }
}

const char send_bad_args[] PROGMEM = "BAD ARGS";
const char send_bad_path[] PROGMEM = "BAD PATH";

void handleFileDelete() {
  const char *txtplain = "text/plain";
  if (server.args() == 0) {
    return server.send_P(500, txtplain, send_bad_args);
  }
  String path = server.arg(0);
  Serial.print(F("handleFileDelete: ")); Serial.println(path);
  if (path == "/") {
    return server.send_P(500, txtplain, send_bad_path);
  }
  if (!LittleFS.exists(path)) {
    return server.send(404, txtplain, "404");
  }
  LittleFS.remove(path);
  server.send(200, txtplain, "");
  path = String();
}

void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void serverSetup() {
  Serial.println();
  Serial.print(F("Configuring access point..."));
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print(F("AP IP address: "));
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/l", HTTP_GET, handleFileList);
  server.on("/e", HTTP_DELETE, handleFileDelete);
  server.on("/e", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);
  MDNS.begin(host);
  httpUpdater.setup(&server);
  server.begin();
  MDNS.addService("http", "tcp", 80);
  delay(50);
  Serial.println(F("HTTP server started"));
}

void serverLoop() {
  server.handleClient();
}
