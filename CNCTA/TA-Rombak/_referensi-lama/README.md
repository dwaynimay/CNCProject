# Kenapa file ini dipindah ke sini

Kedua file ini adalah hasil konversi langsung dari docx asli, tapi isinya
sekarang di-generate otomatis oleh pandoc, jadi disimpan di sini sebagai
arsip/pembanding saja - **tidak diikutkan ke build**.

- `00a-daftar-isi-gambar-tabel-singkatan.md` - Daftar Isi, Daftar Gambar,
  Daftar Tabel, Daftar Singkatan manual dari Word. Digantikan oleh `toc: true`
  di `pandoc-defaults.yaml` (Daftar Isi otomatis). Daftar Gambar/Tabel/Singkatan
  otomatis belum ada padanannya di pandoc - kalau versi manual ini masih
  dibutuhkan, perlu diaudit dan disatukan manual ke salah satu bab.
- `07-daftar-pustaka.md` - Daftar Pustaka manual dari Word (referensi format
  IEEE apa adanya). Digantikan oleh sitasi `[@citekey]` + `referensi.bib` +
  filter `citeproc`, yang otomatis generate ulang di akhir dokumen setiap build.

Jangan dihapus dulu sampai audit selesai - dipakai untuk membandingkan apakah
hasil auto-generate sudah sama persis (urutan, isi) dengan versi asli.
