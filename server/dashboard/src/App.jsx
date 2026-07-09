import { useState, useEffect, useRef } from 'react';
import {
  LineChart, Line, XAxis, YAxis, Tooltip, ResponsiveContainer,
} from 'recharts';

const DEVICE_ID      = 'cnc-esp32';
const hostname       = window.location.hostname;
const API            = import.meta.env.VITE_API_URL || `http://${hostname}:3001/api`;
const WS_URL         = import.meta.env.VITE_WS_URL  || `ws://${hostname}:3002`;

const CURRENT_NAMES  = ['Stepper X', 'Stepper Y1', 'Stepper Y2', 'Stepper Z', 'Spindle'];
const CURRENT_LIMITS = [3.0, 3.0, 3.0, 2.0, 8.0];
const TEMP_NAMES     = ['Spindle', 'Stepper Z'];
const TEMP_LIMITS    = [60, 55];

// Non-blue, non-purple — warm palette for chart lines
const CHART_COLORS = ['#f0b429', '#22c55e', '#a3e635', '#f97316', '#cbd5e1'];

const HISTORY_LEN = 90;

// ── helpers ───────────────────────────────────────────────────────────
async function post(url, body = {}) {
  try {
    const r = await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    });
    return r.json();
  } catch (e) {
    return { error: e.message };
  }
}

function fmt(v, d = 4) { return v != null ? Number(v).toFixed(d) : '—'; }
function nowTs() {
  return new Date().toLocaleTimeString('id-ID', { hour12: false });
}

// ── CurrentCard ───────────────────────────────────────────────────────
function CurrentCard({ sensor, name, limit, onCal }) {
  const a     = sensor?.a;
  const alarm = sensor?.alarm;
  const nc    = a == null;
  const pct   = nc ? 0 : Math.min((Math.abs(a) / limit) * 100, 100);
  const cls   = nc ? '' : alarm ? 'alarm' : pct > 75 ? 'warn' : 'ok';
  const valColor = alarm ? 'var(--red)' : pct > 75 ? 'var(--orange)' : 'var(--text)';
  const barColor = alarm ? 'var(--red)' : pct > 75 ? 'var(--orange)' : 'var(--green)';

  return (
    <div className={`card ${cls}`}>
      <div className="card-label">{name}</div>
      <div className="card-value" style={{ color: valColor }}>
        {nc ? '—' : fmt(a, 4)}
        <span className="card-unit">A</span>
      </div>
      <div className="bar-track">
        <div className="bar-fill" style={{ width: `${pct}%`, background: barColor }} />
      </div>
      <div className="card-status">
        <span className={nc ? 'status-nc' : alarm ? 'status-alarm' : pct > 75 ? 'status-warn' : 'status-ok'}>
          {nc ? '● MENUNGGU' : alarm ? '⚠ ALARM' : pct > 75 ? '▲ TINGGI' : '● NORMAL'}
        </span>
        <button className="btn btn-ghost btn-sm" onClick={onCal} title="Set Offset Nol & Simpan">⚙ Set Nol</button>
      </div>
    </div>
  );
}

// ── TempCard ──────────────────────────────────────────────────────────
function TempCard({ sensor, name, limit }) {
  const c     = sensor?.c;
  const alarm = sensor?.alarm;
  const nc    = c == null;
  const pct   = nc ? 0 : Math.min((c / limit) * 100, 100);
  const cls   = nc ? '' : alarm ? 'alarm' : c > limit * 0.8 ? 'warn' : 'ok';
  const valColor = nc
    ? 'var(--muted)'
    : alarm ? 'var(--red)' : c > limit * 0.8 ? 'var(--orange)' : 'var(--text)';

  return (
    <div className={`card temp-card ${cls}`}>
      <div className="card-label">{name}</div>
      <div className="card-value" style={{ color: valColor }}>
        {nc ? '—' : fmt(c, 1)}
        <span className="card-unit">°C</span>
      </div>
      <div className="bar-track">
        <div className="bar-fill" style={{
          width: `${pct}%`,
          background: alarm ? 'var(--red)' : pct > 80 ? 'var(--orange)' : 'var(--green)',
        }} />
      </div>
      <div className="card-status">
        <span className={nc ? 'status-nc' : alarm ? 'status-alarm' : c > limit * 0.8 ? 'status-warn' : 'status-ok'}>
          {nc ? '● MENUNGGU' : alarm ? '⚠ OVERHEAT' : c > limit * 0.8 ? '▲ PANAS' : '● NORMAL'}
        </span>
        <span style={{ fontSize: 10, color: 'var(--muted)' }}>maks {limit}°C</span>
      </div>
    </div>
  );
}

