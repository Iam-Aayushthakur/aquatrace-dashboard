#include <WiFi.h>
#include <HTTPClient.h>

// ==========================================
// WIFI SETTINGS
// ==========================================
const char* ssid = "YOUR_WIFI_NETWORK_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// ==========================================
// SUPABASE SETTINGS
// ==========================================
// Your Project URL + the REST endpoint for the 'telemetry' table
const char* supabase_url = "https://lxeysecudqdlxnssuzml.supabase.co/rest/v1/telemetry";

// Your Anon Public Key
const char* supabase_key = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imx4ZXlzZWN1ZHFkbHhuc3N1em1sIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzczNzM5OTQsImV4cCI6MjA5Mjk0OTk5NH0.lLFUCAUa-sf8gdN8kZm-ucoN575q7k414SUXe-X2SKQ";

// Unique ID for this specific IoT hardware device
const char* device_id = "AQT-MP-01";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // 1. Connect to Wi-Fi
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // 2. Setup the HTTP connection
    http.begin(supabase_url);
    
    // 3. Add Supabase Required Headers
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", supabase_key);
    
    String authHeader = "Bearer ";
    authHeader += supabase_key;
    http.addHeader("Authorization", authHeader);
    
    // 4. Read your sensors here! (These are simulated values)
    float microplastics = random(10, 50); 
    float turbidity = random(1, 10) / 2.0;
    float waterTemp = 24.5;
    float tds = random(100, 200);
    float ph = 7.1;
    float size = random(20, 80);
    float flow = 1.5;
    
    // 5. Construct the JSON payload (Format must match your database columns)
    String jsonPayload = "{";
    jsonPayload += "\"device_id\":\"" + String(device_id) + "\",";
    jsonPayload += "\"microplastics\":" + String(microplastics) + ",";
    jsonPayload += "\"turbidity\":" + String(turbidity) + ",";
    jsonPayload += "\"waterTemp\":" + String(waterTemp) + ",";
    jsonPayload += "\"tds\":" + String(tds) + ",";
    jsonPayload += "\"ph\":" + String(ph) + ",";
    jsonPayload += "\"size\":" + String(size) + ",";
    jsonPayload += "\"flow\":" + String(flow);
    jsonPayload += "}";
    
    Serial.println("Sending data: " + jsonPayload);
    
    // 6. Send the POST Request to insert the data into the database
    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode); // A 201 code means successfully created
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    
    // Free resources
    http.end();
  } else {
    Serial.println("WiFi Disconnected. Reconnecting...");
    WiFi.begin(ssid, password);
  }
  
  // Wait 10 seconds before sending the next reading
  delay(10000); 
}
