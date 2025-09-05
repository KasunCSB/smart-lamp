/*
 * Smart Lamp Controller - RECOMMENDED VERSION
 * 
 * WiFi-enabled lamp controller with web interface and timer functionality.
 * This version uses standard Arduino functions and is perfect for beginners.
 * 
 * Features:
 * - Web interface for lamp control
 * - Timer function (1 minute to 12 hours)
 * - Mobile-friendly responsive design
 * - Real-time countdown display
 * 
 * Hardware: ESP8266 + 5V Relay Module
 * 
 * Setup Instructions:
 * 1. Change WiFi credentials below
 * 2. Connect relay: VIN→VCC, GND→GND, D1→IN
 * 3. Upload code and access via web browser
 * 
 * ⚠️  AC voltage is dangerous - get professional help if unsure!
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// WiFi network credentials
const char* ssid = "REPLACE_WITH_YOUR_WIFI_SSID";
const char* password = "REPLACE_WITH_YOUR_WIFI_PASSWORD";

ESP8266WebServer server(80);

const int relayPin = 5; // GPIO5 (D1 on NodeMCU)
bool relayState = false; // Legacy variable for backward compatibility

// Timer functionality
unsigned long currentTimeout = 0;  // Timeout duration in minutes
bool timeoutActive = false;
unsigned long timeoutStart = 0;

// Automatic reboot for system stability
const unsigned long SAFETY_REBOOT_INTERVAL = 12UL * 60UL * 60UL * 1000UL; // 12 hours
unsigned long moduleStartTime = 0;

// Check if lamp is on (relay is active-low)
inline bool isLampOn() {
  return digitalRead(relayPin) == LOW;
}

// Control the lamp relay
inline void setLamp(bool on) {
  digitalWrite(relayPin, on ? LOW : HIGH);
}

// Build JSON status response
String buildStatusJson() {
  bool on = isLampOn();
  unsigned long remainingSeconds = 0;
  if (timeoutActive && on) {
    unsigned long elapsed = millis() - timeoutStart;
    long remainingMs = (long)(currentTimeout * 60000UL) - (long)elapsed;
    if (remainingMs < 0) remainingMs = 0;
    remainingSeconds = (unsigned long)(remainingMs / 1000);
  }
  String json = String("{") +
                "\"on\":" + (on ? "true" : "false") + "," +
                "\"timeoutActive\":" + (timeoutActive ? "true" : "false") + "," +
                "\"remainingSeconds\":" + String(remainingSeconds) +
                "}";
  return json;
}

void setup() {
  // Start serial communication for debugging
  Serial.begin(9600);
  delay(1000); // Brief delay for stability

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Connect to WiFi network
  WiFi.begin(ssid, password);

  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Setup relay pin and turn lamp off initially
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // Turn off (relay is active-low)
  
  // Record startup time for safety reboot feature
  moduleStartTime = millis();
  
  // Configure web server routes
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/timeout", handleTimeout);
  server.on("/status", handleStatus);
  // API endpoints for AJAX requests (no page reload)
  server.on("/api/on", handleOnApi);
  server.on("/api/off", handleOffApi);
  
  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();
  
  // Check if timer has expired
  bool lampOn = isLampOn();
  if (timeoutActive && lampOn && (millis() - timeoutStart) >= (currentTimeout * 60000UL)) {
    setLamp(false);
    relayState = false; // Keep legacy variable in sync
    timeoutActive = false;
    Serial.println("Lamp automatically turned OFF by timer");
  }
  
  // Cancel timer if lamp was manually turned off
  if (timeoutActive && !isLampOn()) {
    timeoutActive = false;
    Serial.println("Timer cancelled - lamp manually turned off");
  }
  
  // Reboot if module has been running 12 hours and lamp is off
  if (!isLampOn() && (millis() - moduleStartTime) >= SAFETY_REBOOT_INTERVAL) {
    Serial.println("Safety reboot: Module has been on for 12 hours with lamp off. Rebooting...");
    delay(1000); // Allow message to be sent
    ESP.restart();
  }
  
  // Essential for ESP8266 WiFi stability
  yield();
}

void handleRoot() {
  // Prevent page caching for real-time updates
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  
  bool lampOn = isLampOn();
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'>";
  html += "<meta charset='UTF-8'><title>Smart Lamp Controller</title>";
  html += "<link rel='icon' href='data:image/svg+xml,%3Csvg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 24 24%22%3E%3Ccircle cx=%2212%22 cy=%2210%22 r=%226%22 fill=%22%23FFD700%22 stroke=%22%23FFA500%22 stroke-width=%221%22/%3E%3Crect x=%2210%22 y=%2216%22 width=%224%22 height=%224%22 fill=%22%23888%22 rx=%221%22/%3E%3C/svg%3E'>";
  html += "<meta name='theme-color' content='#4CAF50'>";
  html += "<style>";
  html += "*{margin:0;padding:0;box-sizing:border-box;}";
  html += "body{font-family:'Segoe UI',sans-serif;background:#fff;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px;}";
  html += ".app{width:100%;max-width:350px;background:#fff;border-radius:30px;padding:40px 30px;box-shadow:0 20px 60px rgba(0,0,0,0.1);text-align:center;border:1px solid #f0f0f0;position:relative;}";
  html += ".title{font-size:24px;font-weight:400;color:#333;margin-bottom:40px;letter-spacing:1px;}";
  html += ".power-section{margin-bottom:40px;}";
  html += ".power-btn{width:140px;height:140px;border-radius:50%;border:none;background:linear-gradient(135deg,#4CAF50,#45a049);color:#fff;font-size:18px;font-weight:600;cursor:pointer;transition:all 0.3s cubic-bezier(0.4,0,0.2,1);box-shadow:0 10px 30px rgba(76,175,80,0.4);margin:0 auto;display:flex;align-items:center;justify-content:center;}";
  html += ".power-btn.off{background:linear-gradient(135deg,#e0e0e0,#bdbdbd);color:#757575;box-shadow:0 10px 30px rgba(0,0,0,0.1);}";
  html += ".power-btn:hover{transform:translateY(-2px);box-shadow:0 15px 40px rgba(76,175,80,0.5);}";
  html += ".power-btn.off:hover{box-shadow:0 15px 40px rgba(0,0,0,0.15);}";
  html += ".power-btn:active{transform:scale(0.95);}";
  html += ".status{margin-top:20px;font-size:16px;color:#666;}";
  html += ".status-sub{margin-top:8px;font-size:14px;color:#999;}";
  html += ".timer-section{background:#f8f9fa;border-radius:20px;padding:25px;border:1px solid #e9ecef;}";
  html += ".section-title{font-size:18px;font-weight:400;color:#333;margin-bottom:20px;}";
  html += ".timer-input{width:100%;padding:15px 20px;border:2px solid #e9ecef;border-radius:15px;background:#fff;font-size:16px;text-align:center;margin-bottom:20px;}";
  html += ".timer-input:focus{outline:none;border-color:#2196F3;box-shadow:0 0 0 3px rgba(33,150,243,0.2);}";
  html += ".btn{padding:12px 25px;border:none;border-radius:15px;background:linear-gradient(135deg,#2196F3,#1976D2);color:#fff;font-size:14px;font-weight:500;cursor:pointer;transition:all 0.3s ease;margin:5px;}";
  html += ".btn:hover{transform:translateY(-1px);box-shadow:0 5px 15px rgba(33,150,243,0.4);}";
  html += ".btn-cancel{background:linear-gradient(135deg,#f44336,#d32f2f);}";
  html += ".btn-cancel:hover{box-shadow:0 5px 15px rgba(244,67,54,0.35);}";
  html += ".btn:active{transform:scale(0.98);}";
  html += ".quick-btns{display:flex;gap:10px;margin-top:15px;}";
  html += ".quick-btn{flex:1;padding:10px;background:#fff;border:1px solid #e9ecef;border-radius:12px;color:#666;font-size:12px;cursor:pointer;transition:all 0.3s ease;}";
  html += ".quick-btn:hover{background:#f8f9fa;transform:translateY(-1px);border-color:#2196F3;}";
  html += ".btn:disabled,.quick-btn:disabled{opacity:0.6;cursor:not-allowed;box-shadow:none;transform:none;}";
  html += ".timer-input:disabled{background:#f5f5f5;color:#aaa;}";
  html += "@media(max-width:400px){body{padding:10px;}.app{margin:0;padding:30px 25px;}.power-btn{width:120px;height:120px;font-size:16px;}}";
  html += "@media(max-height:600px){body{align-items:flex-start;padding-top:20px;}}";
  html += "</style></head><body>";
  
  html += "<div class='app'>";
  html += "<h1 class='title'>Smart Lamp</h1>";
  
  // Power Section
  html += "<div class='power-section'>";
  html += "<form id='powerForm' action='" + String(lampOn ? "/off" : "/on") + "'>";
  html += "<button id='powerBtn' type='button' onclick='togglePower()' class='power-btn " + String(lampOn ? "" : "off") + "'>";
  html += String(lampOn ? "ON" : "OFF");
  html += "</button></form>";
  html += "<div id='lampStatus' class='status'>" + String(lampOn ? "Lamp is ON" : "Lamp is OFF") + "</div>";

  // Calculate and display timer countdown
  unsigned long totalSec = 0;
  String timeDisplayInit = "00:00:00";
  if (timeoutActive) {
    unsigned long elapsed = millis() - timeoutStart;
    unsigned long remainingMs = (currentTimeout * 60000) - elapsed;
    if ((long)remainingMs < 0) remainingMs = 0; // Prevent negative values
    totalSec = remainingMs / 1000;
    unsigned long hours = totalSec / 3600;
    unsigned long minutes = (totalSec % 3600) / 60;
    unsigned long seconds = totalSec % 60;

    timeDisplayInit = "";
    if (hours < 10) timeDisplayInit += "0";
    timeDisplayInit += String(hours) + ":";
    if (minutes < 10) timeDisplayInit += "0";
    timeDisplayInit += String(minutes) + ":";
    if (seconds < 10) timeDisplayInit += "0";
    timeDisplayInit += String(seconds);
  }

  html += "<div class='status-sub' id='cdWrap' data-remaining='" + String(totalSec) + "' style='display:" + String(timeoutActive ? "block" : "none") + ";'>";
  html += "Auto off in <span id='cdText'>" + timeDisplayInit + "</span></div>";

  html += "</div>";
  
  // Timer Section
  html += "<div class='timer-section'>";
  html += "<div class='section-title'>Timer</div>";

  html += "<form id='timeoutForm' action='/timeout'>";
  html += "<input id='minutesInput' type='number' name='minutes' class='timer-input' value='0' min='0' max='720' placeholder='Minutes (0 to disable)'>";
  html += "<button id='cancelBtn' type='button' class='btn btn-cancel' style='display:" + String(timeoutActive ? "inline-block" : "none") + "' onclick='cancelTimer()'>Cancel Timer</button>";
  html += "<button id='setBtn' class='btn' style='display:" + String(timeoutActive ? "none" : "inline-block") + "'>Set Timer</button>";
  html += "</form>";
  html += "<div class='quick-btns'>";
  html += "<button id='q1' type='button' class='quick-btn' onclick='setQuick(1)'>1m</button>";
  html += "<button id='q5' type='button' class='quick-btn' onclick='setQuick(5)'>5m</button>";
  html += "<button id='q30' type='button' class='quick-btn' onclick='setQuick(30)'>30m</button>";
  html += "<button id='q60' type='button' class='quick-btn' onclick='setQuick(60)'>1hr</button>";
  html += "</div>";
  html += "</div>";
  
  html += "</div>";
  html += "<script>";
  html += "function setQuick(m){var inp=document.getElementById('minutesInput'); if(inp) inp.value=m; fetch('/timeout?minutes='+encodeURIComponent(m),{cache:'no-store'}).then(function(){return getStatus();}).catch(function(){});}";
  html += "function cancelTimer(){fetch('/timeout?minutes=0',{cache:'no-store'}).then(function(){return getStatus();}).catch(function(){});}";
  html += "(function(){var inp=document.getElementById('minutesInput');var btn=document.getElementById('setBtn'); var form=document.getElementById('timeoutForm'); if(!inp||!btn||!form) return; inp.addEventListener('input', function(){}); form.addEventListener('submit', function(e){ e.preventDefault(); if(inp.disabled) return; if(!inp.checkValidity()){ inp.reportValidity(); return;} var v=parseInt(inp.value)||0; fetch('/timeout?minutes='+encodeURIComponent(v),{cache:'no-store'}).then(function(){return getStatus();}).catch(function(){}); }); })();";
  html += "function pad(n){return n<10?'0'+n:n;}";
  html += "var wrap=document.getElementById('cdWrap');";
  html += "var remainingSeconds=wrap?parseInt(wrap.getAttribute('data-remaining')):0;";
  html += "function renderCountdown(){if(!wrap)return; if(remainingSeconds>0){var h=Math.floor(remainingSeconds/3600),m=Math.floor((remainingSeconds%3600)/60),s=remainingSeconds%60; var txt=document.getElementById('cdText'); if(txt){txt.textContent=pad(h)+':'+pad(m)+':'+pad(s);} wrap.style.display='block';}else{wrap.style.display='none';}}";
  html += "function applyStatus(s){try{window.__lastStatus=s; var on=!!s.on; var pf=document.getElementById('powerForm'); var pb=document.getElementById('powerBtn'); var st=document.getElementById('lampStatus'); var inp=document.getElementById('minutesInput'); var setBtn=document.getElementById('setBtn'); var cancelBtn=document.getElementById('cancelBtn'); var q1=document.getElementById('q1'), q5=document.getElementById('q5'), q30=document.getElementById('q30'), q60=document.getElementById('q60'); if(pf) pf.action = on?'/off':'/on'; if(pb){pb.className = 'power-btn ' + (on?'':'off'); pb.textContent = on?'ON':'OFF';} if(st) st.textContent = on?'Lamp is ON':'Lamp is OFF'; var tActive = !!s.timeoutActive && (s.remainingSeconds||0) > 0 && on; if(cancelBtn) cancelBtn.style.display = tActive ? 'inline-block' : 'none'; if(setBtn) setBtn.style.display = tActive ? 'none' : 'inline-block'; remainingSeconds = s.remainingSeconds||0; renderCountdown(); }catch(e){}}";
  html += "function togglePower(){var pb=document.getElementById('powerBtn'); if(!pb) return; pb.disabled=true; var on=(window.__lastStatus && window.__lastStatus.on) || !pb.className.includes('off'); var next= on?'/api/off':'/api/on'; fetch(next,{cache:'no-store'}).then(function(){return getStatus();}).catch(function(){}).finally(function(){pb.disabled=false;}); }";
  html += "function getStatus(){return fetch('/status',{cache:'no-store'}).then(function(r){return r.json()}).then(function(s){applyStatus(s); return s;});}";
  html += "renderCountdown();";
  html += "setInterval(function(){getStatus().catch(function(){})}, 1000);";
  html += "</script></body></html>";
  
  server.send(200, "text/html", html);
}

void handleOn() {
  setLamp(true);
  relayState = true;
  Serial.println("Lamp turned ON");
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleOff() {
  setLamp(false);
  relayState = false;
  // Cancel timer when turning lamp off manually
  timeoutActive = false;
  currentTimeout = 0;
  Serial.println("Lamp turned OFF");
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleTimeout() {
  if (server.hasArg("minutes")) {
  long minutes = server.arg("minutes").toInt();
  if (minutes < 0) minutes = 0;
  if (minutes > 720) minutes = 720; // Maximum 12 hours

    if (minutes > 0 && minutes <= 720) { // Max 12 hours
      // Turn lamp on if it's off and timer is being set
      if (!isLampOn()) {
        setLamp(true);
        relayState = true;
        Serial.println("Lamp turned ON for timer");
      }
      
      // Start new timer
      currentTimeout = minutes;
      timeoutActive = true;
      timeoutStart = millis();
      
      String timeMsg;
      if (minutes >= 60) {
        int hours = minutes / 60;
        int mins = minutes % 60;
        timeMsg = String(hours) + "h";
        if (mins > 0) timeMsg += " " + String(mins) + "m";
      } else {
        timeMsg = String(minutes) + "m";
      }
      
      Serial.println("Timer set for " + timeMsg);
    } else {
      // Disable timer if minutes is 0
      timeoutActive = false;
      currentTimeout = 0;
      Serial.println("Timer disabled");
    }
  }
  
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleStatus() {
  // Return current lamp and timer status as JSON
  String json = buildStatusJson();
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.send(200, "application/json", json);
}

// API endpoints for AJAX requests (no page reload)
void handleOnApi() {
  setLamp(true);
  relayState = true;
  Serial.println("Lamp turned ON");
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.send(200, "application/json", buildStatusJson());
}

void handleOffApi() {
  setLamp(false);
  relayState = false;
  // Cancel timer when turning lamp off
  timeoutActive = false;
  currentTimeout = 0;
  Serial.println("Lamp turned OFF");
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.send(200, "application/json", buildStatusJson());
}