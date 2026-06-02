# ╔══════════════════════════════════════════════════════════╗
#   SMART TOOL MANAGEMENT SYSTEM — main.py
#   Board   : ESP32-C3 XIAO / ESP32-C6 DevKit
#   Runtime : MicroPython (deploy via Thonny)
#   Backend : Firebase Realtime Database (REST API)
# ╚══════════════════════════════════════════════════════════╝
#
#   YOUR WIRING:
#   OLED SDA → GPIO 4    OLED SCL → GPIO 5
#   RFID SS  → GPIO 10   RFID SCK → GPIO 6
#   RFID MOSI→ GPIO 7    RFID MISO→ GPIO 2
#   RFID RST → GPIO 3    BUZZER   → GPIO 19
#
#   FILES TO COPY TO ESP32 via Thonny (Device → Upload):
#     main.py       ← this file
#     config.py     ← your credentials file
#     /lib/mfrc522.py
#     /lib/ssd1306.py
#     /lib/urequests.py
#
#   LIBRARY SOURCES:
#     mfrc522  : https://github.com/cefn/micropython-mfrc522
#     ssd1306  : built into MicroPython firmware OR
#                https://github.com/micropython/micropython-lib
#     urequests: https://github.com/micropython/micropython-lib

import machine
import time
import network
import urequests
import ujson
from machine import Pin, SPI, I2C
from ssd1306 import SSD1306_I2C
from mfrc522 import MFRC522

# ── Load credentials from config.py ───────────────────────
from config import (
    WIFI_SSID, WIFI_PASSWORD,
    FIREBASE_HOST, FIREBASE_API_KEY,
    FIREBASE_EMAIL, FIREBASE_PASS
)

# ── Time ───────────────────────────────────────────────────
NTP_SERVER     = "pool.ntp.org"
GMT_OFFSET_SEC = 19800   # IST = UTC + 5:30

# ── Pin Definitions ────────────────────────────────────────
I2C_SDA      = 4
I2C_SCL      = 5
BUZZER_PIN   = 19
RFID_SS_PIN  = 10
RFID_RST_PIN = 3
SPI_SCK      = 6
SPI_MOSI     = 7
SPI_MISO     = 2

# ── OLED ───────────────────────────────────────────────────
SCREEN_W  = 128
SCREEN_H  = 64
OLED_ADDR = 0x3C

# ── Timeouts ───────────────────────────────────────────────
ID_TIMEOUT = 15000   # ms — time to scan tool after ID card
MSG_SHOW   =  3000   # ms — how long result stays on screen

# ── State Machine ──────────────────────────────────────────
STATE_IDLE       = 0
STATE_ID_SCANNED = 1
STATE_PROCESSING = 2

# ── Runtime Variables ──────────────────────────────────────
fb_id_token  = ""
fb_token_exp = 0
sys_state    = STATE_IDLE
worker_uid   = ""
worker_name  = ""
state_timer  = 0
wlan         = None

# ══════════════════════════════════════════════════════════
#   HARDWARE INIT
# ══════════════════════════════════════════════════════════

buzzer = Pin(BUZZER_PIN, Pin.OUT, value=0)

i2c  = I2C(0, sda=Pin(I2C_SDA), scl=Pin(I2C_SCL), freq=100000)
oled = SSD1306_I2C(SCREEN_W, SCREEN_H, i2c, addr=OLED_ADDR)

spi  = SPI(1, baudrate=1000000, polarity=0, phase=0,
           sck=Pin(SPI_SCK), mosi=Pin(SPI_MOSI), miso=Pin(SPI_MISO))
rfid = MFRC522(spi=spi, gpioRst=RFID_RST_PIN, gpioCs=RFID_SS_PIN)

# ══════════════════════════════════════════════════════════
#   UTILITY FUNCTIONS
# ══════════════════════════════════════════════════════════

def rfid_uid(uid_bytes):
    """Convert RFID UID byte list → uppercase hex string e.g. 'A3F2C1D0'"""
    return ''.join('{:02X}'.format(b) for b in uid_bytes)


def get_epoch():
    """Unix epoch — MicroPython counts from 2000-01-01 so add offset."""
    return time.time() + 946684800


