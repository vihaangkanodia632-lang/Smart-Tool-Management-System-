import { initializeApp } from "https://www.gstatic.com/firebasejs/9.23.0/firebase-app.js";
import {
  getAuth,
  signInWithEmailAndPassword,
  onAuthStateChanged,
  signOut
} from "https://www.gstatic.com/firebasejs/9.23.0/firebase-auth.js";
import {
  getDatabase,
  ref,
  set,
  push,
  update,
  remove,
  onValue,
  get
} from "https://www.gstatic.com/firebasejs/9.23.0/firebase-database.js";

// ══════════════════════════════════════════
//   ★  REPLACE WITH YOUR FIREBASE CONFIG  ★
// ══════════════════════════════════════════
const firebaseConfig = {
  apiKey: "AIzaSyCUszk8Y0jZ77AY4VR_qrd5uxsdCo8jxLQ",
  authDomain: "esp32-22be8.firebaseapp.com",
  databaseURL: "https://esp32-22be8-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "esp32-22be8",
  storageBucket: "esp32-22be8.firebasestorage.app",
  messagingSenderId: "284941248840",
  appId: "1:284941248840:web:f4ddedcc8c0db0b7e26b7d"
};
// ══════════════════════════════════════════

const app  = initializeApp(firebaseConfig);
const auth = getAuth();
const db   = getDatabase(app);

// ── UI Elements ──────────────────────────────────────────
const authBox    = document.getElementById("authBox");
const mainApp    = document.getElementById("mainApp");
const loginBtn   = document.getElementById("loginBtn");
const logoutBtn  = document.getElementById("logoutBtn");
const authMsg    = document.getElementById("authMsg");
const badge      = document.getElementById("statusBadge");
const userEmail  = document.getElementById("userEmail");

// Tabs
const tabBtns    = document.querySelectorAll(".tab-btn");
const tabPanes   = document.querySelectorAll(".tab-pane");

// Stats
const statBorrowed = document.getElementById("statBorrowed");
const statAvail    = document.getElementById("statAvail");
const statTotal    = document.getElementById("statTotal");
const statOverdue  = document.getElementById("statOverdue");
const overdueBar   = document.getElementById("overdueBar");
const overdueText  = document.getElementById("overdueText");

// Tables
const loansBody   = document.getElementById("loansBody");
const toolsBody   = document.getElementById("toolsBody");
const histBody    = document.getElementById("histBody");
const alertsBody  = document.getElementById("alertsBody");

// Admin forms
const wUID   = document.getElementById("wUID");
const wName  = document.getElementById("wName");
const wEmpID = document.getElementById("wEmpID");
const wDept  = document.getElementById("wDept");
const regWorkerBtn = document.getElementById("regWorkerBtn");

const tUID    = document.getElementById("tUID");
const tName   = document.getElementById("tName");
const tCat    = document.getElementById("tCat");
const tSerial = document.getElementById("tSerial");
const regToolBtn = document.getElementById("regToolBtn");

const workerListBody = document.getElementById("workerListBody");
const toolListBody   = document.getElementById("toolListBody");
const clearAlertsBtn = document.getElementById("clearAlertsBtn");

// Search inputs
const srchLoans = document.getElementById("srchLoans");
const srchTools = document.getElementById("srchTools");
const srchHist  = document.getElementById("srchHist");
const histFilter = document.getElementById("histFilter");

const OVERDUE_HOURS = 8;

// ── Global Data ───────────────────────────────────────────
let allTools   = {};
let allWorkers = {};
let allLoans   = {};
let allHistory = {};
let allAlerts  = {};
let dbListeners = [];

// ══════════════════════════════════════════════════════════
//   AUTH
// ══════════════════════════════════════════════════════════

loginBtn.onclick = async () => {
  authMsg.textContent = "";
  loginBtn.textContent = "Signing in...";
  loginBtn.disabled = true;
  try {
    await signInWithEmailAndPassword(
      auth,
      document.getElementById("emailField").value,
      document.getElementById("passwordField").value
    );
  } catch (e) {
    authMsg.textContent = friendlyError(e.code);
    loginBtn.textContent = "Sign In";
    loginBtn.disabled = false;
  }
};

