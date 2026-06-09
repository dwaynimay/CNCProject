import { useState, useEffect, useRef } from 'react';

const DEVICE_ID = 'cnc-esp32';
const API       = import.meta.env.VITE_API_URL || 'http://localhost:3001/api';
const WS_URL    = import.meta.env.VITE_WS_URL  || 'ws://localhost:3002';

const CURRENT_NAMES  = ['Stepper_X', 'Stepper_Y1', 'Stepper_Y2', 'Stepper_Z', 'Spindle'];
const CURRENT_LIMITS = [3.0, 3.0, 3.0, 2.0, 8.0];   // alarm threshold per sensor
const TEMP_NAMES     = ['Spindle', 'Stepper_Z'];
const TEMP_LIMITS    = [60, 55];

// ── helpers ──────────────────────────────────────────────────────
async function post(url, body = {}) {
  try {
    const r = await fetch(url, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) });
    return r.json();
  } catch (e) { return { error: e.message }; }
}

function fmt(v, d = 4) { return v != null ? Number(v).toFixed(d) : '—'; }
function ts() { return new Date().toLocaleTimeString('id-ID', { hour12: false }); }

// ── sub-components ───────────────────────────────────────────────
function CurrentCard({ sensor, name, limit, onCal }) {
  const a     = sensor?.a;
  const alarm = sensor?.alarm;
  const nc    = a == null;
  const pct   = nc ? 0 : Math.min((Math.abs(a) / limit) * 100, 100);
  const cls   = nc ? '' : alarm ? 'alarm' : 'ok';
  const barColor = alarm ? 'var(--red)' : pct > 70 ? 'var(--amber)' : 'var(--green)';

  return (
    <div className={`card ${cls}`}>
      <div className="card-label">{name}</div>
      <div className="card-value">
        {nc ? '—' : fmt(a, 4)}
        <span className="card-unit">A</span>
      </div>
      <div className="bar-track">
        <div className="bar-fill" style={{ width: `${pct}%`, background: barColor }} />
      </div>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginTop: 8 }}>
        <span className={`card-status ${nc ? 'status-nc' : alarm ? 'status-alarm' : 'status-ok'}`}>
          {nc ? '● MENUNGGU' : alarm ? '⚠ ALARM' : '● NORMAL'}
        </span>
        <button className="btn btn-ghost btn-sm" onClick={onCal} title="Buka panel kalibrasi">⚙</button>
      </div>
    </div>
  );
}

function TempCard({ sensor, name, limit }) {
  const c     = sensor?.c;
  const alarm = sensor?.alarm;
  const nc    = c == null;
  const pct   = nc ? 0 : Math.min((c / limit) * 100, 100);
  const cls   = nc ? '' : alarm ? 'alarm' : c > limit * 0.8 ? 'warn' : 'ok';

  return (
    <div className={`card temp-card ${cls}`}>
      <span className="temp-icon">🌡️</span>
      <div className="card-label">{name}</div>
      <div className="card-value" style={{ color: alarm ? 'var(--red)' : c > limit * 0.8 ? 'var(--amber)' : 'var(--cyan)' }}>
        {nc ? '—' : fmt(c, 1)}
        <span className="card-unit">°C</span>
      </div>
      <div className="bar-track">
        <div className="bar-fill" style={{ width: `${pct}%`, background: alarm ? 'var(--red)' : pct > 80 ? 'var(--amber)' : 'var(--cyan)' }} />
      </div>
      <div className="card-status" style={{ marginTop: 8 }}>
        <span className={nc ? 'status-nc' : alarm ? 'status-alarm' : 'status-ok'}>
          {nc ? '● MENUNGGU' : alarm ? '⚠ OVERHEAT' : '● NORMAL'}
        </span>
        <span style={{ marginLeft: 8, fontSize: 10, color: 'var(--muted)' }}>maks {limit}°C</span>
      </div>
    </div>
  );
}

function RelayCard({ relayOn, onOn, onOff }) {
  return (
    <div className="card relay-card">
      <div className="card-label">Kontrol Relay</div>
      <div>
        <div className="relay-status-label">Status Mesin</div>
        <div className="relay-status-value" style={{ color: relayOn ? 'var(--green)' : 'var(--red)' }}>
          {relayOn ? '🟢 MENYALA (ON)' : '🔴 TRIP / MATI (OFF)'}
        </div>
      </div>
      <div className="relay-btns">
        <button className="btn btn-green" onClick={onOn}>▶ Nyalakan Mesin</button>
        <button className="btn btn-red"   onClick={onOff}>⏹ Matikan / Trip</button>
      </div>
    </div>
  );
}

