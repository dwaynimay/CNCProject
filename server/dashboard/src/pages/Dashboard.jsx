import { useTelemetry }  from '../hooks/useTelemetry';
import { CurrentCard }   from '../components/CurrentCard';
import { TempCard }      from '../components/TempCard';
import { RelayButton }   from '../components/RelayButton';
import { CurrentChart }  from '../components/CurrentChart';

export function Dashboard() {
    const { devices, history, connected } = useTelemetry();
    const deviceId = Object.keys(devices)[0] || 'cnc-esp32';
    const latest   = devices[deviceId];
    const hist     = history[deviceId] || [];

    return (
        <div className="min-h-screen bg-gray-100 p-6">
            <div className="flex items-center justify-between mb-6">
                <h1 className="text-xl font-semibold text-gray-800">CNC IoT Monitor</h1>
                <span className={`text-xs px-2 py-1 rounded-full ${connected ? 'bg-green-100 text-green-700' : 'bg-red-100 text-red-700'}`}>
                    {connected ? 'Live' : 'Disconnected'}
                </span>
            </div>
            {!latest ? <p className="text-gray-500">Menunggu data dari ESP32...</p> : (
                <>
                    <div className="bg-white rounded-lg p-4 mb-4 border border-gray-200">
                        <p className="text-sm font-medium text-gray-700 mb-3">Kontrol Spindle</p>
                        <RelayButton deviceId={deviceId} />
                    </div>
                    <div className="mb-4">
                        <p className="text-sm font-medium text-gray-700 mb-2">Arus Motor</p>
                        <div className="grid grid-cols-2 md:grid-cols-5 gap-3">
                            {latest.current?.map(s => <CurrentCard key={s.id} sensor={s} />)}
                        </div>
                    </div>
                    <div className="mb-4">
                        <p className="text-sm font-medium text-gray-700 mb-2">Suhu</p>
                        <div className="grid grid-cols-2 gap-3">
                            {latest.temp?.map(s => <TempCard key={s.id} sensor={s} />)}
                        </div>
                    </div>
                    <div className="bg-white rounded-lg p-4 border border-gray-200">
                        <p className="text-sm font-medium text-gray-700 mb-3">Tren Arus</p>
                        <CurrentChart history={hist} />
                    </div>
                </>
            )}
        </div>
    );
}