logoutBtn.onclick = () => {
  dbListeners.forEach(unsub => unsub());
  dbListeners = [];
  signOut(auth);
};

onAuthStateChanged(auth, (user) => {
  if (user) {
    authBox.style.display  = "none";
    mainApp.style.display  = "block";
    badge.className = "status-badge online";
    badge.textContent = "● Live";
    userEmail.textContent = user.email;
    loginBtn.textContent = "Sign In";
    loginBtn.disabled = false;
    startListeners();
    startClock();
  } else {
    authBox.style.display  = "block";
    mainApp.style.display  = "none";
    badge.className = "status-badge offline";
    badge.textContent = "● Offline";
  }
});

function friendlyError(code) {
  const map = {
    "auth/wrong-password":      "Incorrect password.",
    "auth/user-not-found":      "No account with that email.",
    "auth/invalid-email":       "Invalid email address.",
    "auth/too-many-requests":   "Too many attempts. Try later.",
    "auth/network-request-failed": "Network error. Check connection."
  };
  return map[code] || "Login failed. Check credentials.";
}

// ══════════════════════════════════════════════════════════
//   REALTIME LISTENERS
// ══════════════════════════════════════════════════════════

function startListeners() {
  const listen = (path, cb) => {
    const dbRef = ref(db, path);
    const unsub = onValue(dbRef, cb);
    dbListeners.push(unsub);
  };

  listen("/tools", snap => {
    allTools = snap.val() || {};
    renderToolsTable();
    renderLoansTable();
    updateStats();
    renderAdminToolList();
  });

  listen("/workers", snap => {
    allWorkers = snap.val() || {};
    renderLoansTable();
    renderAdminWorkerList();
  });

  listen("/activeLoans", snap => {
    allLoans = snap.val() || {};
    renderLoansTable();
    updateStats();
    updateLastSync();
  });

  listen("/history", snap => {
    allHistory = snap.val() || {};
    renderHistory();
  });

  listen("/alerts", snap => {
    allAlerts = snap.val() || {};
    renderAlerts();
  });
}

// ══════════════════════════════════════════════════════════
//   TABS
// ══════════════════════════════════════════════════════════

tabBtns.forEach(btn => {
  btn.onclick = () => {
    tabBtns.forEach(b => b.classList.remove("active"));
    tabPanes.forEach(p => p.classList.remove("active"));
    btn.classList.add("active");
    document.getElementById("pane-" + btn.dataset.tab).classList.add("active");
  };
});

// ══════════════════════════════════════════════════════════
//   STATS
// ══════════════════════════════════════════════════════════

function updateStats() {
  const arr      = Object.values(allTools);
  const borrowed = arr.filter(t => t.status === "borrowed").length;
  const avail    = arr.filter(t => t.status !== "borrowed").length;
  let overdueCount = 0;

  for (const wLoans of Object.values(allLoans))
    for (const loan of Object.values(wLoans))
      if (isOverdue(loan.checkedOut)) overdueCount++;

  animateCount(statBorrowed, borrowed);
  animateCount(statAvail,    avail);
  animateCount(statTotal,    arr.length);
  animateCount(statOverdue,  overdueCount);

  if (overdueCount > 0) {
    overdueBar.classList.remove("hidden");
    overdueText.textContent =
      overdueCount + " tool(s) have been out for more than " +
      OVERDUE_HOURS + " hours — follow up required.";
  } else {
    overdueBar.classList.add("hidden");
  }
}

function animateCount(el, target) {
  if (!el) return;
  const start = parseInt(el.textContent) || 0;
  if (start === target) return;
  let step = 0;
  const steps = 12;
  const timer = setInterval(() => {
    step++;
    el.textContent = Math.round(start + (target - start) * (step / steps));
    if (step >= steps) { el.textContent = target; clearInterval(timer); }
  }, 30);
}

// ══════════════════════════════════════════════════════════
//   LOANS TABLE
// ══════════════════════════════════════════════════════════

