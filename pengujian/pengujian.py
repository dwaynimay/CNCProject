"""
Skrip Otomasi Uji Histeresis End-to-End — Sistem Pengawasan Mesin CNC
========================================================================
Menguji perilaku resume pada 3 zona nilai arus secara berurutan, sepenuhnya
otomatis, TANPA perlu arus fisik sungguhan atau power supply terkendali.

Prasyarat: firmware sudah diupdate dengan command test_hold/test_clear
(lihat diff_test_hold_firmware.patch), sudah di-compile dan di-upload ke ESP32.

Alur per kanal per pengulangan:
  1. test_hold:<idx>:<150% ambang>   -> relay harus cutoff otomatis
  2. Coba relay_off saat masih tinggi -> HARUS DITOLAK
  3. test_hold:<idx>:<zona histeresis> -> masih di atas ambang resume
  4. Coba relay_off di zona histeresis -> HARUS TETAP DITOLAK
  5. test_hold:<idx>:<di bawah ambang resume>
  6. Coba relay_off -> HARUS DITERIMA
  7. test_clear:<idx> -> kembali ke pembacaan sensor asli

Kebutuhan: pip install requests
"""

import requests
import time
import csv
from datetime import datetime

# ── KONFIGURASI ──────────────────────────────────────────────────────────
# ── KONFIGURASI ──────────────────────────────────────────────────────────
BASE_URL = "http://192.168.1.8:3010"   # sesuai HTTP_PORT di .env backend
DEVICE_ID = "cnc-esp32"                  # sesuai MQTT_ID di credentials.h ESP32

PENGULANGAN = 5

# Ambang alarm & histeresis per kanal arus, sesuai SensorConfig.h firmware
# (Stepper_X, Stepper_Y1, Stepper_Y2, Stepper_Z, Spindle)
NAMA_KANAL   = ["Stepper_X", "Stepper_Y1", "Stepper_Y2", "Stepper_Z", "Spindle"]
AMBANG_ALARM = [3.0, 3.0, 3.0, 2.0, 3.0]
HISTERESIS   = 0.5  # sama untuk semua kanal, sesuai CURRENT_HYSTERESIS di firmware

# Kanal suhu (Spindle, Stepper_Z), sesuai TEMP_ALARM & TEMP_HYSTERESIS di firmware
NAMA_KANAL_SUHU   = ["Spindle", "Stepper_Z"]
AMBANG_ALARM_SUHU = [60.0, 55.0]
HISTERESIS_SUHU   = 5.0

JEDA_SIKLUS = 3          # detik, > 1 siklus publish (2 detik) supaya nilai baru terbaca
JEDA_ANTAR_PERCOBAAN = 5  # detik, jeda antar pengulangan


def kirim_perintah(cmd):
    url = f"{BASE_URL}/api/command/{DEVICE_ID}"
    try:
        r = requests.post(url, json={"cmd": cmd}, timeout=5)
        r.raise_for_status()
        return r.json().get("ok", False)
    except Exception as e:
        print(f"    [ERROR] Gagal kirim '{cmd}': {e}")
        return False


def ambil_data_terkini():
    url = f"{BASE_URL}/api/latest/{DEVICE_ID}"
    try:
        r = requests.get(url, timeout=5)
        r.raise_for_status()
        return r.json()
    except Exception as e:
        print(f"    [ERROR] Gagal ambil data terkini: {e}")
        return None


def tahan_nilai(kanal_idx, nilai):
    """Kirim test_hold untuk menahan nilai arus kanal tertentu."""
    return kirim_perintah(f"test_hold:{kanal_idx}:{nilai}")


def hentikan_penahanan(kanal_idx):
    """Kirim test_clear untuk kembali ke pembacaan sensor asli (arus)."""
    return kirim_perintah(f"test_clear:{kanal_idx}")


def tahan_nilai_suhu(kanal_idx, nilai):
    """Kirim test_hold_temp untuk menahan nilai suhu kanal tertentu."""
    return kirim_perintah(f"test_hold_temp:{kanal_idx}:{nilai}")


def hentikan_penahanan_suhu(kanal_idx):
    """Kirim test_clear_temp untuk kembali ke pembacaan sensor asli (suhu)."""
    return kirim_perintah(f"test_clear_temp:{kanal_idx}")


def coba_resume():
    """Kirim relay_off, lalu cek status relay setelahnya."""
    kirim_perintah("relay_off")
    time.sleep(JEDA_SIKLUS)
    data = ambil_data_terkini()
    if data is None:
        return None
    relay_on = data.get("relayOn")
    # relay NC: relayOn=True berarti energized = kontak putus = MESIN MATI
    mesin_menyala = (relay_on is False) or (relay_on == 0)
    return mesin_menyala


