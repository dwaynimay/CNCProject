import { LineChart, Line, XAxis, YAxis, Tooltip, ResponsiveContainer, Legend } from 'recharts';
const COLORS = ['#3b82f6','#10b981','#f59e0b','#8b5cf6','#ef4444'];
const KEYS   = ['Stepper_X','Stepper_Y1','Stepper_Y2','Stepper_Z','Spindle'];

export function CurrentChart({ history }) {
    const data = history.map((h, i) => {
        const pt = { t: i };
        h.current?.forEach(s => { pt[s.id] = parseFloat(s.a); });
        return pt;
    });
    return (
        <ResponsiveContainer width="100%" height={240}>
            <LineChart data={data}>
                <XAxis dataKey="t" hide />
                <YAxis unit="A" width={45} />
                <Tooltip formatter={v => `${v.toFixed(3)} A`} />
                <Legend />
                {KEYS.map((k, i) => <Line key={k} type="monotone" dataKey={k}
                    stroke={COLORS[i]} dot={false} strokeWidth={1.5} />)}
            </LineChart>
        </ResponsiveContainer>
    );
}
