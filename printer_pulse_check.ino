#include <WiFi.h>
#include <HTTPClient.h>
#include <ESPmDNS.h>

#define uS_TO_S_FACTOR 1000000ULL // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP 604800 // ESP32 will go to sleep for 7 days (in seconds), i.e wake and perform task weekly

const char* WifiName = "your-wifi-ssid";
const char* WifiPass = "your-wifi-password";
const char* PrinterHostname = "your-printer-hostname";

/*
 * findPrinterIp
 * 
 * Attempt to determine printer IP of given hostname
 */
String findPrinterIp(const char * hostname) {
  Serial.printf("[findPrinterIp] Attempting to find printer IP for %s\n", hostname);
  
  int n = MDNS.queryService("http", "tcp");
  if (n == 0) {
    Serial.println("[findPrinterIp] No http service found");
    return "";
  }

  Serial.printf("[findPrinterIp] %u http service(s) found\n", n);
  
  for (int i = 0; i < n; ++i) {
    if (MDNS.hostname(i) == hostname) {
      IPAddress ipAddr = MDNS.IP(i);
      Serial.printf("[findPrinterIp] Printer IP found: %s\n", ipAddr.toString().c_str());
      return String(ipAddr.toString().c_str());
    }
  }
  
  return "";
}

/*
 * sendPrintCmd
 * 
 * Send print diagnostic print command to local printer
 */
void sendPrintCmd(String printerIp) {
  HTTPClient http;

  Serial.println("[sendPrintCmd] Sending print command via HTTP");
  
  http.begin("http://" + printerIp + "/DevMgmt/InternalPrintDyn.xml");
  http.addHeader("Content-Type", "text/xml");

  int httpCode = http.POST("<ipdyn:InternalPrintDyn xmlns:ipdyn=\"http://www.hp.com/schemas/imaging/con/ledm/internalprintdyn/2008/03/21\" xmlns:dd=\"http://www.hp.com/schemas/imaging/con/dictionaries/1.0/\" xmlns:dd3=\"http://www.hp.com/schemas/imaging/con/dictionaries/2009/04/06\" xmlns:fw=\"http://www.hp.com/schemas/imaging/con/firewall/2011/01/05\"><ipdyn:JobType>pqDiagnosticsPage</ipdyn:JobType></ipdyn:InternalPrintDyn>");

  if(httpCode > 0) {
    if(httpCode == HTTP_CODE_CREATED) {
      Serial.println("[sendPrintCmd] Job queued");
    }
  } else {
    Serial.printf("[sendPrintCmd] Failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

/*
 * deepSleep
 * 
 * Setup wakeup by timer then enters deep sleep mode
 */
void deepSleep() {
  Serial.println("[deepSleep] Bye world");
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
  Serial.flush();
}

void setup() {
  Serial.begin(115200);

  // Attempt WIFI Connect
  Serial.printf("[WifiSetup] Connecting to %s\n", WifiName);

  int connectionTries = 0;
     
  while (WiFi.status() != WL_CONNECTED && connectionTries < 5) {
    connectionTries++;
    Serial.printf("[WifiSetup] Connection try %u\n", connectionTries);
    WiFi.begin(WifiName, WifiPass);
    delay(1000);
  }

  // If connection failed, go to sleep
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WifiSetup] Unable to connect to WIFI");
    deepSleep();
  }

  Serial.println("[WifiSetup] CONNECTED");

  // Attempt mDNS browser initialisation
  if (!MDNS.begin("ESP32_Browser")) {
    Serial.println("[mDNSBrowserSetup] Error setting up MDNS responder!");
    WiFi.disconnect();
    deepSleep();
  }

  // Attempt to look for printer IP
  String printerIp = findPrinterIp(PrinterHostname);

  if (printerIp == "") {
    WiFi.disconnect();
    deepSleep();
  }

  // Perform print job
  sendPrintCmd(printerIp);
  WiFi.disconnect();
  deepSleep();
}

void loop() {}
