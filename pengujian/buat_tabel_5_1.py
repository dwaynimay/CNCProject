"""
Pembuat Tabel 5.1 — Ringkasan Keberhasilan Uji Ambang Proteksi dan Histeresis
================================================================================
Membaca hasil_histeresis_end_to_end.csv (arus) dan
hasil_histeresis_suhu_end_to_end.csv (suhu), lalu menghitung ringkasan per
kanal: berapa kali cutoff berhasil, berapa kali resume berjalan sesuai
ekspektasi (ditolak saat tinggi, ditolak saat histeresis, diterima saat
rendah — ketiganya harus benar), dan persentase keberhasilan keseluruhan.

Cara pakai:
1. Taruh skrip ini di folder yang sama dengan kedua file CSV.
2. Jalankan: python buat_tabel_5_1.py
3. Hasil tercetak ke layar (format tabel rapi) DAN tersimpan sebagai
   tabel_5_1_ringkasan.csv serta tabel_5_1_ringkasan.md (siap tempel ke Bab 5).

Kebutuhan: pip install pandas
"""

import pandas as pd
import os

FILE_ARUS = "data/hasil-pengujian/hasil_histeresis_end_to_end.csv"
FILE_SUHU = "data/hasil-pengujian/hasil_histeresis_suhu_end_to_end.csv"


def ringkas(file_csv, jenis):
    if not os.path.exists(file_csv):
        print(f"[WARN] {file_csv} tidak ditemukan, dilewati.")
        return pd.DataFrame()

    df = pd.read_csv(file_csv)

    # Resume dianggap "sesuai ekspektasi" hanya jika ketiga tahap resume benar:
    # ditolak saat tinggi, ditolak saat zona histeresis, diterima saat rendah.
    df["resume_sesuai"] = (
        df["resume_ditolak_saat_tinggi"]
        & df["resume_ditolak_saat_histeresis"]
        & df["resume_diterima_saat_rendah"]
    )

    ringkasan = df.groupby("kanal").agg(
        jumlah_percobaan=("percobaan", "count"),
        cutoff_berhasil=("cutoff_terjadi", "sum"),
        resume_sesuai=("resume_sesuai", "sum"),
        sesuai_ekspektasi_penuh=("sesuai_ekspektasi", "sum"),
    ).reset_index()

    ringkasan["jenis"] = jenis
    ringkasan["persentase"] = (
        ringkasan["sesuai_ekspektasi_penuh"] / ringkasan["jumlah_percobaan"] * 100
    ).round(1)

    # Format kolom jadi "x/y" supaya langsung enak dibaca di tabel
    ringkasan["cutoff_berhasil_fmt"] = (
        ringkasan["cutoff_berhasil"].astype(str) + "/" + ringkasan["jumlah_percobaan"].astype(str)
    )
    ringkasan["resume_sesuai_fmt"] = (
        ringkasan["resume_sesuai"].astype(str) + "/" + ringkasan["jumlah_percobaan"].astype(str)
    )
    ringkasan["persentase_fmt"] = ringkasan["persentase"].astype(str) + "%"

    return ringkasan[[
        "kanal", "jenis", "jumlah_percobaan",
        "cutoff_berhasil_fmt", "resume_sesuai_fmt", "persentase_fmt",
    ]].rename(columns={
        "kanal": "Kanal",
        "jenis": "Jenis",
        "jumlah_percobaan": "Jumlah Percobaan",
        "cutoff_berhasil_fmt": "Cutoff Berhasil",
        "resume_sesuai_fmt": "Resume Sesuai Ekspektasi",
        "persentase_fmt": "Persentase",
    })


def main():
    tabel_arus = ringkas(FILE_ARUS, "Arus")
    tabel_suhu = ringkas(FILE_SUHU, "Suhu")

    tabel_gabungan = pd.concat([tabel_arus, tabel_suhu], ignore_index=True)

    if tabel_gabungan.empty:
        print("Tidak ada data untuk ditabelkan. Cek apakah file CSV sumber ada di folder ini.")
        return

    print("\n" + "=" * 80)
    print("Tabel 5.1 Ringkasan Keberhasilan Uji Ambang Proteksi dan Histeresis")
    print("=" * 80)
    print(tabel_gabungan.to_string(index=False))
    print("=" * 80)

    total_percobaan = tabel_gabungan["Jumlah Percobaan"].sum()

    # Hitung total keseluruhan langsung dari data mentah (lebih akurat
    # daripada menjumlahkan persentase per baris)
    total_sesuai_penuh = 0
    total_semua = 0
    for df_raw, file_csv in [(tabel_arus, FILE_ARUS), (tabel_suhu, FILE_SUHU)]:
        if os.path.exists(file_csv):
            df_mentah = pd.read_csv(file_csv)
            total_sesuai_penuh += df_mentah["sesuai_ekspektasi"].sum()
            total_semua += len(df_mentah)

    if total_semua > 0:
        print(f"\nTotal keseluruhan: {total_sesuai_penuh}/{total_semua} percobaan "
              f"sesuai ekspektasi penuh ({total_sesuai_penuh/total_semua*100:.1f}%)")

    tabel_gabungan.to_csv("tabel_5_1_ringkasan.csv", index=False)
    print("\nTersimpan: tabel_5_1_ringkasan.csv")

    with open("tabel_5_1_ringkasan.md", "w", encoding="utf-8") as f:
        f.write("**Tabel 5.1 Ringkasan Keberhasilan Uji Ambang Proteksi dan Histeresis**\n\n")
        f.write(tabel_gabungan.to_markdown(index=False))
        f.write(f"\n\nTotal keseluruhan: {total_sesuai_penuh}/{total_semua} percobaan "
                f"sesuai ekspektasi penuh ({total_sesuai_penuh/total_semua*100:.1f}%).\n")
    print("Tersimpan: tabel_5_1_ringkasan.md (siap tempel ke draf Bab 5)")


if __name__ == "__main__":
    main()
