#include <Wire.h>
#include <WiFi.h>
#include <MPU6050.h>
#include <HTTPClient.h>
#include <ESP32_MailClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "fall_detection_model.h"  // model_data[]

#include <TensorFlowLite_ESP32.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>

MPU6050 mpu;

// WiFi
const char* ssid = "LUONGDATTHIEN";
const char* password = "123123123";
const char* serverName = "http://172.20.10.4:5000/result";

// Email
const char* smtp_server = "smtp.gmail.com";
const int smtp_port = 465;
const char* email_sender = "leepro0611@gmail.com";
const char* email_password = "sqlm bhsv najb kylm";
const char* email_recipient = "datthien0611@gmail.com";

ESP32_MailClient mailClient;

// TensorFlow
const int kTensorArenaSize = 10 * 1024;
uint8_t tensor_arena[kTensorArenaSize] __attribute__((aligned(16)));

const tflite::Model* model;
tflite::MicroInterpreter* interpreter;
TfLiteTensor* input;
TfLiteTensor* output;
static tflite::MicroErrorReporter micro_error_reporter;

const char* labels[] = { "falling", "normal", "walking" };
float data[25][6];
int sample_index = 0;
const int num_samples = 25;

// WebSocket
AsyncWebServer server(80);
AsyncWebSocket asyncWs("/ws");

struct TaskParam {
  const char* label;
  float confidence;
};

// Gửi email
void sendEmail(const char* subject, const char* message) {
  SMTPData smtpData;
  smtpData.setLogin(smtp_server, smtp_port, email_sender, email_password);
  smtpData.setSender("ESP32", email_sender);
  smtpData.setPriority("High");
  smtpData.setSubject(subject);
  smtpData.setMessage(message, true);
  smtpData.addRecipient(email_recipient);

  if (!mailClient.sendMail(smtpData)) {
    Serial.println("Gửi email thất bại");
  } else {
    Serial.println("Gửi email thành công");
  }
  smtpData.empty();
}

// Task gửi email
void sendEmailTask(void* parameter) {
  TaskParam* param = (TaskParam*)parameter;
  char subject[64];
  char body[256];

  sprintf(subject, "[CẢNH BÁO] Phát hiện ngã!");
  sprintf(body, "Trạng thái: %s\nĐộ tin cậy: %.2f%%", param->label, param->confidence * 100);

  sendEmail(subject, body);
  free(param);
  vTaskDelete(NULL);
}

// Task gửi HTTP
void sendHttpTask(void* parameter) {
  TaskParam* param = (TaskParam*)parameter;

  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/json");

  String json = "{\"label\":\"" + String(param->label) + "\",\"confidence\":" + String(param->confidence) + "}";
  int code = http.POST(json);
  Serial.print("HTTP response: ");
  Serial.println(code);
  http.end();

  free(param);
  vTaskDelete(NULL);
}

// WebSocket xử lý client
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  }
}

// Gửi dữ liệu qua WebSocket
void sendSensorData(float ax, float ay, float az, float gx, float gy, float gz, const char* label, float confidence) {
  DynamicJsonDocument doc(256);
  doc["ax"] = ax;
  doc["ay"] = ay;
  doc["az"] = az;
  doc["gx"] = gx;
  doc["gy"] = gy;
  doc["gz"] = gz;
  doc["label"] = label;
  doc["confidence"] = confidence;
  doc["system_status"] = WiFi.status() == WL_CONNECTED ? "Đang hoạt động" : "Ngắt kết nối";
  doc["uptime"] = millis() / 1000;
  doc["battery_level"] = random(80, 100); // Giả lập mức pin
  doc["sample_rate"] = 25; // 25Hz (40ms/sample)
  doc["connection_status"] = WiFi.status() == WL_CONNECTED ? "Kết nối" : "Mất kết nối";

  String json;
  serializeJson(doc, json);
  asyncWs.textAll(json);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  mpu.initialize();

  // WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // WebSocket setup
  asyncWs.onEvent(onWebSocketEvent);
  server.addHandler(&asyncWs);
  server.begin();
  Serial.println("WebSocket server started");

  // TensorFlow setup
  model = tflite::GetModel(model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model version mismatch!");
    while (1);
  }

  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize, &micro_error_reporter);
  interpreter = &static_interpreter;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("Tensor allocation failed");
    while (1);
  }

  input = interpreter->input(0);
  output = interpreter->output(0);
}

void loop() {
  asyncWs.cleanupClients(); // Dọn dẹp client ngắt kết nối

  if (sample_index < num_samples) {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    data[sample_index][0] = ax / 16384.0;
    data[sample_index][1] = ay / 16384.0;
    data[sample_index][2] = az / 16384.0;
    data[sample_index][3] = gx / 131.0;
    data[sample_index][4] = gy / 131.0;
    data[sample_index][5] = gz / 131.0;

    sample_index++;
    delay(40);
  }

  if (sample_index >= num_samples) {
    // Copy vào input tensor
    for (int i = 0; i < num_samples; i++) {
      for (int j = 0; j < 6; j++) {
        input->data.f[i * 6 + j] = data[i][j];
      }
    }

    if (interpreter->Invoke() != kTfLiteOk) {
      Serial.println("Lỗi khi chạy model");
      return;
    }

    int label = -1;
    float max_score = -1;
    for (int i = 0; i < output->dims->data[1]; i++) {
      float score = output->data.f[i];
      if (score > max_score) {
        max_score = score;
        label = i;
      }
    }

    const char* label_text = labels[label];
    Serial.print("Kết quả dự đoán: ");
    Serial.print(label_text);
    Serial.print(" (Confidence: ");
    Serial.print(max_score, 4);
    Serial.println(")");

    // Gửi dữ liệu qua WebSocket
    float ax_avg = 0, ay_avg = 0, az_avg = 0, gx_avg = 0, gy_avg = 0, gz_avg = 0;
    for (int i = 0; i < num_samples; i++) {
      ax_avg += data[i][0]; ay_avg += data[i][1]; az_avg += data[i][2];
      gx_avg += data[i][3]; gy_avg += data[i][4]; gz_avg += data[i][5];
    }
    ax_avg /= num_samples; ay_avg /= num_samples; az_avg /= num_samples;
    gx_avg /= num_samples; gy_avg /= num_samples; gz_avg /= num_samples;

    sendSensorData(ax_avg, ay_avg, az_avg, gx_avg, gy_avg, gz_avg, label_text, max_score);

    // Gửi HTTP
    if (WiFi.status() == WL_CONNECTED) {
      TaskParam* httpParam = (TaskParam*)malloc(sizeof(TaskParam));
      httpParam->label = label_text;
      httpParam->confidence = max_score;
      xTaskCreatePinnedToCore(sendHttpTask, "HttpTask", 8000, httpParam, 1, NULL, 1);
    }

    // Chỉ gửi email nếu là "falling"
    if (strcmp(label_text, "falling") == 0 && WiFi.status() == WL_CONNECTED) {
      TaskParam* emailParam = (TaskParam*)malloc(sizeof(TaskParam));
      emailParam->label = label_text;
      emailParam->confidence = max_score;
      xTaskCreatePinnedToCore(sendEmailTask, "EmailTask", 10000, emailParam, 1, NULL, 1);
    }

    sample_index = 0;
  }
}