def timestamp():
    """Return formatted IST timestamp: DD-Mon-YYYY HH:MM:SS"""
    try:
        t = time.gmtime(get_epoch() + GMT_OFFSET_SEC)
        months = ["Jan","Feb","Mar","Apr","May","Jun",
                  "Jul","Aug","Sep","Oct","Nov","Dec"]
        return "{:02d}-{}-{:04d} {:02d}:{:02d}:{:02d}".format(
            t[2], months[t[1]-1], t[0], t[3], t[4], t[5])
    except Exception:
        return "N/A"

# ── Buzzer Patterns ───────────────────────────────────────

def beep_ok():
    """Two short beeps = success"""
    buzzer.value(1); time.sleep_ms(80)
    buzzer.value(0); time.sleep_ms(100)
    buzzer.value(1); time.sleep_ms(120)
    buzzer.value(0)

def beep_fail():
    """One long beep = error"""
    buzzer.value(1); time.sleep_ms(500)
    buzzer.value(0)

def beep_welcome():
    """Two quick beeps = card recognised"""
    buzzer.value(1); time.sleep_ms(60)
    buzzer.value(0); time.sleep_ms(80)
    buzzer.value(1); time.sleep_ms(60)
    buzzer.value(0)

def beep_alarm():
    """Three beeps = unknown tag or conflict"""
    for _ in range(3):
        buzzer.value(1); time.sleep_ms(150)
        buzzer.value(0); time.sleep_ms(150)

# ══════════════════════════════════════════════════════════
#   OLED DISPLAY
# ══════════════════════════════════════════════════════════

def oled_idle():
    oled.fill(0)
    oled.rect(0, 0, 128, 64, 1)
    oled.text("TOOL MANAGEMENT", 8,  6, 1)
    oled.hline(1, 16, 125, 1)
    oled.text("Scan your ID card", 2, 22, 1)
    oled.text("to get started",    8, 34, 1)
    oled.hline(1, 47, 125, 1)
    wifi_txt = "WiFi OK" if (wlan and wlan.isconnected()) else "No WiFi!"
    fb_txt   = "FB OK"   if fb_id_token else "FB..."
    oled.text(wifi_txt, 2,  53, 1)
    oled.text(fb_txt,   72, 53, 1)
    oled.show()


def oled_welcome(name):
    oled.fill(0)
    oled.fill_rect(0, 0, 128, 14, 1)
    oled.text("Welcome Back!", 14, 3, 0)
    oled.text(name[:18], 2, 18, 1)
    oled.hline(0, 30, 128, 1)
    oled.text("Now scan the tool", 2, 36, 1)
    oled.text("to borrow/return",  2, 48, 1)
    oled.show()


def oled_result(ok, line1, line2):
    oled.fill(0)
    if ok:
        oled.fill_rect(0, 0, 128, 16, 1)
        oled.text("SUCCESS!", 28, 4, 0)
    else:
        oled.rect(0, 0, 128, 16, 1)
        oled.text("WARNING!", 28, 4, 1)
    oled.text(line1[:20], 2, 22, 1)
    oled.text(line2[:20], 2, 36, 1)
    oled.show()


def oled_msg(l1, l2="", l3=""):
    oled.fill(0)
    oled.text(l1[:16], 2,  8, 1)
    oled.text(l2[:16], 2, 24, 1)
    oled.text(l3[:16], 2, 40, 1)
    oled.show()

# ══════════════════════════════════════════════════════════
#   FIREBASE — AUTH & REST API
# ══════════════════════════════════════════════════════════

def fb_authenticate():
    """Sign in with email + password via Firebase Auth REST API."""
    global fb_id_token, fb_token_exp
    url = ("https://identitytoolkit.googleapis.com/v1/accounts"
           ":signInWithPassword?key=" + FIREBASE_API_KEY)
    print("[FB] Authenticating:", FIREBASE_EMAIL)
    print("[FB] Host:", FIREBASE_HOST)
    body = ujson.dumps({
        "email":             FIREBASE_EMAIL,
        "password":          FIREBASE_PASS,
        "returnSecureToken": True
    })
    try:
        r = urequests.post(url, data=body,
                           headers={"Content-Type": "application/json"})
        print("[FB] Auth HTTP:", r.status_code)
        d = r.json()
        r.close()
        if "idToken" in d:
            fb_id_token  = d["idToken"]
            fb_token_exp = get_epoch() + int(d.get("expiresIn", 3600)) - 60
            print("[FB] Auth OK - logged in as", FIREBASE_EMAIL)
            return True
        err = d.get("error", {})
        print("[FB] Auth FAILED code:", err.get("code"), "msg:", err.get("message"))
        return False
    except Exception as e:
        print("[FB] Auth exception:", e)
        return False


