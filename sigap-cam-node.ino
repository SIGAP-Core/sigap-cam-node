#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// 1. PANGGIL FILE ENV DI SINI
#include "env.h"

// --- PINOUT KAMERA (Dipersingkat agar rapi) ---
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// Client khusus MQTT
WiFiClientSecure mqtt_net;
PubSubClient mqttClient(mqtt_net);

// Deklarasi fungsi potret
void sendPhotoToServer(); 

// Callback MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) { message += (char)payload[i]; }
  
  // 2. topic_request = ENV_MQTT_TOPIC_REQUEST
  if (String(topic) == ENV_MQTT_TOPIC_REQUEST && message == "TAKE_PHOTO") {
    Serial.println("🎯 Perintah dari HiveMQ Diterima! Mulai memotret...");
    sendDebugLog("🎯 ESP32 Cam: Perintah Diterima! Memulai proses potret...");
    sendPhotoToServer();
  }
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("🔄 Menghubungkan Kamera ke HiveMQ Cloud...");
    String clientId = "ESP32CAM-Kamera-";
    clientId += String(random(0xffff), HEX);
    
    // 3. mqtt_user & mqtt_password = VARIABEL ENV
    if (mqttClient.connect(clientId.c_str(), ENV_MQTT_USER, ENV_MQTT_PASSWORD)) {
      Serial.println("✅ Terhubung!");
      // 4. topic_request = TOPIC REQUEST
      mqttClient.subscribe(ENV_MQTT_TOPIC_REQUEST);
    } else {
      Serial.print("❌ Gagal, rc=");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

void sendDebugLog(String logMessage) {
  if (mqttClient.connected()) {
    mqttClient.publish(ENV_MQTT_TOPIC_UI_STATE, logMessage.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  
  // Konfigurasi Kamera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM; config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM; config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM; config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM; config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM; config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000; config.pixel_format = PIXFORMAT_JPEG;
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA; 
    config.jpeg_quality = 10; config.fb_count = 1; 
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12; config.fb_count = 1;
  }

  esp_camera_init(&config);

  // ROTASI 180 DERAJAT
  sensor_t * s = esp_camera_sensor_get();
  // Memutar 180 derajat = Flip Vertikal + Mirror Horizontal
  s->set_vflip(s, 1);    // 1 = Aktifkan flip vertikal
  s->set_hmirror(s, 1);  // 1 = Aktifkan mirror horizontal

  // 5. SSID & PASSWORD = WIFI_SSID & WIFI_PASSWORD
  WiFi.begin(ENV_WIFI_SSID, ENV_WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n✅ WiFi Terhubung!");

  mqtt_net.setInsecure(); // Bypass SSL untuk MQTT
  
  // 6. MQTT SERVER & PORT
  mqttClient.setServer(ENV_MQTT_SERVER, ENV_MQTT_PORT);
  mqttClient.setCallback(callback);
}

void loop() {
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();
}

void sendPhotoToServer() {
  //// 1. TRIK HEMAT RAM: Putus koneksi MQTT sementara!
  Serial.println("🧹 Membebaskan memori: Memutus koneksi MQTT sementara...");
  sendDebugLog("🧹 ESP32 Cam: Memutus koneksi MQTT sementara untuk alokasi RAM...");
  mqttClient.disconnect();
  mqtt_net.stop();
  delay(500); // Memberi waktu 0.5 detik agar RAM dan antena WiFi stabil

  // --- FLUSHING FRAME LAMA ---
  // Kita "pancing" kamera untuk mengambil 2 foto secara diam-diam
  // lalu langsung kita buang/kembalikan ke memori (tidak dikirim)
  camera_fb_t * fb = NULL;
  for (int i = 0; i < 2; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
  }

  //// 2. Ambil Gambar
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Gagal mengambil gambar");
    return;
  }

  Serial.println("📸 Memotret gambar REAL-TIME... Mengirim ke Flask/Ngrok...");

  //// 3. Buka koneksi ke Ngrok
  WiFiClientSecure https_client;
  https_client.setInsecure(); 

  // Tambahan: Set timeout lebih panjang jika internet sedang lambat
  https_client.setTimeout(10000);

  // 7. HOST & PORT NGROK
  if (!https_client.connect(ENV_SERVER_HOST.c_str(), ENV_SERVER_PORT)) {
    Serial.println("❌ Gagal terhubung ke Ngrok");
    esp_camera_fb_return(fb);
    return;
  }

  //// 4. Format Pengiriman
  String boundary = "ESP32Boundary12345";
  String head = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"image\"; filename=\"esp32_capture.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";
  uint32_t totalLen = head.length() + fb->len + tail.length();

  https_client.println("POST /upload HTTP/1.1");
  // 8. HOST NGROK
  https_client.println("Host: " + ENV_SERVER_HOST);
  https_client.println("Content-Length: " + String(totalLen));
  https_client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  https_client.println("Connection: close"); 
  https_client.println(); 

  https_client.print(head);

  uint8_t *fbBuf = fb->buf;
  size_t fbLen = fb->len;
  for (size_t n = 0; n < fbLen; n = n + 1024) {
    if (n + 1024 < fbLen) {
      https_client.write(fbBuf, 1024);
      fbBuf += 1024;
    } else if (fbLen % 1024 > 0) {
      size_t remainder = fbLen % 1024;
      https_client.write(fbBuf, remainder);
    }
  }

  https_client.print(tail);

  String response = "";
  long startTime = millis();
  while (millis() - startTime < 5000) {
    while (https_client.available()) {
      response += (char)https_client.read();
    }
  }
  
  int bodyPos = response.indexOf("\r\n\r\n");
  if(bodyPos != -1) {
      Serial.println("✅ Respons Server Python: " + response.substring(bodyPos + 4));
  } else {
      Serial.println("✅ Respons Server Python: " + response);
  }

  //// 6. Membersihkan Memori
  https_client.stop();
  esp_camera_fb_return(fb);
  
  Serial.println("🔄 Pengiriman selesai. Mengembalikan koneksi MQTT...");
  // Setelah ini fungsi selesai, dan loop() akan otomatis memanggil reconnectMQTT()

  reconnectMQTT();

  sendDebugLog("✅ ESP32 Cam: Foto berhasil dikirim ke Endpoint Ngrok! Menunggu AI..."); 
}