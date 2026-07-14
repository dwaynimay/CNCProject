"""
Skrip Otomasi Uji Histeresis End-to-End — Sistem Pengawasan Mesin CNC
========================================================================
Menguji perilaku resume pada 3 zona nilai (arus & suhu) secara berurutan,
sepenuhnya otomatis, TANPA perlu arus/suhu fisik sungguhan.

Prasyarat: firmware sudah diupdate dengan command test_hold/test_clear
dan test_hold_temp/test_clear_temp, TERMASUK penambahan timestamp epoch
di log Serial (lihat diff_test_hold_firmware.patch versi terbaru).

REVISI (setelah hasil awal 17/25 & 7/10 di jaringan nyata):
Ditemukan kegagalan bukan karena logika histeresis salah, tapi karena
command MQTT (QoS 0) kadang belum "mendarat" di ESP32 saat skrip sudah
lanjut ke pengecekan berikutnya (server tiruan sebelumnya responsnya
instan, jaringan nyata punya latensi bervariasi). Perbaikan:
  1. JEDA_SIKLUS diperpanjang dari 3 -> 6 detik
  2. Tiap tahap yang bisa diverifikasi (1, 2, 4, 6) sekarang otomatis
     KIRIM ULANG perintah kalau kondisi belum sesuai ekspektasi setelah
     jeda pertama, sampai maksimal 3 percobaan sebelum menyerah dan
     mencatat hasil apa adanya (supaya kegagalan asli tetap tercatat,
     bukan dipaksa "sukses" secara artifisial)

PENCATATAN WAKTU (PENTING):
Setiap tahap mencatat TIGA sumber waktu: pc_waktu, device_epoch (field
"ts" dari payload MQTT ESP32, sama persis dengan log Serial epoch=...),
dan device_epoch_wib (format terbaca). Kolom jumlah_percobaan_kirim
menunjukkan berapa kali command dikirim ulang sebelum kondisi sesuai
(1 = langsung berhasil di percobaan pertama, tanpa retry).

Semua checkpoint ditulis LANGSUNG ke file CSV detail (mode append).

Kebutuhan: pip install requests
"""

import requests
import time
import csv
import os
from datetime import datetime

# ── KONFIGURASI ──────────────────────────────────────────────────────────
BASE_URL = "http://192.168.1.8:3010"
DEVICE_ID = "cnc-esp32"

PENGULANGAN = 10

NAMA_KANAL   = ["Stepper_X", "Stepper_Y1", "Stepper_Y2", "Stepper_Z", "Spindle"]
AMBANG_ALARM = [3.0, 3.0, 3.0, 2.0, 3.0]
HISTERESIS   = 0.5

NAMA_KANAL_SUHU   = ["Spindle", "Stepper_Z"]
AMBANG_ALARM_SUHU = [60.0, 55.0]
HISTERESIS_SUHU   = 5.0

JEDA_SIKLUS = 6          # diperpanjang dari 3 -> 6 detik (margin latensi jaringan nyata)
JEDA_ANTAR_PERCOBAAN = 5
MAX_RETRY = 3            # maksimal percobaan kirim ulang sebelum menyerah

FILE_DETAIL_ARUS = "log_detail_histeresis_arus.csv"
FILE_DETAIL_SUHU = "log_detail_histeresis_suhu.csv"
FILE_RINGKASAN_ARUS = "hasil_histeresis_end_to_end.csv"
FILE_RINGKASAN_SUHU = "hasil_histeresis_suhu_end_to_end.csv"

KOLOM_DETAIL = [
    "kanal", "percobaan", "tahap", "deskripsi",
    "pc_waktu", "device_epoch", "device_epoch_wib",
    "nilai_disuntik", "relayOn", "mesin_menyala", "jumlah_percobaan_kirim",
]


# ── FUNGSI BANTUAN API ───────────────────────────────────────────────────

def kirim_perintah(cmd):
    url = f"{BASE_URL}/api/command/{DEVICE_ID}"
    try:
        r = requests.post(url, json={"cmd": cmd}, timeout=5)
        r.raise_for_status()
        return r.json().get("ok", False)
    except Exception as e:
        print(f"        [ERROR] Gagal kirim '{cmd}': {e}")
        return False


