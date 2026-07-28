# Konteks: Penulisan Tugas Akhir

Folder ini adalah vault Obsidian untuk penulisan Tugas Akhir (topik: sistem monitoring CNC berbasis IoT — ESP32, ACS712, DS18B20, MQTT). Instruksi di sini berlaku untuk semua file `.md` di dalam `CNCTA/`, terpisah dari CLAUDE.md di root repo yang mengatur firmware/coding.

## Gaya penulisan
- Bahasa Indonesia akademik formal — hindari kalimat percakapan, kontraksi, atau nada AI-generated generik.
- Pertahankan istilah teknis asli, jangan disederhanakan: "sparse recovery", "sampling rate", "compressive sensing", nama algoritma, nama protokol (MQTT, QoS), nama komponen (ACS712, DS18B20).
- Sitasi memakai gaya **IEEE** — format `[1]`, `[2]`, referensi diambil dari `referensi.bib` (auto-export dari Zotero via Better BibTeX, JANGAN edit manual — edit lewat Zotero lalu re-export).
- Gambar dan tabel wajib diberi label untuk cross-reference `pandoc-crossref` (`{#fig:nama}`, `{#tbl:nama}`), bukan hardcode nomor manual.

## Batasan
- Jangan mengubah struktur/urutan bab tanpa konfirmasi eksplisit dari user.
- Jangan menghapus atau mengganti isi sitasi yang sudah ada tanpa diminta.
- Jalankan skill Anti AI Slop Writing sebelum menganggap draf bab selesai.

## Build ke DOCX
```
pandoc --defaults CNCTA/pandoc-defaults.yaml -o CNCTA/output/TA-final.docx CNCTA/01-pendahuluan.md CNCTA/02-tinjauan-pustaka.md ...
```
