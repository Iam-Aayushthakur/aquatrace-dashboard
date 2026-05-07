#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==========================================
// WIFI SETTINGS
// ==========================================
const char* ssid = "Code";
const char* password = "1234567890";

// ==========================================
// SUPABASE SETTINGS
// ==========================================
const char* supabase_base_url = "https://lxeysecudqdlxnssuzml.supabase.co/rest/v1";
const char* telemetry_url = "https://lxeysecudqdlxnssuzml.supabase.co/rest/v1/telemetry";
const char* commands_query_url = "https://lxeysecudqdlxnssuzml.supabase.co/rest/v1/device_commands?select=id,command&device_id=eq.AQT-MP-01&status=eq.pending&order=created_at.asc&limit=1";
const char* supabase_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imx4ZXlzZWN1ZHFkbHhuc3N1em1sIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzczNzM5OTQsImV4cCI6MjA5Mjk0OTk5NH0.lLFUCAUa-sf8gdN8kZm-ucoN575q7k414SUXe-X2SKQ";

const char* device_id = "AQT-MP-01";
const unsigned long commandPollIntervalMs = 2000;
unsigned long lastCommandPollAt = 0;

const uint8_t lcdSdaPin = 21;
const uint8_t lcdSclPin = 22;
const uint8_t lcdColumns = 16;
const uint8_t lcdRows = 2;

LiquidCrystal_I2C lcd27(0x27, lcdColumns, lcdRows);
LiquidCrystal_I2C lcd3F(0x3F, lcdColumns, lcdRows);
LiquidCrystal_I2C* lcd = nullptr;
bool lcdReady = false;
String lastLcdLine1 = "";
String lastLcdLine2 = "";

String fitLcdLine(String text) {
  if (text.length() > lcdColumns) {
    return text.substring(0, lcdColumns);
  }

  while (text.length() < lcdColumns) {
    text += " ";
  }

  return text;
}

bool i2cDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void showLcd(const String& line1, const String& line2) {
  if (!lcdReady || lcd == nullptr) return;

  String lcdLine1 = fitLcdLine(line1);
  String lcdLine2 = fitLcdLine(line2);

  if (lcdLine1 == lastLcdLine1 && lcdLine2 == lastLcdLine2) return;

  lcd->setCursor(0, 0);
  lcd->print(lcdLine1);
  lcd->setCursor(0, 1);
  lcd->print(lcdLine2);

  lastLcdLine1 = lcdLine1;
  lastLcdLine2 = lcdLine2;
}

void setupLcd() {
  Wire.begin(lcdSdaPin, lcdSclPin);

  if (i2cDevicePresent(0x27)) {
    lcd = &lcd27;
  } else if (i2cDevicePresent(0x3F)) {
    lcd = &lcd3F;
  }

  if (lcd == nullptr) {
    Serial.println("No I2C LCD found on 0x27 or 0x3F. Continuing without LCD.");
    return;
  }

  lcd->init();
  lcd->backlight();
  lcdReady = true;
  showLcd("AquaTrace", "Booting...");
}

void addSupabaseHeaders(HTTPClient& http) {
  http.addHeader("apikey", supabase_key);

  String authHeader = "Bearer ";
  authHeader += supabase_key;
  http.addHeader("Authorization", authHeader);
}

void addJsonSupabaseHeaders(HTTPClient& http) {
  addSupabaseHeaders(http);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Prefer", "return=minimal");
}

String parseJsonField(const String& json, const String& fieldName) {
  String key = "\"" + fieldName + "\":";
  int keyIndex = json.indexOf(key);
  if (keyIndex < 0) return "";

  int valueStart = keyIndex + key.length();
  while (valueStart < json.length() && isspace(json[valueStart])) {
    valueStart++;
  }

  if (valueStart >= json.length()) return "";

  if (json[valueStart] == '"') {
    valueStart++;
    int valueEnd = json.indexOf('"', valueStart);
    if (valueEnd < 0) return "";
    return json.substring(valueStart, valueEnd);
  }

  int valueEnd = valueStart;
  while (
    valueEnd < json.length() &&
    json[valueEnd] != ',' &&
    json[valueEnd] != '}' &&
    json[valueEnd] != ']'
  ) {
    valueEnd++;
  }

  return json.substring(valueStart, valueEnd);
}

void connectWiFi() {
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  showLcd("WiFi connecting", ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startAttemptAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptAt < 20000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    showLcd("WiFi connected", WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi connection failed. Will retry.");
    showLcd("WiFi failed", "Retrying...");
  }
}

void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("WiFi disconnected. Reconnecting...");
  showLcd("WiFi lost", "Reconnecting...");
  WiFi.disconnect();
  delay(500);
  connectWiFi();
}

