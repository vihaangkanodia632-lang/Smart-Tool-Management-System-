/*
 * ╔══════════════════════════════════════════════════════════╗
 *   SMART TOOL MANAGEMENT SYSTEM
 *   Board   : XIAO ESP32-C3
 *   RFID    : RC522 (SPI)
 *   OLED    : SSD1306 0.96" (I2C)
 *   Buzzer  : Active Buzzer
 *   Backend : Firebase Realtime Database
 * ╚══════════════════════════════════════════════════════════╝
 *
 * ┌─────────────────────────────────────────────────────────┐
 *   YOUR EXACT WIRING
 * ├─────────────────────────────────────────────────────────┤
 *
 *   OLED SSD1306 (I2C)        XIAO ESP32-C3
 *   ─────────────────────     ─────────────
 *   GND                  →    GND
 *   VCC                  →    3V3
 *   SDA                  →    GPIO 4
 *   SCL                  →    GPIO 5
 *
 *   RC522 RFID (SPI)          XIAO ESP32-C3
 *   ─────────────────────     ─────────────
 *   SDA / SS / CS        →    GPIO 10
 *   SCK                  →    GPIO 6
 *   MOSI                 →    GPIO 7
 *   MISO                 →    GPIO 2
 *   GND                  →    GND
 *   RST                  →    GPIO 3
 *   VCC                  →    3V3   ← NEVER use 5V!
 *
 *   Active Buzzer              XIAO ESP32-C3
 *   ─────────────────────     ─────────────
 *   + (signal)           →    GPIO 19
 *   - (other end)        →    GND
 *
 * ├─────────────────────────────────────────────────────────┤
 *
 *   ARDUINO IDE SETTINGS:
 *     Board     : XIAO_ESP32C3
 *     Partition : Huge APP (3MB No OTA / 1MB SPIFFS)
 *     Upload Speed: 921600
 *
 *   LIBRARIES (install via Library Manager):
 *     - MFRC522            by GithubCommunity
 *     - Adafruit SSD1306   by Adafruit
 *     - Adafruit GFX       by Adafruit
 *     - Firebase ESP Client by Mobizt
 *     - ArduinoJson         v6.x
 *
 * └─────────────────────────────────────────────────────────┘
 */

// ── Disable unused Firebase modules FIRST (saves ~150KB flash) ──
#define FIREBASE_DISABLE_FIRESTORE
#define FIREBASE_DISABLE_FCM
#define FIREBASE_DISABLE_MESSAGING
#define FIREBASE_DISABLE_FUNCTIONS
#define FIREBASE_DISABLE_STORAGE
#define FIREBASE_DISABLE_GCS

#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <time.h>

// ══════════════════════════════════════════════════════════
//   ★  EDIT THESE — YOUR CREDENTIALS  ★
// ══════════════════════════════════════════════════════════
#define WIFI_SSID         "YOUR_WIFI_SSID"
#define WIFI_PASSWORD     "YOUR_WIFI_PASSWORD"
#define FIREBASE_HOST     "YOUR_PROJECT_ID-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_API_KEY  "YOUR_WEB_API_KEY"
#define FIREBASE_EMAIL    "admin@example.com"
#define FIREBASE_PASS     "your_password"
// ══════════════════════════════════════════════════════════

// NTP — Indian Standard Time UTC+5:30
#define NTP_SERVER      "pool.ntp.org"
#define GMT_OFFSET_SEC  19800
#define DST_OFFSET_SEC  0

// ── YOUR PIN DEFINITIONS ───────────────────────────────────
// OLED (I2C)
#define I2C_SDA       4
#define I2C_SCL       5

// RC522 RFID (SPI)
#define RFID_SS_PIN  10    // SDA / SS / CS
#define RFID_RST_PIN  3    // RST
#define SPI_SCK       6    // SCK
#define SPI_MOSI      7    // MOSI
#define SPI_MISO      2    // MISO

// Buzzer
#define BUZZER_PIN   19

// ── OLED Setup ─────────────────────────────────────────────
#define SCREEN_W   128
#define SCREEN_H    64
#define OLED_ADDR  0x3C
Adafruit_SSD1306 oled(SCREEN_W, SCREEN_H, &Wire, -1);

// ── RFID Setup ─────────────────────────────────────────────
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

// ── Firebase Setup ─────────────────────────────────────────
FirebaseData   fbdo;
FirebaseAuth   fbAuth;
FirebaseConfig fbConfig;
bool           fbReady = false;

