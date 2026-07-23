"""
Skrip Uji Notifikasi Telegram — Sistem Pengawasan Mesin CNC
================================================================================
Menguji jalur notifikasi Telegram end-to-end lewat 3 skenario:

  A. Keandalan API Telegram murni
     Kirim beberapa pesan tes berturut-turut lewat POST /api/test-alert,
     ukur latensi request-response dan berapa kali Telegram Bot API
     benar-benar merespons ok (bukan cuma diterima backend).

  B. End-to-end per kanal arus & suhu
     Picu kondisi alarm ASLI lewat test_hold / test_hold_temp (mekanisme
     sama seperti uji_histeresis_otomatis.py — bukan simulasi murni),
     lalu verifikasi baris alert baru benar-benar tercatat di
     GET /api/alerts/:id dengan alert_type & nama sensor yang sesuai
     (proxy bahwa notifikasi memang diproses & dikirim ke Telegram).
     Latensi diukur dari saat command dikirim sampai baris alert muncul.

  C. Kontrol relay manual vs otomatis
     Kirim relay_on/relay_off BIASA (bukan test_hold) lewat
     POST /api/command/:id, lalu verifikasi alert_type yang tercatat
     adalah 'relay_manual' (bukan 'relay_trip') — memastikan fitur
     pembeda "kontrol manual dari dashboard" vs "proteksi otomatis"
     di alertEngine.js bekerja sesuai desain.

CATATAN: self-test gagal & device offline TIDAK diuji otomatis di sini
karena butuh kondisi yang tidak bisa disimulasikan murni lewat API
(self-test gagal butuh sensor benar-benar rusak/short; device offline
butuh device benar-benar berhenti mengirim MQTT selama > ALERT_OFFLINE_TIMEOUT
detik). Uji dua skenario itu secara manual: cabut sensor / matikan device,
lalu cek Telegram & GET /api/alerts/:id secara manual.

Prasyarat:
- Backend berjalan & bisa diakses di BASE_URL.
- TELEGRAM_ENABLED=true dan sudah dikonfigurasi benar di server/backend/.env.
- Device ESP32 terhubung MQTT, firmware mendukung command test_hold/test_clear
  dan test_hold_temp/test_clear_temp (lihat uji_histeresis_otomatis.py).

Kebutuhan: pip install requests
"""

import requests
import time
import csv
import os
from datetime import datetime

# ── KONFIGURASI ──────────────────────────────────────────────────────────
BASE_URL  = "http://localhost:3001"   # sesuaikan dgn HTTP_PORT di server/backend/.env
DEVICE_ID = "cnc-esp32"

# -- Skenario A: keandalan API Telegram murni --
JUMLAH_TES_KONEKTIVITAS = 20    # berapa kali kirim test-alert berturut-turut
JEDA_ANTAR_TES_KONEKTIVITAS = 2  # detik, jeda antar tes (hindari flood ke Telegram)

# -- Skenario B: end-to-end per kanal --
NAMA_KANAL_ARUS = ["Stepper_X", "Stepper_Y1", "Stepper_Y2", "Stepper_Z", "Spindle"]
AMBANG_ARUS     = [3.0, 3.0, 3.0, 2.0, 3.0]
NAMA_KANAL_SUHU = ["Spindle", "Stepper_Z"]
AMBANG_SUHU     = [60.0, 55.0]

JEDA_SEBELUM_POLL = 6     # detik, tunggu awal sebelum mulai polling /api/alerts
                           # (mencakup roundtrip MQTT + proses alertEngine + panggilan Telegram)
JEDA_ANTAR_POLL   = 3     # detik, jeda antar polling ulang kalau belum muncul
MAX_POLL          = 4     # jumlah polling maksimum sebelum menyerah
JEDA_ANTAR_KANAL  = 5     # detik, jeda antar kanal supaya kondisi benar-benar reset

