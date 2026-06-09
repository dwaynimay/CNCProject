export function TempCard({ sensor }) {
    const { id, c, alarm } = sensor;
    return (
        <div className={`rounded-lg p-4 border ${alarm ? 'border-orange-500 bg-orange-50' : 'border-gray-200 bg-white'}`}>
            <p className="text-xs text-gray-500 mb-1">{id}</p>
            <p className={`text-2xl font-mono font-bold ${alarm ? 'text-orange-600' : 'text-gray-800'}`}>
                {parseFloat(c).toFixed(1)} °C
            </p>
            {alarm && <p className="text-xs text-orange-500 mt-1">OVERHEAT</p>}
        </div>
    );
}
