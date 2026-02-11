/* From GitHub.com (08/28/2025)
 * ESP32-S3 Dev Module: Wi-Fi, Web Server, OTA, and calibrated readings from two ADS1115 modules.
 * - Three ZMPT101B voltage sensors (ADS #1, addr 0x48)
 * - Three SCT-013 current sensors (ADS #2, addr 0x49)
 * Edit from Gemini to add web server refresh rate, channel names, sensor readings use core 0
 * All good without calibration (08/29/2025)
 * Voltage calibration acceptable until final installation. (08/29/2025)
 * Calibration factors for voltage and current can be set per channel.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <Adafruit_ADS1X15.h>
#include <ESPmDNS.h>

// --- Wi-Fi Credentials ---
const char* ssids[] = {"ssids"};
const char* passwords[] = {"password"};
const int wifiCount = sizeof(ssids) / sizeof(ssids[0]);

// Web server
WebServer server(80);

// ADS1115 instances
Adafruit_ADS1115 adsVoltage;
Adafruit_ADS1115 adsCurrent;

// --- Calibration Factors (set these as needed) ---
float voltageCalibration[3] = { 1330.00, 819.00, 900.00 };  // ZMPT101B
float currentCalibration[3] = { 30.0, 30.0, 30.0 };  // SCT-013
const char* channelNames[3] = {"Shore", "Inverter IN", "Inverter OUT"};

// Sensor values
float voltage[3];
float current[3];

// Timing
unsigned long lastUpdate = 0;
const unsigned long interval = 20000; // 20 seconds

// --- Function Prototypes ---
void handleRoot();
void connectToWiFi();
void setupOTA();
void readSensors();
float readRMSVoltage(int channel);
float rawToVoltage(int16_t raw, adsGain_t gain);
float rawToCurrent(int16_t raw, adsGain_t gain, int channel);

// ------------------------ SETUP ------------------------
void setup() {
  // SDA on GPIO 39, SCL on GPIO 40
  Wire.begin(39, 40, 100000);
  Serial.begin(115200);
  delay(100);

  connectToWiFi();

  // OTA
  setupOTA();

  // mDNS
  if (!MDNS.begin("esp32s3-monitor")) {
    Serial.println("Error setting up MDNS responder. Restarting...");
    delay(2000);
    ESP.restart();
  }

  // Web server setup
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started.");

  // Initialize ADS1115 modules
  if (!adsVoltage.begin(0x48)) {
    Serial.println("Failed to initialize ADS1115 (Voltage) at 0x48");
    while (1);
  }
  if (!adsCurrent.begin(0x49)) {
    Serial.println("Failed to initialize ADS1115 (Current) at 0x49");
    while (1);
  }

  adsVoltage.setGain(GAIN_ONE);  // ±4.096V for ZMPT101B
  adsCurrent.setGain(GAIN_TWO);  // ±2.048V for SCT013
}

// ------------------------ LOOP ------------------------
void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  if (millis() - lastUpdate >= interval) {
    readSensors();
    lastUpdate = millis();
  }
}

// ------------------------ Wi-Fi Logic ------------------------
void connectToWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  for (int i = 0; i < wifiCount; i++) {
    Serial.print("Trying: ");
    Serial.println(ssids[i]);
    WiFi.begin(ssids[i], passwords[i]);

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 8000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to " + String(ssids[i]));
      Serial.println("IP address: " + WiFi.localIP().toString());
      return;
    } else {
      Serial.println("\nFailed to connect to " + String(ssids[i]));
    }
  }

  Serial.println("Could not connect to any network. Restarting...");
  delay(3000);
  ESP.restart();
}

// ------------------------ OTA Setup ------------------------
void setupOTA() {
  ArduinoOTA.setHostname("esp32s3-monitor");

  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA update");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA update");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("OTA ready. Use IDE > Port > Network Ports to upload");
}

// ------------------------ Sensor Reading ------------------------
void readSensors() {
  Serial.println("Reading sensor data...");

  for (int i = 0; i < 3; i++) {
    // Use the new RMS function to get a stable voltage reading
    voltage[i] = readRMSVoltage(i);

    // Current reading logic remains the same
    int16_t rawC = adsCurrent.readADC_SingleEnded(i);
    current[i] = rawToCurrent(rawC, adsCurrent.getGain(), i);
  }

  for (int i = 0; i < 3; i++) {
    Serial.printf("Channel %d - Voltage: %.2f V, Current: %.2f A\n", i + 1, voltage[i], current[i]);
  }
}
// ------------------------ Core 0 Task ------------------------
// Task to handle sensor readings on Core 0
void TaskReadSensors(void *pvParameters) {
  Serial.println("Sensor reading task running on Core " + String(xPortGetCoreID()));

  for (;;) {
    readSensors();
    // Delay the task to avoid a watchdog timer timeout
    vTaskDelay(interval / portTICK_PERIOD_MS);
  }
}

float readRMSVoltage(int channel) {
  long sumOfSquares = 0;
  unsigned long sampleCount = 0;
  unsigned long startTime = millis();
  const unsigned long sampleWindow = 200;
  
  while (millis() - startTime < sampleWindow) {
    int16_t raw = adsVoltage.readADC_SingleEnded(channel);
    float V = rawToVoltage(raw, adsVoltage.getGain());
    float V_centered = V - 1.65;
    sumOfSquares += (long)(V_centered * V_centered * 1000.0);
    sampleCount++;
  }
  
  if (sampleCount == 0) return 0.0;
  
  float meanSquare = (float)sumOfSquares / sampleCount / 1000.0;
  float rmsVoltage = sqrt(meanSquare);
  
  return rmsVoltage * voltageCalibration[channel];
}

// Use Adafruit's multiplier for the ADC, then apply calibration
float rawToVoltage(int16_t raw, adsGain_t gain) {
  float multiplier = (gain == GAIN_TWO) ? 0.0625f : 0.125f; // mV/bit
  return raw * multiplier / 1000.0f; // to Volts
}

// Pass channel for per-channel calibration
float rawToCurrent(int16_t raw, adsGain_t gain, int channel) {
  float v = rawToVoltage(raw, gain);
  return v * currentCalibration[channel];
}

// ------------------------ Web Server ------------------------
void handleRoot() {
  String html = "<html><head><title>ESP32-S3 Monitor</title><meta http-equiv='refresh' content='20'></head><body>";
  html += "<h1>Voltage & Current Monitor</h1><p>Data updated every 20 seconds</p>";
  html += "<p>Connected to: " + String(WiFi.SSID()) + "</p>";
  html += "<table border='1'><tr><th>Channel</th><th>Voltage (V)</th><th>Current (A)</th></tr>";

  for (int i = 0; i < 3; i++) {
    html += "<tr><td>" + String(channelNames[i]) + "</td><td>" + String(voltage[i], 2) + "</td><td>" + String(current[i], 2) + "</td></tr>";
  }

  html += "</table></body></html>";
  server.send(200, "text/html", html);
}