def fb_ensure_token():
    """Re-authenticate if token is expired or missing."""
    global fb_id_token
    if not fb_id_token or get_epoch() >= fb_token_exp:
        print("[FB] Token missing - re-authenticating")
        if not fb_authenticate():
            fb_id_token = ""


def fb_url(path):
    """Build a Firebase REST URL - strips accidental https:// from host."""
    host = FIREBASE_HOST.replace("https://", "").replace("http://", "").rstrip("/")
    return "https://{}{}.json?auth={}".format(host, path, fb_id_token)


def fb_get(path):
    """GET a value from Firebase. Returns parsed JSON or None."""
    fb_ensure_token()
    if not fb_id_token:
        print("[FB] GET skipped - no token")
        return None
    print("[FB] GET", path)
    try:
        r = urequests.get(fb_url(path))
        print("[FB] GET status:", r.status_code)
        data = r.json()
        r.close()
        print("[FB] GET result:", data)
        return data
    except Exception as e:
        print("[FB] GET error", path, e)
        return None


def fb_set(path, data):
    """PUT (overwrite) data at a Firebase path. Returns True on success."""
    fb_ensure_token()
    if not fb_id_token:
        return False
    try:
        r = urequests.put(fb_url(path),
                          data=ujson.dumps(data),
                          headers={"Content-Type": "application/json"})
        r.close()
        return True
    except Exception as e:
        print("[FB] SET error", path, e)
        return False


def fb_update(path, data):
    """PATCH (merge update) fields at a Firebase path."""
    fb_ensure_token()
    if not fb_id_token:
        return False
    try:
        r = urequests.patch(fb_url(path),
                            data=ujson.dumps(data),
                            headers={"Content-Type": "application/json"})
        r.close()
        return True
    except Exception as e:
        print("[FB] UPDATE error", path, e)
        return False


def fb_delete(path):
    """DELETE a node at a Firebase path."""
    fb_ensure_token()
    if not fb_id_token:
        return False
    try:
        r = urequests.delete(fb_url(path))
        r.close()
        return True
    except Exception as e:
        print("[FB] DELETE error", path, e)
        return False

# ── Firebase Data Lookups ─────────────────────────────────

def lookup_worker(uid):
    """Return worker name string, or '' if not found."""
    val = fb_get("/workers/{}/name".format(uid))
    if val is None or val == "null":
        return ""
    return str(val)


def lookup_tool(uid):
    """Return tool name string, or '' if not found."""
    val = fb_get("/tools/{}/name".format(uid))
    if val is None or val == "null":
        return ""
    return str(val)


def tool_borrowed_by(tool_uid):
    """Return worker UID who has the tool, or '' if available."""
    val = fb_get("/tools/{}/borrowedBy".format(tool_uid))
    if val is None or val == "null" or val == "":
        return ""
    return str(val)

# ── CHECKOUT ──────────────────────────────────────────────

def do_checkout(w_uid, w_name, t_uid, t_name):
    ts = timestamp()
    ep = get_epoch()

    # 1. Mark tool as borrowed
    if not fb_update("/tools/{}".format(t_uid), {
        "borrowedBy":    w_uid,
        "borrowedAt":    ts,
        "borrowedEpoch": ep,
        "status":        "borrowed"
    }):
        return False

    # 2. Write active loan record
    if not fb_set("/activeLoans/{}/{}".format(w_uid, t_uid), {
        "workerUID":       w_uid,
        "workerName":      w_name,
        "toolUID":         t_uid,
        "toolName":        t_name,
        "checkedOut":      ts,
        "checkedOutEpoch": ep,
        "returned":        False
    }):
        return False

    # 3. Append to history (non-critical — don't fail on error)
    fb_set("/history/{}_{}" .format(ep, t_uid[:4]), {
        "type":       "CHECKOUT",
        "workerUID":  w_uid,
        "workerName": w_name,
        "toolUID":    t_uid,
        "toolName":   t_name,
        "timestamp":  ts,
        "epoch":      ep
    })
    return True

# ── RETURN ────────────────────────────────────────────────

