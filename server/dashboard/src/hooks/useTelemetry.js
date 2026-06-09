import { useEffect } from 'react';
import { useCncStore } from '../store/useCncStore';

export function useTelemetry() {
    const connect   = useCncStore(s => s.connect);
    const devices   = useCncStore(s => s.devices);
    const history   = useCncStore(s => s.history);
    const connected = useCncStore(s => s.connected);
    useEffect(() => { connect(); }, []);
    return { devices, history, connected };
}
