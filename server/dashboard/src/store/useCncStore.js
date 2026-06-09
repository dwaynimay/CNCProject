import { create } from 'zustand';
const WS_URL  = import.meta.env.VITE_WS_URL  || 'ws://localhost:3002';
const API_URL = import.meta.env.VITE_API_URL || 'http://localhost:3001/api';

export const useCncStore = create((set) => ({
    devices:   {},
    history:   {},
    connected: false,

    connect() {
        const ws = new WebSocket(WS_URL);
        ws.onopen    = () => set({ connected: true });
        ws.onclose   = () => set({ connected: false });
        ws.onmessage = (e) => {
            const { type, deviceId, data } = JSON.parse(e.data);
            if (type !== 'telemetry') return;
            set(state => ({
                devices: { ...state.devices, [deviceId]: data },
                history: {
                    ...state.history,
                    [deviceId]: [...(state.history[deviceId] || []).slice(-99), data],
                },
            }));
        };
    },

    async sendCommand(deviceId, cmd) {
        await fetch(`${API_URL}/command/${deviceId}`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ cmd }),
        });
    },
}));
