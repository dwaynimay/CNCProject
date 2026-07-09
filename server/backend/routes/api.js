const router  = require('express').Router();
const mqttSvc = require('../services/mqttService');
const { getHistory, getSelfTestHistory } = require('../db');

// GET  /api/latest/:id?  — data terbaru per device (live, dari memori)
router.get('/latest/:id?', (req, res) => {
    res.json(mqttSvc.getLatest(req.params.id));
});

// GET  /api/history/:id  — histori telemetri dari database
// Query: ?limit=200
router.get('/history/:id', (req, res) => {
    const { id }    = req.params;
    const limit     = Math.min(parseInt(req.query.limit) || 200, 1000);
    const rows      = getHistory(id, limit);
    res.json({ deviceId: id, count: rows.length, data: rows });
});

// POST /api/command/:id  — kirim perintah ke ESP32
// Body: { "cmd": "relay_on" | "relay_off" | "cal_save" | "cal_reset" }
router.post('/command/:id', (req, res) => {
    const { id }  = req.params;
    const { cmd } = req.body;
    if (!cmd) return res.status(400).json({ error: 'cmd required' });
    mqttSvc.sendCommand(id, cmd, 'dashboard');
    res.json({ ok: true, sent: cmd });
});

// POST /api/calibrate/:id/offset — kalibrasi offset sensor ke-N
// Body: { "index": 0 }
router.post('/calibrate/:id/offset', (req, res) => {
    const { id } = req.params;
    const { index } = req.body;
    if (index === undefined) return res.status(400).json({ error: 'index required' });
    mqttSvc.sendCommand(id, `cal_offset:${index}`, 'dashboard');
    res.json({ ok: true, sent: `cal_offset:${index}` });
});

// POST /api/calibrate/:id/save  — simpan semua kalibrasi ke NVS
router.post('/calibrate/:id/save', (req, res) => {
    const { id } = req.params;
    mqttSvc.sendCommand(id, 'cal_save', 'dashboard');
    res.json({ ok: true, sent: 'cal_save' });
});

// POST /api/calibrate/:id/reset — reset kalibrasi ke default
router.post('/calibrate/:id/reset', (req, res) => {
    const { id } = req.params;
    mqttSvc.sendCommand(id, 'cal_reset', 'dashboard');
    res.json({ ok: true, sent: 'cal_reset' });
});

// POST /api/selftest/:id — jalankan self-test diagnostik lengkap di device
router.post('/selftest/:id', (req, res) => {
    const { id } = req.params;
    mqttSvc.sendCommand(id, 'self_test', 'dashboard');
    res.json({ ok: true, sent: 'self_test' });
});

// GET  /api/selftest/:id — histori hasil self-test dari database
// Query: ?limit=20
router.get('/selftest/:id', (req, res) => {
    const { id }  = req.params;
    const limit   = Math.min(parseInt(req.query.limit) || 20, 200);
    const rows    = getSelfTestHistory(id, limit);
    res.json({ deviceId: id, count: rows.length, data: rows });
});

module.exports = router;