function renderLoansTable() {
  if (!loansBody) return;
  let rows = "";
  for (const [wUID, wLoans] of Object.entries(allLoans)) {
    for (const [tUID, loan] of Object.entries(wLoans)) {
      const od  = isOverdue(loan.checkedOut);
      const dur = getDuration(loan.checkedOut);
      const cat = allTools[tUID]?.category || "—";
      rows += `
        <tr class="${od ? "overdue-row" : ""}">
          <td><span class="worker-name">${esc(loan.workerName || wUID)}</span></td>
          <td>${esc(loan.toolName || tUID)}</td>
          <td><span class="badge b-blue">${esc(cat)}</span></td>
          <td class="small-text">${esc(loan.checkedOut || "—")}</td>
          <td><span class="mono ${od ? "text-red" : "text-amber"}">${dur}</span></td>
          <td>${od
            ? '<span class="badge b-red">⚑ OVERDUE</span>'
            : '<span class="badge b-amber">Borrowed</span>'}</td>
        </tr>`;
    }
  }
  loansBody.innerHTML = rows ||
    `<tr><td colspan="6" class="empty-cell">✓ No tools currently borrowed</td></tr>`;
}

// ══════════════════════════════════════════════════════════
//   TOOLS TABLE
// ══════════════════════════════════════════════════════════

function renderToolsTable() {
  if (!toolsBody) return;
  const q = (srchTools?.value || "").toLowerCase();
  let rows = "";
  for (const [uid, t] of Object.entries(allTools)) {
    if (q && !JSON.stringify(t).toLowerCase().includes(q)) continue;
    const borrowed = t.status === "borrowed";
    const who = borrowed
      ? (allWorkers[t.borrowedBy]?.name || t.borrowedBy || "—")
      : "—";
    rows += `
      <tr>
        <td><strong>${esc(t.name || uid)}</strong></td>
        <td><span class="badge b-blue">${esc(t.category || "—")}</span></td>
        <td class="mono small-text">${esc(t.serialNumber || "—")}</td>
        <td class="mono small-text uid-cell">${uid}</td>
        <td>${borrowed
          ? '<span class="badge b-amber">Borrowed</span>'
          : '<span class="badge b-green">Available</span>'}</td>
        <td>${esc(who)}</td>
        <td class="small-text muted">${esc(t.borrowedAt || "—")}</td>
      </tr>`;
  }
  toolsBody.innerHTML = rows ||
    `<tr><td colspan="7" class="empty-cell">No tools registered yet</td></tr>`;
}

// ══════════════════════════════════════════════════════════
//   HISTORY
// ══════════════════════════════════════════════════════════

function renderHistory() {
  if (!histBody) return;
  const filter = histFilter?.value || "all";
  const q      = (srchHist?.value || "").toLowerCase();
  const entries = Object.entries(allHistory)
    .sort(([a], [b]) => b.localeCompare(a));
  let rows = "";
  for (const [, ev] of entries) {
    if (filter !== "all" && ev.type !== filter) continue;
    if (q && !JSON.stringify(ev).toLowerCase().includes(q)) continue;
    const isOut = ev.type === "CHECKOUT";
    rows += `
      <tr>
        <td>${isOut
          ? '<span class="badge b-amber">↑ Checkout</span>'
          : '<span class="badge b-green">↓ Return</span>'}</td>
        <td>${esc(ev.workerName || ev.workerUID || "—")}</td>
        <td>${esc(ev.toolName   || ev.toolUID   || "—")}</td>
        <td class="small-text muted">${esc(ev.timestamp || "—")}</td>
      </tr>`;
  }
  histBody.innerHTML = rows ||
    `<tr><td colspan="4" class="empty-cell">No history yet</td></tr>`;
}

// ══════════════════════════════════════════════════════════
//   ALERTS
// ══════════════════════════════════════════════════════════