HASIL_DIR = "hasil pengujian"
FILE_KONEKTIVITAS = os.path.join(HASIL_DIR, "hasil_notifikasi_konektivitas.csv")
FILE_E2E          = os.path.join(HASIL_DIR, "hasil_notifikasi_e2e.csv")

KOLOM_E2E = [
    "skenario", "kanal_atau_aksi", "alert_type_diharapkan",
    "pc_waktu_trigger", "muncul", "recorded_at", "jumlah_polling", "latensi_detik_perkiraan",
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


def tahan_nilai_arus(idx, nilai):
    return kirim_perintah(f"test_hold:{idx}:{nilai}")


def hentikan_arus(idx):
    return kirim_perintah(f"test_clear:{idx}")


def tahan_nilai_suhu(idx, nilai):
    return kirim_perintah(f"test_hold_temp:{idx}:{nilai}")


def hentikan_suhu(idx):
    return kirim_perintah(f"test_clear_temp:{idx}")


def bersihkan_semua_kondisi_uji():
    """Hentikan semua test_hold yang mungkin masih aktif dari sesi sebelumnya."""
    for idx in range(len(NAMA_KANAL_ARUS)):
        hentikan_arus(idx)
    for idx in range(len(NAMA_KANAL_SUHU)):
        hentikan_suhu(idx)
    kirim_perintah("relay_off")


def kirim_test_alert(teks):
    """Skenario A: panggil endpoint test-alert, ukur latensi & hasil asli dari Telegram Bot API."""
    url = f"{BASE_URL}/api/test-alert"
    t0 = time.time()
    try:
        r = requests.post(url, json={"text": teks}, timeout=10)
        latensi_ms = round((time.time() - t0) * 1000, 1)
        data = r.json()
        return data.get("ok", False), latensi_ms, data.get("error", "")
    except Exception as e:
        latensi_ms = round((time.time() - t0) * 1000, 1)
        return False, latensi_ms, str(e)


def ambil_alert_terbaru(limit=15):
    url = f"{BASE_URL}/api/alerts/{DEVICE_ID}?limit={limit}"
    try:
        r = requests.get(url, timeout=5)
        r.raise_for_status()
        return r.json().get("data", [])
    except Exception as e:
        print(f"        [ERROR] Gagal ambil histori alert: {e}")
        return []


def tunggu_alert_muncul(alert_type, sejak_str, kandungan_pesan=None):
    """
    Poll GET /api/alerts/:id sampai muncul baris dengan alert_type tertentu
    yang recorded_at >= sejak_str (dan opsional mengandung teks tertentu di
    pesan, untuk memastikan itu baris yang benar-benar baru & relevan).
    Return: (muncul: bool, recorded_at, jumlah_polling)
    """
    time.sleep(JEDA_SEBELUM_POLL)
    for percobaan in range(1, MAX_POLL + 1):
        rows = ambil_alert_terbaru()
        for row in rows:
            cocok_tipe  = row.get("alertType") == alert_type
            cocok_waktu = row.get("recordedAt", "") >= sejak_str
            cocok_isi   = (kandungan_pesan is None) or (kandungan_pesan in row.get("message", ""))
            if cocok_tipe and cocok_waktu and cocok_isi:
                return True, row.get("recordedAt"), percobaan
        if percobaan < MAX_POLL:
            time.sleep(JEDA_ANTAR_POLL)
    return False, None, MAX_POLL


# ── PENCATATAN ───────────────────────────────────────────────────────────

def buat_folder_hasil():
    os.makedirs(HASIL_DIR, exist_ok=True)


def buat_file_e2e_jika_belum_ada():
    if not os.path.exists(FILE_E2E):
        with open(FILE_E2E, "w", newline="", encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=KOLOM_E2E)
            writer.writeheader()


def catat_baris_e2e(skenario, kanal_atau_aksi, alert_type, pc_waktu_trigger, muncul, recorded_at, jumlah_polling):
    latensi_perkiraan = JEDA_SEBELUM_POLL + (jumlah_polling - 1) * JEDA_ANTAR_POLL if muncul else ""
    baris = {
        "skenario": skenario,
        "kanal_atau_aksi": kanal_atau_aksi,
        "alert_type_diharapkan": alert_type,
        "pc_waktu_trigger": pc_waktu_trigger,
        "muncul": muncul,
        "recorded_at": recorded_at or "",
        "jumlah_polling": jumlah_polling,
        "latensi_detik_perkiraan": latensi_perkiraan,
    }
    with open(FILE_E2E, "a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=KOLOM_E2E)
        writer.writerow(baris)

    status = "OK MUNCUL" if muncul else "GAGAL TIDAK MUNCUL"
    print(f"        [hasil] {status} — alert_type={alert_type} recorded_at={recorded_at} (polling ke-{jumlah_polling})")
    return baris


# ── SKENARIO A: KEANDALAN API TELEGRAM ───────────────────────────────────

def uji_konektivitas_telegram():
    print("=" * 70)
    print(f"SKENARIO A — Keandalan API Telegram ({JUMLAH_TES_KONEKTIVITAS}x kirim berturut-turut)")
    print("=" * 70)

    hasil = []
    for i in range(1, JUMLAH_TES_KONEKTIVITAS + 1):
        teks = f"🧪 Uji konektivitas notifikasi #{i}/{JUMLAH_TES_KONEKTIVITAS} — {datetime.now().strftime('%H:%M:%S')}"
        berhasil, latensi_ms, error = kirim_test_alert(teks)
        print(f"  [{i}/{JUMLAH_TES_KONEKTIVITAS}] {'OK' if berhasil else 'GAGAL'} "
              f"(latensi {latensi_ms} ms){'' if berhasil else ' - ' + error}")
        hasil.append({
            "percobaan": i,
            "pc_waktu": datetime.now().isoformat(),
            "berhasil": berhasil,
            "latensi_ms": latensi_ms,
            "error": error,
        })
        if i < JUMLAH_TES_KONEKTIVITAS:
            time.sleep(JEDA_ANTAR_TES_KONEKTIVITAS)

    with open(FILE_KONEKTIVITAS, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["percobaan", "pc_waktu", "berhasil", "latensi_ms", "error"])
        writer.writeheader()
        writer.writerows(hasil)

    total     = len(hasil)
    sukses    = sum(1 for h in hasil if h["berhasil"])
    latensi_ok = [h["latensi_ms"] for h in hasil if h["berhasil"]]
    rata_latensi = round(sum(latensi_ok) / len(latensi_ok), 1) if latensi_ok else 0

    print(f"\nRINGKASAN A: {sukses}/{total} pesan berhasil dikirim ({sukses/total*100:.1f}%), "
          f"rata-rata latensi {rata_latensi} ms")
    print(f"Detail tersimpan: {FILE_KONEKTIVITAS}\n")
    return hasil


# ── SKENARIO B: END-TO-END PER KANAL ─────────────────────────────────────

def uji_e2e_arus():
    print("=" * 70)
    print("SKENARIO B1 — End-to-end Overcurrent per Kanal Arus")
    print("=" * 70)

    for idx, nama in enumerate(NAMA_KANAL_ARUS):
        ambang = AMBANG_ARUS[idx]
        nilai_tinggi = round(ambang * 1.5, 2)
        print(f"\n  Kanal: {nama} (ambang={ambang}A, suntik {nilai_tinggi}A)")

        sejak = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        tahan_nilai_arus(idx, nilai_tinggi)
        muncul, recorded_at, n_poll = tunggu_alert_muncul("overcurrent", sejak, kandungan_pesan=nama.replace("_", " "))
        catat_baris_e2e("overcurrent", nama, "overcurrent", sejak, muncul, recorded_at, n_poll)

        hentikan_arus(idx)
        kirim_perintah("relay_off")
        time.sleep(JEDA_ANTAR_KANAL)


def uji_e2e_suhu():
    print("=" * 70)
    print("SKENARIO B2 — End-to-end Overtemp per Kanal Suhu")
    print("=" * 70)

    for idx, nama in enumerate(NAMA_KANAL_SUHU):
        ambang = AMBANG_SUHU[idx]
        nilai_tinggi = round(ambang * 1.2, 2)
        print(f"\n  Kanal: {nama} (ambang={ambang}°C, suntik {nilai_tinggi}°C)")

        sejak = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        tahan_nilai_suhu(idx, nilai_tinggi)
        muncul, recorded_at, n_poll = tunggu_alert_muncul("overtemp", sejak, kandungan_pesan=nama.replace("_", " "))
        catat_baris_e2e("overtemp", nama, "overtemp", sejak, muncul, recorded_at, n_poll)

        hentikan_suhu(idx)
        kirim_perintah("relay_off")
        time.sleep(JEDA_ANTAR_KANAL)


# ── SKENARIO C: RELAY MANUAL VS OTOMATIS ─────────────────────────────────

def uji_relay_manual():
    print("=" * 70)
    print("SKENARIO C — Kontrol Relay Manual (relay_on/relay_off biasa dari dashboard)")
    print("=" * 70)
    bersihkan_semua_kondisi_uji()
    time.sleep(2)

    print("\n  [C1] Kirim relay_off manual (matikan mesin lewat dashboard)")
    sejak = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    kirim_perintah("relay_off")
    muncul, recorded_at, n_poll = tunggu_alert_muncul("relay_manual", sejak)
    catat_baris_e2e("relay_manual", "relay_off (manual)", "relay_manual", sejak, muncul, recorded_at, n_poll)
    time.sleep(JEDA_ANTAR_KANAL)

    print("\n  [C2] Kirim relay_on manual (nyalakan mesin lewat dashboard)")
    sejak = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    kirim_perintah("relay_on")
    muncul, recorded_at, n_poll = tunggu_alert_muncul("relay_manual", sejak)
    catat_baris_e2e("relay_manual", "relay_on (manual)", "relay_manual", sejak, muncul, recorded_at, n_poll)


# ── RINGKASAN AKHIR SKENARIO B & C ───────────────────────────────────────

def cetak_ringkasan_e2e():
    if not os.path.exists(FILE_E2E):
        return
    with open(FILE_E2E, newline="", encoding="utf-8") as f:
        rows = list(csv.DictReader(f))

    if not rows:
        return

    total  = len(rows)
    sukses = sum(1 for r in rows if r["muncul"] == "True")

    print("\n" + "=" * 70)
    print(f"RINGKASAN B & C: {sukses}/{total} notifikasi end-to-end terverifikasi muncul di histori alert")
    print(f"Detail lengkap: {FILE_E2E}")
    print("=" * 70)


# ── MAIN ──────────────────────────────────────────────────────────────────

def main():
    print("Uji Notifikasi Telegram — Sistem Pengawasan Mesin CNC")
    print(f"BASE_URL = {BASE_URL} | DEVICE_ID = {DEVICE_ID}\n")
    print("PERINGATAN: skenario B & C akan memicu kondisi alarm & mengubah")
    print("relay device SUNGGUHAN (test_hold + relay_on/off). Pastikan mesin")
    print("dalam kondisi aman untuk diuji (tidak sedang memotong benda kerja).\n")

    konfirmasi = input("Backend & Telegram sudah aktif, device aman untuk diuji? (y/n): ")
    if konfirmasi.lower() != "y":
        print("Dibatalkan.")
        return

    buat_folder_hasil()
    buat_file_e2e_jika_belum_ada()
    bersihkan_semua_kondisi_uji()

    mulai = time.time()

    uji_konektivitas_telegram()
    uji_e2e_arus()
    uji_e2e_suhu()
    uji_relay_manual()

    bersihkan_semua_kondisi_uji()
    cetak_ringkasan_e2e()

    selesai = time.time()
    print(f"\nTotal waktu pengujian: {(selesai - mulai) / 60:.1f} menit")


if __name__ == "__main__":
    main()
