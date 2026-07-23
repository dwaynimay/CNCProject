/**
 * alertEngine.js — Logika deteksi kondisi alarm + anti-spam.
 *
 * Prinsip:
 *  - Edge-trigger: kirim notifikasi SEKALI saat masuk kondisi alarm,
 *    kirim notifikasi recovery saat pulih.
 *  - Cooldown: cegah alert jenis sama dikirim berulang dalam waktu singkat.
 *  - Offline watcher: cek berkala apakah device berhenti mengirim telemetri.
 */
require('dotenv').config();
const telegram = require('./telegramService');
const { logAlert } = require('../db');

const OFFLINE_TIMEOUT_MS = (parseInt(process.env.ALERT_OFFLINE_TIMEOUT) || 60)  * 1000;
const COOLDOWN_MS        = (parseInt(process.env.ALERT_COOLDOWN)        || 300) * 1000;
const MANUAL_WINDOW_MS   = 5000; // durasi tunggu telemetri konfirmasi setelah command manual dari dashboard

// Konfigurasi grup sensor yang dipantau. Menambah grup sensor baru
// (misal 'vibration') = tambah satu entri di sini, TIDAK perlu kode baru,
// selama firmware mengirim data[key] sebagai array [{id, alarm, [valueField]}].
const SENSOR_GROUPS = [
    { key: 'current', type: 'overcurrent', valueField: 'a', unit: 'A',  icon: '⚡',  label: 'OVERCURRENT', recoveryText: 'Arus kembali normal.' },
    { key: 'temp',     type: 'overtemp',    valueField: 'c', unit: '°C', icon: '🌡️', label: 'OVERTEMP',    recoveryText: 'Suhu kembali normal.' },
];

// state[deviceId] = { lastSeen, lastRelayOn, manualUntil, manualAction, active: { [type]: { on, lastSentAt } } }
const state = {};

function ensure(deviceId) {
    if (!state[deviceId]) {
        state[deviceId] = {
            lastSeen:    Date.now(),
            lastRelayOn: undefined,
            manualUntil: 0,
            manualAction: null,
            active: {},
        };
    }
    return state[deviceId];
}

/** Tandai bahwa command relay manual baru saja dikirim dari dashboard. */
function markManualCommand(deviceId, action) {
    const st = ensure(deviceId);
    st.manualUntil  = Date.now() + MANUAL_WINDOW_MS;
    st.manualAction = action;
}

function nowStr() {
    return new Date().toLocaleString('id-ID', { hour12: false });
}

/** Kirim + catat satu alert ke Telegram & database. */
function dispatch(deviceId, type, severity, message) {
    try {
        logAlert(deviceId, type, severity, message);
    } catch (e) {
        console.error('[DB] Gagal simpan alert:', e.message);
    }
    telegram.sendMessage(message).then(r => {
        if (r.ok)          console.log(`[Alert] → Telegram terkirim: ${type} (${deviceId})`);
        else if (r.skipped) console.log(`[Alert] (Telegram nonaktif) ${type} (${deviceId})`);
    });
}

/**
 * Transisi state satu jenis alert.
 * @param {boolean} shouldBeActive  Apakah kondisi alarm sedang aktif.
 * @param {string}  raiseMsg        Pesan saat masuk alarm.
 * @param {string}  recoveryMsg     Pesan saat pulih (opsional).
 */
function transition(deviceId, type, severity, shouldBeActive, raiseMsg, recoveryMsg) {
    const st  = ensure(deviceId);
    const cur = st.active[type] || { on: false, lastSentAt: 0 };
    const now = Date.now();

    if (shouldBeActive && !cur.on) {
        // Masuk kondisi alarm (off → on)
        if (now - cur.lastSentAt >= COOLDOWN_MS) {
            dispatch(deviceId, type, severity, raiseMsg);
            cur.lastSentAt = now;
        }
        cur.on = true;
    } else if (!shouldBeActive && cur.on) {
        // Pulih (on → off)
        if (recoveryMsg) dispatch(deviceId, type, 'info', recoveryMsg);
        cur.on = false;
    }

    st.active[type] = cur;
}

