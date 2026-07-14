"""
Pembuat Grafik Bab 5 — Gaya Dual-Axis dengan Status Relay
============================================================
Adaptasi dari template grafik CD, disesuaikan supaya:
1. Membaca langsung dari database SQLite backend (data kontinu tiap 2 detik),
   bukan dari CSV snapshot hasil skrip pengujian.
2. Jendela waktu tiap grafik DITENTUKAN OTOMATIS dari timestamp yang sudah
   tercatat di log_detail_histeresis_*.csv / hasil_ambang_*.csv, tidak perlu
   ketik manual tanggal-jam satu-satu seperti template asal.

Cara pakai:
1. Sesuaikan DB_PATH ke lokasi cnc_iot.db kamu (biasanya di
   server/backend/data/cnc_iot.db).
2. Pastikan CSV hasil pengujian (log_detail_histeresis_arus.csv, dst.)
   ada di folder yang sama dengan skrip ini.
3. Jalankan: python buat_grafik_bab5_v2.py
4. Hasil PNG tersimpan di folder OUTPUT_DIR.

Kebutuhan: pip install matplotlib pandas
"""

import sqlite3
import json
import os
import pandas as pd
from datetime import datetime, timedelta

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

# --- Argparse: pilih folder target dan opsi output/db ---
import argparse

script_dir = os.path.dirname(os.path.abspath(__file__))
parser = argparse.ArgumentParser(description="Buat grafik dari hasil pengujian dan/atau DB telemetry.")
parser.add_argument("subfolder", nargs="?", default=".",
                    help="Subfolder di bawah folder skrip yang berisi CSV hasil pengujian (default: folder skrip).")
parser.add_argument("--out-subdir", default="grafik_bab5_v2",
                    help="Nama folder output di dalam folder target. Gunakan '.' untuk simpan langsung di folder target.")
parser.add_argument("--db", default=None,
                    help="Path ke database sqlite (opsional). Jika tidak diberikan, konfigurasi DB_PATH akan dipakai.")
args = parser.parse_args()

if os.path.isabs(args.subfolder):
    WORK_DIR = os.path.abspath(args.subfolder)
else:
    WORK_DIR = os.path.abspath(os.path.join(script_dir, args.subfolder))

if args.out_subdir == ".":
    OUTPUT_DIR = WORK_DIR
else:
    OUTPUT_DIR = os.path.join(WORK_DIR, args.out_subdir)

os.makedirs(OUTPUT_DIR, exist_ok=True)

# ============================================================
# KONFIGURASI
# ============================================================

# DB untuk grafik 1 (tren arus periode pemantauan) — rentang waktunya cocok
# dengan hasil_periode_pemantauan.csv / hasil_ambang_*.csv (sesi jam 19:13-19:59).
DB_PATH = "..\\..\\server\\backend\\data_hasil pengujian dirumah fix ini gess masih 70 persen\\cnc_iot.db"

# DB untuk grafik histeresis (2, 3, 4) — log_detail_histeresis_*.csv berisi
# sesi baru 10 percobaan pada jam 20:20-20:56, yang cocok dengan DB ini.
DB_PATH_HISTERESIS = "..\\..\\server\\backend\\data_hail pengujian di rumah ke 2\\cnc_iot.db"

# Resolve DB path: prefer CLI --db, then look in WORK_DIR, else relative to script
if args.db:
    if os.path.isabs(args.db):
        DB_PATH = args.db
    elif os.path.exists(os.path.join(WORK_DIR, args.db)):
        DB_PATH = os.path.join(WORK_DIR, args.db)
    else:
        DB_PATH = os.path.join(script_dir, args.db)
else:
    if not os.path.isabs(DB_PATH):
        DB_PATH = os.path.join(script_dir, DB_PATH)

if not os.path.isabs(DB_PATH_HISTERESIS):
    DB_PATH_HISTERESIS = os.path.join(script_dir, DB_PATH_HISTERESIS)

plt.rcParams.update({
    "font.family": "DejaVu Sans",
    "font.size": 10,
    "axes.grid": True,
    "grid.alpha": 0.3,
})

