export function CurrentCard({ sensor }) {
    const { id, a, alarm } = sensor;
    return (
        <div className={`rounded-lg p-4 border ${alarm ? 'border-red-500 bg-red-50' : 'border-gray-200 bg-white'}`}>
            <p className="text-xs text-gray-500 mb-1">{id}</p>
            <p className={`text-2xl font-mono font-bold ${alarm ? 'text-red-600' : 'text-gray-800'}`}>
                {parseFloat(a).toFixed(3)} A
            </p>
            {alarm && <p className="text-xs text-red-500 mt-1">ALARM</p>}
        </div>
    );
}