function renderAlerts() {
  if (!alertsBody) return;
  const entries = Object.entries(allAlerts)
    .sort(([a], [b]) => b.localeCompare(a));

  // Update alert badge on tab
  const pip = document.getElementById("alertPip");
  if (pip) {
    pip.textContent = entries.length;
    pip.style.display = entries.length ? "inline" : "none";
  }

  let rows = "";
  for (const [, al] of entries) {
    let badge = "", detail = "";
    if (al.type === "UNKNOWN_ID") {
      badge  = '<span class="badge b-amber">Unknown ID</span>';
      detail = "UID: <span class='mono'>" + esc(al.uid || "—") + "</span>";
    } else if (al.type === "UNKNOWN_TOOL") {
      badge  = '<span class="badge b-amber">Unknown Tool</span>';
      detail = "Tag: <span class='mono'>" + esc(al.uid || "—") + "</span>";
    } else if (al.type === "CONFLICT") {
      badge  = '<span class="badge b-red">Conflict</span>';
      detail = '"' + esc(al.toolName) + '" already held by ' + esc(al.currentlyWithName);
    } else {
      badge  = '<span class="badge b-muted">' + esc(al.type) + '</span>';
      detail = "—";
    }
    rows += `
      <tr>
        <td>${badge}</td>
        <td class="small-text">${detail}</td>
        <td>${esc(al.workerName || al.attemptedByName || "—")}</td>
        <td class="small-text muted">${esc(al.timestamp || "—")}</td>
      </tr>`;
  }
  alertsBody.innerHTML = rows ||
    `<tr><td colspan="4" class="empty-cell">✓ No alerts</td></tr>`;
}

// ══════════════════════════════════════════════════════════
//   ADMIN — REGISTER
// ══════════════════════════════════════════════════════════

if (regWorkerBtn) {
  regWorkerBtn.onclick = async () => {
    const uid  = wUID?.value.trim().toUpperCase();
    const name = wName?.value.trim();
    const emp  = wEmpID?.value.trim();
    const dept = wDept?.value.trim();
    if (!uid || !name) return showToast("UID and Name are required", "error");

    regWorkerBtn.textContent = "Registering...";
    regWorkerBtn.disabled = true;
    try {
      await set(ref(db, "/workers/" + uid), {
        name, employeeId: emp, department: dept,
        registeredAt: nowString()
      });
      showToast("Worker registered: " + name, "success");
      [wUID, wName, wEmpID, wDept].forEach(el => { if (el) el.value = ""; });
    } catch (e) {
      showToast("Error: " + e.message, "error");
    }
    regWorkerBtn.textContent = "Register Worker";
    regWorkerBtn.disabled = false;
  };
}

if (regToolBtn) {
  regToolBtn.onclick = async () => {
    const uid    = tUID?.value.trim().toUpperCase();
    const name   = tName?.value.trim();
    const cat    = tCat?.value;
    const serial = tSerial?.value.trim();
    if (!uid || !name) return showToast("UID and Name are required", "error");

    regToolBtn.textContent = "Registering...";
    regToolBtn.disabled = true;
    try {
      await set(ref(db, "/tools/" + uid), {
        name, category: cat, serialNumber: serial,
        status: "available", borrowedBy: "",
        borrowedAt: "", borrowedEpoch: 0,
        registeredAt: nowString()
      });
      showToast("Tool registered: " + name, "success");
      [tUID, tName, tSerial].forEach(el => { if (el) el.value = ""; });
    } catch (e) {
      showToast("Error: " + e.message, "error");
    }
    regToolBtn.textContent = "Register Tool";
    regToolBtn.disabled = false;
  };
}

// ── Admin Lists ───────────────────────────────────────────

function renderAdminWorkerList() {
  if (!workerListBody) return;
  let rows = "";
  for (const [uid, w] of Object.entries(allWorkers)) {
    rows += `
      <tr>
        <td>${esc(w.name)}</td>
        <td class="mono small-text">${esc(w.employeeId || "—")}</td>
        <td>${esc(w.department || "—")}</td>
        <td>
          <button class="del-btn" onclick="deleteWorker('${uid}')">✕</button>
        </td>
      </tr>`;
  }
  workerListBody.innerHTML = rows ||
    `<tr><td colspan="4" class="empty-cell">No workers registered</td></tr>`;
}

function renderAdminToolList() {
  if (!toolListBody) return;
  let rows = "";
  for (const [uid, t] of Object.entries(allTools)) {
    rows += `
      <tr>
        <td>${esc(t.name)}</td>
        <td><span class="badge b-blue" style="font-size:10px">${esc(t.category || "—")}</span></td>
        <td class="mono small-text">${esc(t.serialNumber || "—")}</td>
        <td>
          <button class="del-btn" onclick="deleteTool('${uid}')">✕</button>
        </td>
      </tr>`;
  }
  toolListBody.innerHTML = rows ||
    `<tr><td colspan="4" class="empty-cell">No tools registered</td></tr>`;
}

