#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <MPU6050.h>

const char* ssid = "LUONGDATTHIEN";
const char* password = "123123123";
const char* serverName = "http://172.20.10.4:5000/data"; // Flask server nội bộ

MPU6050 mpu;

// Biến lưu dữ liệu cảm biến
float accelX = 0, accelY = 0, accelZ = 0;
float gyroX = 0, gyroY = 0, gyroZ = 0;

// Semaphore để tránh xung đột dữ liệu giữa loop và task
SemaphoreHandle_t dataMutex;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Wire.begin();
  mpu.initialize();

  // Chờ WiFi kết nối
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Tạo semaphore
  dataMutex = xSemaphoreCreateMutex();

  // Tạo task gửi dữ liệu song song
  xTaskCreatePinnedToCore(
    sendDataTask,      // tên hàm xử lý task
    "SendDataTask",    // tên task
    4096,              // stack size
    NULL,              // tham số truyền vào (không cần)
    1,                 // ưu tiên
    NULL,              // handle
    1                  // core 1 (ESP32 có 2 core: 0 và 1)
  );
}

void loop() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float aX = ax / 16384.0;
  float aY = ay / 16384.0;
  float aZ = az / 16384.0;
  float gX = gx / 131.0;
  float gY = gy / 131.0;
  float gZ = gz / 131.0;

  if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
    accelX = aX;
    accelY = aY;
    accelZ = aZ;
    gyroX = gX;
    gyroY = gY;
    gyroZ = gZ;
    xSemaphoreGive(dataMutex);
  }

  delay(5);
}

void sendDataTask(void* pvParameters) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      float aX, aY, aZ, gX, gY, gZ;

      // Lấy dữ liệu cảm biến từ biến toàn cục
      if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
        aX = accelX;
        aY = accelY;
        aZ = accelZ;
        gX = gyroX;
        gY = gyroY;
        gZ = gyroZ;
        xSemaphoreGive(dataMutex);
      }

      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "application/json");
      http.addHeader("Connection", "close");
      http.setTimeout(1000); // timeout ngắn

      // Tạo JSON để gửi
      String postData = "{";
      postData += "\"accelX\":" + String(aX, 3) + ",";
      postData += "\"accelY\":" + String(aY, 3) + ",";
      postData += "\"accelZ\":" + String(aZ, 3) + ",";
      postData += "\"gyroX\":" + String(gX, 3) + ",";
      postData += "\"gyroY\":" + String(gY, 3) + ",";
      postData += "\"gyroZ\":" + String(gZ, 3);
      postData += "}";

      // Gửi POST request
      unsigned long startTime = millis();
      int httpResponseCode = http.POST(postData);
      unsigned long elapsed = millis() - startTime;

      if (httpResponseCode > 0) {
        Serial.printf("POST OK (%d) in %lu ms\n", httpResponseCode, elapsed);
      } else {
        Serial.printf("POST ERROR (%d)\n", httpResponseCode);
      }

      http.end();
    } else {
      Serial.println("WiFi not connected");
    }

    vTaskDelay(40 / portTICK_PERIOD_MS); // Delay 40ms = 25Hz
  }
}