def ambil_data_terkini():
    url = f"{BASE_URL}/api/latest/{DEVICE_ID}"
    try:
        r = requests.get(url, timeout=5)
        r.raise_for_status()
        return r.json()
    except Exception as e:
        print(f"        [ERROR] Gagal ambil data terkini: {e}")
        return None


def tahan_nilai(kanal_idx, nilai):
    return kirim_perintah(f"test_hold:{kanal_idx}:{nilai}")


def hentikan_penahanan(kanal_idx):
    return kirim_perintah(f"test_clear:{kanal_idx}")


def tahan_nilai_suhu(kanal_idx, nilai):
    return kirim_perintah(f"test_hold_temp:{kanal_idx}:{nilai}")


def hentikan_penahanan_suhu(kanal_idx):
    return kirim_perintah(f"test_clear_temp:{kanal_idx}")


# ── EKSEKUSI DENGAN VERIFIKASI + RETRY (INTI PERBAIKAN) ──────────────────

def eksekusi_dengan_verifikasi(fungsi_kirim, relay_diharapkan=None, max_retry=MAX_RETRY):
    """
    Jalankan fungsi_kirim(), tunggu JEDA_SIKLUS, cek relayOn via API.
    Kalau relay_diharapkan diberikan (True/False) dan hasil belum sesuai,
    kirim ulang sampai maksimal max_retry kali sebelum menyerah.
    relay_diharapkan=None berarti tidak ada kondisi spesifik untuk diverifikasi
    (dipakai untuk tahap 3 & 5 yang cuma menahan nilai tanpa cek relay langsung).

    Return: (device_epoch, relay_on, jumlah_percobaan)
    """
    device_epoch, relay_on = None, None
    for percobaan in range(1, max_retry + 1):
        fungsi_kirim()
        time.sleep(JEDA_SIKLUS)
        data = ambil_data_terkini()
        device_epoch = data.get("ts") if data else None
        relay_on = data.get("relayOn") if data else None

        if relay_diharapkan is None:
            return device_epoch, relay_on, percobaan

        if relay_on == relay_diharapkan:
            return device_epoch, relay_on, percobaan

        if percobaan < max_retry:
            print(f"        [retry {percobaan}/{max_retry}] relayOn={relay_on}, "
                  f"diharapkan={relay_diharapkan} — kirim ulang...")

    return device_epoch, relay_on, max_retry


# ── PENCATATAN CHECKPOINT ────────────────────────────────────────────────

def buat_file_detail_jika_belum_ada(nama_file):
    if not os.path.exists(nama_file):
        with open(nama_file, "w", newline="", encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=KOLOM_DETAIL)
            writer.writeheader()


