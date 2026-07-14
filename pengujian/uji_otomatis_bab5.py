"""
Skrip Otomasi Pengujian Bab 5 — Sistem Pengawasan Mesin CNC
=============================================================
Mengotomasi 4 pengujian kelompok A/B:
  1. Ambang Proteksi Arus     (5 kanal x 5 pengulangan = 25 kejadian)
  2. Ambang Proteksi Suhu     (2 kanal x 5 pengulangan = 10 kejadian)
  3. Waktu Pemutusan          (dihitung dari data kejadian no. 1 & 2)
  4. Penahanan sampai Reset   (percobaan resume tepat setelah tiap kejadian)
  5. Periode Pemantauan       (observasi pasif durasi tertentu)

Cara pakai:
  1. Sesuaikan BASE_URL dan DEVICE_ID di bagian KONFIGURASI di bawah.
  2. Pastikan server (backend Node.js) dan ESP32 sudah menyala dan terhubung.
  3. Pastikan mesin dalam kondisi AMAN untuk diuji (motor tidak sedang
     memotong benda kerja — trip-test tetap memutus relay sungguhan).
  4. Jalankan:  python uji_otomatis_bab5.py
  5. Hasil tersimpan otomatis sebagai file CSV di folder ini, siap dibuka
     di Excel atau diplot langsung.

Kebutuhan: pip install requests
"""

import requests
import time
import csv
import json
from datetime import datetime

# ── KONFIGURASI ──────────────────────────────────────────────────────────
# ── KONFIGURASI ──────────────────────────────────────────────────────────
BASE_URL = "http://192.168.1.8:3010"   # sesuai HTTP_PORT di .env backend
DEVICE_ID = "cnc-esp32"                  # sesuai MQTT_ID di credentials.h ESP32

JUMLAH_KANAL_ARUS = 5      # Stepper X, Y1, Y2, Z, Spindle (indeks 0-4)
JUMLAH_KANAL_SUHU = 2       # Spindle, Stepper Z (indeks 0-1)
PENGULANGAN = 10             # sesuai kesepakatan: 5x tiap titik

JEDA_ANTAR_TRIGGER = 8       # detik, beri waktu siklus 2 detik + margin
JEDA_TUNGGU_ALARM = 3        # detik, tunggu telemetri baru masuk setelah trigger

NAMA_KANAL_ARUS = ["Stepper_X", "Stepper_Y1", "Stepper_Y2", "Stepper_Z", "Spindle"]
NAMA_KANAL_SUHU = ["Spindle", "Stepper_Z"]

# ── FUNGSI BANTUAN API ───────────────────────────────────────────────────

def kirim_perintah(cmd):
    """Kirim perintah via POST /api/command/:id, kembalikan True jika sukses."""
    url = f"{BASE_URL}/api/command/{DEVICE_ID}"
    try:
        r = requests.post(url, json={"cmd": cmd}, timeout=5)
        r.raise_for_status()
        return r.json().get("ok", False)
    except Exception as e:
        print(f"  [ERROR] Gagal kirim '{cmd}': {e}")
        return False


def ambil_riwayat(limit=50):
    """Ambil riwayat telemetri terbaru via GET /api/history/:id."""
    url = f"{BASE_URL}/api/history/{DEVICE_ID}"
    try:
        r = requests.get(url, params={"limit": limit}, timeout=5)
        r.raise_for_status()
        return r.json().get("data", [])
    except Exception as e:
        print(f"  [ERROR] Gagal ambil riwayat: {e}")
        return []


def ambil_data_terkini():
    """Ambil data telemetri terkini via GET /api/latest/:id."""
    url = f"{BASE_URL}/api/latest/{DEVICE_ID}"
    try:
        r = requests.get(url, timeout=5)
        r.raise_for_status()
        return r.json()
    except Exception as e:
        print(f"  [ERROR] Gagal ambil data terkini: {e}")
        return None


# ── PENGUJIAN 1 & 3: AMBANG PROTEKSI ARUS + WAKTU PEMUTUSAN ─────────────

