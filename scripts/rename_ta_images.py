import os
import re

MEDIA_DIR = r"D:\Github\CNCProject\CNCTA\TA-Rombak\assets\media"
TA_DIR = r"D:\Github\CNCProject\CNCTA\TA-Rombak"

RENAME_MAP = {
    # Bab 2
    "image5.png": "fig_mesin_cnc.png",
    "image6.jpeg": "fig_esp32_devkit.jpeg",
    "image7.jpeg": "fig_acs712.jpeg",
    "image8.jpeg": "fig_ds18b20.jpeg",
    "image9.jpeg": "fig_relay.jpeg",
    "image60.jpeg": "fig_bc547.jpeg",
    # Bab 3
    "image10.png": "fig_arsitektur_sistem.png",
    "image11.png": "fig_blok_mesin_cnc.png",
    "image12.png": "fig_blok_iot_device.png",
    "image13.png": "fig_blok_server.png",
    "image14.png": "fig_skema_rangkaian.png",
    # Bab 4
    "image22.png": "fig_topologi_sistem.png",
    "image23.png": "fig_perakitan_box_1.png",
    "image24.png": "fig_perakitan_box_2.png",
    "image25.png": "fig_perakitan_box_3.png",
    "image26.png": "fig_perakitan_box_4.png",
    "image27.png": "fig_perakitan_box_5.png",
    "image28.png": "fig_perakitan_box_6.png",
    "image29.png": "fig_integrasi_cnc.png",
    "image30.png": "fig_kartu_arus.png",
    "image31.png": "fig_kartu_suhu.png",
    "image32.png": "fig_tombol_relay_aktif.png",
    "image33.png": "fig_tombol_relay_trip.png",
    "image34.png": "fig_grafik_tren_dashboard.png",
    "image45.png": "fig_panel_uji_injeksi.png",
    "image36.png": "fig_panel_log_dashboard.png",
    "image37.png": "fig_ekstensi_platformio.png",
    "image38.png": "fig_konfigurasi_credentials.png",
    "image39.png": "fig_build_upload_firmware.png",
    "image40.png": "fig_konfirmasi_koneksi.png",
    "image41.png": "fig_konfigurasi_env.png",
    "image42.png": "fig_server_dijalankan.png",
    "image43.png": "fig_layanan_systemd.png",
    "image44.png": "fig_status_systemd.png",
    "image46.png": "fig_status_mqtt_broker.png",
    # Bab 5
    "image47.png": "fig_skema_bench_test_arus.png",
    "image48.jpeg": "fig_perakitan_bench_test.jpeg",
    "image49.png": "fig_overlay_arus_spindle.png",
    "image50.png": "fig_overlay_arus_stepper_x.png",
    "image51.png": "fig_overlay_arus_stepper_y1.png",
    "image52.png": "fig_overlay_arus_stepper_y2.png",
    "image53.png": "fig_overlay_arus_stepper_z.png",
    "image54.png": "fig_overlay_suhu_spindle.png",
    "image55.png": "fig_overlay_suhu_stepper_z.png",
    "image56.png": "fig_interval_periode_pemantauan.png",
    "image57.png": "fig_tren_arus_idle.png",
    "image58.png": "fig_tren_arus_suhu_pemesinan.png",
    "image61.jpeg": "fig_bukti_notifikasi_telegram.jpeg",
    # Lampiran
    "image59.jpeg": "fig_bimbingan_dosen.jpeg",
}

def main():
    print("--- STEP 1: Renaming physical image files ---")
    renamed_count = 0
    for old_name, new_name in RENAME_MAP.items():
        old_path = os.path.join(MEDIA_DIR, old_name)
        new_path = os.path.join(MEDIA_DIR, new_name)
        if os.path.exists(old_path):
            os.rename(old_path, new_path)
            print(f"Renamed: {old_name} -> {new_name}")
            renamed_count += 1
        elif os.path.exists(new_path):
            print(f"Already exists: {new_name}")
        else:
            print(f"WARNING: File not found: {old_name}")

    print(f"Physical files renamed: {renamed_count}/{len(RENAME_MAP)}")

    print("\n--- STEP 2: Updating image references in Markdown files ---")
    md_files = [f for f in os.listdir(TA_DIR) if f.endswith(".md")]
    updated_files = 0
    for md_file in md_files:
        md_path = os.path.join(TA_DIR, md_file)
        with open(md_path, "r", encoding="utf-8") as f:
            content = f.read()

        new_content = content
        replacements_in_file = 0
        for old_name, new_name in RENAME_MAP.items():
            if old_name in new_content:
                new_content = new_content.replace(old_name, new_name)
                replacements_in_file += 1

        if replacements_in_file > 0:
            with open(md_path, "w", encoding="utf-8") as f:
                f.write(new_content)
            print(f"Updated {md_file}: {replacements_in_file} image reference(s) replaced.")
            updated_files += 1

    print(f"Markdown files updated: {updated_files}")

if __name__ == "__main__":
    main()
