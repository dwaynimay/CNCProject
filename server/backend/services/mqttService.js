const mqtt = require('mqtt');
const cfg  = require('../config/mqtt');
const { insertTelemetry, logCommand, insertSelfTestResult } = require('../db');
const alertEngine = require('./alertEngine');

let client;
let lastPayload = {};   // { deviceId: payload } — cache memori untuk akses cepat

function start(wss) {
    console.log(`[MQTT] Connecting to mqtt://${cfg.host}:${cfg.port} ...`);
    client = mqtt.connect(`mqtt://${cfg.host}:${cfg.port}`, {
        clientId: 'cnc-backend-' + Math.random().toString(16).slice(2, 8),
        reconnectPeriod: 3000,
        connectTimeout: 10000,
    });

    client.on('connect', () => {
        console.log(`[MQTT] ✓ Connected to ${cfg.host}:${cfg.port}`);
        client.subscribe([cfg.topicSub, cfg.topicSelfTest], (err) => {
            if (err) console.error('[MQTT] Subscribe error:', err.message);
            else     console.log(`[MQTT] Subscribed to: ${cfg.topicSub}, ${cfg.topicSelfTest}`);
        });
    });

    client.on('error',       (e) => console.error('[MQTT] Error:', e.message));
    client.on('reconnect',   ()  => console.log('[MQTT] Reconnecting...'));
    client.on('offline',     ()  => console.warn('[MQTT] Client offline'));
    client.on('disconnect',  ()  => console.warn('[MQTT] Disconnected'));

    client.on('message', (topic, message) => {
        const deviceId = topic.split('/')[1];
        console.log(`[MQTT] ← msg from ${deviceId} (${message.length} bytes)`);
        try {
            const data = JSON.parse(message.toString());

            if (topic.endsWith('/selftest_result')) {
                // ── Simpan hasil self-test ke database SQLite ──────────────
                try {
                    insertSelfTestResult(deviceId, data);
                } catch (dbErr) {
                    console.error('[DB] Gagal simpan hasil self-test:', dbErr.message);
                }

                alertEngine.checkSelfTest(deviceId, data);

                // ── Forward ke semua dashboard via WebSocket ───────────────
                const frame = JSON.stringify({ type: 'selftest_result', deviceId, data });
                let sent = 0;
                wss.clients.forEach(ws => { if (ws.readyState === 1) { ws.send(frame); sent++; } });
                console.log(`[WS]   → Forwarded selftest_result to ${sent} dashboard client(s)`);
                return;
            }

            lastPayload[deviceId] = { deviceId, ...data };

            // ── Simpan ke database SQLite ──────────────────────────────────
            try {
                insertTelemetry(deviceId, data);
            } catch (dbErr) {
                console.error('[DB] Gagal simpan telemetri:', dbErr.message);
            }

            alertEngine.checkTelemetry(deviceId, data);

            // ── Forward ke semua dashboard via WebSocket ───────────────────
            const frame = JSON.stringify({ type: 'telemetry', deviceId, data });
            let sent = 0;
            wss.clients.forEach(ws => { if (ws.readyState === 1) { ws.send(frame); sent++; } });
            console.log(`[WS]   → Forwarded to ${sent} dashboard client(s)`);
        } catch (e) {
            console.error('[MQTT] Parse error:', e.message);
        }
    });
}

function sendCommand(deviceId, payload, sentBy = 'system') {
    const topic = cfg.topicCmd.replace('%s', deviceId);
    const msg   = typeof payload === 'string' ? payload : JSON.stringify(payload);
    console.log(`[MQTT] → Publish to ${topic}: ${msg} (by: ${sentBy})`);
    client.publish(topic, msg);

    // ── Simpan log command ke database ────────────────────────────────────
    try {
        logCommand(deviceId, msg, sentBy);
    } catch (dbErr) {
        console.error('[DB] Gagal simpan command log:', dbErr.message);
    }
}

function getLatest(deviceId) {
    return deviceId ? lastPayload[deviceId] : lastPayload;
}

module.exports = { start, sendCommand, getLatest };
