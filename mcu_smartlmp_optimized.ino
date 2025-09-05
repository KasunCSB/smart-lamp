/*
 * Smart Lamp Controller - OPTIMIZED VERSION
 * 
 * Performance-optimized version with direct GPIO control and memory management.
 * Use this only if you need maximum performance or are an advanced user.
 * 
 * For beginners, use mcu_smartlmp.ino instead.
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

// Performance optimization intervals
static unsigned long lastSafetyCheck = 0;
const unsigned long SAFETY_CHECK_INTERVAL = 30UL * 60UL * 1000UL;   // Check every 30 minutes

// Memory buffers to avoid dynamic allocation
static char jsonBuffer[256];
static char htmlBuffer[8192];  // Large buffer for HTML content
static char tempBuffer[64];

// Fast GPIO operations using direct register access
// GPIO5 controls the relay
#define GPIO_SET_HIGH(pin) GPOS = (1 << pin)
#define GPIO_READ(pin) ((GPI >> pin) & 1)

// Optimized lamp control functions
inline bool isLampOn() __attribute__((always_inline));
inline void setLamp(bool on) __attribute__((always_inline));

inline bool isLampOn() {
  // Relay is active-low: LOW = lamp ON, HIGH = lamp OFF
  return !GPIO_READ(5);
}

// Control relay using direct register manipulation for speed
inline void setLamp(bool on) {
  // Use direct register access for fastest GPIO control
  if (on) {
    GPOC = (1 << 5);  // Clear bit 5 (GPIO5) - Turn lamp ON (active-low)
  } else {
    GPOS = (1 << 5);  // Set bit 5 (GPIO5) - Turn lamp OFF (active-low)  
  }
}

// Optimized string concatenation
inline void fastStrcat(char* dest, const char* src) __attribute__((always_inline));
inline void fastStrcat(char* dest, const char* src) {
  // Find end of destination string
  while (*dest) dest++;
  // Copy source to end of destination
  while ((*dest++ = *src++));
}

// Fast string copy using assembly optimization
void fastStrcpy(char* dest, const char* src) {
  asm volatile (
    "1: \n\t"
    "l8ui a3, %1, 0 \n\t"
    "s8i a3, %0, 0 \n\t"
    "addi.n %0, %0, 1 \n\t"
    "addi.n %1, %1, 1 \n\t"
    "bnez a3, 1b"
    : "+r" (dest), "+r" (src)
    :
    : "a3", "memory"
  );
}

// Convert integer to string without String class
void fastItoa(unsigned long value, char* buffer) {
  if (value == 0) {
    buffer[0] = '0';
    buffer[1] = '\0';
    return;
  }
  
  char temp[16];
  int i = 0;
  
  while (value > 0) {
    temp[i++] = '0' + (value % 10);
    value /= 10;
  }
  
  int j = 0;
  while (i > 0) {
    buffer[j++] = temp[--i];
  }
  buffer[j] = '\0';
}

// Function declarations
void buildHtmlResponse(char* html, bool lampOn, bool timeoutActive, unsigned long totalSec, const char* timeDisplayInit);

// HTTP request handlers
void handleRoot();
void handleOn();
void handleOff();
void handleTimeout();
void handleStatus();
void handleOnApi();
void handleOffApi();

// Calculate remaining timer seconds
unsigned long calculateRemainingSeconds() {
  if (!timeoutActive) return 0;
  
  unsigned long elapsed = millis() - timeoutStart;
  long remainingMs = (long)(currentTimeout * 60000UL) - (long)elapsed;
  if (remainingMs < 0) remainingMs = 0;
  return (unsigned long)(remainingMs / 1000);
}

// Build JSON status response
void buildStatusJson(char* buffer) {
  bool on = isLampOn();
  unsigned long remainingSeconds = (timeoutActive && on) ? calculateRemainingSeconds() : 0;
  
  // Manually construct JSON for optimal performance
  fastStrcpy(buffer, "{\"on\":");
  if (on) {
    fastStrcat(buffer, "true");
  } else {
    fastStrcat(buffer, "false");
  }
  fastStrcat(buffer, ",\"timeoutActive\":");
  if (timeoutActive) {
    fastStrcat(buffer, "true");
  } else {
    fastStrcat(buffer, "false");
  }
  fastStrcat(buffer, ",\"remainingSeconds\":");
  fastItoa(remainingSeconds, tempBuffer);
  fastStrcat(buffer, tempBuffer);
  fastStrcat(buffer, "}");
}

void setup() {
  // Skip Serial output for performance
  delay(100); // Brief delay for stability
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  
  // Wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(100); // Fast polling for quicker startup
  }
  
  // Setup relay pin and turn lamp off initially
  pinMode(relayPin, OUTPUT);
  GPIO_SET_HIGH(5); // Turn off initially (relay is active-low)
  
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
}

void loop() {
  server.handleClient();
  
  unsigned long currentMillis = millis();
  
  // Check if timer has expired (critical - check every loop)
  bool lampOn = isLampOn();
  if (timeoutActive && lampOn && (currentMillis - timeoutStart) >= (currentTimeout * 60000UL)) {
    setLamp(false);
    relayState = false; // Keep legacy variable in sync
    timeoutActive = false;
  }
  
  // Cancel timer if lamp was manually turned off
  if (timeoutActive && !isLampOn()) {
    timeoutActive = false;
  }
  
  // Check for safety reboot periodically (every 30 minutes)
  if (currentMillis - lastSafetyCheck >= SAFETY_CHECK_INTERVAL) {
    lastSafetyCheck = currentMillis;
    
    // Reboot if module has been running 12 hours and lamp is off
    if (!isLampOn() && (currentMillis - moduleStartTime) >= SAFETY_REBOOT_INTERVAL) {
      delay(1000); // Allow pending operations to complete
      ESP.restart();
    }
  }
  
  // Essential for ESP8266 WiFi stability
  yield();
}

void handleRoot() {
  // Prevent page caching for real-time updates
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  
  bool lampOn = isLampOn();
  
  // Calculate timer display
  unsigned long totalSec = calculateRemainingSeconds();
  char timeDisplayInit[16] = "00:00:00";
  if (timeoutActive && totalSec > 0) {
    unsigned long hours = totalSec / 3600;
    unsigned long minutes = (totalSec % 3600) / 60;
    unsigned long seconds = totalSec % 60;

    // Format time as HH:MM:SS
    sprintf(timeDisplayInit, "%02lu:%02lu:%02lu", hours, minutes, seconds);
  }
  
  // Build and send HTML response
  buildHtmlResponse(htmlBuffer, lampOn, timeoutActive, totalSec, timeDisplayInit);
  
  server.send(200, "text/html", htmlBuffer);
}

// Build HTML response using pointer advancement for speed
void buildHtmlResponse(char* html, bool lampOn, bool timeoutActive, unsigned long totalSec, const char* timeDisplayInit) {
  char* p = html;  // Pointer for sequential writing
  
  // Build HTML using strcpy and pointer advancement
  strcpy(p, "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'>");
  p += strlen(p);
  
  strcpy(p, "<meta charset='UTF-8'><title>Smart Lamp Controller</title>");
  p += strlen(p);
  
  strcpy(p, "<link rel='icon' href='data:image/svg+xml,%3Csvg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 24 24%22%3E%3Ccircle cx=%2212%22 cy=%2210%22 r=%226%22 fill=%22%23FFD700%22 stroke=%22%23FFA500%22 stroke-width=%221%22/%3E%3Crect x=%2210%22 y=%2216%22 width=%224%22 height=%224%22 fill=%22%23888%22 rx=%221%22/%3E%3C/svg%3E'>");
  p += strlen(p);
  
  strcpy(p, "<meta name='theme-color' content='#4CAF50'><style>");
  p += strlen(p);
  
  strcpy(p, "*{margin:0;padding:0;box-sizing:border-box;}");
  p += strlen(p);
  
  strcpy(p, "body{font-family:'Segoe UI',sans-serif;background:#fff;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px;}");
  p += strlen(p);
  
  strcpy(p, ".app{width:100%;max-width:350px;background:#fff;border-radius:30px;padding:40px 30px;box-shadow:0 20px 60px rgba(0,0,0,0.1);text-align:center;border:1px solid #f0f0f0;position:relative;}");
  p += strlen(p);
  
  strcpy(p, ".title{font-size:24px;font-weight:400;color:#333;margin-bottom:40px;letter-spacing:1px;}");
  p += strlen(p);
  
  strcpy(p, ".power-section{margin-bottom:40px;}");
  p += strlen(p);
  
  strcpy(p, ".power-btn{width:140px;height:140px;border-radius:50%;border:none;background:linear-gradient(135deg,#4CAF50,#45a049);color:#fff;font-size:18px;font-weight:600;cursor:pointer;transition:all 0.3s cubic-bezier(0.4,0,0.2,1);box-shadow:0 10px 30px rgba(76,175,80,0.4);margin:0 auto;display:flex;align-items:center;justify-content:center;}");
  p += strlen(p);
  
  strcpy(p, ".power-btn.off{background:linear-gradient(135deg,#e0e0e0,#bdbdbd);color:#757575;box-shadow:0 10px 30px rgba(0,0,0,0.1);}");
  p += strlen(p);
  
  strcpy(p, ".power-btn:hover{transform:translateY(-2px);box-shadow:0 15px 40px rgba(76,175,80,0.5);}");
  p += strlen(p);
  
  strcpy(p, ".power-btn.off:hover{box-shadow:0 15px 40px rgba(0,0,0,0.15);}");
  p += strlen(p);
  
  strcpy(p, ".power-btn:active{transform:scale(0.95);}");
  p += strlen(p);
  
  strcpy(p, ".status{margin-top:20px;font-size:16px;color:#666;}");
  p += strlen(p);
  
  strcpy(p, ".status-sub{margin-top:8px;font-size:14px;color:#999;}");
  p += strlen(p);
  
  strcpy(p, ".timer-section{background:#f8f9fa;border-radius:20px;padding:25px;border:1px solid #e9ecef;}");
  p += strlen(p);
  
  strcpy(p, ".section-title{font-size:18px;font-weight:400;color:#333;margin-bottom:20px;}");
  p += strlen(p);
  
  strcpy(p, ".timer-input{width:100%;padding:15px 20px;border:2px solid #e9ecef;border-radius:15px;background:#fff;font-size:16px;text-align:center;margin-bottom:20px;}");
  p += strlen(p);
  
  strcpy(p, ".timer-input:focus{outline:none;border-color:#2196F3;box-shadow:0 0 0 3px rgba(33,150,243,0.2);}");
  p += strlen(p);
  
  strcpy(p, ".btn{padding:12px 25px;border:none;border-radius:15px;background:linear-gradient(135deg,#2196F3,#1976D2);color:#fff;font-size:14px;font-weight:500;cursor:pointer;transition:all 0.3s ease;margin:5px;}");
  p += strlen(p);
  
  strcpy(p, ".btn:hover{transform:translateY(-1px);box-shadow:0 5px 15px rgba(33,150,243,0.4);}");
  p += strlen(p);
  
  strcpy(p, ".btn-cancel{background:linear-gradient(135deg,#f44336,#d32f2f);}");
  p += strlen(p);
  
  strcpy(p, ".btn-cancel:hover{box-shadow:0 5px 15px rgba(244,67,54,0.35);}");
  p += strlen(p);
  
  strcpy(p, ".btn:active{transform:scale(0.98);}");
  p += strlen(p);
  
  strcpy(p, ".quick-btns{display:flex;gap:10px;margin-top:15px;}");
  p += strlen(p);
  
  strcpy(p, ".quick-btn{flex:1;padding:10px;background:#fff;border:1px solid #e9ecef;border-radius:12px;color:#666;font-size:12px;cursor:pointer;transition:all 0.3s ease;}");
  p += strlen(p);
  
  strcpy(p, ".quick-btn:hover{background:#f8f9fa;transform:translateY(-1px);border-color:#2196F3;}");
  p += strlen(p);
  
  strcpy(p, ".btn:disabled,.quick-btn:disabled{opacity:0.6;cursor:not-allowed;box-shadow:none;transform:none;}");
  p += strlen(p);
  
  strcpy(p, ".timer-input:disabled{background:#f5f5f5;color:#aaa;}");
  p += strlen(p);
  
  strcpy(p, "@media(max-width:400px){body{padding:10px;}.app{margin:0;padding:30px 25px;}.power-btn{width:120px;height:120px;font-size:16px;}}");
  p += strlen(p);
  
  strcpy(p, "@media(max-height:600px){body{align-items:flex-start;padding-top:20px;}}");
  p += strlen(p);
  
  strcpy(p, "</style></head><body><div class='app'><h1 class='title'>Smart Lamp</h1>");
  p += strlen(p);
  
  // Power Section
  strcpy(p, "<div class='power-section'><form id='powerForm' action='");
  p += strlen(p);
  strcpy(p, lampOn ? "/off" : "/on");
  p += strlen(p);
  strcpy(p, "'><button id='powerBtn' type='button' onclick='togglePower()' class='power-btn ");
  p += strlen(p);
  if (!lampOn) {
    strcpy(p, "off");
    p += strlen(p);
  }
  strcpy(p, "'>");
  p += strlen(p);
  strcpy(p, lampOn ? "ON" : "OFF");
  p += strlen(p);
  strcpy(p, "</button></form><div id='lampStatus' class='status'>");
  p += strlen(p);
  strcpy(p, lampOn ? "Lamp is ON" : "Lamp is OFF");
  p += strlen(p);
  strcpy(p, "</div>");
  p += strlen(p);

  // Timer countdown display
  strcpy(p, "<div class='status-sub' id='cdWrap' data-remaining='");
  p += strlen(p);
  fastItoa(totalSec, tempBuffer);
  strcpy(p, tempBuffer);
  p += strlen(p);
  strcpy(p, "' style='display:");
  p += strlen(p);
  strcpy(p, timeoutActive ? "block" : "none");
  p += strlen(p);
  strcpy(p, ";'>Auto off in <span id='cdText'>");
  p += strlen(p);
  strcpy(p, timeDisplayInit);
  p += strlen(p);
  strcpy(p, "</span></div></div>");
  p += strlen(p);
  
  // Timer Section
  strcpy(p, "<div class='timer-section'><div class='section-title'>Timer</div>");
  p += strlen(p);
  strcpy(p, "<form id='timeoutForm' action='/timeout'>");
  p += strlen(p);
  strcpy(p, "<input id='minutesInput' type='number' name='minutes' class='timer-input' value='0' min='0' max='720' placeholder='Minutes (0 to disable)'>");
  p += strlen(p);
  strcpy(p, "<button id='cancelBtn' type='button' class='btn btn-cancel' style='display:");
  p += strlen(p);
  strcpy(p, timeoutActive ? "inline-block" : "none");
  p += strlen(p);
  strcpy(p, "' onclick='cancelTimer()'>Cancel Timer</button>");
  p += strlen(p);
  strcpy(p, "<button id='setBtn' class='btn' style='display:");
  p += strlen(p);
  strcpy(p, timeoutActive ? "none" : "inline-block");
  p += strlen(p);
  strcpy(p, "'>Set Timer</button></form>");
  p += strlen(p);
  strcpy(p, "<div class='quick-btns'>");
  p += strlen(p);
  strcpy(p, "<button id='q1' type='button' class='quick-btn' onclick='setQuick(1)'>1m</button>");
  p += strlen(p);
  strcpy(p, "<button id='q5' type='button' class='quick-btn' onclick='setQuick(5)'>5m</button>");
  p += strlen(p);
  strcpy(p, "<button id='q30' type='button' class='quick-btn' onclick='setQuick(30)'>30m</button>");
  p += strlen(p);
  strcpy(p, "<button id='q60' type='button' class='quick-btn' onclick='setQuick(60)'>1hr</button>");
  p += strlen(p);
  strcpy(p, "</div></div></div>");
  p += strlen(p);
  
  strcpy(p, "<script>");
  p += strlen(p);
  strcpy(p, "function setQuick(m){var inp=document.getElementById('minutesInput'); if(inp) inp.value=m; fetch('/timeout?minutes='+encodeURIComponent(m),{cache:'no-store'}).then(function(){return getStatus();}).catch(function(){});}");
  p += strlen(p);
  strcpy(p, "function cancelTimer(){fetch('/timeout?minutes=0',{cache:'no-store'}).then(function(){return getStatus();}).catch(function(){});}");
  p += strlen(p);
  strcpy(p, "(function(){var inp=document.getElementById('minutesInput');var btn=document.getElementById('setBtn'); var form=document.getElementById('timeoutForm'); if(!inp||!btn||!form) return; inp.addEventListener('input', function(){}); form.addEventListener('submit', function(e){ e.preventDefault(); if(inp.disabled) return; if(!inp.checkValidity()){ inp.reportValidity(); return;} var v=parseInt(inp.value)||0; fetch('/timeout?minutes='+encodeURIComponent(v),{cache:'no-store'}).then(function(){return getStatus();}).catch(function(){}); }); })();");
  p += strlen(p);
  strcpy(p, "function pad(n){return n<10?'0'+n:n;}");
  p += strlen(p);
  strcpy(p, "var wrap=document.getElementById('cdWrap');");
  p += strlen(p);
  strcpy(p, "var remainingSeconds=wrap?parseInt(wrap.getAttribute('data-remaining')):0;");
  p += strlen(p);
  strcpy(p, "function renderCountdown(){if(!wrap)return; if(remainingSeconds>0){var h=Math.floor(remainingSeconds/3600),m=Math.floor((remainingSeconds%3600)/60),s=remainingSeconds%60; var txt=document.getElementById('cdText'); if(txt){txt.textContent=pad(h)+':'+pad(m)+':'+pad(s);} wrap.style.display='block';}else{wrap.style.display='none';}}");
  p += strlen(p);
  strcpy(p, "function applyStatus(s){try{window.__lastStatus=s; var on=!!s.on; var pf=document.getElementById('powerForm'); var pb=document.getElementById('powerBtn'); var st=document.getElementById('lampStatus'); var inp=document.getElementById('minutesInput'); var setBtn=document.getElementById('setBtn'); var cancelBtn=document.getElementById('cancelBtn'); var q1=document.getElementById('q1'), q5=document.getElementById('q5'), q30=document.getElementById('q30'), q60=document.getElementById('q60'); if(pf) pf.action = on?'/off':'/on'; if(pb){pb.className = 'power-btn ' + (on?'':'off'); pb.textContent = on?'ON':'OFF';} if(st) st.textContent = on?'Lamp is ON':'Lamp is OFF'; var tActive = !!s.timeoutActive && (s.remainingSeconds||0) > 0 && on; if(cancelBtn) cancelBtn.style.display = tActive ? 'inline-block' : 'none'; if(setBtn) setBtn.style.display = tActive ? 'none' : 'inline-block'; remainingSeconds = s.remainingSeconds||0; renderCountdown(); }catch(e){}}");
  p += strlen(p);
  strcpy(p, "function togglePower(){var pb=document.getElementById('powerBtn'); if(!pb) return; pb.disabled=true; var on=(window.__lastStatus && window.__lastStatus.on) || !pb.className.includes('off'); var next= on?'/api/off':'/api/on'; fetch(next,{cache:'no-store'}).then(function(){return getStatus();}).catch(function(){}).finally(function(){pb.disabled=false;}); }");
  p += strlen(p);
  strcpy(p, "function getStatus(){return fetch('/status',{cache:'no-store'}).then(function(r){return r.json()}).then(function(s){applyStatus(s); return s;});}");
  p += strlen(p);
  strcpy(p, "renderCountdown();");
  p += strlen(p);
  strcpy(p, "setInterval(function(){getStatus().catch(function(){})}, 1000);");
  p += strlen(p);
  strcpy(p, "</script></body></html>");
}

// Set lamp state and update legacy variable
void setLampWithState(bool state) {
  setLamp(state);
  relayState = state;
  if (!state) {
    // Cancel timer when turning lamp off
    timeoutActive = false;
    currentTimeout = 0;
  }
}

// Send HTTP redirect response
void sendRedirect() {
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// Send JSON API response
void sendJsonResponse() {
  buildStatusJson(jsonBuffer);
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.send(200, "application/json", jsonBuffer);
}

void handleOn() {
  setLampWithState(true);
  sendRedirect();
}

void handleOff() {
  setLampWithState(false);
  sendRedirect();
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
      }
      
      // Start new timer
      currentTimeout = minutes;
      timeoutActive = true;
      timeoutStart = millis();
      
    } else {
      // Disable timer if minutes is 0
      timeoutActive = false;
      currentTimeout = 0;
    }
  }
  
  server.sendHeader("Cache-Control", "no-store");
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleStatus() {
  // Return current lamp and timer status as JSON
  buildStatusJson(jsonBuffer);
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.send(200, "application/json", jsonBuffer);
}

// API endpoints for AJAX requests (no page reload)
void handleOnApi() {
  setLampWithState(true);
  sendJsonResponse();
}

void handleOffApi() {
  setLampWithState(false);
  sendJsonResponse();
}
