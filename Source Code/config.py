# ╔══════════════════════════════════════════════════════════╗
#   config.py — Firebase + WiFi credentials
#   Save this file to the ROOT of your ESP32 via Thonny
#   (same folder as main.py)
# ╚══════════════════════════════════════════════════════════╝

# ── WiFi ──────────────────────────────────────────────────
WIFI_SSID     = "YOUR_WIFI_NAME"        # ← replace with your WiFi name
WIFI_PASSWORD = "YOUR_WIFI_PASSWORD"    # ← replace with your WiFi password

# ── Firebase — YOUR REAL PROJECT VALUES ───────────────────
# ⚠ FIREBASE_HOST must NOT start with https://
# ⚠ FIREBASE_HOST must NOT have a trailing slash /

FIREBASE_HOST    = "esp32-22be8-default-rtdb.asia-southeast1.firebasedatabase.app"
FIREBASE_API_KEY = "AIzaSyCUszk8Y0jZ77AY4VR_qrd5uxsdCo8jxLQ"
FIREBASE_EMAIL   = "YOUR_AUTH_EMAIL"    # ← email you created in Firebase Auth
FIREBASE_PASS    = "YOUR_AUTH_PASSWORD" # ← that email's password
