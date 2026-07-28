# DAFTAR ISI {#daftar-isi .Style-1}

[LEMBAR PENGESAHAN BUKU CAPSTONE DESIGN [i](#lembar-pengesahan-buku-capstone-design)](#lembar-pengesahan-buku-capstone-design)

[LEMBAR PERNYATAAN ORISINALITAS [ii](#lembar-pernyataan-orisinalitas)](#lembar-pernyataan-orisinalitas)

[LEMBAR PERNYATAAN ORISINALITAS [iii](#lembar-pernyataan-orisinalitas-1)](#lembar-pernyataan-orisinalitas-1)

[LEMBAR PERNYATAAN ORISINALITAS [iv](#lembar-pernyataan-orisinalitas-2)](#lembar-pernyataan-orisinalitas-2)

[ABSTRAK [v](#abstrak)](#abstrak)

[ABSTRACT [vi](#abstract)](#abstract)

[KATA PENGANTAR [vii](#kata-pengantar)](#kata-pengantar)

[UCAPAN TERIMAKASIH [viii](#ucapan-terimakasih)](#ucapan-terimakasih)

[DAFTAR ISI [ix](#daftar-isi)](#daftar-isi)

[DAFTAR GAMBAR [xii](#daftar-gambar)](#daftar-gambar)

[DAFTAR TABEL [xiv](#daftar-tabel)](#daftar-tabel)

[DAFTAR SINGKATAN [xv](#daftar-singkatan)](#daftar-singkatan)

[BAB 1 PENDAHULUAN [1](#pendahuluan)](#pendahuluan)

[1.1 Deskripsi Umum Masalah [1](#deskripsi-umum-masalah)](#deskripsi-umum-masalah)

[1.2 Analisis Masalah [2](#analisis-masalah)](#analisis-masalah)

[1.2.1 Aspek Teknis [2](#aspek-teknis)](#aspek-teknis)

[1.2.2 Aspek Ekonomi [2](#aspek-ekonomi)](#aspek-ekonomi)

[1.2.3 Aspek Operasional [2](#aspek-operasional)](#aspek-operasional)

[1.3 Analisis Solusi yang Sudah Ada [3](#analisis-solusi-yang-sudah-ada)](#analisis-solusi-yang-sudah-ada)

[1.3.1 Pemeriksaan Manual oleh Operator [3](#pemeriksaan-manual-oleh-operator)](#pemeriksaan-manual-oleh-operator)

[1.3.2 Sistem Monitoring Berbasis Sensor [3](#sistem-monitoring-berbasis-sensor)](#sistem-monitoring-berbasis-sensor)

[1.3.3 Sistem SCADA Industri Komersial [4](#sistem-scada-industri-komersial)](#sistem-scada-industri-komersial)

[1.3.4 Sistem Pengawasan Berbasis IoT dengan Mekanisme *Cutoff* Otomatis [4](#sistem-pengawasan-berbasis-iot-dengan-mekanisme-cutoff-otomatis)](#sistem-pengawasan-berbasis-iot-dengan-mekanisme-cutoff-otomatis)

[1.4 Tujuan Capstone [6](#tujuan-capstone)](#tujuan-capstone)

[1.5 Batasan Tugas Akhir [6](#batasan-tugas-akhir)](#batasan-tugas-akhir)

[BAB 2 TINJAUAN PUSTAKA [8](#tinjauan-pustaka)](#tinjauan-pustaka)

[2.1 Internet of Things (IoT) [8](#internet-of-things-iot)](#internet-of-things-iot)

[2.2 Mesin CNC (Computer Numerical Control) [8](#mesin-cnc-computer-numerical-control)](#mesin-cnc-computer-numerical-control)

[2.3 Mikrokontroler, Sensor, dan Aktuator untuk Pemantauan [10](#mikrokontroler-sensor-dan-aktuator-untuk-pemantauan)](#mikrokontroler-sensor-dan-aktuator-untuk-pemantauan)

[2.3.1 ESP32 [10](#esp32)](#esp32)

[2.3.2 Sensor Arus ACS712 [10](#sensor-arus-acs712)](#sensor-arus-acs712)

[2.3.3 Sensor Suhu DS18B20 [11](#sensor-suhu-ds18b20)](#sensor-suhu-ds18b20)

[2.3.4 Relay [12](#relay)](#relay)

[2.4 Komunikasi Data MQTT [13](#komunikasi-data-mqtt)](#komunikasi-data-mqtt)

[2.5 Arsitektur Client-Server untuk Sistem Pemantauan [14](#arsitektur-client-server-untuk-sistem-pemantauan)](#arsitektur-client-server-untuk-sistem-pemantauan)

[2.5.1 REST API [14](#rest-api)](#rest-api)

[2.5.2 WebSocket [15](#websocket)](#websocket)

[2.5.3 Database [15](#database)](#database)

[BAB 3 SPESIFIKASI DAN DESAIN SISTEM [16](#spesifikasi-dan-desain-sistem)](#spesifikasi-dan-desain-sistem)

[3.1 Spesifikasi Sistem [16](#spesifikasi-sistem)](#spesifikasi-sistem)

[3.2 Desain Sistem [19](#desain-sistem)](#desain-sistem)

[3.2.1 Arsitektur Sistem [20](#arsitektur-sistem)](#arsitektur-sistem)

[3.2.2 Perangkat Keras [22](#perangkat-keras)](#perangkat-keras)

[3.2.3 Skema Rangkain Elektronik [25](#skema-rangkain-elektronik)](#skema-rangkain-elektronik)

[3.2.4 Ambang Batas Sistem [31](#ambang-batas-sistem)](#ambang-batas-sistem)

[3.3 Cara Kerja Sistem [32](#cara-kerja-sistem)](#cara-kerja-sistem)

[3.3.1 Siklus Pemantauan dan *Cutoff* Otomatis [32](#siklus-pemantauan-dan-cutoff-otomatis)](#siklus-pemantauan-dan-cutoff-otomatis)

[3.3.2 Fail-Safe Heartbeat [33](#fail-safe-heartbeat)](#fail-safe-heartbeat)

[3.3.3 Eksekusi Perintah Resume dari Server [34](#eksekusi-perintah-resume-dari-server)](#eksekusi-perintah-resume-dari-server)

[3.3.4 Eksekusi Perintah Kalibrasi [35](#eksekusi-perintah-kalibrasi)](#eksekusi-perintah-kalibrasi)

[3.3.5 Penerimaan Perintah di Sisi Server [36](#penerimaan-perintah-di-sisi-server)](#penerimaan-perintah-di-sisi-server)

[3.3.6 Penerimaan Telemetri di Sisi Server [37](#penerimaan-telemetri-di-sisi-server)](#penerimaan-telemetri-di-sisi-server)

[3.3.7 Diagnostik Self-Test dan Trip-Test [38](#diagnostik-self-test-dan-trip-test)](#diagnostik-self-test-dan-trip-test)

[3.3.8 Struktur Komunikasi MQTT [39](#struktur-komunikasi-mqtt)](#struktur-komunikasi-mqtt)

[3.4 Metode Pengukuran yang Sesuai dengan Solusi Terpilih [39](#metode-pengukuran-yang-sesuai-dengan-solusi-terpilih)](#metode-pengukuran-yang-sesuai-dengan-solusi-terpilih)

[3.4.1 Pengukuran Akurasi Sensor [39](#pengukuran-akurasi-sensor)](#pengukuran-akurasi-sensor)

[3.4.2 Pengukuran Perilaku Proteksi dan Keselamatan [41](#pengukuran-perilaku-proteksi-dan-keselamatan)](#pengukuran-perilaku-proteksi-dan-keselamatan)

[3.4.3 Pengukuran Konsistensi Operasional [42](#pengukuran-konsistensi-operasional)](#pengukuran-konsistensi-operasional)

[3.4.4 Pengukuran Operasional pada Kondisi Nyata [44](#pengukuran-operasional-pada-kondisi-nyata)](#pengukuran-operasional-pada-kondisi-nyata)

[BAB 4 IMPLEMENTASI [46](#implementasi)](#implementasi)

[4.1 Deskripsi Umum Implementasi [46](#deskripsi-umum-implementasi)](#deskripsi-umum-implementasi)

[4.2 Detail Implementasi [47](#detail-implementasi)](#detail-implementasi)

[4.2.1 Hardware [47](#hardware)](#hardware)

[4.2.2 Software Firmware ESP32 [51](#software-firmware-esp32)](#software-firmware-esp32)

[4.2.3 Software Server [72](#software-server)](#software-server)

[4.2.4 Software Dashboard [78](#software-dashboard)](#software-dashboard)

[4.3 Prosedur Pengoperasian Solusi [88](#prosedur-pengoperasian-solusi)](#prosedur-pengoperasian-solusi)

[4.3.1 Menyiapkan dan Menanam Firmware [88](#menyiapkan-dan-menanam-firmware)](#menyiapkan-dan-menanam-firmware)

[4.3.2 Menjalankan Server dan Database [90](#menjalankan-server-dan-database)](#menjalankan-server-dan-database)

[4.3.3 Mengoperasikan Dashboard [94](#mengoperasikan-dashboard)](#mengoperasikan-dashboard)

[BAB 5 PENGUJIAN DAN ANALISIS [97](#pengujian-dan-analisis)](#pengujian-dan-analisis)

[5.1 Skema Pengujian Sistem [97](#skema-pengujian-sistem)](#skema-pengujian-sistem)

[5.1.1 Skema Pengujian Akurasi Sensor [97](#skema-pengujian-akurasi-sensor)](#skema-pengujian-akurasi-sensor)

[5.1.2 Skema Pengujian Perilaku Proteksi dan Keselamatan [98](#skema-pengujian-perilaku-proteksi-dan-keselamatan)](#skema-pengujian-perilaku-proteksi-dan-keselamatan)

[5.1.3 Skema Pengujian Konsistensi Operasional [99](#skema-pengujian-konsistensi-operasional)](#skema-pengujian-konsistensi-operasional)

[5.1.4 Skema Pengujian Operasional pada Kondisi Nyata [100](#skema-pengujian-operasional-pada-kondisi-nyata)](#skema-pengujian-operasional-pada-kondisi-nyata)

[5.2 Proses Pengujian dan Analisis Hasil [101](#proses-pengujian-dan-analisis-hasil)](#proses-pengujian-dan-analisis-hasil)

[5.2.1 Pengujian Akurasi Sensor [101](#pengujian-akurasi-sensor)](#pengujian-akurasi-sensor)

[5.2.2 Pengujian Perilaku Proteksi dan Keselamatan [104](#pengujian-perilaku-proteksi-dan-keselamatan)](#pengujian-perilaku-proteksi-dan-keselamatan)

[5.2.3 Pengujian Konsistensi Operasional [110](#pengujian-konsistensi-operasional)](#pengujian-konsistensi-operasional)

[5.2.4 Pengujian Operasional pada Kondisi Nyata [111](#pengujian-operasional-pada-kondisi-nyata)](#pengujian-operasional-pada-kondisi-nyata)

[5.2.5 Analisis Hasil [114](#analisis-hasil)](#analisis-hasil)

[BAB 6 EVALUASI [116](#evaluasi)](#evaluasi)

[6.1 Rangkuman Hasil Pengujian [116](#rangkuman-hasil-pengujian)](#rangkuman-hasil-pengujian)

[6.2 Kesimpulan [118](#kesimpulan)](#kesimpulan)

[6.3 Saran [119](#saran)](#saran)

[DAFTAR PUSTAKA [121](#daftar-pustaka)](#daftar-pustaka)

[LAMPIRAN [123](#lampiran)](#lampiran)

# DAFTAR GAMBAR {#daftar-gambar .Style-1}

[Gambar 2.1 Mesin CNC (Sumber: Dokumentasi Penulis, 2026) [9](#_Ref233964494)](#_Ref233964494)

[Gambar 2.2 ESP32 DevKit [10](#_Ref233964416)](#_Ref233964416)

[Gambar 2.3 Sensor Arus ACS712 [10](#_Ref233964428)](#_Ref233964428)

[Gambar 2.4 Sensor Suhu DS18B2 [11](#_Ref233964445)](#_Ref233964445)

[Gambar 2.5 Modul Relay [12](#_Ref233964456)](#_Ref233964456)

[Gambar 3.1 Arsitektur Sistem Pengawasan Mesin CNC [19](#_Ref234770407)](#_Ref234770407)

[Gambar 3.2 Diagram Blok Terperinci Mesin CNC [20](#_Ref233895395)](#_Ref233895395)

[Gambar 3.3 Diagram Blok Terperinci IoT Device [21](#_Ref233895426)](#_Ref233895426)

[Gambar 3.4 Diagram Blok Terperinci Server [22](#_Ref233895507)](#_Ref233895507)

[Gambar 3.5 Skema Rangkaian Elektronik Sistem Pengawasan [26](#_Ref234789075)](#_Ref234789075)

[Gambar 3.6 Detail Rangkaian Pengkondisian Sinyal Sensor Arus (kanal Spindle) [27](#_Ref234791896)](#_Ref234791896)

[Gambar 3.7 Detail Rangkaian Sensor Suhu Bus 1-Wire [29](#_Ref234791728)](#_Ref234791728)

[Gambar 3.8 Flowchart Siklus Pemantauan dan *Cutoff* Otomatis [32](#_Ref234795111)](#_Ref234795111)

[Gambar 3.9 Flowchart Fail-Safe Heartbeat [33](#_Ref234796851)](#_Ref234796851)

[Gambar 3.10 Flowchart Eksekusi Perintah Relay_off [34](#_Ref234796863)](#_Ref234796863)

[Gambar 3.11 Flowchart Eksekusi Perintah Kalibrasi [35](#_Ref234796869)](#_Ref234796869)

[Gambar 3.12 Flowchart Penerimaan Perintah di Sisi Server [36](#_Ref234796875)](#_Ref234796875)

[Gambar 3.13 Flowchart Penerimaan Telemetri di Sisi Server [37](#_Ref234796890)](#_Ref234796890)

[Gambar 3.14 Flowchart Diagnostik Self-Test [38](#_Ref234796897)](#_Ref234796897)

[Gambar 4.1 Alat Sistem Implementasi [46](#_Toc235108804)](#_Toc235108804)

[Gambar 4.2 Rangkaian ESP32 dan Sensor Arus ACS712 [48](#_Toc235108805)](#_Toc235108805)

[Gambar 4.3 Sensor Suhu DS18B20 pada Spindle (kiri) dan Stepper Z (kanan) [49](#_Toc235108806)](#_Toc235108806)

[Gambar 4.4 Rangkaian Relay (kanan) dan Jalur Sinyal E-Stop (kiri) [50](#_Toc235108807)](#_Toc235108807)

[Gambar 4.5 Integrasi ESP32 dengan Mesin CNC [51](#_Toc235108808)](#_Toc235108808)

[Gambar 4.6 Tampilan Kartu Sensor Arus pada Dashboard [81](#_Toc235108809)](#_Toc235108809)

[Gambar 4.7 Tampilan Kartu Sensor Suhu pada Dashboard [82](#_Toc235108810)](#_Toc235108810)

[Gambar 4.8 Tampilan Tombol Kontrol Mesin [83](#_Toc235108811)](#_Toc235108811)

[Gambar 4.9 Grafik Tren Arus dan Tren Suhu [84](#_Toc235108812)](#_Toc235108812)

[Gambar 4.10 Panel Uji Injeksi Arus Lebih [86](#_Toc235108813)](#_Toc235108813)

[Gambar 4.11 Panel Log Aktivitas Dashboard [88](#_Toc235108814)](#_Toc235108814)

[Gambar 4.12 Ekstensi VS Code Platfrom10 IDE [89](#_Toc235108815)](#_Toc235108815)

[Gambar 4.13 File credentials.h dalam Folder Proyek Firmware [89](#_Toc235108816)](#_Toc235108816)

[Gambar 4.14 Proses Build dan Upload Firmware pada ESP32 [90](#_Toc235108817)](#_Toc235108817)

[Gambar 4.15 Sistem Berhasil Terhubung ke WIFI dan MQTT serta Sinkoriisai NTP [90](#_Toc235108818)](#_Toc235108818)

[Gambar 4.16 Membuat file env pada Direktori dan Dashboard [93](#_Toc235108819)](#_Toc235108819)

[Gambar 4.17 Server Berhasil Dijalankan [93](#_Toc235108820)](#_Toc235108820)

[Gambar 4.18 Dashboard Terhubung dengan Backend dan ESP32 [94](#_Toc235108821)](#_Toc235108821)

[Gambar 4.19 Kalibrasi Set Nol Sensor Arus [94](#_Toc235108822)](#_Toc235108822)

[Gambar 4.20 Panel Test Arus Berlebih [95](#_Toc235108823)](#_Toc235108823)

[Gambar 4.21 Respon Sistem Ketika Tes Arus Berlebih [95](#_Toc235108824)](#_Toc235108824)

[Gambar 4.22 Log Aktivitas Perintah yang Dikirimkan [96](#_Toc235108825)](#_Toc235108825)

[Gambar 5.1 Skema Rangkaian Bench test Validasi Sensor Arus [97](#_Ref235039499)](#_Ref235039499)

[Gambar 5.2 Hasil Perakitan Fisik Rangkaian Bench test [98](#_Ref235039589)](#_Ref235039589)

[Gambar 5.3 Overlay Arus Spindle [106](#_Ref235040825)](#_Ref235040825)

[Gambar 5.4 Overlay Arus Stepper_X [106](#_Toc235108829)](#_Toc235108829)

[Gambar 5.5 Overlay Arus Stepper_Y1 [107](#_Toc235108830)](#_Toc235108830)

[Gambar 5.6 Overlay Arus Stepper_Y2 [107](#_Toc235108831)](#_Toc235108831)

[Gambar 5.7 Overlay Arus Stepper_Z [107](#_Ref235040850)](#_Ref235040850)

[Gambar 5.8 Overlay Suhu Spindle [108](#_Ref235040934)](#_Ref235040934)

[Gambar 5.9 Overlay Suhu Stepper_Z [108](#_Ref235040943)](#_Ref235040943)

[Gambar 5.10 Interval Periode Pemantauan [110](#_Ref235041900)](#_Ref235041900)

[Gambar 5.11 Tren Arus Periode Pemantauan (Idle) [110](#_Ref235042040)](#_Ref235042040)

[Gambar 5.12 Tren Arus dan Suhu Selama Proses Pemesinan Sungguhan [112](#_Ref235042932)](#_Ref235042932)

# DAFTAR TABEL {#daftar-tabel .Style-1}

[Tabel 1.1 Perbandingan Solusi Pengawasan Mesin CNC yang Sudah Ada [5](#_Ref234717937)](#_Ref234717937)

[Tabel 2.1 Varian Sensor ACS712 [11](#_Ref234758366)](#_Ref234758366)

[Tabel 2.2 Tingkat Quality of Service (QoS) pada MQTT [13](#_Ref234766597)](#_Ref234766597)

[Tabel 3.1 Spesifikasi Sistem Pengawasan Mesin CNC [16](#_Ref234953292)](#_Ref234953292)

[Tabel 3.2 Spesifikasi Perangkat Keras [23](#_Ref234783327)](#_Ref234783327)

[Tabel 3.3 Pinout ESP32 [24](#_Toc235108843)](#_Toc235108843)

[Tabel 3.4 Parameter Desigator Rangkaian Pengondisian Sinyal Sensor Arus [28](#_Ref234790916)](#_Ref234790916)

[Tabel 3.5 Ambang Batas dan Histeresis Sistem [31](#_Ref234798698)](#_Ref234798698)

[Tabel 3.6 Struktur Topik MQTT [39](#_Ref234797077)](#_Ref234797077)

[Tabel 3.7 Metode Pengukuran dan Verifikasi Akurasi Sensor [40](#_Ref234802564)](#_Ref234802564)

[Tabel 3.8 Metode Pengukuran dan Verifikasi Perilaku Proteksi dan Keselamatan [41](#_Ref234802809)](#_Ref234802809)

[Tabel 3.9 Metode Pengukuran dan Verifikasi Konsistensi Operasional [43](#_Ref234838927)](#_Ref234838927)

[Tabel 3.10 Metode Pengukuran dan Verifikasi Operasional pada Kondisi Nyata [44](#_Ref234967090)](#_Ref234967090)

[Tabel 4.1 Analog Batas dan Elektronik pada Firmware [54](#_Toc235108851)](#_Toc235108851)

[Tabel 4.2 Spesifikasi Komputer Server [91](#_Toc235108852)](#_Toc235108852)

[Tabel 5.1 Ringkasan Hasil Pengukuran Akurasi Sensor Arus Stepper X [101](#_Ref235100997)](#_Ref235100997)

[Tabel 5.2 Ringkasan Hasil Pengukuran Akurasi Sensor Arus Stepper Y1 [102](#_Toc235108854)](#_Toc235108854)

[Tabel 5.3 Ringkasan Hasil Pengukuran Akurasi Sensor Arus Stepper Y2 [102](#_Toc235108855)](#_Toc235108855)

[Tabel 5.4 Ringkasan Hasil Pengukuran Akurasi Sensor Arus Stepper Z [102](#_Ref235101003)](#_Ref235101003)

[Tabel 5.5 Ringkasan Hasil Pengukuran Akurasi Sensor Arus Spindle [103](#_Ref235101281)](#_Ref235101281)

[Tabel 5.6 Ringkasan Hasil Pengukuran Akurasi Sensor Suhu [103](#_Ref235100963)](#_Ref235100963)

[Tabel 5.1 Ringkasan Hasil Pengujian Ambang Proteksi [104](#_Ref235040539)](#_Ref235040539)

[Tabel 5.2 Waktu Pemutusan per Kanal [105](#_Ref235040613)](#_Ref235040613)

[Tabel 5.3 Ringkasan Keberhasilan Uji Ambang Proteksi dan Histeresis [106](#_Toc235108861)](#_Toc235108861)

[Tabel 5.4 Ringkasan Uji Respons Fail-Safe [109](#_Ref235041630)](#_Ref235041630)

[Tabel 5.5 Ringkasan Uji Kontrol dan Kesesuaian Data Dashboard [111](#_Ref235042710)](#_Ref235042710)

[Tabel 5.6 Perbandingan Statistik Interval antara Kondisi Idle dan Kondisi Kerja Nyata [113](#_Ref235043358)](#_Ref235043358)

[Tabel 6.1 Perbandingan Spesifikasi Rancangan dan Realisasi Pengujian [116](#_Ref235104479)](#_Ref235104479)

# DAFTAR SINGKATAN {#daftar-singkatan .Style-1}

  -----------------------------------------------------------------------
  CNC             : Computer Numerical Control
  --------------- -------------------------------------------------------
  ACS712          : Allegro Current Sensor 712

  DS18B20         : Digital Serial Temperature Sensor 18B20

  ADC             : Analog-to-Digital Converter

  CAD             : Computer-Aided Design

  CAM             : Computer-Aided Manufacturing

  CPU             : Central Processing Unit

  EMA             : Exponential Moving Average

  ESP32           : Espressif 32-bit Microcontroller

  GPIO            : General Purpose Input/Output

  HTTP            : Hypertext Transfer Protocol

  IoT             : Internet of Things

  IP              : Internet Protocol

  MQTT            : Message Queuing Telemetry Transport

  PCB             : Printed Circuit Board

  QoS             : Quality of Service

  SQL             : Structured Query Language

  WS              : WebSocket

  Wi-Fi           : Wireless Fidelity
  -----------------------------------------------------------------------