/** Cek satu grup sensor (arus/suhu/dst) berdasarkan konfigurasi SENSOR_GROUPS. */
function checkSensorGroup(deviceId, group, data) {
    const sensors = Array.isArray(data[group.key]) ? data[group.key] : [];
    const alarms  = sensors
        .map(s => (s && s.alarm ? { name: (s.id || '?').replace(/_/g, ' '), v: s[group.valueField] } : null))
        .filter(Boolean);

    transition(
        deviceId, group.type, 'critical',
        alarms.length > 0,
        `${group.icon} *${group.label} ALERT*\nDevice: \`${deviceId}\`\n` +
            alarms.map(s => `${s.name}: *${s.v} ${group.unit}* (limit terlampaui)`).join('\n') +
            `\n🕒 ${nowStr()}`,
        `✅ *${group.label} PULIH*\nDevice: \`${deviceId}\`\n${group.recoveryText}\n🕒 ${nowStr()}`
    );
}

// ── Pengecekan telemetri ────────────────────────────────────────────────────
function checkTelemetry(deviceId, data) {
    const st = ensure(deviceId);
    st.lastSeen = Date.now();

    // Jika device sempat offline lalu kirim data lagi → recovery online
    transition(
        deviceId, 'offline', 'info',
        false,
        null,
        `✅ *DEVICE ONLINE*\nDevice: \`${deviceId}\`\nTelemetri kembali diterima.\n🕒 ${nowStr()}`
    );

    SENSOR_GROUPS.forEach(group => checkSensorGroup(deviceId, group, data));

    // ── Relay trip (otomatis proteksi) vs kontrol manual dari dashboard ──
    // relayOn === true berarti relay memutus daya (mesin mati).
    if (typeof data.relayOn === 'boolean') {
        const isManual = st.manualUntil > Date.now();

        if (isManual) {
            transition(
                deviceId, 'relay_manual', 'info',
                data.relayOn === true,
                `🖐️ *KONTROL MANUAL*\nDevice: \`${deviceId}\`\nMesin dimatikan lewat dashboard.\n🕒 ${nowStr()}`,
                `🖐️ *KONTROL MANUAL*\nDevice: \`${deviceId}\`\nMesin dinyalakan lewat dashboard.\n🕒 ${nowStr()}`
            );
        } else {
            transition(
                deviceId, 'relay_trip', 'critical',
                data.relayOn === true,
                `🔌 *RELAY TRIP — MESIN MATI*\nDevice: \`${deviceId}\`\nProteksi memutus daya mesin.\n🕒 ${nowStr()}`,
                `✅ *MESIN AKTIF KEMBALI*\nDevice: \`${deviceId}\`\nRelay tersambung kembali.\n🕒 ${nowStr()}`
            );
        }
        st.lastRelayOn = data.relayOn;
    }
}

// ── Pengecekan hasil self-test ──────────────────────────────────────────────
function checkSelfTest(deviceId, data) {
    ensure(deviceId);
    const overall = String(data.overall || '').toUpperCase();
    const passed  = overall === 'PASS' || overall === 'OK';

    if (!passed) {
        const failed = Array.isArray(data.checks)
            ? data.checks.filter(c => c && String(c.status).toUpperCase() !== 'PASS' && String(c.status).toUpperCase() !== 'OK')
            : [];
        dispatch(
            deviceId, 'selftest_fail', 'critical',
            `🔴 *SELF-TEST GAGAL*\nDevice: \`${deviceId}\`\nHasil: *${overall || 'UNKNOWN'}*\n` +
                (failed.length ? failed.map(c => `❌ ${c.name || c.check || '?'}: ${c.status}`).join('\n') + '\n' : '') +
                `🕒 ${nowStr()}`
        );
    }
}

// ── Offline watcher ─────────────────────────────────────────────────────────
let watcherHandle = null;
function startOfflineWatcher(intervalMs = 15000) {
    if (watcherHandle) return;
    watcherHandle = setInterval(() => {
        const now = Date.now();
        for (const deviceId of Object.keys(state)) {
            const st = state[deviceId];
            const offline = now - st.lastSeen > OFFLINE_TIMEOUT_MS;
            transition(
                deviceId, 'offline', 'warning',
                offline,
                `📴 *DEVICE OFFLINE*\nDevice: \`${deviceId}\`\nTidak ada telemetri > ${Math.round(OFFLINE_TIMEOUT_MS / 1000)} detik.\n🕒 ${nowStr()}`,
                null   // recovery ditangani oleh checkTelemetry saat data masuk lagi
            );
        }
    }, intervalMs);
    console.log(`[Alert] Offline watcher aktif (timeout ${OFFLINE_TIMEOUT_MS / 1000}s, cek tiap ${intervalMs / 1000}s)`);
}

function stopOfflineWatcher() {
    if (watcherHandle) { clearInterval(watcherHandle); watcherHandle = null; }
}

module.exports = {
    checkTelemetry,
    checkSelfTest,
    startOfflineWatcher,
    stopOfflineWatcher,
    markManualCommand,
};
