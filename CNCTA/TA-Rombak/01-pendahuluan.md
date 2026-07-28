#  PENDAHULUAN

## Deskripsi Umum Masalah

Mesin CNC *(Computer Numerical Control)* menjalankan proses pemesinan melalui kendali komputer, sehingga pemotongan benda kerja berlangsung otomatis dengan ketelitian yang sulit dicapai secara manual [@altintas2012]. Pada bengkel dan industri skala kecil, satu unit mesin sering menjadi tumpuan produksi sehari-hari. Spindle, Motor Stepper, dan jalur kelistrikan adalah tiga komponen yang bekerja paling keras selama mesin beroperasi. Mesin yang dipakai pada penelitian ini menggunakan Spindle NRT-Pro 3709 HD dengan daya 530 watt dan kecepatan hingga 30.000 putaran per menit.

Persoalan muncul ketika mesin dipakai terus-menerus dalam durasi panjang. Suhu pada Spindle dan Motor Stepper naik seiring beban kerja, sementara arus listrik ikut berubah ketika beban motor tidak stabil. Dua gejala ini, panas berlebih dan lonjakan arus, adalah penanda awal adanya gangguan pada mesin yang lazim dimanfaatkan pada pendekatan pemeliharaan prediktif [@hector2024]. Bila terlambat diketahui, panas dan arus abnormal dapat merusak lilitan motor dan memperpendek umur Spindle, bahkan menghentikan produksi karena komponen terbakar.

Pengawasan kondisi mesin pada industri skala kecil umumnya masih bertumpu pada operator. Satu sampai dua orang menjalankan mesin sekaligus mengurus penyiapan benda kerja, pemantauan jalannya program, dan pengecekan fisik. Operator tidak mungkin terus berdiri di samping mesin sepanjang proses berlangsung, sehingga perubahan suhu atau arus yang terjadi di tengah operasi kerap baru disadari setelah muncul gejala yang kasat mata, seperti bau hangus atau suara yang berubah. Pemantauan yang bergantung pada kehadiran fisik operator menyisakan jeda waktu yang signifikan antara munculnya gangguan dan penanganannya.

Teknologi *Internet of Things* (IoT) membuka jalan untuk menutup celah ini. Data dari sensor yang terpasang pada mesin dapat dikirim melalui jaringan, lalu ditampilkan pada perangkat lain tanpa operator perlu mengecek mesin secara langsung [@soori2023]. Suhu dan arus listrik dipakai sebagai parameter terukur kondisi mesin, dibaca masing-masing oleh sensor suhu dan sensor arus yang sudah lazim diterapkan pada sistem pemantauan berbasis mikrokontroler [@ibrahim2023], [@nurfalaq2025]. Dengan pendekatan ini, kondisi suhu dan arus mesin CNC dapat dipantau dari jarak jauh, dan perubahan yang terjadi dapat terdeteksi lebih awal sebelum berkembang menjadi kerusakan.

## Analisis Masalah

Keterlambatan dalam mendeteksi gangguan pada Spindle maupun Motor Stepper tidak bisa dipandang sekadar sebagai masalah teknis. Dampaknya bersifat sistemik dan meluas ke tiga aspek sekaligus: memburuknya kondisi fisik mesin, membengkaknya biaya akibat kerusakan dan waktu henti produksi (*downtime*), serta meningkatnya beban operasional pekerja. Ketiga aspek ini saling berkaitan erat membentuk sebuah siklus. Kegagalan deteksi teknis akan memicu kerugian ekonomi, sementara keterbatasan operasional justru memperbesar risiko kegagalan teknis tersebut. Oleh karena itu, urgensi pengawasan mesin CNC pada industri kecil harus dikaji secara komprehensif melalui tiga pendekatan utama: teknis, ekonomi, dan operasional.

### Aspek Teknis

