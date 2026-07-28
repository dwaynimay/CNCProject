**BUKU TUGAS AKHIR**

**CAPSTONE DESIGN**

![TELKOM UNIVERSITY - LOGO HORIZONTAL - INFO KAMPUS DAN SUMBER REFERENSI \...](assets/media/image1.png){width="3.8854166666666665in"}

**RANCANG BANGUN SISTEM PENGAWASAN MESIN** **CNC BERBASIS *INTERNET OF THINGS*** **MENGGUNAKAN MEKANISME *CUTOFF* OTOMATIS** **MENGGUNAKAN ESP32**

***DESIGN AND IMPLEMENTATION OF CNC MACHINE*** ***MONITORING SYSTEM BASED ON INTERNET OF THINGS*** ***WITH AUTOMATIC CUTOFF MECHANISM USING ESP32***

**Oleh :**

**Sulthon Hasan Ridhwan / 1101223252**

**Nurul Widia Hasanah Samahuddin / 1101220129**

**Panji Wijaya / 1101223413**

**PRODI S1 TEKNIK TELEKOMUNIKASI**

**FAKULTAS TEKNIK ELEKTRO**

**UNIVERSITAS TELKOM**

**BANDUNG**

**2026**

# LEMBAR PENGESAHAN BUKU CAPSTONE DESIGN {#lembar-pengesahan-buku-capstone-design .Style-1 .unnumbered}

**RANCANG BANGUN SISTEM PENGAWASAN MESIN** **CNC BERBASIS *INTERNET OF THINGS*** **MENGGUNAKAN MEKANISME *CUTOFF* OTOMATIS** **MENGGUNAKAN ESP32**

***DESIGN AND IMPLEMENTATION OF CNC MACHINE*** ***MONITORING SYSTEM BASED ON INTERNET OF THINGS*** ***WITH AUTOMATIC CUTOFF MECHANISM USING ESP32***

**Telah disetujui dan disahkan sebagai bagian dari Capstone Design**

**Program S1 Teknik Telekomunikasi**

**Fakultas Teknik Elektro**

**Universitas Telkom**

**Bandung**

Disusun oleh:

**Sulthon Hasan Ridhwan / 1101223252**

**Nurul Widia Hasanah Samahuddin / 1101220129**

**Panji Wijaya / 1101223413**

**Bandung, 16 Juli 2026**

**Menyetujui,**

  --------------------------------------------------------------------------------------------
                  Pembimbing 1                                   Pembimbing 2
  --------------------------------------------- ----------------------------------------------
                                                

   [Yulinda Eliskar, S.Si., M.Si.]{.underline}   [Aminah Indahsari Marsuki, S.T., M.T.]{.underline}

                  NIP. 25750004                                 NIP. 24200003
  --------------------------------------------------------------------------------------------

# LEMBAR PERNYATAAN ORISINALITAS  {#lembar-pernyataan-orisinalitas .Style-1 .unnumbered}

Saya, yang bertanda tangan di bawah ini

Nama : Sulthon Hasan Ridhwan

NIM : 1101223252

Alamat : Jln. Rafledia, Kota Metro Lampung.

No. Telepon : 0821-7947-5141

Email : Sulthonhr1818@gmail.com

Menyatakan bahwa Buku Capstone Design ini merupakan karya orisinal saya sendiri bersama dengan kelompok Capstone Design saya, dengan judul:

**RANCANG BANGUN SISTEM PENGAWASAN MESIN** **CNC BERBASIS *INTERNET OF THINGS*** **MENGGUNAKAN MEKANISME *CUTOFF* OTOMATIS** **MENGGUNAKAN ESP32**

***DESIGN AND IMPLEMENTATION OF CNC MACHINE*** ***MONITORING SYSTEM BASED ON INTERNET OF THINGS*** ***WITH AUTOMATIC CUTOFF MECHANISM USING ESP32***

Atas pernyataan ini, saya siap menanggung resiko/sanksi yang dijatuhkan kepada saya apabila dikemudian ditemukan adanya pelanggaran terhadap kejujuran akademik atau etika keilmuan dalam karya ini, atau ditemukan bukti yang menunjukkan ketidak aslian karya ini.

  ----------------------------------------------------------------------------------------------------------
                                            Bandung, 16 Juli 2026
  ----------------------------------------------------------------------------------------------------------
   ![](assets/media/image2.png){width="1.1243055555555554in"}

                                            Sulthon Hasan Ridhwan

                                                  1101223252
  ----------------------------------------------------------------------------------------------------------

#  LEMBAR PERNYATAAN ORISINALITAS  {#lembar-pernyataan-orisinalitas-1 .Style-1 .unnumbered}

Saya, yang bertanda tangan di bawah ini

