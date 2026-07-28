# TA-Rombak

Hasil konversi buku TA (docx) ke markdown, dipecah per bab, untuk proses
rombak/edit. Sumber asli: `docs/archive/susunBukuTA/RANCANG BANGUN SISTEM
PENGAWASAN MESIN CNC BERBASIS INTERNET OF THINGS MENGGUNAKAN MEKANISME
CUTOFF OTOMATIS MENGGUNAKAN ESP32 (1).docx` (tidak diubah, tetap sebagai
arsip sumber).

## Struktur file

| File | Isi |
|---|---|
| `00-sampul.md` | Cover, lembar pengesahan, orisinalitas, abstrak/abstract, kata pengantar, ucapan terima kasih |
| `01-pendahuluan.md` | BAB I |
| `02-tinjauan-pustaka.md` | BAB II |
| `03-spesifikasi-desain-sistem.md` | BAB III |
| `04-implementasi.md` | BAB IV |
| `05-pengujian-analisis.md` | BAB V |
| `06-evaluasi.md` | BAB VI |
| `07-lampiran.md` | Lampiran |
| `assets/media/` | 60 gambar hasil extract dari docx asli |
| `_referensi-lama/` | Daftar isi/gambar/tabel/singkatan + daftar pustaka manual dari Word - diarsipkan, tidak dipakai di build (lihat README di dalamnya) |

Penomoran bab (BAB I, II, III, ...) tidak dituliskan manual - itu tugas
`number-sections: true` di pandoc-defaults.yaml, otomatis mengikuti urutan
file saat build.

## Build ke docx

```
pandoc --defaults CNCTA/pandoc-defaults.yaml -o CNCTA/output/TA-final.docx CNCTA/TA-Rombak/0*.md
```

(Jangan ikutkan `_referensi-lama/*.md` dalam daftar file build.)

## Sitasi

- Sitasi di teks pakai `[@citekey]` (mis. `[@altintas2012]`), citekey
  sementara berformat `namaauthor+tahun`, di-generate dari data Zotero yang
  ada di dalam docx asli (field `ZOTERO_ITEM CSL_CITATION`), BUKAN dari
  export Better BibTeX Zotero langsung.
- `CNCTA/referensi.bib` berisi 22 entri unik hasil extract tsb.
- **Kalau Zotero library berubah / kamu re-export via Better BibTeX**,
  citekey hasil export asli kemungkinan beda dari yang di sini - perlu
  dicocokkan ulang (find & replace citekey lama -> baru di semua file bab).
- Beri tahu posisi edit-mu, biar sitasi terkait bisa saya sesuaikan/refresh.

## Status audit (per 2026-07-24)

Belum diaudit isi per bab. Temuan teknis dari proses konversi, perlu
diverifikasi manusia:

1. **Gambar `assets/media/image2.emf`** - awalnya nyempil di heading BAB III
   (`SPESIFIKASI DAN DESAIN SISTEM`), sudah dikeluarkan dari heading (lihat
   komentar HTML di `03-spesifikasi-desain-sistem.md`). File EMF ini kecil
   (624 byte) - kemungkinan besar cuma artifact bullet/simbol Word, bukan
   gambar konten. Perlu dicek visual di Word (buka docx asli) untuk pastikan.
2. **Rumus matematika** (BAB IV/V: `V_ADC`, MAE, standar deviasi, dll) -
   dikonversi pandoc jadi TeX math (`$...$`). Perlu cek visual hasil docx
   build untuk pastikan Word merender sebagai equation asli, bukan teks.
3. **pandoc-crossref** versi terpasang (v0.3.24) di-compile untuk pandoc
   3.9.0.2, sedangkan pandoc terpasang 3.10 - ada warning version mismatch
   tiap build. Build sejauh ini tetap benar, tapi worth diawasi.
4. **1 dari 22 sitasi** ("MQTT Version 5.0") datanya tidak punya nama
   penulis di CSL asli, jadi citekey-nya jadi `anon2019` - cek apakah ini
   sengaja (standar OASIS, memang tanpa penulis individu) atau ada data
   yang hilang.