CHANNEL_COLORS = {
    "Stepper_X": "#1f77b4",
    "Stepper_Y1": "#ff7f0e",
    "Stepper_Y2": "#2ca02c",
    "Stepper_Z": "#d62728",
    "Spindle": "#9467bd",
}


# ============================================================
# HELPER — ambil data dari database
# ============================================================

def fetch_telemetry(conn, start, end):
    """Ambil baris telemetry antara dua timestamp 'YYYY-MM-DD HH:MM:SS'."""
    cur = conn.cursor()
    cur.execute(
        "SELECT recorded_at, payload FROM telemetry "
        "WHERE recorded_at >= ? AND recorded_at <= ? ORDER BY recorded_at ASC",
        (start, end),
    )
    rows = cur.fetchall()
    times = [datetime.strptime(r[0], "%Y-%m-%d %H:%M:%S") for r in rows]
    payloads = [json.loads(r[1]) for r in rows]
    return times, payloads


def current_series(payloads, channel):
    return [next((c["a"] for c in p["current"] if c["id"] == channel), None) for p in payloads]


def temp_series(payloads, channel):
    return [next((t["c"] for t in p["temp"] if t["id"] == channel), None) for p in payloads]


def relay_series(payloads):
    return [1 if p["relayOn"] else 0 for p in payloads]


# ============================================================
# HELPER BARU — deteksi jendela waktu otomatis dari CSV hasil pengujian
# ============================================================

_KOLOM_LOG_HISTERESIS = [
    "kanal", "percobaan", "tahap", "deskripsi", "pc_waktu",
    "device_epoch", "device_epoch_wib", "nilai_disuntik", "relayOn", "mesin_menyala",
]

_cache_log_histeresis = {}


def _baca_log_histeresis(file_csv):
    """
    Baca log_detail_histeresis_*.csv dengan tahan banting terhadap file yang
    berisi dua sesi pengujian tergabung (baris lama 10 kolom + baris sesi baru
    dengan 1 kolom tambahan penanda sesi). csv.reader dipakai langsung karena
    pandas.read_csv gagal/skip total begitu jumlah kolom antar baris berbeda.

    Kalau ditemukan baris dengan kolom ekstra (sesi baru), HANYA baris sesi
    baru itu yang dipakai (sesi lama dibuang) supaya nomor percobaan yang
    tabrakan (mis. sesi lama & baru sama-sama punya percobaan 1-5) tidak
    tercampur.
    """
    fullpath = file_csv if os.path.isabs(file_csv) else os.path.join(WORK_DIR, file_csv)
    if fullpath in _cache_log_histeresis:
        return _cache_log_histeresis[fullpath]
    if not os.path.exists(fullpath):
        _cache_log_histeresis[fullpath] = None
        return None

    import csv as _csv
    with open(fullpath, encoding="utf-8") as f:
        rows = list(_csv.reader(f))
    n_expected = len(_KOLOM_LOG_HISTERESIS)
    data_rows = [r for r in rows[1:] if len(r) >= n_expected]

    ada_sesi_baru = any(len(r) > n_expected for r in data_rows)
    if ada_sesi_baru:
        data_rows = [r for r in data_rows if len(r) > n_expected]

    data_rows = [r[:n_expected] for r in data_rows]
    df = pd.DataFrame(data_rows, columns=_KOLOM_LOG_HISTERESIS)
    df["percobaan"] = df["percobaan"].astype(int)
    df["tahap"] = df["tahap"].astype(int)
    _cache_log_histeresis[fullpath] = df
    return df