Nama : Nurul Widia Hasanah Samahuddin

NIM : 1101220129

Alamat : Citra Garden Cluster Elegantu E/01, Makassar

No. Telepon : 0812-4376-7132

Email : nurulwidia0307@gmail.com

Menyatakan bahwa Buku Capstone Design ini merupakan karya orisinal saya sendiri bersama dengan kelompok Capstone Design saya, dengan judul:

**RANCANG BANGUN SISTEM PENGAWASAN MESIN** **CNC BERBASIS *INTERNET OF THINGS*** **MENGGUNAKAN MEKANISME *CUTOFF* OTOMATIS** **MENGGUNAKAN ESP32**

***DESIGN AND IMPLEMENTATION OF CNC MACHINE*** ***MONITORING SYSTEM BASED ON INTERNET OF THINGS*** ***WITH AUTOMATIC CUTOFF MECHANISM USING ESP32***

Atas pernyataan ini, saya siap menanggung resiko/sanksi yang dijatuhkan kepada saya apabila dikemudian ditemukan adanya pelanggaran terhadap kejujuran akademik atau etika keilmuan dalam karya ini, atau ditemukan bukti yang menunjukkan ketidak aslian karya ini.

  ----------------------------------------------------------------------------------------------------------
                                            Bandung, 16 Juli 2026
  ----------------------------------------------------------------------------------------------------------
   ![](assets/media/image3.png){width="1.7222222222222223in"}

                                        Nurul Widia Hasanah Samahuddin

                                                  1101220129
  ----------------------------------------------------------------------------------------------------------

# LEMBAR PERNYATAAN ORISINALITAS  {#lembar-pernyataan-orisinalitas-2 .Style-1 .unnumbered}

Saya, yang bertanda tangan di bawah ini

Nama : Panji Wijaya

NIM : 1101223413

Alamat : Jln. Ethanol, Unit 2 Tulang Bawang

No. Telepon : 0822-7815-3805

Email : panjiwijaya254@gmail.com

Menyatakan bahwa Buku Capstone Design ini merupakan karya orisinal saya sendiri bersama dengan kelompok Capstone Design saya, dengan judul:

**RANCANG BANGUN SISTEM PENGAWASAN MESIN** **CNC BERBASIS *INTERNET OF THINGS*** **MENGGUNAKAN MEKANISME *CUTOFF* OTOMATIS** **MENGGUNAKAN ESP32**

***DESIGN AND IMPLEMENTATION OF CNC MACHINE*** ***MONITORING SYSTEM BASED ON INTERNET OF THINGS*** ***WITH AUTOMATIC CUTOFF MECHANISM USING ESP32***

Atas pernyataan ini, saya siap menanggung resiko/sanksi yang dijatuhkan kepada saya apabila dikemudian ditemukan adanya pelanggaran terhadap kejujuran akademik atau etika keilmuan dalam karya ini, atau ditemukan bukti yang menunjukkan ketidak aslian karya ini.

  ----------------------------------------------------------------------------------------------
                                      Bandung, 16 Juli 2026
  ----------------------------------------------------------------------------------------------
   ![](assets/media/image4.png){width="1.73125in"}

                                           Panji Wijaya

                                            1101223413
  ----------------------------------------------------------------------------------------------

# ABSTRAK {#abstrak .Style-1 .unnumbered}

Bengkel dan industri skala kecil sering menggantungkan produksi harian pada satu unit mesin CNC *milling* yang pengawasannya bergantung penuh pada kehadiran operator. Kenaikan suhu atau arus pada Spindle dan Motor Stepper yang melampaui batas aman dapat merusak komponen dalam hitungan detik, jauh lebih cepat daripada reaksi manual operator. Permasalahan utama penelitian ini adalah belum tersedianya mekanisme pengawasan otomatis yang mampu memantau kondisi mesin CNC secara berkelanjutan dan memutus daya tanpa menunggu reaksi operator.

Penelitian ini merancang dan mengimplementasikan sistem pengawasan mesin CNC berbasis *Internet of Things* dengan mekanisme *cutoff* otomatis menggunakan ESP32. Sistem membaca suhu melalui dua sensor DS18B20 dan arus melalui lima sensor ACS712, lalu mengirim data ke server melalui MQTT untuk ditampilkan secara *real-time* pada dashboard web. Saat ambang batas terlampaui, ESP32 memutus daya Spindle melalui relay, mengirim sinyal E-Stop ke CNC *shield*, dan mengirim notifikasi ke Telegram operator. Mekanisme *heartbeat fail-safe* mengunci sistem ke *Safe Mode* apabila komunikasi terputus lebih dari 60 detik, dilengkapi fitur kalibrasi dan *self-test* untuk menjaga akurasi dan kesiapan sistem.