Mesin CNC tersusun dari beberapa subsistem yang bekerja bersamaan: sistem kendali gerak pada sumbu X, Y, dan Z, motor Spindle sebagai pemutar mata pahat, serta jalur kelistrikan yang memasok daya ke seluruh bagian. Selama pemesinan, beban pada tiap subsistem berubah mengikuti kekerasan material dan kedalaman pemotongan. Perubahan beban ini terbaca sebagai kenaikan suhu pada Spindle dan Motor Stepper, serta perubahan arus listrik yang ditarik tiap motor [@javaid2021]. Persoalan teknisnya terletak pada sifat perubahan yang berlangsung cepat dan tidak selalu menampakkan gejala luar. Lonjakan arus akibat motor yang macet, misalnya, dapat terjadi dalam hitungan detik, jauh lebih cepat daripada kemampuan operator untuk menyadari dan bereaksi.

### Aspek Ekonomi

Kerusakan komponen mesin CNC menimbulkan biaya yang berlapis. Spindle dan Motor Stepper bukan komponen murah, dan penggantiannya pada industri kecil sering berarti menunggu ketersediaan suku cadang sambil produksi terhenti. Selain biaya penggantian, ada kerugian dari waktu henti mesin *(downtime)* yang memotong jam produktif. Bagi bengkel yang menggantungkan pemasukan pada satu atau dua unit mesin, satu kejadian kerusakan besar dapat mengganggu pemenuhan pesanan.

### Aspek Operasional

Beban kerja operator pada industri kecil sudah padat. Satu sampai dua orang menangani banyak tugas sekaligus, sehingga pengawasan kondisi mesin menjadi pekerjaan tambahan yang sulit dijalankan secara konsisten. Ketika operator harus meninggalkan mesin untuk menyiapkan benda kerja berikutnya atau mengurus pekerjaan lain, mesin praktis berjalan tanpa pengawasan. Tidak adanya mekanisme yang dapat menghentikan mesin secara mandiri ketika kondisi membahayakan membuat keselamatan mesin sepenuhnya bergantung pada kehadiran dan kewaspadaan manusia.

## Analisis Solusi yang Sudah Ada

Bagian ini membahas pendekatan pengawasan mesin CNC yang sudah dipakai sebelum sistem pada penelitian ini dirancang. Empat pendekatan dibahas: pemeriksaan manual oleh operator, sistem monitoring berbasis sensor, sistem SCADA industri komersial, dan solusi yang diusulkan pada penelitian ini. Tiap pendekatan ditinjau dari kemampuan deteksi, kemampuan bertindak, dan keterjangkauan biaya. Perbandingan keempatnya dirangkum pada [@tbl:perbandingan-solusi] di akhir bagian ini.

### Pemeriksaan Manual oleh Operator

Pemeriksaan manual mengandalkan pengamatan langsung oleh operator terhadap kondisi mesin selama proses kerja berlangsung. Operator memeriksa mesin ketika terlihat gejala tidak normal, seperti suara yang berubah atau bau hangus, tanpa bantuan perangkat pengukur apa pun. Cara ini tidak memerlukan investasi perangkat tambahan, sehingga banyak dipakai pada industri kecil yang anggarannya terbatas. Industri kecil dan menengah pada umumnya memang belum memiliki sumber daya yang memadai untuk membangun infrastruktur pemantauan otomatis, sehingga pengawasan proses produksi masih banyak bergantung pada tenaga manusia [@li2022]. Pengamatan manual hanya berlangsung pada waktu tertentu, dan ketelitiannya bergantung penuh pada kondisi serta kewaspadaan operator saat itu. Perubahan suhu atau arus yang terjadi di luar waktu pengamatan dapat luput sepenuhnya, terutama ketika operator sedang menangani pekerjaan lain di luar mesin.

### Sistem Monitoring Berbasis Sensor