// ── State Machine ──────────────────────────────────────────
enum State { IDLE, ID_SCANNED, PROCESSING };
State sysState = IDLE;

String workerUID  = "";
String workerName = "";

unsigned long stateTimer = 0;
const unsigned long ID_TIMEOUT = 15000;  // 15s to scan tool after ID card
const unsigned long MSG_SHOW   =  3000;  // 3s to show result on OLED

// ══════════════════════════════════════════════════════════
//   UTILITY FUNCTIONS
// ══════════════════════════════════════════════════════════

// Convert RFID UID bytes → uppercase hex string
String getUID(MFRC522::Uid uid) {
  String s = "";
  for (byte i = 0; i < uid.size; i++) {
    if (uid.uidByte[i] < 0x10) s += "0";
    s += String(uid.uidByte[i], HEX);
  }
  s.toUpperCase();
  return s;
}

// Get formatted timestamp string
String getTimestamp() {
  struct tm t;
  if (!getLocalTime(&t)) return "Time N/A";
  char buf[24];
  strftime(buf, sizeof(buf), "%d-%b-%Y %H:%M:%S", &t);
  return String(buf);
}

// Get Unix epoch for ordering records
unsigned long getEpoch() {
  struct tm t;
  if (!getLocalTime(&t)) return 0;
  return (unsigned long)mktime(&t);
}

// ══════════════════════════════════════════════════════════
//   BUZZER PATTERNS
// ══════════════════════════════════════════════════════════

// Success: two short beeps
void beepOK() {
  digitalWrite(BUZZER_PIN, HIGH); delay(80);
  digitalWrite(BUZZER_PIN, LOW);  delay(100);
  digitalWrite(BUZZER_PIN, HIGH); delay(120);
  digitalWrite(BUZZER_PIN, LOW);
}

// Error: one long beep
void beepFail() {
  digitalWrite(BUZZER_PIN, HIGH); delay(500);
  digitalWrite(BUZZER_PIN, LOW);
}

// Welcome: two quick beeps
void beepWelcome() {
  digitalWrite(BUZZER_PIN, HIGH); delay(60);
  digitalWrite(BUZZER_PIN, LOW);  delay(80);
  digitalWrite(BUZZER_PIN, HIGH); delay(60);
  digitalWrite(BUZZER_PIN, LOW);
}

// Alert: three beeps (unknown tag / conflict)
void beepAlarm() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH); delay(150);
    digitalWrite(BUZZER_PIN, LOW);  delay(150);
  }
}

// ══════════════════════════════════════════════════════════
//   OLED DISPLAY FUNCTIONS
// ══════════════════════════════════════════════════════════

void oledIdle() {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.drawRect(0, 0, 128, 64, SSD1306_WHITE);

  oled.setTextSize(1);
  oled.setCursor(14, 6);
  oled.print("TOOL MANAGEMENT");
  oled.drawLine(1, 17, 126, 17, SSD1306_WHITE);

  oled.setCursor(8, 24);
  oled.print("Scan your ID card");
  oled.setCursor(16, 36);
  oled.print("to get started");

  oled.drawLine(1, 48, 126, 48, SSD1306_WHITE);
  oled.setCursor(2, 54);
  oled.print(WiFi.isConnected() ? "WiFi: OK" : "WiFi: X");
  oled.setCursor(70, 54);
  oled.print(fbReady ? "FB: OK" : "FB: ...");

  oled.display();
}

void oledWelcome(String name) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  // Inverted header bar
  oled.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  oled.setTextColor(SSD1306_BLACK);
  oled.setTextSize(1);
  oled.setCursor(26, 3);
  oled.print("Welcome Back!");

  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(2, 18);
  // Truncate long names to fit screen
  oled.print(name.length() > 18 ? name.substring(0, 18) : name);

  oled.drawLine(0, 30, 128, 30, SSD1306_WHITE);
  oled.setCursor(2, 36);
  oled.print("Now scan the tool");
  oled.setCursor(2, 48);
  oled.print("to borrow/return");

  oled.display();
}

void oledResult(bool success, String line1, String line2) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  if (success) {
    oled.fillRect(0, 0, 128, 16, SSD1306_WHITE);
    oled.setTextColor(SSD1306_BLACK);
    oled.setTextSize(1);
    oled.setCursor(36, 4);
    oled.print("SUCCESS!");
  } else {
    oled.drawRect(0, 0, 128, 16, SSD1306_WHITE);
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1);
    oled.setCursor(34, 4);
    oled.print("WARNING!");
  }

  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(2, 22);
  oled.print(line1.length() > 20 ? line1.substring(0, 20) : line1);
  oled.setCursor(2, 36);
  oled.print(line2.length() > 20 ? line2.substring(0, 20) : line2);

  oled.display();
}