def uji_histeresis_end_to_end():
    print("=" * 70)
    print("UJI HISTERESIS END-TO-END (3 ZONA NILAI, OTOMATIS PENUH)")
    print("=" * 70)

    hasil = []

    for kanal_idx in range(len(NAMA_KANAL)):
        nama = NAMA_KANAL[kanal_idx]
        ambang = AMBANG_ALARM[kanal_idx]
        ambang_resume = ambang - HISTERESIS

        nilai_tinggi = round(ambang * 1.5, 2)          # di atas ambang alarm
        nilai_histeresis = round(ambang - 0.2, 2)        # di zona histeresis (antara resume & alarm)
        nilai_rendah = round(ambang_resume - 0.3, 2)     # di bawah ambang resume

        print(f"\n--- Kanal: {nama} (ambang={ambang}A, resume<={ambang_resume}A) ---")

        for ulang in range(1, PENGULANGAN + 1):
            print(f"\n  Percobaan {ulang}/{PENGULANGAN}")
            waktu = datetime.now().isoformat()

            # Tahap 1: nilai tinggi -> harus cutoff
            print(f"    [1] Tahan nilai {nilai_tinggi}A (di atas ambang alarm)")
            tahan_nilai(kanal_idx, nilai_tinggi)
            time.sleep(JEDA_SIKLUS)
            data_setelah_tinggi = ambil_data_terkini()
            relay_setelah_tinggi = data_setelah_tinggi.get("relayOn") if data_setelah_tinggi else None
            cutoff_terjadi = relay_setelah_tinggi is True or relay_setelah_tinggi == 1
            print(f"        -> relayOn={relay_setelah_tinggi} (cutoff terjadi: {cutoff_terjadi})")

            # Tahap 2: coba resume saat masih tinggi -> harus ditolak
            print(f"    [2] Coba resume saat masih {nilai_tinggi}A (harus DITOLAK)")
            resume1_diterima = coba_resume()
            print(f"        -> mesin menyala setelah resume: {resume1_diterima} (harus False)")

            # Tahap 3: turun ke zona histeresis -> masih harus ditolak
            print(f"    [3] Tahan nilai {nilai_histeresis}A (zona histeresis)")
            tahan_nilai(kanal_idx, nilai_histeresis)
            time.sleep(JEDA_SIKLUS)

            print(f"    [4] Coba resume di zona histeresis (harus TETAP DITOLAK)")
            resume2_diterima = coba_resume()
            print(f"        -> mesin menyala setelah resume: {resume2_diterima} (harus False)")

            # Tahap 4: turun di bawah ambang resume -> harus diterima
            print(f"    [5] Tahan nilai {nilai_rendah}A (di bawah ambang resume)")
            tahan_nilai(kanal_idx, nilai_rendah)
            time.sleep(JEDA_SIKLUS)

            print(f"    [6] Coba resume di bawah ambang resume (harus DITERIMA)")
            resume3_diterima = coba_resume()
            print(f"        -> mesin menyala setelah resume: {resume3_diterima} (harus True)")

            # Bersihkan: hentikan penahanan, kembali ke normal
            hentikan_penahanan(kanal_idx)
            time.sleep(1)
            kirim_perintah("relay_off")  # pastikan mesin menyala untuk percobaan berikutnya

            hasil.append({
                "kanal": nama,
                "percobaan": ulang,
                "waktu": waktu,
                "ambang_alarm_A": ambang,
                "ambang_resume_A": ambang_resume,
                "nilai_tinggi_A": nilai_tinggi,
                "cutoff_terjadi": cutoff_terjadi,
                "nilai_histeresis_A": nilai_histeresis,
                "resume_ditolak_saat_tinggi": not resume1_diterima if resume1_diterima is not None else "",
                "resume_ditolak_saat_histeresis": not resume2_diterima if resume2_diterima is not None else "",
                "nilai_rendah_A": nilai_rendah,
                "resume_diterima_saat_rendah": resume3_diterima,
                "sesuai_ekspektasi": (
                    cutoff_terjadi
                    and resume1_diterima is False
                    and resume2_diterima is False
                    and resume3_diterima is True
                ),
            })

            time.sleep(JEDA_ANTAR_PERCOBAAN)

    simpan_csv("hasil_histeresis_end_to_end.csv", hasil)

    total = len(hasil)
    sesuai = sum(1 for h in hasil if h["sesuai_ekspektasi"])
    print("\n" + "=" * 70)
    print(f"RINGKASAN ARUS: {sesuai}/{total} percobaan sesuai ekspektasi penuh")
    print("=" * 70)

    return hasil