def ambil_jendela_dari_log_histeresis(file_csv, kanal, percobaan=1, margin_sebelum=8, margin_sesudah=8):
    """
    Baca log_detail_histeresis_*.csv, cari baris tahap 1 (cutoff) untuk
    kanal & percobaan tertentu, kembalikan (start, end) string timestamp
    yang mencakup seluruh 6 tahap pengujian itu (dari sedikit sebelum
    tahap 1 sampai sedikit sesudah tahap 6).
    """
    df = _baca_log_histeresis(file_csv)
    if df is None:
        print(f"  [WARN] File {file_csv} tidak ditemukan.")
        return None, None
    sub = df[(df["kanal"] == kanal) & (df["percobaan"] == percobaan)]
    if sub.empty:
        print(f"  [WARN] Tidak ada data untuk kanal={kanal} percobaan={percobaan} di {file_csv}")
        return None, None

    waktu_list = pd.to_datetime(sub["device_epoch_wib"])
    mulai = waktu_list.min() - timedelta(seconds=margin_sebelum)
    selesai = waktu_list.max() + timedelta(seconds=margin_sesudah)
    return mulai.strftime("%Y-%m-%d %H:%M:%S"), selesai.strftime("%Y-%m-%d %H:%M:%S")


def ambil_titik_tahap(file_csv, kanal, percobaan, tahap):
    """Ambil timestamp device_epoch_wib untuk satu tahap spesifik (dipakai untuk marker)."""
    df = _baca_log_histeresis(file_csv)
    if df is None:
        return None
    baris = df[(df["kanal"] == kanal) & (df["percobaan"] == percobaan) & (df["tahap"] == tahap)]
    if baris.empty:
        return None
    return pd.to_datetime(baris["device_epoch_wib"].iloc[0])


def hitung_jumlah_percobaan(file_csv, kanal):
    """Hitung berapa nomor percobaan tertinggi yang tercatat untuk kanal ini di CSV."""
    df = _baca_log_histeresis(file_csv)
    if df is None:
        return 0
    sub = df[df["kanal"] == kanal]
    if sub.empty:
        return 0
    return int(sub["percobaan"].max())


# ============================================================
# GRAFIK 1 — Tren multi-kanal dengan label fase
# ============================================================

def plot_multi_channel_trend(
    conn, start, end, channels, series_fn, title, ylabel,
    filename, phase_markers=None, ylim=None,
):
    if conn is None:
        print(f"  [WARN] Tidak ada koneksi DB, lewati {filename}")
        return
    times, payloads = fetch_telemetry(conn, start, end)
    if not times:
        print(f"  [WARN] Tidak ada data telemetri pada rentang {start} - {end}, lewati {filename}")
        return

    fig, ax = plt.subplots(figsize=(9, 4.2))
    for ch in channels:
        vals = series_fn(payloads, ch)
        ax.plot(times, vals, label=ch, color=CHANNEL_COLORS.get(ch, "black"), linewidth=1)

    if phase_markers:
        for m in phase_markers:
            if "time" in m and m["time"] is not None:
                ax.axvline(m["time"], color="gray", linestyle="--", linewidth=1)
            ax.text(
                m["x_rel"], 0.93, m["label"], fontsize=8, ha="center",
                transform=ax.transAxes,
            )

    ax.set_xlabel("Waktu")
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M:%S"))
    ax.legend(loc="lower right", ncol=min(len(channels), 5), fontsize=7.5)
    if ylim:
        ax.set_ylim(*ylim)
    plt.tight_layout()
    plt.savefig(f"{OUTPUT_DIR}/{filename}", dpi=150)
    plt.close()
    print(f"  Selesai: {filename}")


# ============================================================
# GRAFIK 2 — Respons cutoff satu kanal (dual-axis)
# ============================================================