def catat_baris(nama_file, kanal, percobaan, tahap, deskripsi,
                device_epoch, relay_on, jumlah_percobaan_kirim, nilai_disuntik=""):
    pc_waktu = datetime.now().isoformat()
    mesin_menyala = (relay_on is False) or (relay_on == 0) if relay_on is not None else ""

    if device_epoch:
        device_epoch_wib = datetime.fromtimestamp(device_epoch).strftime("%Y-%m-%d %H:%M:%S")
    else:
        device_epoch_wib = ""

    baris = {
        "kanal": kanal,
        "percobaan": percobaan,
        "tahap": tahap,
        "deskripsi": deskripsi,
        "pc_waktu": pc_waktu,
        "device_epoch": device_epoch or "",
        "device_epoch_wib": device_epoch_wib,
        "nilai_disuntik": nilai_disuntik,
        "relayOn": relay_on,
        "mesin_menyala": mesin_menyala,
        "jumlah_percobaan_kirim": jumlah_percobaan_kirim,
    }

    with open(nama_file, "a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=KOLOM_DETAIL)
        writer.writerow(baris)

    tanda_retry = f" ({jumlah_percobaan_kirim}x kirim)" if jumlah_percobaan_kirim > 1 else ""
    print(f"        [checkpoint] pc={pc_waktu[11:19]} device_epoch_wib={device_epoch_wib} "
          f"relayOn={relay_on} mesin_menyala={mesin_menyala}{tanda_retry}")

    return mesin_menyala


# ── FUNGSI GENERIK UNTUK ARUS & SUHU ─────────────────────────────────────

def _uji_histeresis_generik(
    nama_kanal_list, ambang_list, histeresis, satuan,
    fungsi_tahan, fungsi_hentikan, file_detail, file_ringkasan,
    faktor_tinggi, offset_histeresis, offset_rendah, label_epoch,
):
    hasil_ringkasan = []
    buat_file_detail_jika_belum_ada(file_detail)

    for kanal_idx in range(len(nama_kanal_list)):
        nama = nama_kanal_list[kanal_idx]
        ambang = ambang_list[kanal_idx]
        ambang_resume = ambang - histeresis

        nilai_tinggi = round(ambang * faktor_tinggi, 2)
        nilai_histeresis = round(ambang - offset_histeresis, 2)
        nilai_rendah = round(ambang_resume - offset_rendah, 2)

        print(f"\n--- Kanal: {nama} (ambang={ambang}{satuan}, resume<={ambang_resume}{satuan}) ---")

        for ulang in range(1, PENGULANGAN + 1):
            print(f"\n  Percobaan {ulang}/{PENGULANGAN}")

            print(f"    [1] Tahan nilai {nilai_tinggi}{satuan} (di atas ambang alarm)")
            epoch1, relay1, n1 = eksekusi_dengan_verifikasi(
                lambda: fungsi_tahan(kanal_idx, nilai_tinggi), relay_diharapkan=True,
            )
            catat_baris(file_detail, nama, ulang, 1, "Tahan nilai tinggi (harus cutoff)",
                        epoch1, relay1, n1, nilai_tinggi)
            cutoff_terjadi = relay1 is True or relay1 == 1

            print(f"    [2] Coba resume saat masih {nilai_tinggi}{satuan} (harus DITOLAK)")
            epoch2, relay2, n2 = eksekusi_dengan_verifikasi(
                lambda: kirim_perintah("relay_off"), relay_diharapkan=True,
            )
            mesin1 = catat_baris(file_detail, nama, ulang, 2,
                                  "Coba resume saat tinggi (ekspektasi: ditolak)", epoch2, relay2, n2)
            resume1_diterima = mesin1 is True

            print(f"    [3] Tahan nilai {nilai_histeresis}{satuan} (zona histeresis)")
            epoch3, relay3, n3 = eksekusi_dengan_verifikasi(
                lambda: fungsi_tahan(kanal_idx, nilai_histeresis), relay_diharapkan=None,
            )
            catat_baris(file_detail, nama, ulang, 3, "Tahan nilai zona histeresis",
                        epoch3, relay3, n3, nilai_histeresis)

            print(f"    [4] Coba resume di zona histeresis (harus TETAP DITOLAK)")
            epoch4, relay4, n4 = eksekusi_dengan_verifikasi(
                lambda: kirim_perintah("relay_off"), relay_diharapkan=True,
            )
            mesin2 = catat_baris(file_detail, nama, ulang, 4,
                                  "Coba resume di zona histeresis (ekspektasi: ditolak)", epoch4, relay4, n4)
            resume2_diterima = mesin2 is True

            print(f"    [5] Tahan nilai {nilai_rendah}{satuan} (di bawah ambang resume)")
            epoch5, relay5, n5 = eksekusi_dengan_verifikasi(
                lambda: fungsi_tahan(kanal_idx, nilai_rendah), relay_diharapkan=None,
            )
            catat_baris(file_detail, nama, ulang, 5, "Tahan nilai di bawah ambang resume",
                        epoch5, relay5, n5, nilai_rendah)

            print(f"    [6] Coba resume di bawah ambang resume (harus DITERIMA)")
            epoch6, relay6, n6 = eksekusi_dengan_verifikasi(
                lambda: kirim_perintah("relay_off"), relay_diharapkan=False,
            )
            mesin3 = catat_baris(file_detail, nama, ulang, 6,
                                  "Coba resume di bawah ambang resume (ekspektasi: diterima)", epoch6, relay6, n6)
            resume3_diterima = mesin3 is True

            fungsi_hentikan(kanal_idx)
            time.sleep(1)
            kirim_perintah("relay_off")

            hasil_ringkasan.append({
                "kanal": nama,
                "percobaan": ulang,
                f"ambang_alarm_{label_epoch}": ambang,
                f"ambang_resume_{label_epoch}": ambang_resume,
                f"nilai_tinggi_{label_epoch}": nilai_tinggi,
                "cutoff_terjadi": cutoff_terjadi,
                "kirim_ulang_tahap1": n1,
                f"nilai_histeresis_{label_epoch}": nilai_histeresis,
                "resume_ditolak_saat_tinggi": not resume1_diterima,
                "kirim_ulang_tahap2": n2,
                "resume_ditolak_saat_histeresis": not resume2_diterima,
                "kirim_ulang_tahap4": n4,
                f"nilai_rendah_{label_epoch}": nilai_rendah,
                "resume_diterima_saat_rendah": resume3_diterima,
                "kirim_ulang_tahap6": n6,
                "sesuai_ekspektasi": (
                    cutoff_terjadi
                    and resume1_diterima is False
                    and resume2_diterima is False
                    and resume3_diterima is True
                ),
            })

            time.sleep(JEDA_ANTAR_PERCOBAAN)

    simpan_csv(file_ringkasan, hasil_ringkasan)

    total = len(hasil_ringkasan)
    sesuai = sum(1 for h in hasil_ringkasan if h["sesuai_ekspektasi"])
    total_retry = sum(
        h["kirim_ulang_tahap1"] + h["kirim_ulang_tahap2"] + h["kirim_ulang_tahap4"] + h["kirim_ulang_tahap6"] - 4
        for h in hasil_ringkasan
    )
    print("\n" + "=" * 70)
    print(f"RINGKASAN {label_epoch.upper()}: {sesuai}/{total} percobaan sesuai ekspektasi penuh")
    print(f"Total kiriman ulang yang terpakai di seluruh pengujian ini: {total_retry}")
    print(f"Detail lengkap tiap tahap (dengan waktu): {file_detail}")
    print("=" * 70)

    return hasil_ringkasan


def uji_histeresis_end_to_end():
    print("=" * 70)
    print("UJI HISTERESIS END-TO-END ARUS (3 ZONA NILAI, OTOMATIS PENUH)")
    print("=" * 70)
    return _uji_histeresis_generik(
        NAMA_KANAL, AMBANG_ALARM, HISTERESIS, "A",
        tahan_nilai, hentikan_penahanan,
        FILE_DETAIL_ARUS, FILE_RINGKASAN_ARUS,
        faktor_tinggi=1.5, offset_histeresis=0.2, offset_rendah=0.3, label_epoch="A",
    )


def uji_histeresis_suhu():
    print("=" * 70)
    print("UJI HISTERESIS END-TO-END SUHU (3 ZONA NILAI, OTOMATIS PENUH)")
    print("=" * 70)
    return _uji_histeresis_generik(
        NAMA_KANAL_SUHU, AMBANG_ALARM_SUHU, HISTERESIS_SUHU, "C",
        tahan_nilai_suhu, hentikan_penahanan_suhu,
        FILE_DETAIL_SUHU, FILE_RINGKASAN_SUHU,
        faktor_tinggi=1.2, offset_histeresis=2.0, offset_rendah=3.0, label_epoch="C",
    )


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
    print("Pastikan firmware sudah diupdate dengan command test_hold/test_clear,")
    print("test_hold_temp/test_clear_temp, DAN timestamp epoch di log Serial")
    print("(lihat diff_test_hold_firmware.patch versi terbaru) sebelum lanjut.\n")
    print(f"Konfigurasi revisi: JEDA_SIKLUS={JEDA_SIKLUS}s, MAX_RETRY={MAX_RETRY}x per tahap.\n")
    konfirmasi = input("Firmware sudah diupdate & mesin aman untuk diuji? (y/n): ")
    if konfirmasi.lower() != "y":
        print("Dibatalkan.")
        return

    print(f"\nFile detail per-tahap akan ditulis langsung (append) ke:")
    print(f"  - {FILE_DETAIL_ARUS}")
    print(f"  - {FILE_DETAIL_SUHU}")
    print("Kalau mau mulai bersih, hapus dulu file ini sebelum menjalankan skrip.\n")

    mulai = time.time()
    uji_histeresis_end_to_end()
    uji_histeresis_suhu()
    selesai = time.time()
    print(f"\nTotal waktu: {(selesai - mulai) / 60:.1f} menit")


if __name__ == "__main__":
    main()