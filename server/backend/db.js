/**
 * db.js — Inisialisasi database SQLite untuk CNC IoT Monitor
 * Tabel: users, telemetry, commands_log
 */
require('dotenv').config();
const Database = require('better-sqlite3');
const bcrypt   = require('bcryptjs');
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
  CREATE TABLE IF NOT EXISTS users (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    username   TEXT UNIQUE NOT NULL,
    password   TEXT NOT NULL,
    role       TEXT NOT NULL DEFAULT 'operator',
    created_at DATETIME DEFAULT (datetime('now', 'localtime')),
    last_login DATETIME
  );

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
    sent_by    TEXT DEFAULT 'system',
    sent_at    DATETIME DEFAULT (datetime('now', 'localtime'))
  );
`);

// ── Seed admin default jika belum ada ─────────────────────────────────────
const existing = db.prepare('SELECT id FROM users WHERE username = ?').get('admin');
if (!existing) {
    const hash = bcrypt.hashSync('admin123', 12);
    db.prepare('INSERT INTO users (username, password, role) VALUES (?, ?, ?)').run('admin', hash, 'admin');
    console.log('[DB] ✓ User admin default dibuat (password: admin123) — segera ganti setelah login!');
}

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
function logCommand(deviceId, command, sentBy = 'system') {
    return stmtLogCmd.run(deviceId, command, sentBy);
}

/** Ambil user berdasarkan username */
const stmtGetUser = db.prepare('SELECT * FROM users WHERE username = ?');
function getUserByUsername(username) {
    return stmtGetUser.get(username);
}

/** Update last_login */
const stmtUpdateLogin = db.prepare(
    "UPDATE users SET last_login = datetime('now', 'localtime') WHERE id = ?"
);
function updateLastLogin(userId) {
    return stmtUpdateLogin.run(userId);
}

/** Ganti password user */
const stmtChangePassword = db.prepare('UPDATE users SET password = ? WHERE id = ?');
function changePassword(userId, newPassword) {
    const hash = bcrypt.hashSync(newPassword, 12);
    return stmtChangePassword.run(hash, userId);
}

module.exports = {
    db,
    insertTelemetry,
    getHistory,
    logCommand,
    getUserByUsername,
    updateLastLogin,
    changePassword,
};