bool updateCommandStatus(const String& commandId, const String& status) {
  if (commandId.length() == 0) return false;

  HTTPClient http;
  String url = String(supabase_base_url) + "/device_commands?id=eq." + commandId;
  http.begin(url);
  http.setTimeout(10000);
  addJsonSupabaseHeaders(http);

  String payload = "{\"status\":\"" + status + "\"}";
  int responseCode = http.PATCH(payload);
  String responseBody = http.getString();

  Serial.print("Command status update '");
  Serial.print(status);
  Serial.print("' response: ");
  Serial.println(responseCode);
  if (responseBody.length() > 0) {
    Serial.print("Command status body: ");
    Serial.println(responseBody);
  }

  http.end();
  return responseCode >= 200 && responseCode < 300;
}

int sendTelemetry() {
  HTTPClient http;
  http.begin(telemetry_url);
  http.setTimeout(10000);
  addJsonSupabaseHeaders(http);

  // Replace these simulated values with real sensor reads when the sensors are wired.
  float microplastics = random(10, 50);
  float turbidity = random(1, 10) / 2.0;
  float waterTemp = 24.5;
  float tds = random(100, 200);
  float ph = 7.1;
  float size = random(20, 80);
  float flow = 1.5;

  showLcd("Testing water...", "Please wait");

  String jsonPayload = "{";
  jsonPayload += "\"device_id\":\"" + String(device_id) + "\",";
  jsonPayload += "\"microplastics\":" + String(microplastics) + ",";
  jsonPayload += "\"turbidity\":" + String(turbidity) + ",";
  jsonPayload += "\"watertemp\":" + String(waterTemp) + ",";
  jsonPayload += "\"tds\":" + String(tds) + ",";
  jsonPayload += "\"ph\":" + String(ph) + ",";
  jsonPayload += "\"size\":" + String(size) + ",";
  jsonPayload += "\"flow\":" + String(flow);
  jsonPayload += "}";

  Serial.println("Sending telemetry: " + jsonPayload);

  int responseCode = http.POST(jsonPayload);
  String responseBody = http.getString();

  Serial.print("Telemetry response code: ");
  Serial.println(responseCode);
  if (responseBody.length() > 0) {
    Serial.print("Telemetry response body: ");
    Serial.println(responseBody);
  }

  http.end();

  if (responseCode >= 200 && responseCode < 300) {
    showLcd("Result uploaded", "MP:" + String((int)microplastics) + " TDS:" + String((int)tds));
  } else {
    showLcd("Upload failed", "HTTP " + String(responseCode));
  }

  return responseCode;
}

void executeCommand(const String& commandId, const String& command) {
  Serial.print("Executing command: ");
  Serial.println(command);
  showLcd("Command received", command);

  updateCommandStatus(commandId, "running");

  if (command == "start_testing") {
    int telemetryCode = sendTelemetry();
    bool sent = telemetryCode >= 200 && telemetryCode < 300;
    updateCommandStatus(commandId, sent ? "completed" : "failed");
    return;
  }

  Serial.println("Unknown command. Marking failed.");
  showLcd("Unknown command", command);
  updateCommandStatus(commandId, "failed");
}

void pollForCommands() {
  HTTPClient http;
  http.begin(commands_query_url);
  http.setTimeout(10000);
  addSupabaseHeaders(http);

  int responseCode = http.GET();
  String responseBody = http.getString();

  if (responseCode != 200) {
    Serial.print("Command poll response code: ");
    Serial.println(responseCode);
    if (responseBody.length() > 0) {
      Serial.print("Command poll response body: ");
      Serial.println(responseBody);
    }
    showLcd("Backend error", "Poll HTTP " + String(responseCode));
    http.end();
    return;
  }

  http.end();

  if (responseBody == "[]" || responseBody.length() < 5) {
    return;
  }

  String commandId = parseJsonField(responseBody, "id");
  String command = parseJsonField(responseBody, "command");

  if (commandId.length() == 0 || command.length() == 0) {
    Serial.print("Could not parse command body: ");
    Serial.println(responseBody);
    showLcd("Command error", "Parse failed");
    return;
  }

  executeCommand(commandId, command);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  randomSeed(esp_random());
  setupLcd();
  connectWiFi();
  Serial.println("ESP32 command listener ready.");
  showLcd("Ready", "Start on site");
}

void loop() {
  ensureWiFi();

  if (WiFi.status() == WL_CONNECTED && millis() - lastCommandPollAt >= commandPollIntervalMs) {
    lastCommandPollAt = millis();
    pollForCommands();
  }

  delay(50);
}
