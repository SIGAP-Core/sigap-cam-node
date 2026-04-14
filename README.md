# 📸 SIGAP Cam Node

[![C++](https://img.shields.io/badge/C++-00599C?logo=c%2B%2B&logoColor=white)](https://isocpp.org/)
[![Arduino](https://img.shields.io/badge/Arduino-00979D?logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![ESP32-CAM](https://img.shields.io/badge/Hardware-ESP32--CAM-red)](#)
[![HTTP/MQTT](https://img.shields.io/badge/Network-MQTT%20%7C%20HTTP-00557E)](#)

**SIGAP Cam Node** - Modul kamera IoT berbasis ESP32-CAM yang berfungsi sebagai "Mata" dari sistem **SIGAP (Sistem Integrasi Gerbang & Akses Pintar)**.

## Fitur Utama

- **MQTT Listener**: Bersiap siaga (standby) mendengarkan perintah `TAKE_PHOTO` dari Gate Controller via HiveMQ Cloud.
- **Image Capture**: Memotret kendaraan dengan format JPEG menggunakan modul kamera OV2640.
- **HTTP Multipart Upload**: Mengirimkan gambar (foto fisik) secara langsung ke API Python/Flask (Ngrok) menggunakan protokol `multipart/form-data`.
- **Memory Management**: Menggunakan trik pemutusan sesi MQTT sementara (_flushing_) untuk menjaga kestabilan RAM ESP32-CAM saat mengirim file besar.

## Instalasi & Persiapan

1. Buka file `sigap-cam-node.ino` menggunakan **Arduino IDE**.
2. Set Board di Arduino IDE ke **AI Thinker ESP32-CAM**.
3. Pastikan library `PubSubClient` sudah terinstal. (Library kamera `esp_camera` sudah bawaan dari core ESP32).
4. Gunakan modul FTDI (USB to TTL) untuk melakukan _flashing_ program ke ESP32-CAM.

## Konfigurasi Keamanan (`env.h`)

Proyek ini memisahkan kredensial jaringan dan URL API ke dalam file header `env.h`.

> [!IMPORTANT]
> File `env.h` **DIKECUALIKAN** dari repositori ini untuk mencegah kebocoran _API endpoint_ dan _password_ WiFi.

**Cara Menggunakan:**
Hubungi **Pemilik Proyek (Owner)** untuk mendapatkan file `env.h` yang valid. Tempatkan file tersebut dalam satu direktori dengan file `.ino`.

File `env.h` tersebut memuat variabel berikut:

- `ENV_WIFI_SSID` & `ENV_WIFI_PASSWORD`
- `ENV_SERVER_HOST` & `ENV_SERVER_PORT` (URL Flask/Ngrok)
- `ENV_MQTT_SERVER`, `ENV_MQTT_PORT`, `ENV_MQTT_USER`, `ENV_MQTT_PASSWORD`
- `ENV_MQTT_TOPIC_REQUEST`

---

_Developed with 💡 by the SIGAP-Core Team._