Pengujian pada mesin nyata menunjukkan galat suhu rata-rata 0,4°C, MAE arus 0,0069-0,0403 A, seluruh percobaan ambang proteksi (100%) berhasil memicu *cutoff* di bawah tiga detik, *fail-safe* terkunci mandiri rata-rata 62 detik, dan notifikasi Telegram terkirim dengan latensi rata-rata 418,4 ms tanpa alarm keliru pada 970 baris data telemetri pemesinan nyata. Sistem terbukti mampu memantau kondisi mesin secara berkelanjutan dan memutus daya otomatis tanpa menunggu reaksi operator, menurunkan risiko kerusakan komponen dan waktu henti mesin pada industri kecil.

Kata kunci: *cutoff* otomatis, ESP32, Internet of Things, mesin CNC, notifikasi Telegram

# ABSTRACT {#abstract .Style-1 .unnumbered}

Small workshops and small-scale industries often rely on a single CNC *milling* machine whose supervision depends entirely on operator presence. Temperature or current spikes on the Spindle and stepper motors beyond safe limits can damage components within seconds, far faster than manual operator response. The main problem addressed in this research is the lack of an automatic monitoring mechanism able to continuously observe machine condition and cut power without waiting for operator response.

This research designs and implements an IoT-based CNC monitoring system with an automatic *cutoff* mechanism using ESP32. The system reads temperature through two DS18B20 sensors and current through five ACS712 sensors, then sends data to a server via MQTT for *real-time* display on a web dashboard. When a threshold is exceeded, the ESP32 cuts Spindle power through a relay, sends an E-Stop signal to the CNC *shield*, and sends a Telegram notification to the operator. A *heartbeat fail-safe* mechanism locks the system into *Safe Mode* if communication is lost for more than 60 seconds, supported by calibration and *self-test* features to maintain accuracy and readiness.

Testing on an actual machine shows an average temperature error of 0.4°C, a current MAE of 0.0069-0.0403 A, all protection threshold trials (100%) triggering *cutoff* under three seconds, the *fail-safe* mechanism locking independently within an average of 62 seconds, and Telegram notifications delivered with an average latency of 418.4 ms, with no false alarms across 970 rows of real machining telemetry. The system proves capable of continuously monitoring machine condition and cutting power automatically without waiting for operator response, reducing the risk of component damage and downtime in small industries.

Keywords: automatic *cutoff*, CNC machine, ESP32, Internet of Things, Telegram notification

# KATA PENGANTAR {#kata-pengantar .Style-1 .unnumbered}

Puji syukur penulis panjatkan ke hadirat Allah SWT atas rahmat, karunia, serta hidayah-Nya sehingga penulis dapat menyelesaikan Tugas Akhir Capstone Design yang berjudul \"Rancang Bangun Sistem Pengawasan Mesin CNC Berbasis *Internet of Things* Menggunakan Mekanisme *Cutoff* Otomatis\" dengan baik. Proses penyusunan Tugas Akhir ini melalui beberapa tahapan, mulai dari studi literatur, identifikasi kebutuhan sistem, perancangan, implementasi, pengujian, analisis hasil, hingga penyusunan laporan.

Penyusunan Tugas Akhir Capstone Design ini adalah salah satu tahapan akademik pada Program Studi S1 Teknik Telekomunikasi, Fakultas Teknik Elektro, Telkom University, sekaligus sarana bagi penulis menerapkan ilmu yang diperoleh selama masa perkuliahan. Melalui Tugas Akhir ini, penulis merancang dan mengimplementasikan sistem pengawasan mesin *Computer Numerical Control* (CNC) berbasis *Internet of Things* (IoT) yang memanfaatkan sensor suhu dan sensor arus untuk memantau kondisi operasional mesin secara *real-time*. Sistem tersebut dilengkapi dengan mekanisme *cutoff* otomatis untuk menghentikan operasi mesin ketika parameter yang dipantau melebihi batas yang telah ditentukan.

Penulis menyadari bahwa buku ini masih memiliki keterbatasan, baik dari sisi pembahasan maupun implementasi sistem. Kritik dan saran yang membangun sangat diharapkan sebagai bahan evaluasi bagi penyempurnaan pengembangan sistem maupun penyusunan karya ilmiah selanjutnya. Akhir kata, penulis berharap buku Tugas Akhir ini bermanfaat dan menjadi salah satu referensi bagi pembaca dalam pengembangan sistem monitoring berbasis IoT, khususnya pada penerapan sistem pengawasan dan proteksi mesin CNC. Penulis juga berharap hasil Tugas Akhir Capstone Design ini dapat menjadi dasar pengembangan sistem yang lebih baik pada penelitian maupun implementasi selanjutnya.