void oledMsg(String l1, String l2 = "", String l3 = "") {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(2,  8); oled.print(l1);
  oled.setCursor(2, 24); oled.print(l2);
  oled.setCursor(2, 40); oled.print(l3);
  oled.display();
}

// ══════════════════════════════════════════════════════════
//   FIREBASE HELPER FUNCTIONS
// ══════════════════════════════════════════════════════════

// Generic: read a string value from a path
String fbGetString(String path) {
  if (Firebase.RTDB.getString(&fbdo, path))
    return fbdo.stringData();
  return "";
}

// Generic: write a FirebaseJson object to a path
bool fbSetJson(String path, FirebaseJson &json) {
  return Firebase.RTDB.set(&fbdo, path, &json);
}

// Generic: update fields at a path (merge, not overwrite)
bool fbUpdate(String path, FirebaseJson &json) {
  return Firebase.RTDB.updateNode(&fbdo, path, &json);
}

// Generic: delete a node
bool fbDelete(String path) {
  return Firebase.RTDB.deleteNode(&fbdo, path);
}

// Look up worker name from /workers/{uid}/name
String lookupWorker(String uid) {
  return fbGetString("/workers/" + uid + "/name");
}

// Look up tool name from /tools/{uid}/name
String lookupTool(String uid) {
  return fbGetString("/tools/" + uid + "/name");
}

// Get who currently has a tool — returns "" if available
String toolBorrowedBy(String toolUID) {
  String val = fbGetString("/tools/" + toolUID + "/borrowedBy");
  if (val == "null" || val == "") return "";
  return val;
}

// ── CHECKOUT: mark tool as borrowed, log to DB ─────────────
bool doCheckout(String wUID, String wName, String tUID, String tName) {
  String ts = getTimestamp();
  unsigned long ep = getEpoch();
  FirebaseJson j;

  // 1. Update tool status → borrowed
  j.set("borrowedBy",    wUID);
  j.set("borrowedAt",    ts);
  j.set("borrowedEpoch", (int)ep);
  j.set("status",        "borrowed");
  if (!fbUpdate("/tools/" + tUID, j)) return false;

  // 2. Write to activeLoans
  j.clear();
  j.set("workerUID",       wUID);
  j.set("workerName",      wName);
  j.set("toolUID",         tUID);
  j.set("toolName",        tName);
  j.set("checkedOut",      ts);
  j.set("checkedOutEpoch", (int)ep);
  j.set("returned",        false);
  { String p = "/activeLoans/" + wUID + "/" + tUID;
    if (!fbSetJson(p, j)) return false; }

  // 3. Append to history log (non-critical — don't return false on failure)
  j.clear();
  j.set("type",       "CHECKOUT");
  j.set("workerUID",  wUID);
  j.set("workerName", wName);
  j.set("toolUID",    tUID);
  j.set("toolName",   tName);
  j.set("timestamp",  ts);
  j.set("epoch",      (int)ep);
  { String p = "/history/" + String(ep) + "_" + tUID.substring(0, 4);
    fbSetJson(p, j); }

  return true;
}

// ── RETURN: clear borrow status, log to DB ─────────────────
bool doReturn(String wUID, String wName, String tUID, String tName) {
  String ts = getTimestamp();
  unsigned long ep = getEpoch();
  FirebaseJson j;

  // 1. Update tool status → available
  j.set("borrowedBy",    "");
  j.set("borrowedAt",    "");
  j.set("borrowedEpoch", 0);
  j.set("status",        "available");
  if (!fbUpdate("/tools/" + tUID, j)) return false;

  // 2. Remove from activeLoans
  { String p = "/activeLoans/" + wUID + "/" + tUID;
    if (!fbDelete(p)) return false; }

  // 3. Append to history log
  j.clear();
  j.set("type",       "RETURN");
  j.set("workerUID",  wUID);
  j.set("workerName", wName);
  j.set("toolUID",    tUID);
  j.set("toolName",   tName);
  j.set("timestamp",  ts);
  j.set("epoch",      (int)ep);
  { String p = "/history/" + String(ep) + "_" + tUID.substring(0, 4) + "_R";
    fbSetJson(p, j); }

  return true;
}