// ── Delete handlers (exposed globally for inline onclick) ─
window.deleteWorker = async (uid) => {
  if (!confirm("Delete worker with UID " + uid + "?")) return;
  await remove(ref(db, "/workers/" + uid));
  showToast("Worker removed", "success");
};

window.deleteTool = async (uid) => {
  if (!confirm("Delete tool with UID " + uid + "?")) return;
  await remove(ref(db, "/tools/" + uid));
  showToast("Tool removed", "success");
};

// ── Clear alerts ──────────────────────────────────────────
if (clearAlertsBtn) {
  clearAlertsBtn.onclick = async () => {
    if (!confirm("Clear all alerts?")) return;
    await remove(ref(db, "/alerts"));
    showToast("Alerts cleared", "success");
  };
}

// ── Search / Filter bindings ──────────────────────────────
srchLoans?.addEventListener("input", () => filterTableRows("loansBody", srchLoans.value));
srchTools?.addEventListener("input", renderToolsTable);
srchHist?.addEventListener("input",  renderHistory);
histFilter?.addEventListener("change", renderHistory);

function filterTableRows(tbodyId, q) {
  const tbody = document.getElementById(tbodyId);
  if (!tbody) return;
  tbody.querySelectorAll("tr").forEach(row => {
    row.style.display =
      row.textContent.toLowerCase().includes(q.toLowerCase()) ? "" : "none";
  });
}

// ══════════════════════════════════════════════════════════
//   HELPERS
// ══════════════════════════════════════════════════════════

function getDuration(since) {
  if (!since) return "—";
  try {
    const mo = {Jan:0,Feb:1,Mar:2,Apr:3,May:4,Jun:5,
                Jul:6,Aug:7,Sep:8,Oct:9,Nov:10,Dec:11};
    const p = since.match(/(\d+)-(\w+)-(\d+) (\d+):(\d+):(\d+)/);
    if (!p) return since;
    const d = new Date(+p[3], mo[p[2]], +p[1], +p[4], +p[5], +p[6]);
    const m = Math.floor((Date.now() - d) / 60000);
    if (m < 60)   return m + "m";
    if (m < 1440) return Math.floor(m / 60) + "h " + (m % 60) + "m";
    return Math.floor(m / 1440) + "d " + Math.floor((m % 1440) / 60) + "h";
  } catch { return "—"; }
}

function isOverdue(since) {
  if (!since) return false;
  try {
    const mo = {Jan:0,Feb:1,Mar:2,Apr:3,May:4,Jun:5,
                Jul:6,Aug:7,Sep:8,Oct:9,Nov:10,Dec:11};
    const p = since.match(/(\d+)-(\w+)-(\d+) (\d+):(\d+):(\d+)/);
    if (!p) return false;
    const d = new Date(+p[3], mo[p[2]], +p[1], +p[4], +p[5], +p[6]);
    return (Date.now() - d) / 3600000 > OVERDUE_HOURS;
  } catch { return false; }
}

function nowString() {
  return new Date().toLocaleString("en-IN", {
    day: "2-digit", month: "short", year: "numeric",
    hour: "2-digit", minute: "2-digit", second: "2-digit", hour12: false
  }).replace(",", "");
}

function esc(s) {
  return String(s)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;");
}

function showToast(msg, type = "success") {
  let toast = document.getElementById("toast");
  if (!toast) {
    toast = document.createElement("div");
    toast.id = "toast";
    document.body.appendChild(toast);
  }
  toast.textContent = msg;
  toast.className = "toast show toast-" + type;
  clearTimeout(toast._t);
  toast._t = setTimeout(() => toast.className = "toast", 3000);
}

function updateLastSync() {
  const el = document.getElementById("lastSync");
  if (el) el.textContent = "Synced " + new Date().toLocaleTimeString("en-IN", {hour12: false});
}

function startClock() {
  const el = document.getElementById("clockEl");
  if (!el) return;
  const tick = () => {
    el.textContent = new Date().toLocaleTimeString("en-IN", {hour12: false});
  };
  tick();
  setInterval(tick, 1000);
}

// Refresh durations every minute
setInterval(() => {
  renderLoansTable();
  updateStats();
}, 60000);