function CalRow({ idx, name, scaleVal, onScaleChange, onOffset, onScale }) {
  return (
    <tr>
      <td><strong>{idx}</strong></td>
      <td>{name}</td>
      <td>
        <button className="btn btn-ghost btn-sm" onClick={() => onOffset(idx)}>
          Ambil Offset
        </button>
        <span style={{ marginLeft: 6, fontSize: 10, color: 'var(--muted)' }}>(mesin mati)</span>
      </td>
      <td style={{ display: 'flex', gap: 6, alignItems: 'center' }}>
        <input
          className="cal-input"
          type="number" step="0.01" placeholder="Ref (A)"
          value={scaleVal}
          onChange={e => onScaleChange(idx, e.target.value)}
        />
        <button className="btn btn-amber btn-sm" onClick={() => onScale(idx)}>
          Terapkan
        </button>
      </td>
    </tr>
  );
}

// ── main ─────────────────────────────────────────────────────────
export default function App() {
  const [connected, setConnected] = useState(false);
  const [data, setData]           = useState(null);
  const [relayOn, setRelayOn]     = useState(true);
  const [scaleInputs, setScaleInputs] = useState(Array(5).fill(''));
  const [log, setLog]             = useState([]);
  const [calOpen, setCalOpen]     = useState(false);
  const logRef = useRef(null);
  const wsRef  = useRef(null);

  // WebSocket
  useEffect(() => {
    function connect() {
      const ws = new WebSocket(WS_URL);
      wsRef.current = ws;
      ws.onopen  = () => { setConnected(true);  addLog('WebSocket terhubung ke backend.', 'ok'); };
      ws.onclose = () => { setConnected(false); addLog('WebSocket terputus, menyambung ulang...', 'error'); setTimeout(connect, 3000); };
      ws.onerror = () => ws.close();
      ws.onmessage = (e) => {
        try {
          const frame = JSON.parse(e.data);
          if (frame.type === 'telemetry' && frame.deviceId === DEVICE_ID) setData(frame.data);
        } catch (_) {}
      };
    }
    connect();
    return () => wsRef.current?.close();
  }, []);

  // Auto-scroll log
  useEffect(() => { if (logRef.current) logRef.current.scrollTop = 0; }, [log]);

  function addLog(msg, type = '') {
    setLog(prev => [{ msg, type, ts: ts() }, ...prev].slice(0, 50));
  }

  async function sendCmd(cmd, label) {
    addLog(`→ Kirim: ${label || cmd}`, 'send');
    const r = await post(`${API}/command/${DEVICE_ID}`, { cmd });
    addLog(r.ok ? `✓ Berhasil: ${r.sent}` : `✗ Error: ${r.error}`, r.ok ? 'ok' : 'error');
  }

  async function doOffset(idx) {
    addLog(`→ Kalibrasi Offset [${idx}] ${CURRENT_NAMES[idx]}...`, 'send');
    const r = await post(`${API}/calibrate/${DEVICE_ID}/offset`, { index: idx });
    addLog(r.ok ? `✓ Offset [${idx}] dikirim` : `✗ ${r.error}`, r.ok ? 'ok' : 'error');
  }

  async function doScale(idx) {
    const a = parseFloat(scaleInputs[idx]);
    if (!a || a <= 0) { addLog(`⚠ Masukkan arus referensi untuk [${idx}] ${CURRENT_NAMES[idx]}`, 'error'); return; }
    addLog(`→ Kalibrasi Skala [${idx}] ref=${a}A...`, 'send');
    const r = await post(`${API}/calibrate/${DEVICE_ID}/scale`, { index: idx, ampere: a });
    addLog(r.ok ? `✓ Skala [${idx}] dikirim (ref ${a}A)` : `✗ ${r.error}`, r.ok ? 'ok' : 'error');
  }

  async function doSave()  {
    addLog('→ Menyimpan semua kalibrasi ke NVS...', 'send');
    const r = await post(`${API}/calibrate/${DEVICE_ID}/save`);
    addLog(r.ok ? '✓ Kalibrasi tersimpan ke NVS (Flash ESP32)' : `✗ ${r.error}`, r.ok ? 'ok' : 'error');
  }

  async function doReset() {
    addLog('→ Reset kalibrasi ke nilai default...', 'send');
    const r = await post(`${API}/calibrate/${DEVICE_ID}/reset`);
    addLog(r.ok ? '✓ Kalibrasi di-reset' : `✗ ${r.error}`, r.ok ? 'ok' : 'error');
  }

  function setScaleInput(idx, val) {
    setScaleInputs(prev => { const n = [...prev]; n[idx] = val; return n; });
  }

  const current = data?.current ?? [];
  const temp    = data?.temp    ?? [];
  // data.ts = millis() dari ESP32 (uptime ms), bukan unix timestamp
  // Gunakan waktu lokal saat data diterima
  const lastTs  = data ? ts() : null;

  return (
    <>
      {/* ── HEADER ── */}
      <header className="header">
        <div className="header-logo">
          <span>⚙ CNC IoT SCADA</span>
          <small>Monitoring &amp; Control System</small>
        </div>
        <div className="header-meta">
          <span className="device-id">Node: <strong>{DEVICE_ID}</strong></span>
          <div className={`conn-badge ${connected ? 'online' : 'offline'}`}>
            <div className="conn-dot" />
            {connected ? 'Terhubung' : 'Terputus'}
          </div>
          {lastTs && <span className="header-ts">Update: {lastTs}</span>}
        </div>
      </header>

      <main className="main">

        {/* ── 1. ARUS ── */}
        <section>
          <div className="section-title">▸ Sensor Arus (Ampere)</div>
          {!data ? (
            <div className="no-data">
              <div className="no-data-icon">📡</div>
              <div>Menunggu data dari ESP32...</div>
              <div style={{ fontSize: 11, marginTop: 6, color: 'var(--muted)' }}>Pastikan ESP32 dan backend sudah berjalan</div>
            </div>
          ) : (
            <div className="cards-5">
              {CURRENT_NAMES.map((name, i) => (
                <CurrentCard
                  key={i} idx={i} name={name}
                  sensor={current[i]} limit={CURRENT_LIMITS[i]}
                  onCal={() => setCalOpen(true)}
                />
              ))}
            </div>
          )}
        </section>

        {/* ── 2. SUHU + RELAY ── */}
        <section>
          <div className="section-title">▸ Sensor Suhu &amp; Kontrol Relay</div>
          <div className="row-3">
            {TEMP_NAMES.map((name, i) => (
              <TempCard key={i} sensor={temp[i]} name={name} limit={TEMP_LIMITS[i]} />
            ))}
            <RelayCard
              relayOn={relayOn}
              onOn={() => { sendCmd('relay_on', 'Relay ON'); setRelayOn(true); }}
              onOff={() => { sendCmd('relay_off', 'Relay OFF'); setRelayOn(false); }}
            />
          </div>
        </section>

        {/* ── 3. KALIBRASI ── */}
        <section>
          <div className="section-header" onClick={() => setCalOpen(v => !v)}>
            <div className="section-title" style={{ marginBottom: 0 }}>▸ Panel Kalibrasi Sensor Arus</div>
            <span className={`collapse-icon ${calOpen ? 'open' : ''}`}>▼</span>
          </div>
          {calOpen && (
            <div className="card" style={{ marginTop: 12 }}>
              <table className="cal-table">
                <thead>
                  <tr>
                    <th>#</th><th>Nama Sensor</th>
                    <th>Offset (Titik Nol)</th>
                    <th>Skala (dengan Beban)</th>
                  </tr>
                </thead>
                <tbody>
                  {CURRENT_NAMES.map((name, i) => (
                    <CalRow
                      key={i} idx={i} name={name}
                      scaleVal={scaleInputs[i]}
                      onScaleChange={setScaleInput}
                      onOffset={doOffset}
                      onScale={doScale}
                    />
                  ))}
                </tbody>
              </table>
              <div className="cal-actions">
                <button className="btn btn-cyan" onClick={doSave}>
                  💾 Simpan Semua ke NVS (Flash)
                </button>
                <button className="btn btn-ghost" onClick={doReset}>
                  ♻ Reset ke Default
                </button>
              </div>
            </div>
          )}
        </section>

        {/* ── 4. LOG ── */}
        <section>
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 10 }}>
            <div className="section-title" style={{ marginBottom: 0 }}>▸ Log Aktivitas</div>
            <button className="btn btn-ghost btn-sm" onClick={() => setLog([])}>Bersihkan</button>
          </div>
          <div className="log-box" ref={logRef}>
            {log.length === 0
              ? <div className="log-empty">(belum ada aktivitas)</div>
              : log.map((l, i) => (
                  <div className="log-line" key={i}>
                    <span className="log-ts">{l.ts}</span>
                    <span className={`log-msg ${l.type}`}>{l.msg}</span>
                  </div>
                ))
            }
          </div>
        </section>

      </main>
    </>
  );
}