// ── LOG ALERT to /alerts/ ──────────────────────────────────
void logAlert(String type, FirebaseJson &j) {
  String p = "/alerts/" + String(getEpoch()) + "_" + type;
  fbSetJson(p, j);
}

// ══════════════════════════════════════════════════════════
//   SETUP
// ══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);

  // Buzzer init
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // ── OLED init ────────────────────────────────────────────
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[ERROR] OLED not found — check SDA=GPIO4 SCL=GPIO5");
    // Flash buzzer to signal hardware error
    for (int i = 0; i < 5; i++) {
      digitalWrite(BUZZER_PIN, HIGH); delay(100);
      digitalWrite(BUZZER_PIN, LOW);  delay(100);
    }
    while (true) delay(1000);
  }
  oledMsg("Booting...", "Tool Management", "v1.0");
  Serial.println("[OK] OLED ready");
  delay(800);

  // ── RC522 RFID init ──────────────────────────────────────
  // SPI.begin(SCK, MISO, MOSI, SS)
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, RFID_SS_PIN);
  rfid.PCD_Init();
  delay(50);
  Serial.print("[OK] RFID ready — version: ");
  rfid.PCD_DumpVersionToSerial();

  // ── WiFi ─────────────────────────────────────────────────
  oledMsg("Connecting WiFi", WIFI_SSID, "Please wait...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int wTry = 0;
  while (WiFi.status() != WL_CONNECTED && wTry < 40) {
    delay(500);
    Serial.print(".");
    wTry++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[ERROR] WiFi failed!");
    oledMsg("WiFi FAILED!", "Check SSID/Pass", "Restarting...");
    beepFail();
    delay(3000);
    ESP.restart();
  }

  Serial.println("\n[OK] WiFi: " + WiFi.localIP().toString());

  // ── NTP Time Sync ─────────────────────────────────────────
  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
  oledMsg("Syncing time...", "IST UTC+5:30", "");
  struct tm ti;
  int ntpTry = 0;
  while (!getLocalTime(&ti) && ntpTry < 20) {
    delay(500);
    ntpTry++;
  }
  if (ntpTry < 20)
    Serial.println("[OK] Time: " + getTimestamp());
  else
    Serial.println("[WARN] NTP sync failed — timestamps may be wrong");

  // ── Firebase Init ─────────────────────────────────────────
  oledMsg("Firebase...", "Connecting", "Please wait");
  fbConfig.host                       = FIREBASE_HOST;
  fbConfig.api_key                    = FIREBASE_API_KEY;
  fbAuth.user.email                   = FIREBASE_EMAIL;
  fbAuth.user.password                = FIREBASE_PASS;
  fbConfig.token_status_callback      = tokenStatusCallback;
  fbConfig.max_token_generation_retry = 5;

  Firebase.begin(&fbConfig, &fbAuth);
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096, 1024);
  fbdo.setResponseSize(2048);

  int fbTry = 0;
  while (!Firebase.ready() && fbTry < 25) {
    delay(500);
    Serial.print("~");
    fbTry++;
  }

  fbReady = Firebase.ready();
  Serial.println(fbReady ? "\n[OK] Firebase ready" : "\n[WARN] Firebase not ready");

  beepWelcome();
  oledIdle();
  Serial.println("[READY] Waiting for ID card scan...");
}