def uji_ambang_arus():
    print("\n" + "=" * 60)
    print("PENGUJIAN: Ambang Proteksi Arus & Waktu Pemutusan")
    print("=" * 60)

    hasil = []

    for kanal_idx in range(JUMLAH_KANAL_ARUS):
        nama_kanal = NAMA_KANAL_ARUS[kanal_idx]
        for ulang in range(1, PENGULANGAN + 1):
            print(f"\n[Arus] Kanal {nama_kanal}, percobaan {ulang}/{PENGULANGAN}")

            # Catat waktu sebelum trigger sebagai acuan
            t_kirim = datetime.now()

            # Kirim perintah trip-test (indeks 0 pakai cmd tanpa suffix, sesuai firmware)
            cmd = "test_overcurrent" if kanal_idx == 0 else f"test_overcurrent:{kanal_idx}"
            sukses_kirim = kirim_perintah(cmd)
            if not sukses_kirim:
                print("  Gagal mengirim perintah, lewati percobaan ini.")
                continue

            # Tunggu firmware memproses & mengirim telemetri alarm
            time.sleep(JEDA_TUNGGU_ALARM)

            # Ambil riwayat, cari baris dengan alarm=true pada kanal ini
            riwayat = ambil_riwayat(limit=10)
            baris_alarm = None
            for baris in riwayat:
                data = baris.get("data", {})
                arus_list = data.get("current", [])
                if kanal_idx < len(arus_list) and arus_list[kanal_idx].get("alarm"):
                    baris_alarm = baris
                    break

            if baris_alarm:
                t_alarm = baris_alarm.get("recordedAt", "")
                device_epoch = baris_alarm["data"].get("ts", "")
                nilai_arus = baris_alarm["data"]["current"][kanal_idx].get("a")
                relay_status = baris_alarm["data"].get("relayOn")
                print(f"  Alarm terdeteksi. Arus tercatat: {nilai_arus} A, relayOn={relay_status}, "
                      f"device_epoch={device_epoch}")
                hasil.append({
                    "kanal": nama_kanal,
                    "percobaan": ulang,
                    "waktu_kirim_pc": t_kirim.isoformat(),
                    "waktu_alarm_server": t_alarm,
                    "device_epoch": device_epoch,
                    "arus_tercatat_A": nilai_arus,
                    "alarm_terpicu": True,
                    "relay_terputus": relay_status is False or relay_status == 0,
                })
            else:
                print("  Alarm TIDAK terdeteksi dalam riwayat — periksa manual.")
                hasil.append({
                    "kanal": nama_kanal,
                    "percobaan": ulang,
                    "waktu_kirim_pc": t_kirim.isoformat(),
                    "waktu_alarm_server": "",
                    "device_epoch": "",
                    "arus_tercatat_A": "",
                    "alarm_terpicu": False,
                    "relay_terputus": "",
                })

            # Coba resume (relay_off) supaya siap untuk percobaan berikutnya
            time.sleep(1)
            kirim_perintah("relay_off")
            time.sleep(JEDA_ANTAR_TRIGGER)

    simpan_csv("hasil_ambang_arus.csv", hasil)
    return hasil


# ── PENGUJIAN 2 & 3: AMBANG PROTEKSI SUHU + WAKTU PEMUTUSAN ─────────────

def uji_ambang_suhu():
    print("\n" + "=" * 60)
    print("PENGUJIAN: Ambang Proteksi Suhu & Waktu Pemutusan")
    print("=" * 60)

    hasil = []

    for kanal_idx in range(JUMLAH_KANAL_SUHU):
        nama_kanal = NAMA_KANAL_SUHU[kanal_idx]
        for ulang in range(1, PENGULANGAN + 1):
            print(f"\n[Suhu] Kanal {nama_kanal}, percobaan {ulang}/{PENGULANGAN}")

            t_kirim = datetime.now()
            cmd = "test_overtemp" if kanal_idx == 0 else f"test_overtemp:{kanal_idx}"
            sukses_kirim = kirim_perintah(cmd)
            if not sukses_kirim:
                print("  Gagal mengirim perintah, lewati percobaan ini.")
                continue

            time.sleep(JEDA_TUNGGU_ALARM)

            riwayat = ambil_riwayat(limit=10)
            baris_alarm = None
            for baris in riwayat:
                data = baris.get("data", {})
                suhu_list = data.get("temp", [])
                if kanal_idx < len(suhu_list) and suhu_list[kanal_idx].get("alarm"):
                    baris_alarm = baris
                    break

            if baris_alarm:
                t_alarm = baris_alarm.get("recordedAt", "")
                device_epoch = baris_alarm["data"].get("ts", "")
                nilai_suhu = baris_alarm["data"]["temp"][kanal_idx].get("c")
                relay_status = baris_alarm["data"].get("relayOn")
                print(f"  Alarm terdeteksi. Suhu tercatat: {nilai_suhu} C, relayOn={relay_status}, "
                      f"device_epoch={device_epoch}")
                hasil.append({
                    "kanal": nama_kanal,
                    "percobaan": ulang,
                    "waktu_kirim_pc": t_kirim.isoformat(),
                    "waktu_alarm_server": t_alarm,
                    "device_epoch": device_epoch,
                    "suhu_tercatat_C": nilai_suhu,
                    "alarm_terpicu": True,
                    "relay_terputus": relay_status is False or relay_status == 0,
                })
            else:
                print("  Alarm TIDAK terdeteksi dalam riwayat — periksa manual.")
                hasil.append({
                    "kanal": nama_kanal,
                    "percobaan": ulang,
                    "waktu_kirim_pc": t_kirim.isoformat(),
                    "waktu_alarm_server": "",
                    "device_epoch": "",
                    "suhu_tercatat_C": "",
                    "alarm_terpicu": False,
                    "relay_terputus": "",
                })

            time.sleep(1)
            kirim_perintah("relay_off")
            time.sleep(JEDA_ANTAR_TRIGGER)

    simpan_csv("hasil_ambang_suhu.csv", hasil)
    return hasil