def plot_cutoff_response(
    conn, start, end, channel, series_fn, threshold, threshold_label,
    title, ylabel, filename, annotations=None,
):
    if conn is None:
        print(f"  [WARN] Tidak ada koneksi DB, lewati {filename}")
        return
    times, payloads = fetch_telemetry(conn, start, end)
    if not times:
        print(f"  [WARN] Tidak ada data telemetri pada rentang {start} - {end}, lewati {filename}")
        return

    vals = series_fn(payloads, channel)
    relay = relay_series(payloads)

    fig, ax1 = plt.subplots(figsize=(8, 4.2))
    ax1.plot(times, vals, color=CHANNEL_COLORS.get(channel, "#1f77b4"),
             marker="o", markersize=3, linewidth=1, label=f"{ylabel} {channel}")
    ax1.axhline(threshold, color="gray", linestyle=":", linewidth=1, label=threshold_label)
    ax1.set_xlabel("Waktu")
    ax1.set_ylabel(ylabel, color=CHANNEL_COLORS.get(channel, "#1f77b4"))
    ax1.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M:%S"))

    ax2 = ax1.twinx()
    ax2.step(times, relay, color="black", where="post", linewidth=1.3, alpha=0.6,
             label="Status Relay")
    ax2.set_ylabel("Status Relay")
    ax2.set_ylim(-0.1, 1.3)
    ax2.set_yticks([0, 1])
    ax2.set_yticklabels(["OFF (nyala)", "ON (mati)"])

    if annotations:
        for a in annotations:
            ax1.annotate(
                a["text"], xy=a["xy"], xytext=a["xytext"], fontsize=8,
                arrowprops=dict(arrowstyle="->", color="gray"),
            )

    fig.suptitle(title)
    fig.legend(loc="upper center", bbox_to_anchor=(0.5, 0.02), ncol=2, fontsize=7.5)
    plt.tight_layout()
    plt.savefig(f"{OUTPUT_DIR}/{filename}", dpi=150, bbox_inches="tight")
    plt.close()
    print(f"  Selesai: {filename}")


# ============================================================
# GRAFIK 3 — Dua panel berdampingan
# ============================================================

def plot_two_panel_cutoff(conn, panel_configs, suptitle, filename):
    if conn is None:
        print(f"  [WARN] Tidak ada koneksi DB, lewati {filename}")
        return
    fig, axes = plt.subplots(1, 2, figsize=(11, 4.2))
    for ax, cfg in zip(axes, panel_configs):
        times, payloads = fetch_telemetry(conn, cfg["start"], cfg["end"])
        if not times:
            print(f"  [WARN] Tidak ada data untuk panel {cfg.get('title')}")
            continue
        vals = cfg["series_fn"](payloads, cfg["channel"])
        relay = relay_series(payloads)

        ax.plot(times, vals, color=CHANNEL_COLORS.get(cfg["channel"], "#d62728"),
                marker="o", markersize=3, label=cfg.get("ylabel", "Nilai"))
        ax.axhline(cfg["threshold"], color="gray", linestyle=":", linewidth=1,
                   label=cfg.get("threshold_label", "Ambang"))
        axb = ax.twinx()
        axb.step(times, relay, color="black", where="post", linewidth=1.3, alpha=0.5)
        axb.set_ylim(-0.1, 1.3)
        axb.set_yticks([0, 1])
        axb.set_yticklabels(["OFF", "ON"])

        ax.set_title(cfg["title"])
        ax.set_xlabel("Waktu")
        ax.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M:%S"))
        ax.legend(fontsize=7, loc="upper right")
        ax.tick_params(axis="x", rotation=30)

    fig.suptitle(suptitle)
    plt.tight_layout()
    plt.savefig(f"{OUTPUT_DIR}/{filename}", dpi=150, bbox_inches="tight")
    plt.close()
    print(f"  Selesai: {filename}")


# ============================================================
# GRAFIK 4 — Overlay semua percobaan (N garis) dalam 1 grafik
# Sumbu-X waktu RELATIF (detik sejak tahap 1/cutoff), supaya bisa
# ditumpuk meski tiap percobaan terjadi di jam berbeda.
# ============================================================

