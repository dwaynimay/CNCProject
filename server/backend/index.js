require('dotenv').config();
const express    = require('express');
const cors       = require('cors');
const { WebSocketServer } = require('ws');
const mqttSvc    = require('./services/mqttService');
const apiRoutes  = require('./routes/api');
const authRoutes = require('./routes/auth');

// Inisialisasi database saat startup
require('./db');

const app  = express();
const HTTP = process.env.HTTP_PORT || 3001;
const WS   = process.env.WS_PORT   || 3002;

app.use(cors());
app.use(express.json());

// Route publik (tidak perlu auth)
app.use('/api/auth', authRoutes);

// Route terproteksi (perlu JWT)
app.use('/api', apiRoutes);

const server = app.listen(HTTP, () =>
    console.log(`[HTTP] REST API running on :${HTTP}`));

const wss = new WebSocketServer({ port: WS });
wss.on('listening', () => console.log(`[WS] WebSocket running on :${WS}`));

// Mulai MQTT bridge
mqttSvc.start(wss);