Sistem monitoring berbasis sensor memperbaiki keterbatasan pengamatan manual dengan memasang sensor suhu dan arus pada mesin. Data dari sensor dikirim melalui jaringan dan ditampilkan pada layar pemantauan, sehingga operator memperoleh angka yang terukur tanpa harus mengamati mesin secara fisik [@soori2023]. Pendekatan ini menutup celah waktu pengamatan yang jadi kelemahan pemeriksaan manual, karena data terus terbaca selama sensor terpasang. Namun, sistem semacam ini umumnya berhenti pada tahap menampilkan data. Tidak ada mekanisme yang memicu tindakan otomatis ketika nilai yang terbaca melampaui batas aman. Keputusan menghentikan mesin tetap sepenuhnya berada di tangan operator, sehingga proteksi terhadap kondisi berbahaya tetap bergantung pada kehadiran dan respons manusia pada saat kejadian.

### Sistem SCADA Industri Komersial

Sistem SCADA *(Supervisory Control and Data Acquisition)* buatan pabrikan industri menggabungkan kemampuan memantau dan mengendalikan mesin dalam satu platform terpadu. Sistem ini sudah teruji luas di lingkungan produksi berskala besar, dengan keandalan dan kelengkapan fitur yang tinggi. Solusi semacam ini pada dasarnya menutup kedua celah yang ditemukan pada dua pendekatan sebelumnya, karena mampu memantau sekaligus bertindak secara otomatis. Meski begitu, solusi SCADA komersial pada umumnya memiliki biaya perangkat dan integrasi yang tinggi serta cenderung kaku ketika perlu digabungkan dengan berbagai jenis mesin dan protokol komunikasi yang berbeda [@amin2025]. Biaya investasi awal ini yang menjadi kendala utama, khususnya bagi bengkel dan industri kecil yang anggarannya jauh lebih terbatas dibanding pabrik skala besar tempat SCADA umumnya diterapkan.

### Sistem Pengawasan Berbasis IoT dengan Mekanisme *Cutoff* Otomatis

Sistem pengawasan berbasis IoT dengan mekanisme *cutoff* otomatis memakai mikrokontroler yang terhubung ke sensor suhu dan arus pada mesin CNC sebagai pusat kendali. Data kondisi mesin dikirim ke server melalui protokol MQTT dan ditampilkan pada dashboard web, sehingga operator dapat memantau dari jarak jauh seperti pada sistem berbasis sensor. Berbeda dari sistem berbasis sensor, pendekatan ini dilengkapi mekanisme yang memutus daya Spindle secara otomatis ketika suhu atau arus melampaui ambang aman, tanpa menunggu tindakan operator. Komponen yang dipakai berupa mikrokontroler dan sensor pasaran umum, sehingga biaya keseluruhan jauh lebih rendah dibanding sistem SCADA komersial. Pendekatan ini meniru kemampuan inti SCADA, yaitu memantau sekaligus bertindak, dengan biaya yang jauh lebih terjangkau bagi bengkel dan industri kecil.

### Perbandingan Metode yang Sudah Ada dengan Solusi yang Diusulkan

Bagian ini menyoroti perbandingan antara ketiga pendekatan yang telah dipaparkan dengan solusi yang diusulkan, dengan menekankan empat aspek pembeda utama: kemampuan pemantauan, kemampuan bertindak otomatis, keterjangkauan biaya, dan keterbatasan yang melekat pada masing-masing pendekatan, sebagaimana dirangkum pada [@tbl:perbandingan-solusi].

| Aspek | Pemeriksaan Manual | Monitoring Berbasis Sensor | SCADA Industri Komersial | IoT dengan *Cutoff* Otomatis |
|---|---|---|---|---|
| Kemampuan Pemantauan | Hanya saat gejala kasat mata muncul | Data terukur berkelanjutan | Pemantauan otomatis terintegrasi | Pemantauan otomatis, interval maksimal 2 detik |
| Kemampuan Bertindak Otomatis | Tidak ada | Tidak ada | Ada | Ada, memutus daya Spindle otomatis |
| Keterjangkauan Biaya | Tidak ada biaya perangkat tambahan | Biaya sensor dan layar tampilan | Tinggi, mahal untuk industri kecil | Rendah, komponen mikrokontroler pasaran umum |
| Keterbatasan Utama | Bergantung penuh pada kewaspadaan operator | Tidak dapat bertindak saat kondisi berbahaya | Biaya di luar jangkauan industri kecil | Belum teruji pada skala produksi industri besar |