def plot_overlay_semua_percobaan(
    conn, file_csv, kanal, jumlah_percobaan, series_fn, threshold, threshold_label,
    title, ylabel, filename, margin_sebelum=8, margin_sesudah=20,
):
    if conn is None:
        print(f"  [WARN] Tidak ada koneksi DB, lewati {filename}")
        return
    if jumlah_percobaan <= 0:
        print(f"  [WARN] Tidak ada percobaan untuk kanal {kanal} di {file_csv}, lewati {filename}")
        return

    fig, ax1 = plt.subplots(figsize=(9, 5))
    # Palet warna kuat (tab10 diulang) — bukan colormap gradien, supaya tidak
    # ada garis yang jatuh di area pucat/kuning terang yang susah dilihat.
    warna_list = plt.cm.tab10.colors
    berhasil = 0

    for p in range(1, jumlah_percobaan + 1):
        t0 = ambil_titik_tahap(file_csv, kanal, p, tahap=1)
        if t0 is None:
            print(f"  [WARN] Percobaan {p} tidak ditemukan, lewati")
            continue

        mulai = (t0 - timedelta(seconds=margin_sebelum)).strftime("%Y-%m-%d %H:%M:%S")
        selesai = (t0 + timedelta(seconds=margin_sesudah)).strftime("%Y-%m-%d %H:%M:%S")
        times, payloads = fetch_telemetry(conn, mulai, selesai)
        if not times:
            continue

        # Konversi ke waktu relatif (detik sejak t0/tahap 1)
        t_relatif = [(t - t0).total_seconds() for t in times]
        vals = series_fn(payloads, kanal)

        warna = warna_list[(p - 1) % len(warna_list)]
        ax1.plot(t_relatif, vals, color=warna, linewidth=1.6, alpha=0.9,
                 marker="o", markersize=3, label=f"Percobaan {p}")
        berhasil += 1

    if berhasil == 0:
        print(f"  [WARN] Tidak ada percobaan bertemu data telemetri, lewati {filename}")
        plt.close(fig)
        return

    ax1.axhline(threshold, color="gray", linestyle=":", linewidth=1.2, label=threshold_label)
    ax1.axvline(0, color="black", linestyle="--", linewidth=0.8, alpha=0.5)
    ax1.set_xlabel("Waktu Relatif terhadap Tahap 1 / Cutoff (detik)")
    ax1.set_ylabel(ylabel)
    ax1.set_title(f"{title}\n(overlay {berhasil} percobaan, sumbu waktu dinormalisasi)")
    ax1.legend(loc="upper right", fontsize=7, ncol=2)
    plt.tight_layout()
    plt.savefig(f"{OUTPUT_DIR}/{filename}", dpi=150, bbox_inches="tight")
    plt.close()
    print(f"  Selesai: {filename} ({berhasil}/{jumlah_percobaan} percobaan tergambar)")


# ============================================================
# ORKESTRATOR — deteksi otomatis dari CSV, lalu buat semua grafik
# ============================================================