def uji_histeresis_suhu():
    """Sama persis dengan uji_histeresis_end_to_end(), tapi untuk 2 kanal suhu."""
    print("=" * 70)
    print("UJI HISTERESIS END-TO-END SUHU (3 ZONA NILAI, OTOMATIS PENUH)")
    print("=" * 70)

    hasil = []

    for kanal_idx in range(len(NAMA_KANAL_SUHU)):
        nama = NAMA_KANAL_SUHU[kanal_idx]
        ambang = AMBANG_ALARM_SUHU[kanal_idx]
        ambang_resume = ambang - HISTERESIS_SUHU

        nilai_tinggi = round(ambang * 1.2, 1)            # di atas ambang alarm
        nilai_histeresis = round(ambang - 2.0, 1)          # di zona histeresis
        nilai_rendah = round(ambang_resume - 3.0, 1)       # di bawah ambang resume

        print(f"\n--- Kanal: {nama} (ambang={ambang}C, resume<={ambang_resume}C) ---")

        for ulang in range(1, PENGULANGAN + 1):
            print(f"\n  Percobaan {ulang}/{PENGULANGAN}")
            waktu = datetime.now().isoformat()

            print(f"    [1] Tahan nilai {nilai_tinggi}C (di atas ambang alarm)")
            tahan_nilai_suhu(kanal_idx, nilai_tinggi)
            time.sleep(JEDA_SIKLUS)
            data_setelah_tinggi = ambil_data_terkini()
            relay_setelah_tinggi = data_setelah_tinggi.get("relayOn") if data_setelah_tinggi else None
            cutoff_terjadi = relay_setelah_tinggi is True or relay_setelah_tinggi == 1
            print(f"        -> relayOn={relay_setelah_tinggi} (cutoff terjadi: {cutoff_terjadi})")

            print(f"    [2] Coba resume saat masih {nilai_tinggi}C (harus DITOLAK)")
            resume1_diterima = coba_resume()
            print(f"        -> mesin menyala setelah resume: {resume1_diterima} (harus False)")

            print(f"    [3] Tahan nilai {nilai_histeresis}C (zona histeresis)")
            tahan_nilai_suhu(kanal_idx, nilai_histeresis)
            time.sleep(JEDA_SIKLUS)

            print(f"    [4] Coba resume di zona histeresis (harus TETAP DITOLAK)")
            resume2_diterima = coba_resume()
            print(f"        -> mesin menyala setelah resume: {resume2_diterima} (harus False)")

            print(f"    [5] Tahan nilai {nilai_rendah}C (di bawah ambang resume)")
            tahan_nilai_suhu(kanal_idx, nilai_rendah)
            time.sleep(JEDA_SIKLUS)

            print(f"    [6] Coba resume di bawah ambang resume (harus DITERIMA)")
            resume3_diterima = coba_resume()
            print(f"        -> mesin menyala setelah resume: {resume3_diterima} (harus True)")

            hentikan_penahanan_suhu(kanal_idx)
            time.sleep(1)
            kirim_perintah("relay_off")

            hasil.append({
                "kanal": nama,
                "percobaan": ulang,
                "waktu": waktu,
                "ambang_alarm_C": ambang,
                "ambang_resume_C": ambang_resume,
                "nilai_tinggi_C": nilai_tinggi,
                "cutoff_terjadi": cutoff_terjadi,
                "nilai_histeresis_C": nilai_histeresis,
                "resume_ditolak_saat_tinggi": not resume1_diterima if resume1_diterima is not None else "",
                "resume_ditolak_saat_histeresis": not resume2_diterima if resume2_diterima is not None else "",
                "nilai_rendah_C": nilai_rendah,
                "resume_diterima_saat_rendah": resume3_diterima,
                "sesuai_ekspektasi": (
                    cutoff_terjadi
                    and resume1_diterima is False
                    and resume2_diterima is False
                    and resume3_diterima is True
                ),
            })

            time.sleep(JEDA_ANTAR_PERCOBAAN)

    simpan_csv("hasil_histeresis_suhu_end_to_end.csv", hasil)

    total = len(hasil)
    sesuai = sum(1 for h in hasil if h["sesuai_ekspektasi"])
    print("\n" + "=" * 70)
    print(f"RINGKASAN SUHU: {sesuai}/{total} percobaan sesuai ekspektasi penuh")
    print("=" * 70)

    return hasil


def simpan_csv(nama_file, data):
    if not data:
        print(f"  (Tidak ada data untuk disimpan ke {nama_file})")
        return
    with open(nama_file, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=data[0].keys())
        writer.writeheader()
        writer.writerows(data)
    print(f"  Tersimpan: {nama_file} ({len(data)} baris)")


def main():
    print("Pastikan firmware sudah diupdate dengan command test_hold/test_clear")
    print("dan test_hold_temp/test_clear_temp (lihat diff_test_hold_firmware.patch)")
    print("sebelum menjalankan skrip ini.\n")
    konfirmasi = input("Firmware sudah diupdate & mesin aman untuk diuji? (y/n): ")
    if konfirmasi.lower() != "y":
        print("Dibatalkan.")
        return

    mulai = time.time()
    uji_histeresis_end_to_end()
    uji_histeresis_suhu()
    selesai = time.time()
    print(f"\nTotal waktu: {(selesai - mulai) / 60:.1f} menit")


if __name__ == "__main__":
    main()