Table: Perbandingan Solusi Pengawasan Mesin CNC yang Sudah Ada {#tbl:perbandingan-solusi}

(Sumber: Diolah oleh Penulis)

Baris Kemampuan Pemantauan pada [@tbl:perbandingan-solusi] menunjukkan bahwa pemeriksaan manual satu-satunya pendekatan yang pemantauannya tidak berkelanjutan, sementara ketiga pendekatan lain sama-sama membaca kondisi mesin secara terus-menerus. Pada baris Kemampuan Bertindak Otomatis, polanya berbeda: pemeriksaan manual dan monitoring berbasis sensor sama-sama tidak dapat bertindak sendiri, sedangkan SCADA dan solusi yang diusulkan sama-sama dilengkapi tindakan otomatis. Baris Keterjangkauan Biaya justru membalik pola itu kembali. Solusi yang mampu bertindak otomatis (SCADA) berbiaya tinggi, sementara Sistem pengawasan berbasis IoT dengan mekanisme *cutoff* otomatis menempati posisi yang sebelumnya belum terisi, yaitu mampu bertindak otomatis dengan biaya yang tetap terjangkau.

## Tujuan Capstone

Penelitian ini ditujukan untuk merancang sistem pengawasan mesin CNC yang mampu membaca kondisi mesin sekaligus bertindak ketika kondisi tersebut membahayakan, tanpa menunggu reaksi operator. Tujuan tersebut diuraikan sebagai berikut.

1.  Merancang sistem pemantauan suhu dan arus listrik mesin CNC berbasis IoT sehingga kondisi mesin dapat diamati selama proses kerja berlangsung.

2.  Menerapkan mekanisme *cutoff* otomatis yang memutus daya Spindle ketika suhu atau arus melampaui batas aman, sehingga kerusakan dapat dicegah lebih awal.

3.  Menyediakan tampilan pemantauan yang menyajikan data suhu dan arus beserta riwayatnya, sehingga operator memperoleh informasi kondisi mesin tanpa mengamati mesin secara langsung.

4.  Mengurangi risiko kerusakan dan waktu henti mesin pada industri kecil melalui pengawasan yang tidak sepenuhnya bergantung pada kehadiran operator.

## Batasan Tugas Akhir

Batasan pada penelitian ini dijabarkan sebagai berikut.

1.  Sistem memantau parameter suhu dan arus listrik pada mesin CNC.

2.  Pemantauan suhu dilakukan pada dua titik, yaitu Spindle dan motor penggerak sumbu Z, sedangkan pemantauan arus dilakukan pada lima titik, yaitu motor penggerak sumbu X, Y1, Y2, Z, dan Spindle.

3.  Data kondisi mesin diperbarui pada interval maksimal dua detik.

4.  Mekanisme *cutoff* memutus daya fisik pada Spindle melalui relay, sedangkan motor penggerak sumbu X, Y, dan Z dihentikan melalui sinyal *E-Stop* ke firmware GRBL pada Arduino.

5.  Sistem menghentikan operasi mesin secara mandiri apabila komunikasi ke server terputus selama 60 detik.

6.  Komunikasi antara ESP32 dan server berjalan melalui jaringan WiFi lokal.

7.  Informasi kondisi mesin ditampilkan melalui dashboard web pada jaringan yang sama.

8.  Notifikasi peringatan jarak jauh dikirim secara otomatis ke aplikasi Telegram operator saat alarm terdeteksi atau koneksi terputus.

9.  Komponen perangkat keras yang digunakan tersedia di pasaran lokal.

10. Mesin CNC yang digunakan bekerja pada material kayu.

11. Pengujian mencakup pembacaan sensor, verifikasi ambang proteksi, waktu respons *cutoff*, mekanisme *fail-safe*, tampilan dashboard, dan notifikasi jarak jauh via Telegram.
