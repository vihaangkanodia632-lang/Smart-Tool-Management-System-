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

FIREBASE_HOST    = "YOUR_FIREBASE_HOST" # ← host you created in Firebase
FIREBASE_API_KEY = "YOUR_FIREBASE_API_KEY" # ← API key you created in Firebase 
FIREBASE_EMAIL   = "YOUR_AUTH_EMAIL"    # ← email you created in Firebase Auth
FIREBASE_PASS    = "YOUR_AUTH_PASSWORD" # ← that email's password