// ══════════════════════════════════════════════════════════
//   MAIN LOOP
// ══════════════════════════════════════════════════════════
void loop() {

  // ──────────────────────────────────────────────────────────
  //  STATE: IDLE — waiting for worker ID card scan
  // ──────────────────────────────────────────────────────────
  if (sysState == IDLE) {
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

    String uid = getUID(rfid.uid);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    Serial.println("[SCAN] ID Card UID: " + uid);

    // Check Firebase is available
    if (!fbReady) {
      oledResult(false, "Firebase offline", "Check WiFi");
      beepFail();
      delay(3000);
      oledIdle();
      return;
    }

    // Look up worker in database
    String name = lookupWorker(uid);
    if (name == "") {
      // Unknown card — not registered
      Serial.println("[WARN] Unknown ID card: " + uid);
      oledResult(false, "Unknown ID card!", uid);
      beepAlarm();

      // Log alert to Firebase
      FirebaseJson j;
      j.set("uid",       uid);
      j.set("timestamp", getTimestamp());
      j.set("type",      "UNKNOWN_ID");
      logAlert("UID", j);

      delay(3000);
      oledIdle();
      return;
    }

    // ✓ Valid worker found
    workerUID  = uid;
    workerName = name;
    sysState   = ID_SCANNED;
    stateTimer = millis();

    beepWelcome();
    oledWelcome(name);
    Serial.println("[OK] Worker identified: " + name);
    return;
  }

  // ──────────────────────────────────────────────────────────
  //  STATE: ID_SCANNED — waiting for tool tag scan
  // ──────────────────────────────────────────────────────────
  if (sysState == ID_SCANNED) {

    // Timeout — worker walked away without scanning tool
    if (millis() - stateTimer > ID_TIMEOUT) {
      Serial.println("[TIMEOUT] No tool scanned — resetting");
      oledResult(false, "Timed out!", "Scan ID again");
      beepFail();
      sysState   = IDLE;
      workerUID  = "";
      workerName = "";
      delay(2000);
      oledIdle();
      return;
    }

    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

    String tUID = getUID(rfid.uid);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    Serial.println("[SCAN] Tool UID: " + tUID);

    // Guard: worker accidentally re-scans their own ID card
    if (tUID == workerUID) {
      oledResult(false, "That is your ID!", "Scan a tool tag");
      beepFail();
      delay(2000);
      oledWelcome(workerName);
      return;
    }

    sysState = PROCESSING;
    oledMsg("Processing...", "Please wait...", "");

    // Look up tool in database
    String tName = lookupTool(tUID);
    if (tName == "") {
      // Tool tag not registered
      Serial.println("[WARN] Unknown tool tag: " + tUID);
      oledResult(false, "Unknown tool!", "Register it first");
      beepAlarm();

      FirebaseJson j;
      j.set("uid",        tUID);
      j.set("scannedBy",  workerUID);
      j.set("workerName", workerName);
      j.set("timestamp",  getTimestamp());
      j.set("type",       "UNKNOWN_TOOL");
      logAlert("TOOL", j);

      delay(3000);
      sysState   = IDLE;
      workerUID  = "";
      workerName = "";
      oledIdle();
      return;
    }

    // ── Auto-detect: is this a borrow or a return? ──────────
    String borrowedBy = toolBorrowedBy(tUID);

    if (borrowedBy == "") {
      // ── CHECKOUT: tool is free ─────────────────────────
      Serial.println("[ACTION] CHECKOUT: " + workerName + " → " + tName);
      if (doCheckout(workerUID, workerName, tUID, tName)) {
        oledResult(true, "CHECKED OUT", tName.length() > 16 ? tName.substring(0,16) : tName);
        beepOK();
      } else {
        Serial.println("[ERROR] Checkout DB write failed");
        oledResult(false, "DB Error!", "Try again");
        beepFail();
      }

    } else if (borrowedBy == workerUID) {
      // ── RETURN: this worker is returning their own tool ─
      Serial.println("[ACTION] RETURN: " + workerName + " returned " + tName);
      if (doReturn(workerUID, workerName, tUID, tName)) {
        oledResult(true, "RETURNED!", tName.length() > 16 ? tName.substring(0,16) : tName);
        beepOK();
      } else {
        Serial.println("[ERROR] Return DB write failed");
        oledResult(false, "DB Error!", "Try again");
        beepFail();
      }

    } else {
      // ── CONFLICT: tool is currently held by someone else ─
      String otherName = lookupWorker(borrowedBy);
      if (otherName == "") otherName = borrowedBy;
      Serial.println("[CONFLICT] " + tName + " is with " + otherName);
      oledResult(false, "Tool held by:", otherName.length() > 16 ? otherName.substring(0,16) : otherName);
      beepAlarm();

      FirebaseJson j;
      j.set("toolUID",           tUID);
      j.set("toolName",          tName);
      j.set("attemptedByUID",    workerUID);
      j.set("attemptedByName",   workerName);
      j.set("currentlyWithUID",  borrowedBy);
      j.set("currentlyWithName", otherName);
      j.set("timestamp",         getTimestamp());
      j.set("type",              "CONFLICT");
      logAlert("CONF", j);

      delay(3000);
      sysState   = IDLE;
      workerUID  = "";
      workerName = "";
      oledIdle();
      return;
    }

    // Reset after showing result
    delay(MSG_SHOW);
    sysState   = IDLE;
    workerUID  = "";
    workerName = "";
    oledIdle();
  }
}
