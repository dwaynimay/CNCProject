import { useCncStore } from '../store/useCncStore';
export function RelayButton({ deviceId }) {
    const sendCommand = useCncStore(s => s.sendCommand);
    return (
        <div className="flex gap-3">
            <button onClick={() => sendCommand(deviceId, 'relay_on')}
                className="px-4 py-2 bg-green-600 text-white rounded-lg hover:bg-green-700 text-sm">
                Spindle ON
            </button>
            <button onClick={() => sendCommand(deviceId, 'relay_off')}
                className="px-4 py-2 bg-red-600 text-white rounded-lg hover:bg-red-700 text-sm">
                Spindle OFF
            </button>
        </div>
    );
}
