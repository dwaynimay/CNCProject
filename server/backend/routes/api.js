const router  = require('express').Router();
const mqttSvc = require('../services/mqttService');

// GET  /api/latest/:id?  — data terbaru per device
router.get('/latest/:id?', (req, res) => {
    res.json(mqttSvc.getLatest(req.params.id));
});

// POST /api/command/:id  — kirim perintah ke ESP32
// Body: { "cmd": "relay_on" | "relay_off" | "cal_save" | "cal_reset" }
router.post('/command/:id', (req, res) => {
    const { id }  = req.params;
    const { cmd } = req.body;
    if (!cmd) return res.status(400).json({ error: 'cmd required' });
    mqttSvc.sendCommand(id, cmd);
    res.json({ ok: true, sent: cmd });
});

// POST /api/calibrate/:id/offset — kalibrasi offset sensor ke-N
// Body: { "index": 0 }
router.post('/calibrate/:id/offset', (req, res) => {
    const { id } = req.params;
    const { index } = req.body;
    if (index === undefined) return res.status(400).json({ error: 'index required' });
    mqttSvc.sendCommand(id, `cal_offset:${index}`);
    res.json({ ok: true, sent: `cal_offset:${index}` });
});

// POST /api/calibrate/:id/scale  — kalibrasi skala sensor ke-N
// Body: { "index": 0, "ampere": 2.5 }
router.post('/calibrate/:id/scale', (req, res) => {
    const { id } = req.params;
    const { index, ampere } = req.body;
    if (index === undefined || !ampere) return res.status(400).json({ error: 'index and ampere required' });
    mqttSvc.sendCommand(id, `cal_scale:${index}:${ampere}`);
    res.json({ ok: true, sent: `cal_scale:${index}:${ampere}` });
});

// POST /api/calibrate/:id/save  — simpan semua kalibrasi ke NVS
router.post('/calibrate/:id/save', (req, res) => {
    const { id } = req.params;
    mqttSvc.sendCommand(id, 'cal_save');
    res.json({ ok: true, sent: 'cal_save' });
});

// POST /api/calibrate/:id/reset — reset kalibrasi ke default
router.post('/calibrate/:id/reset', (req, res) => {
    const { id } = req.params;
    mqttSvc.sendCommand(id, 'cal_reset');
    res.json({ ok: true, sent: 'cal_reset' });
});

module.exports = router;