# ── PENGUJIAN 4 (LAMA) — DIHAPUS, LIHAT CATATAN ─────────────────────────
# Fungsi uji_penahanan_reset() yang sebelumnya ada di sini SUDAH DIGANTIKAN
# oleh uji_histeresis_otomatis.py, yang menguji 3 zona nilai (di atas ambang,
# zona histeresis, di bawah ambang resume) dengan timestamp presisi per-tahap,
# bukan cuma 2 percobaan resume kasar seperti versi lama ini.
# Jalankan uji_histeresis_otomatis.py terpisah untuk pengujian Penahanan
# sampai Reset yang lengkap.


# ── PENGUJIAN 5: PERIODE PEMANTAUAN (OBSERVASI PASIF) ───────────────────

def uji_periode_pemantauan(durasi_detik=180):
    print("\n" + "=" * 60)
    print(f"PENGUJIAN: Periode Pemantauan (observasi pasif {durasi_detik} detik)")
    print("=" * 60)
    print("Pastikan mesin dibiarkan menyala normal tanpa trigger apa pun selama ini.")

    waktu_mulai_pc = datetime.now().isoformat()
    time.sleep(durasi_detik)

    # Hitung interval dari riwayat database (lebih akurat dari polling manual).
    # Ambil cukup banyak baris supaya durasi_detik/2 detik tercakup semua.
    limit_baris = max(200, int(durasi_detik / 2) + 20)
    riwayat = ambil_riwayat(limit=limit_baris)

    hasil = []
    for i in range(len(riwayat) - 1):
        try:
            baris_a = riwayat[i]
            baris_b = riwayat[i + 1]
            t1 = datetime.fromisoformat(baris_a["recordedAt"].replace(" ", "T"))
            t2 = datetime.fromisoformat(baris_b["recordedAt"].replace(" ", "T"))
            selisih = abs((t1 - t2).total_seconds())

            # Simpan JUGA timestamp mentah (bukan cuma selisih hasil hitungan),
            # termasuk device_epoch (field "ts" milik ESP32) untuk cross-check
            # dengan log Serial yang sudah dicetak epoch-nya juga.
            device_ts_a = baris_a.get("data", {}).get("ts", "")
            device_ts_b = baris_b.get("data", {}).get("ts", "")

            hasil.append({
                "index": i,
                "recorded_at_a": baris_a["recordedAt"],
                "recorded_at_b": baris_b["recordedAt"],
                "device_epoch_a": device_ts_a,
                "device_epoch_b": device_ts_b,
                "selisih_detik": selisih,
            })
        except Exception as e:
            print(f"  [WARN] Lewati baris {i}, gagal parse: {e}")
            continue

    if hasil:
        rata2 = sum(h["selisih_detik"] for h in hasil) / len(hasil)
        print(f"Jumlah sampel interval: {len(hasil)}, rata-rata: {rata2:.3f} detik")
    else:
        print("  [WARN] Tidak ada data interval yang berhasil dihitung — cek koneksi/riwayat.")

    print(f"Observasi dimulai (jam PC): {waktu_mulai_pc}")
    simpan_csv("hasil_periode_pemantauan.csv", hasil)
    return hasil


# ── UTILITAS SIMPAN CSV ──────────────────────────────────────────────────

def simpan_csv(nama_file, data):
    if not data:
        print(f"  (Tidak ada data untuk disimpan ke {nama_file})")
        return
    with open(nama_file, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=data[0].keys())
        writer.writeheader()
        writer.writerows(data)
    print(f"  Tersimpan: {nama_file} ({len(data)} baris)")


# ── ORKESTRATOR UTAMA ─────────────────────────────────────────────────────

def main():
    print("=" * 60)
    print("MULAI PENGUJIAN OTOMATIS BAB 5")
    print(f"Target: {BASE_URL} | Device: {DEVICE_ID}")
    print("=" * 60)
    konfirmasi = input(
        "\nPastikan mesin AMAN untuk diuji (motor tidak sedang memotong).\n"
        "Lanjutkan? (y/n): "
    )
    if konfirmasi.lower() != "y":
        print("Dibatalkan.")
        return

    mulai = time.time()

    uji_ambang_arus()
    uji_ambang_suhu()
    uji_periode_pemantauan(durasi_detik=180)

    selesai = time.time()
    print("\n" + "=" * 60)
    print(f"SELESAI. Total waktu: {(selesai - mulai) / 60:.1f} menit")
    print("Semua hasil tersimpan sebagai file CSV di folder ini:")
    print("  - hasil_ambang_arus.csv")
    print("  - hasil_ambang_suhu.csv")
    print("  - hasil_periode_pemantauan.csv")
    print("Untuk pengujian Penahanan sampai Reset (3 zona, timestamp presisi),")
    print("jalankan skrip terpisah: uji_histeresis_otomatis.py")
    print("=" * 60)


if __name__ == "__main__":
    main()
