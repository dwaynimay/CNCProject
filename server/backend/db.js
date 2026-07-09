/**
 * db.js — Inisialisasi database SQLite untuk CNC IoT Monitor
 * Tabel: telemetry, commands_log
 */
require('dotenv').config();
const Database = require('better-sqlite3');
const path     = require('path');

const DB_PATH = process.env.DB_PATH || path.join(__dirname, 'data', 'cnc_iot.db');

// Pastikan direktori data ada
const fs = require('fs');
const dir = path.dirname(DB_PATH);
if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });

const db = new Database(DB_PATH);

// Aktifkan WAL mode untuk performa lebih baik
db.pragma('journal_mode = WAL');
db.pragma('foreign_keys = ON');

// ── Buat tabel ────────────────────────────────────────────────────────────
db.exec(`
  CREATE TABLE IF NOT EXISTS telemetry (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id   TEXT NOT NULL,
    payload     TEXT NOT NULL,
    recorded_at DATETIME DEFAULT (datetime('now', 'localtime'))
  );

  CREATE INDEX IF NOT EXISTS idx_telemetry_device ON telemetry(device_id);
  CREATE INDEX IF NOT EXISTS idx_telemetry_time   ON telemetry(recorded_at);

  CREATE TABLE IF NOT EXISTS commands_log (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id  TEXT NOT NULL,
    command    TEXT NOT NULL,
    sent_by    TEXT DEFAULT 'dashboard',
    sent_at    DATETIME DEFAULT (datetime('now', 'localtime'))
  );

  CREATE TABLE IF NOT EXISTS selftest_results (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id   TEXT NOT NULL,
    overall     TEXT NOT NULL,
    checks      TEXT NOT NULL,
    recorded_at DATETIME DEFAULT (datetime('now', 'localtime'))
  );

  CREATE INDEX IF NOT EXISTS idx_selftest_device ON selftest_results(device_id);
`);

console.log(`[DB] ✓ SQLite terhubung: ${DB_PATH}`);

// ── Helper queries ─────────────────────────────────────────────────────────

/** Simpan satu baris telemetri */
const stmtInsertTelemetry = db.prepare(
    'INSERT INTO telemetry (device_id, payload) VALUES (?, ?)'
);
function insertTelemetry(deviceId, payload) {
    return stmtInsertTelemetry.run(deviceId, JSON.stringify(payload));
}

/** Ambil histori telemetri terbaru (default 200 baris) */
const stmtGetHistory = db.prepare(
    'SELECT id, device_id, payload, recorded_at FROM telemetry WHERE device_id = ? ORDER BY recorded_at DESC LIMIT ?'
);
function getHistory(deviceId, limit = 200) {
    return stmtGetHistory.all(deviceId, limit).map(row => ({
        id:         row.id,
        deviceId:   row.device_id,
        data:       JSON.parse(row.payload),
        recordedAt: row.recorded_at,
    }));
}

/** Simpan log command */
const stmtLogCmd = db.prepare(
    'INSERT INTO commands_log (device_id, command, sent_by) VALUES (?, ?, ?)'
);
function logCommand(deviceId, command, sentBy = 'dashboard') {
    return stmtLogCmd.run(deviceId, command, sentBy);
}

/** Simpan satu hasil self-test (payload = { ts, overall, checks: [...] }) */
const stmtInsertSelfTest = db.prepare(
    'INSERT INTO selftest_results (device_id, overall, checks) VALUES (?, ?, ?)'
);
function insertSelfTestResult(deviceId, payload) {
    return stmtInsertSelfTest.run(deviceId, payload.overall, JSON.stringify(payload.checks || []));
}

/** Ambil histori hasil self-test terbaru (default 20 baris) */
const stmtGetSelfTestHistory = db.prepare(
    'SELECT id, device_id, overall, checks, recorded_at FROM selftest_results WHERE device_id = ? ORDER BY recorded_at DESC LIMIT ?'
);
function getSelfTestHistory(deviceId, limit = 20) {
    return stmtGetSelfTestHistory.all(deviceId, limit).map(row => ({
        id:         row.id,
        deviceId:   row.device_id,
        overall:    row.overall,
        checks:     JSON.parse(row.checks),
        recordedAt: row.recorded_at,
    }));
}

module.exports = {
    db,
    insertTelemetry,
    getHistory,
    logCommand,
    insertSelfTestResult,
    getSelfTestHistory,
};