// ── RelayCard ─────────────────────────────────────────────────────────
// Relay NC: relay ON (energized) → circuit PUTUS → mesin MATI
//           relay OFF              → circuit TERHUBUNG → mesin MENYALA
function RelayCard({ relayOn, disabled, onOn, onOff }) {
  // machineOn = true jika mesin benar-benar menyala (relay OFF pada NC)
  const machineOn = !relayOn;
  return (
    <div className="card relay-card" style={{ alignItems: 'center', textAlign: 'center' }}>
      <div className="card-label">Kontrol Mesin</div>
      <button 
        className={`btn-power ${machineOn ? 'on' : 'off'}`}
        disabled={disabled}
        onClick={machineOn ? onOn : onOff}
        title={disabled ? 'Menunggu koneksi...' : (machineOn ? 'Matikan Mesin' : 'Nyalakan Mesin')}
      >
        <svg viewBox="0 0 24 24" width="28" height="28" fill="none" stroke="currentColor" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round">
          <path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path>
          <line x1="12" y1="2" x2="12" y2="12"></line>
        </svg>
      </button>
      <div className="relay-state-text" style={{ color: disabled ? 'var(--muted)' : (machineOn ? 'var(--green)' : 'var(--red)') }}>
        {disabled ? 'TIDAK TERSEDIA' : (machineOn ? 'MESIN MENYALA' : 'MESIN MATI')}
      </div>
    </div>
  );
}

// ── App ───────────────────────────────────────────────────────────────
// ── SelfTestPanel ─────────────────────────────────────────────────────────
const CATEGORY_LABELS = {
  connectivity: 'Konektivitas',
  current:      'Sensor Arus',
  temp:         'Sensor Suhu',
  relay:        'Relay',
  calibration:  'Kalibrasi',
  eventlog:     'Event Log',
};

function SelfTestPanel({
  connected, data, onRunSelfTest, selfTestRunning, selfTestResults,
  onTripTest, tripTestResult, tripTestSensor, onTripSensorChange,
}) {
  const checks = selfTestResults?.checks ?? [];
  const grouped = checks.reduce((acc, c) => {
    (acc[c.category] ??= []).push(c);
    return acc;
  }, {});
  const passCount = checks.filter(c => c.pass).length;

  return (
    <div className="test-panel">
      <div className="test-panel-header">
        <span className="test-icon">🔬</span>
        <span className="test-title">System Self-Test</span>
        {selfTestRunning && <span className="test-badge running">⏳ Menjalankan diagnostik...</span>}
        {!selfTestRunning && selfTestResults && (
          <span className={`test-badge ${selfTestResults.overall === 'pass' ? 'pass' : 'fail'}`}>
            {selfTestResults.overall === 'pass' ? '✅' : '❌'} {passCount}/{checks.length} check lulus
          </span>
        )}
      </div>
      <div className="test-body">
        <div className="test-desc">
          Jalankan diagnostik lengkap di device: sensor arus &amp; suhu (sanity + logika respons alarm),
          aktuasi relay, kalibrasi NVS, event log, dan konektivitas MQTT/NTP. Hasil per-item ditampilkan
          di bawah dan tersimpan sebagai riwayat.
        </div>
        <div className="test-controls">
          <button
            id="btn-run-selftest"
            className={`btn btn-test${selfTestRunning ? ' running' : ''}`}
            disabled={!connected || !data || selfTestRunning}
            onClick={onRunSelfTest}
          >
            {selfTestRunning ? '⏳ Menguji...' : '▶ Jalankan Self-Test'}
          </button>
        </div>

        {checks.length > 0 && (
          <div className="selftest-results">
            {Object.entries(grouped).map(([category, items]) => (
              <div key={category} className="selftest-category">
                <div className="selftest-category-label">{CATEGORY_LABELS[category] || category}</div>
                {items.map((c, i) => (
                  <div key={i} className={`selftest-item ${c.pass ? 'pass' : 'fail'}`}>
                    <span className="selftest-icon">{c.pass ? '✅' : '❌'}</span>
                    <span className="selftest-name">{c.name}</span>
                    <span className="selftest-detail">{c.detail}</span>
                  </div>
                ))}
              </div>
            ))}
          </div>
        )}

        <details className="selftest-advanced">
          <summary>Trip Test Manual (1 sensor)</summary>
          <div className="test-desc">
            Trigger simulasi arus berlebih ke satu sensor — relay akan benar-benar trip (mesin mati).
            Berbeda dari Self-Test di atas (yang non-destruktif), ini sengaja mematikan mesin untuk
            memverifikasi proteksi overcurrent end-to-end.
          </div>
          <div className="test-controls">
            <select
              id="test-sensor-select"
              className="test-select"
              value={tripTestSensor}
              onChange={e => onTripSensorChange(Number(e.target.value))}
              disabled={tripTestResult === 'running'}
            >
              {CURRENT_NAMES.map((n, i) => (
                <option key={i} value={i}>{n} — batas {CURRENT_LIMITS[i]} A</option>
              ))}
            </select>
            <button
              id="btn-test-overcurrent"
              className={`btn btn-test${tripTestResult === 'running' ? ' running' : ''}`}
              disabled={!connected || !data || tripTestResult === 'running'}
              onClick={() => onTripTest(tripTestSensor)}
            >
              {tripTestResult === 'running' ? '⏳ Menguji...' : '▶ Jalankan Trip Test'}
            </button>
            {tripTestResult === 'pass' && <span className="test-badge pass">✅ Relay Trip Berhasil</span>}
            {tripTestResult === 'fail' && <span className="test-badge fail">❌ Test Gagal</span>}
          </div>
        </details>
      </div>
    </div>
  );
}