def main():
    if not os.path.exists(DB_PATH):
        print(f"[WARN] Database tidak ditemukan di {DB_PATH}. Beberapa grafik berbasis telemetry akan dilewati.")
        conn = None
    else:
        conn = sqlite3.connect(DB_PATH)

    if not os.path.exists(DB_PATH_HISTERESIS):
        print(f"[WARN] Database histeresis tidak ditemukan di {DB_PATH_HISTERESIS}. Grafik cutoff/overlay akan dilewati.")
        conn_hist = None
    else:
        conn_hist = sqlite3.connect(DB_PATH_HISTERESIS)

    # ── Grafik 1: Tren 5 kanal arus selama sesi periode pemantauan ──
    periode_fp = os.path.join(WORK_DIR, "hasil_periode_pemantauan.csv")
    if os.path.exists(periode_fp):
        df_periode = pd.read_csv(periode_fp)
        if not df_periode.empty and "recorded_at_a" in df_periode.columns:
            mulai = df_periode["recorded_at_a"].min()
            selesai = df_periode["recorded_at_b"].max()
            plot_multi_channel_trend(
                conn, start=mulai, end=selesai,
                channels=list(CHANNEL_COLORS.keys()),
                series_fn=current_series,
                title=f"Tren Arus Lima Kanal Selama Sesi Periode Pemantauan",
                ylabel="Arus (A)",
                filename="tren_arus_periode_pemantauan.png",
            )

    # ── Grafik 2: Respons cutoff per kanal arus (ambil percobaan 1 tiap kanal) ──
    log_arus_fp = os.path.join(WORK_DIR, "log_detail_histeresis_arus.csv")
    if os.path.exists(log_arus_fp):
        for kanal, ambang in zip(
            ["Stepper_X", "Stepper_Y1", "Stepper_Y2", "Stepper_Z", "Spindle"],
            [3.0, 3.0, 3.0, 2.0, 3.0],
        ):
            mulai, selesai = ambil_jendela_dari_log_histeresis(
                log_arus_fp, kanal, percobaan=1,
            )
            if mulai:
                plot_cutoff_response(
                    conn_hist, start=mulai, end=selesai,
                    channel=kanal, series_fn=current_series,
                    threshold=ambang, threshold_label=f"Ambang alarm ({ambang} A)",
                    title=f"Respons Cutoff Uji Histeresis Kanal {kanal}",
                    ylabel="Arus",
                    filename=f"cutoff_response_{kanal.lower()}.png",
                )

        # ── Grafik 4: Overlay semua percobaan per kanal arus ──
        for kanal, ambang in zip(
            ["Stepper_X", "Stepper_Y1", "Stepper_Y2", "Stepper_Z", "Spindle"],
            [3.0, 3.0, 3.0, 2.0, 3.0],
        ):
            jumlah = hitung_jumlah_percobaan(log_arus_fp, kanal)
            plot_overlay_semua_percobaan(
                conn_hist, log_arus_fp, kanal, jumlah_percobaan=jumlah,
                series_fn=current_series, threshold=ambang,
                threshold_label=f"Ambang alarm ({ambang} A)",
                title=f"Konsistensi Respons Cutoff Kanal {kanal} — Seluruh Percobaan",
                ylabel="Arus (A)",
                filename=f"overlay_percobaan_{kanal.lower()}.png",
            )

    # ── Grafik 3: Dua panel suhu berdampingan (Spindle vs Stepper_Z, percobaan 1) ──
    log_suhu_fp = os.path.join(WORK_DIR, "log_detail_histeresis_suhu.csv")
    if os.path.exists(log_suhu_fp):
        m1, s1 = ambil_jendela_dari_log_histeresis(log_suhu_fp, "Spindle", 1)
        m2, s2 = ambil_jendela_dari_log_histeresis(log_suhu_fp, "Stepper_Z", 1)
        if m1 and m2:
            plot_two_panel_cutoff(
                conn_hist,
                panel_configs=[
                    {"start": m1, "end": s1, "channel": "Spindle", "series_fn": temp_series,
                     "threshold": 60, "threshold_label": "Ambang alarm (60°C)",
                     "title": "Uji Histeresis Suhu — Spindle", "ylabel": "Suhu (°C)"},
                    {"start": m2, "end": s2, "channel": "Stepper_Z", "series_fn": temp_series,
                     "threshold": 55, "threshold_label": "Ambang alarm (55°C)",
                     "title": "Uji Histeresis Suhu — Stepper_Z", "ylabel": "Suhu (°C)"},
                ],
                suptitle="Respons Cutoff pada Uji Histeresis Suhu (3 Zona)",
                filename="cutoff_response_suhu_dua_panel.png",
            )

        # ── Grafik 4 (suhu): Overlay semua percobaan per kanal suhu ──
        for kanal, ambang in zip(["Spindle", "Stepper_Z"], [60.0, 55.0]):
            jumlah = hitung_jumlah_percobaan(log_suhu_fp, kanal)
            plot_overlay_semua_percobaan(
                conn_hist, log_suhu_fp, kanal, jumlah_percobaan=jumlah,
                series_fn=temp_series, threshold=ambang,
                threshold_label=f"Ambang alarm ({ambang}°C)",
                title=f"Konsistensi Respons Cutoff Suhu {kanal} — Seluruh Percobaan",
                ylabel="Suhu (°C)",
                filename=f"overlay_percobaan_suhu_{kanal.lower()}.png",
                margin_sebelum=8, margin_sesudah=25,
            )

    if conn:
        conn.close()
    if conn_hist:
        conn_hist.close()
    print(f"\nSelesai. Semua grafik tersimpan di: {os.path.abspath(OUTPUT_DIR)}")


if __name__ == "__main__":
    main()