def do_return(w_uid, w_name, t_uid, t_name):
    ts = timestamp()
    ep = get_epoch()

    # 1. Clear tool borrow status
    if not fb_update("/tools/{}".format(t_uid), {
        "borrowedBy":    "",
        "borrowedAt":    "",
        "borrowedEpoch": 0,
        "status":        "available"
    }):
        return False

    # 2. Remove active loan entry
    if not fb_delete("/activeLoans/{}/{}".format(w_uid, t_uid)):
        return False

    # 3. Append to history
    fb_set("/history/{}_{}_R".format(ep, t_uid[:4]), {
        "type":       "RETURN",
        "workerUID":  w_uid,
        "workerName": w_name,
        "toolUID":    t_uid,
        "toolName":   t_name,
        "timestamp":  ts,
        "epoch":      ep
    })
    return True

# ── ALERT ─────────────────────────────────────────────────

def log_alert(alert_type, data):
    fb_set("/alerts/{}_{}".format(get_epoch(), alert_type), data)

# ══════════════════════════════════════════════════════════
#   WIFI & NTP
# ══════════════════════════════════════════════════════════

def connect_wifi():
    global wlan
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(WIFI_SSID, WIFI_PASSWORD)
    oled_msg("Connecting WiFi", WIFI_SSID[:16], "Please wait...")
    print("[WiFi] Connecting to", WIFI_SSID)
    for _ in range(40):
        if wlan.isconnected():
            break
        time.sleep_ms(500)
        print(".", end="")
    print()
    if not wlan.isconnected():
        print("[WiFi] FAILED!")
        oled_msg("WiFi FAILED!", "Check SSID/Pass", "Restarting...")
        beep_fail()
        time.sleep(3)
        machine.reset()
    ip = wlan.ifconfig()[0]
    print("[WiFi] Connected:", ip)
    oled_msg("WiFi Connected!", ip, "")
    time.sleep_ms(800)


def sync_ntp():
    import ntptime
    ntptime.host = NTP_SERVER
    print("[NTP] Syncing time...")
    for _ in range(15):
        try:
            ntptime.settime()
            print("[NTP] Synced:", timestamp())
            return
        except Exception:
            time.sleep_ms(500)
    print("[NTP] Sync failed — using internal clock")

# ══════════════════════════════════════════════════════════
#   SETUP
# ══════════════════════════════════════════════════════════

def setup():
    print("=" * 40)
    print("  SMART TOOL MANAGEMENT SYSTEM")
    print("  MicroPython + Firebase REST")
    print("=" * 40)

    oled_msg("Booting...", "Tool System", "Please wait")
    time.sleep_ms(800)

    # Step 1: WiFi
    connect_wifi()

    # Step 2: NTP time sync
    sync_ntp()

    # Step 3: Firebase auth
    oled_msg("Firebase...", "Authenticating", "Please wait")
    if not fb_authenticate():
        oled_msg("Firebase FAILED", "Check config.py", "Restarting...")
        beep_fail()
        time.sleep(3)
        machine.reset()

    print("[READY] System ready — waiting for scan")
    beep_welcome()
    oled_idle()

# ══════════════════════════════════════════════════════════
#   MAIN LOOP
# ══════════════════════════════════════════════════════════