// ── App ───────────────────────────────────────────────────────────────
export default function App() {
  const [connected,    setConnected]    = useState(false);
  const [data,         setData]         = useState(null);
  const [history,      setHistory]      = useState([]);
  const [relayOn,      setRelayOn]      = useState(true);
  const [log,          setLog]          = useState([]);
  const [logOpen,      setLogOpen]      = useState(false);
  const [lastTs,       setLastTs]       = useState(null);
  const [theme,        setTheme]        = useState(() => localStorage.getItem('theme') || 'dark');
  const [testResult,   setTestResult]   = useState(null);  // null | 'running' | 'pass' | 'fail' (trip test manual)
  const [testSensor,   setTestSensor]   = useState(0);     // index sensor yang di-test (trip test manual)
  const [selfTestRunning, setSelfTestRunning] = useState(false);
  const [selfTestResults, setSelfTestResults] = useState(null); // { ts, overall, checks: [] }
  const testResultRef  = useRef(null);  // mirror testResult untuk akses di ws.onmessage
  const selfTestRunningRef = useRef(false);
  const relayPendingRef = useRef(false); // true = abaikan relayOn dari telemetri (anti-blink)
  const logRef = useRef(null);
  const wsRef  = useRef(null);

  useEffect(() => {
    document.documentElement.setAttribute('data-theme', theme);
    localStorage.setItem('theme', theme);
  }, [theme]);

  // WebSocket with auto-reconnect
  useEffect(() => {

    function connect() {
      const ws = new WebSocket(WS_URL);
      wsRef.current = ws;

      ws.onopen = () => {
        setConnected(true);
        addLog('WebSocket terhubung ke backend.', 'ok');
      };

      ws.onclose = () => {
        setConnected(false);
        addLog('WebSocket terputus, menyambung ulang...', 'error');
        setTimeout(connect, 3000);
      };

      ws.onerror = () => ws.close();

      ws.onmessage = (e) => {
        try {
          const frame = JSON.parse(e.data);
          if (frame.deviceId !== DEVICE_ID) return;

          if (frame.type === 'selftest_result') {
            setSelfTestResults(frame.data);
            setSelfTestRunning(false);
            selfTestRunningRef.current = false;
            const passCount = frame.data.checks?.filter(c => c.pass).length ?? 0;
            const total     = frame.data.checks?.length ?? 0;
            addLog(
              `[SELF-TEST] ${frame.data.overall === 'pass' ? '✅' : '❌'} Selesai: ${passCount}/${total} check lulus`,
              frame.data.overall === 'pass' ? 'ok' : 'error'
            );
            return;
          }

          if (frame.type !== 'telemetry') return;

          setData(frame.data);
          setLastTs(nowTs());
          // Hanya update relayOn dari telemetri jika tidak ada command yang baru dikirim.
          // Ini mencegah blink: paket telemetri lama (state sebelum relay gerak) tidak
          // override optimistic update yang sudah kita set saat klik tombol.
          if (!relayPendingRef.current && frame.data.relayOn !== undefined) {
            setRelayOn(frame.data.relayOn);
          }

          // ── Deteksi hasil test overcurrent via telemetri masuk ──
          if (testResultRef.current === 'running') {
            const anyAlarm = frame.data.current?.some(s => s?.alarm);
            if (anyAlarm && frame.data.relayOn) {
              setTestResult('pass');
              testResultRef.current = 'pass';
              addLog('[TEST] ✓ BERHASIL — Relay berhasil trip! Arus lebih terdeteksi.', 'ok');
            }
          }

          setHistory(prev => {
            const entry = { tick: prev.length };
            CURRENT_NAMES.forEach((_, i) => {
              const val = frame.data.current?.[i]?.a;
              entry[`c${i}`] = val != null ? val : null;
            });
            TEMP_NAMES.forEach((_, i) => {
              const val = frame.data.temp?.[i]?.c;
              entry[`t${i}`] = val != null ? val : null;
            });
            return [...prev.slice(-(HISTORY_LEN - 1)), entry];
          });
        } catch (_) {}
      };
    }

    connect();
    return () => wsRef.current?.close();
  }, []);

  useEffect(() => {
    if (logRef.current) logRef.current.scrollTop = 0;
  }, [log]);

  function addLog(msg, type = '') {
    setLog(prev => [{ msg, type, ts: nowTs() }, ...prev].slice(0, 80));
  }

  async function sendCmd(cmd, label) {
    addLog(`Kirim: ${label || cmd}`, 'send');
    // Untuk perintah relay: set pending flag agar telemetri lama tidak override UI
    if (cmd === 'relay_on' || cmd === 'relay_off') {
      relayPendingRef.current = true;
      setTimeout(() => { relayPendingRef.current = false; }, 4000); // 2× publish interval
    }
    const r = await post(`${API}/command/${DEVICE_ID}`, { cmd });
    addLog(r.ok ? `Berhasil: ${r.sent}` : `Error: ${r.error}`, r.ok ? 'ok' : 'error');
    // Reset hasil test jika relay dihidupkan kembali
    if (cmd === 'relay_off') { setTestResult(null); testResultRef.current = null; }
  }

  async function doTestOvercurrent(sensorIdx) {
    setTestResult('running');
    testResultRef.current = 'running';
    addLog(`[TEST] Trigger overcurrent sensor [${sensorIdx}] ${CURRENT_NAMES[sensorIdx]}...`, 'send');
    const cmd = sensorIdx === 0 ? 'test_overcurrent' : `test_overcurrent:${sensorIdx}`;
    const r = await post(`${API}/command/${DEVICE_ID}`, { cmd });
    if (!r.ok) {
      addLog(`[TEST] Gagal kirim: ${r.error}`, 'error');
      setTestResult('fail');
      testResultRef.current = 'fail';
      return;
    }
    addLog('[TEST] Perintah terkirim — menunggu respons firmware (~2 dtk)...', 'ok');
    // Auto-timeout 10 detik jika tidak ada respons
    setTimeout(() => {
      if (testResultRef.current === 'running') {
        setTestResult('fail');
        testResultRef.current = 'fail';
        addLog('[TEST] ✗ Timeout — tidak ada respons alarm dari firmware.', 'error');
      }
    }, 10000);
  }

  async function doRunSelfTest() {
    setSelfTestRunning(true);
    selfTestRunningRef.current = true;
    addLog('[SELF-TEST] Menjalankan diagnostik lengkap...', 'send');
    const r = await post(`${API}/selftest/${DEVICE_ID}`, {});
    if (!r.ok) {
      addLog(`[SELF-TEST] Gagal kirim: ${r.error}`, 'error');
      setSelfTestRunning(false);
      selfTestRunningRef.current = false;
      return;
    }
    // Auto-timeout 15 detik jika tidak ada respons (self-test lebih lama dari trip test biasa)
    setTimeout(() => {
      if (selfTestRunningRef.current) {
        setSelfTestRunning(false);
        selfTestRunningRef.current = false;
        addLog('[SELF-TEST] ✗ Timeout — tidak ada respons dari firmware.', 'error');
      }
    }, 15000);
  }

  async function doOffset(idx) {
    addLog(`Kalibrasi offset [${idx}] ${CURRENT_NAMES[idx]}...`, 'send');
    const r = await post(`${API}/calibrate/${DEVICE_ID}/offset`, { index: idx });
    addLog(r.ok ? `Offset [${idx}] dikirim` : `Error: ${r.error}`, r.ok ? 'ok' : 'error');
  }

  async function doSave() {
    addLog('Menyimpan kalibrasi ke NVS...', 'send');
    const r = await post(`${API}/calibrate/${DEVICE_ID}/save`);
    addLog(r.ok ? 'Kalibrasi tersimpan ke NVS (Flash ESP32)' : `Error: ${r.error}`, r.ok ? 'ok' : 'error');
  }

  async function doReset() {
    addLog('Reset kalibrasi ke default...', 'send');
    const r = await post(`${API}/calibrate/${DEVICE_ID}/reset`);
    addLog(r.ok ? 'Kalibrasi di-reset' : `Error: ${r.error}`, r.ok ? 'ok' : 'error');
  }

  const current = data?.current ?? [];
  const temp    = data?.temp    ?? [];

  return (
    <>
      {/* ── HEADER ── */}
      <header className="header">
        <div className="header-brand">
          <div>
          </div>
        </div>
        <div className="header-right">
          <label className="theme-switch" title="Toggle Light/Dark Mode">
            <input 
              type="checkbox" 
              checked={theme === 'light'} 
              onChange={() => setTheme(t => t === 'dark' ? 'light' : 'dark')} 
            />
            <span className="slider round"></span>
          </label>
          <span className="device-id">Node: <strong>{DEVICE_ID}</strong></span>
          <div className={`conn-badge ${connected ? 'online' : 'offline'}`}>
            <div className="conn-dot" />
            {connected ? 'Terhubung' : 'Terputus'}
          </div>
        </div>
      </header>

      <main className="main">

        {/* ── SENSOR ARUS ── */}
        <section>
          <p className="section-label">Sensor Arus</p>
          {!data ? (
            <div className="no-data">
              Menunggu data...
            </div>
          ) : (
            <div className="cards-auto">
              {CURRENT_NAMES.map((name, i) => (
                <CurrentCard
                  key={i}
                  name={name}
                  sensor={current[i]}
                  limit={CURRENT_LIMITS[i]}
                  onCal={async () => {
                    await doOffset(i);
                    setTimeout(doSave, 500);
                  }}
                />
              ))}
            </div>
          )}
        </section>

        {/* ── TREN ARUS + SUHU + RELAY ── */}
        <section className="pane-chart-side">

          {/* Charts Column */}
          <div style={{ display: 'flex', flexDirection: 'column', gap: '14px', minWidth: 0 }}>
            {/* Tren Arus */}
            <div className="chart-card">
              <div className="chart-header">
                <span className="chart-title">Tren Arus</span>
                <div className="chart-legend">
                  {CURRENT_NAMES.map((name, i) => (
                    <div key={i} className="legend-item">
                      <div className="legend-dot" style={{ background: CHART_COLORS[i] }} />
                      {name}
                    </div>
                  ))}
                </div>
              </div>
              {history.length < 2 ? (
                <div className="chart-empty">Menunggu data...</div>
              ) : (
                <ResponsiveContainer width="100%" height={180}>
                  <LineChart data={history} margin={{ top: 4, right: 4, bottom: 0, left: -16 }}>
                    <XAxis dataKey="tick" hide />
                    <YAxis
                      domain={[0, 'auto']}
                      tick={{ fontSize: 10, fill: '#7a7060' }}
                      tickLine={false}
                      axisLine={false}
                    />
                    <Tooltip
                      contentStyle={{
                        background: '#1a1510',
                        border: '1px solid #2d2418',
                        borderRadius: '6px',
                        fontSize: 11,
                        color: '#ede8df',
                      }}
                      cursor={{ stroke: '#2d2418', strokeWidth: 1 }}
                      labelFormatter={() => ''}
                      formatter={(v, name) => [v != null ? `${Number(v).toFixed(3)} A` : '—', name]}
                    />
                    {CURRENT_NAMES.map((name, i) => (
                      <Line
                        key={i}
                        type="monotone"
                        dataKey={`c${i}`}
                        name={name}
                        stroke={CHART_COLORS[i]}
                        strokeWidth={1.5}
                        dot={false}
                        connectNulls={false}
                        isAnimationActive={false}
                      />
                    ))}
                  </LineChart>
                </ResponsiveContainer>
              )}
            </div>

            {/* Tren Suhu */}
            <div className="chart-card">
              <div className="chart-header">
                <span className="chart-title">Tren Suhu</span>
                <div className="chart-legend">
                  {TEMP_NAMES.map((name, i) => (
                    <div key={i} className="legend-item">
                      <div className="legend-dot" style={{ background: CHART_COLORS[i] }} />
                      {name}
                    </div>
                  ))}
                </div>
              </div>
              {history.length < 2 ? (
                <div className="chart-empty">Menunggu data...</div>
              ) : (
                <ResponsiveContainer width="100%" height={180}>
                  <LineChart data={history} margin={{ top: 4, right: 4, bottom: 0, left: -16 }}>
                    <XAxis dataKey="tick" hide />
                    <YAxis
                      domain={['auto', 'auto']}
                      tick={{ fontSize: 10, fill: '#7a7060' }}
                      tickLine={false}
                      axisLine={false}
                    />
                    <Tooltip
                      contentStyle={{
                        background: '#1a1510',
                        border: '1px solid #2d2418',
                        borderRadius: '6px',
                        fontSize: 11,
                        color: '#ede8df',
                      }}
                      cursor={{ stroke: '#2d2418', strokeWidth: 1 }}
                      labelFormatter={() => ''}
                      formatter={(v, name) => [v != null ? `${Number(v).toFixed(1)} °C` : '—', name]}
                    />
                    {TEMP_NAMES.map((name, i) => (
                      <Line
                        key={i}
                        type="monotone"
                        dataKey={`t${i}`}
                        name={name}
                        stroke={CHART_COLORS[i]}
                        strokeWidth={1.5}
                        dot={false}
                        connectNulls={false}
                        isAnimationActive={false}
                      />
                    ))}
                  </LineChart>
                </ResponsiveContainer>
              )}
            </div>
          </div>

          {/* Right: temp sensors + relay */}
          <div className="side-stack">
            {TEMP_NAMES.map((name, i) => (
              <TempCard key={i} sensor={temp[i]} name={name} limit={TEMP_LIMITS[i]} />
            ))}
            <RelayCard
              relayOn={relayOn}
              disabled={!connected || !data}
              onOn={() => { sendCmd('relay_on', 'Relay ON'); setRelayOn(true); }}
              onOff={() => { sendCmd('relay_off', 'Relay OFF'); setRelayOn(false); }}
            />
          </div>
        </section>


        {/* ── SELF-TEST SISTEM ── */}
        <section>
          <SelfTestPanel
            connected={connected}
            data={data}
            onRunSelfTest={doRunSelfTest}
            selfTestRunning={selfTestRunning}
            selfTestResults={selfTestResults}
            onTripTest={doTestOvercurrent}
            tripTestResult={testResult}
            tripTestSensor={testSensor}
            onTripSensorChange={setTestSensor}
          />
        </section>

        {/* ── LOG AKTIVITAS ── */}
        <section>
          <button
            className={`cal-toggle ${logOpen ? 'open' : ''}`}
            onClick={() => setLogOpen(v => !v)}
          >
            <span className="cal-toggle-label">Log Aktivitas</span>
            <span className={`cal-chevron ${logOpen ? 'open' : ''}`}>▼</span>
          </button>
          {logOpen && (
            <div className="log-card" style={{ borderTop: 'none', borderRadius: '0 0 var(--radius) var(--radius)' }}>
            <div className="log-body" ref={logRef} style={{ paddingTop: '8px' }}>
              {log.length === 0 ? (
                <div className="log-empty">Belum ada aktivitas</div>
              ) : (
                log.map((entry, i) => (
                  <div key={i} className="log-entry">
                    <span className="log-icon" style={{
                      color: entry.type === 'send'  ? 'var(--amber)'
                           : entry.type === 'ok'    ? 'var(--green)'
                           : entry.type === 'error' ? 'var(--red)'
                           : 'var(--muted)',
                    }}>
                      {entry.type === 'send' ? '→' : entry.type === 'ok' ? '✓' : entry.type === 'error' ? '✗' : '·'}
                    </span>
                    <span className="log-ts">{entry.ts}</span>
                    <span className={`log-msg ${entry.type}`}>{entry.msg}</span>
                  </div>
                ))
              )}
            </div>
          </div>
          )}
        </section>

      </main>
    </>
  );
}