# UCAPAN TERIMAKASIH {#ucapan-terimakasih .Style-1 .unnumbered}

Puji syukur penulis panjatkan ke hadirat Allah SWT atas rahmat, hidayah, dan karunia-Nya sehingga penulis dapat menyelesaikan Tugas Akhir yang berjudul \"Rancang Bangun Sistem Pengawasan Mesin CNC Berbasis Internet of Things Menggunakan Mekanisme Cutoff Otomatis Menggunakan ESP32\" dengan baik. Penyusunan Tugas Akhir ini adalah bagian dari pelaksanaan Tugas Akhir Capstone Design pada Program Studi S1 Teknik Telekomunikasi, Fakultas Teknik Elektro, Telkom University, yang terselesaikan berkat bantuan, bimbingan, doa, serta dukungan dari berbagai pihak selama seluruh tahapan penyusunan. Pada kesempatan ini, penulis menyampaikan ucapan terima kasih yang sebesar-besarnya kepada:

1.  Allah SWT atas segala rahmat, karunia, kesehatan, dan kemudahan yang diberikan kepada penulis.

2.  Kedua orang tua dan seluruh anggota keluarga, atas doa, kasih sayang, motivasi, serta dukungan moral dan material tanpa henti selama masa studi.

3.  Ibu **Yulinda Eliskar, S.Si., M.Si.**, selaku dosen pembimbing 1, atas arahan, bimbingan, masukan, dan motivasi selama penelitian dan penyusunan Tugas Akhir.

4.  Ibu **Aminah Indahsari Marsuki, S.T., M.T**., selaku dosen pembimbing 2, atas arahan, bimbingan, masukan, dan motivasi selama penelitian dan penyusunan Tugas Akhir.

5.  Bapak **Roy Chandra Kusnadi, S.T.**, selaku pembimbing luar, atas waktu, arahan, saran, dan dukungan selama penelitian dan penyusunan Tugas Akhir.

6.  Seluruh dosen dan staf pengajar Fakultas Teknik Elektro, atas ilmu, pengalaman, dan pelayanan selama masa perkuliahan.

7.  Teman-teman seperjuangan Program Studi Teknik Telekomunikasi, atas semangat, bantuan, dan motivasi selama penyelesaian Tugas Akhir.

8.  Semua pihak yang membantu baik secara langsung maupun tidak langsung hingga selesainya penyusunan Tugas Akhir ini.

Penulis berharap seluruh bantuan, dukungan, dan doa yang diberikan memperoleh balasan kebaikan dari Allah SWT, serta penelitian ini bermanfaat dan menjadi salah satu referensi bagi pengembangan penelitian selanjutnya.

# DAFTAR SINGKATAN {#daftar-singkatan .Style-1 .unnumbered}

| Singkatan | Kepanjangan |
|---|---|
| ACS712 | Allegro Current Sensor 712 |
| ADC | Analog-to-Digital Converter |
| API | Application Programming Interface |
| CNC | Computer Numerical Control |
| DS18B20 | Digital Serial Temperature Sensor 18B20 |
| EMA | Exponential Moving Average |
| EMI | Electromagnetic Interference |
| ESP32 | Espressif 32-bit Microcontroller |
| FIFO | First-In First-Out |
| GPIO | General Purpose Input/Output |
| HTTP | Hypertext Transfer Protocol |
| HTTPS | Hypertext Transfer Protocol Secure |
| IDE | Integrated Development Environment |
| IEC | International Electrotechnical Commission |
| IoT | Internet of Things |
| IP | Internet Protocol |
| ISO | International Organization for Standardization |
| JSON | JavaScript Object Notation |
| LAN | Local Area Network |
| LWT | Last Will and Testament |
| MAE | Mean Absolute Error |
| MQTT | Message Queuing Telemetry Transport |
| NC | Normally Closed |
| NO | Normally Open |
| NPN | Negative-Positive-Negative Transistor |
| NTP | Network Time Protocol |
| NVS | Non-Volatile Storage |
| OOP | Object-Oriented Programming |
| PCB | Printed Circuit Board |
| QoS | Quality of Service |
| RAM | Random Access Memory |
| REST | Representational State Transfer |
| RTC | Real-Time Clock |
| SCADA | Supervisory Control and Data Acquisition |
| SPA | Single Page Application |
| SPDT | Single Pole Double Throw |
| SQL | Structured Query Language |
| TCP | Transmission Control Protocol |
| USB | Universal Serial Bus |
| UTC | Coordinated Universal Time |
| WAL | Write-Ahead Logging |
| WIB | Waktu Indonesia Barat |
| Wi-Fi | Wireless Fidelity |
| WS | WebSocket |