def loop():
    global sys_state, worker_uid, worker_name, state_timer

    while True:

        # ──────────────────────────────────────────────────
        #  STATE: IDLE — waiting for worker ID card
        # ──────────────────────────────────────────────────
        if sys_state == STATE_IDLE:

            stat, tag_type = rfid.request(rfid.REQIDL)
            if stat != rfid.OK:
                time.sleep_ms(50)
                continue

            stat, raw_uid = rfid.anticoll()
            if stat != rfid.OK:
                time.sleep_ms(50)
                continue

            uid = rfid_uid(raw_uid)
            rfid.halt_a()
            print("[SCAN] ID Card UID:", uid)

            # Check Firebase token
            if not fb_id_token:
                oled_result(False, "Firebase offline", "Check WiFi")
                beep_fail()
                time.sleep_ms(3000)
                oled_idle()
                continue

            # Look up worker
            name = lookup_worker(uid)
            if not name:
                print("[WARN] Unknown card:", uid)
                oled_result(False, "Unknown ID card!", uid[:16])
                beep_alarm()
                log_alert("UID", {
                    "uid":       uid,
                    "timestamp": timestamp(),
                    "type":      "UNKNOWN_ID"
                })
                time.sleep_ms(3000)
                oled_idle()
                continue

            # Valid worker
            worker_uid  = uid
            worker_name = name
            sys_state   = STATE_ID_SCANNED
            state_timer = time.ticks_ms()
            beep_welcome()
            oled_welcome(name)
            print("[OK] Worker identified:", name)
            continue

        # ──────────────────────────────────────────────────
        #  STATE: ID_SCANNED — waiting for tool tag
        # ──────────────────────────────────────────────────
        if sys_state == STATE_ID_SCANNED:

            # 15 second timeout
            if time.ticks_diff(time.ticks_ms(), state_timer) > ID_TIMEOUT:
                print("[TIMEOUT] No tool scanned — resetting")
                oled_result(False, "Timed out!", "Scan ID again")
                beep_fail()
                sys_state   = STATE_IDLE
                worker_uid  = ""
                worker_name = ""
                time.sleep_ms(2000)
                oled_idle()
                continue

            stat, tag_type = rfid.request(rfid.REQIDL)
            if stat != rfid.OK:
                time.sleep_ms(50)
                continue

            stat, raw_uid = rfid.anticoll()
            if stat != rfid.OK:
                time.sleep_ms(50)
                continue

            t_uid = rfid_uid(raw_uid)
            rfid.halt_a()
            print("[SCAN] Tool UID:", t_uid)

            # Guard: worker re-scanned own ID card
            if t_uid == worker_uid:
                oled_result(False, "That is your ID!", "Scan a tool tag")
                beep_fail()
                time.sleep_ms(2000)
                oled_welcome(worker_name)
                continue

            sys_state = STATE_PROCESSING
            oled_msg("Processing...", "Please wait...", "")

            # Look up tool name
            t_name = lookup_tool(t_uid)
            if not t_name:
                print("[WARN] Unknown tool tag:", t_uid)
                oled_result(False, "Unknown tool!", "Register first")
                beep_alarm()
                log_alert("TOOL", {
                    "uid":        t_uid,
                    "scannedBy":  worker_uid,
                    "workerName": worker_name,
                    "timestamp":  timestamp(),
                    "type":       "UNKNOWN_TOOL"
                })
                time.sleep_ms(3000)
                sys_state   = STATE_IDLE
                worker_uid  = ""
                worker_name = ""
                oled_idle()
                continue

            # ── Auto-detect: borrow or return? ─────────────
            borrowed_by = tool_borrowed_by(t_uid)

            if borrowed_by == "":
                # ── CHECKOUT ─────────────────────────────
                print("[ACTION] CHECKOUT:", worker_name, "->", t_name)
                if do_checkout(worker_uid, worker_name, t_uid, t_name):
                    oled_result(True, "CHECKED OUT", t_name[:16])
                    beep_ok()
                else:
                    print("[ERROR] Checkout failed")
                    oled_result(False, "DB write error", "Try again")
                    beep_fail()

            elif borrowed_by == worker_uid:
                # ── RETURN ───────────────────────────────
                print("[ACTION] RETURN:", worker_name, "returned", t_name)
                if do_return(worker_uid, worker_name, t_uid, t_name):
                    oled_result(True, "RETURNED!", t_name[:16])
                    beep_ok()
                else:
                    print("[ERROR] Return failed")
                    oled_result(False, "DB write error", "Try again")
                    beep_fail()

            else:
                # ── CONFLICT: tool with someone else ─────
                other = lookup_worker(borrowed_by) or borrowed_by
                print("[CONFLICT]", t_name, "is with", other)
                oled_result(False, "Tool held by:", other[:16])
                beep_alarm()
                log_alert("CONF", {
                    "toolUID":           t_uid,
                    "toolName":          t_name,
                    "attemptedByUID":    worker_uid,
                    "attemptedByName":   worker_name,
                    "currentlyWithUID":  borrowed_by,
                    "currentlyWithName": other,
                    "timestamp":         timestamp(),
                    "type":              "CONFLICT"
                })
                time.sleep_ms(3000)
                sys_state   = STATE_IDLE
                worker_uid  = ""
                worker_name = ""
                oled_idle()
                continue

            # Reset after result display
            time.sleep_ms(MSG_SHOW)
            sys_state   = STATE_IDLE
            worker_uid  = ""
            worker_name = ""
            oled_idle()

# ══════════════════════════════════════════════════════════
#   ENTRY POINT
# ══════════════════════════════════════════════════════════
setup()
loop()
