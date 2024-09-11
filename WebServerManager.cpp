#include "WebServerManager.h"
#include <HTTPClient.h>
#include <Update.h>
#include "Globals.h"
#include "AudioFunctions.h"

const char* firmwareUrl = "http://81.70.165.38/2024.bin";
volatile int updateProgress = 0;
volatile bool isUpdating = false;

WebServer server(80); // 创建Web服务器对象，监听80端口

void initWebServer() {
    // 配置处理程序
    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/volume/up", HTTP_GET, handleVolumeUp);
    server.on("/volume/down", HTTP_GET, handleVolumeDown);
    server.on("/ota", HTTP_GET, handleOTA);
    server.on("/progress", HTTP_GET, handleProgress);
    server.on("/restart", HTTP_GET, handleRestart);
    server.begin(); // 启动Web服务器
    Serial.println("Web server started on port 80");
}
void handleRoot() {
    String ssid = "";
    String password = "";
    String mqttTopic = "";

    File file = SPIFFS.open("/config.txt", "r");
    if (file) {
        ssid = file.readStringUntil('\n'); ssid.trim();
        password = file.readStringUntil('\n'); password.trim();
        mqttTopic = file.readStringUntil('\n'); mqttTopic.trim();
        file.close();
    }

    String html = "<!DOCTYPE html>"
                  "<html><head><meta charset='UTF-8'>"
                  "<style>"
                  "body { font-size: 2.28rem; display: flex; flex-direction: column; justify-content: center; align-items: center; height: 100vh; margin: 0; }"
                  "label { margin-bottom: 5px; }"
                  "input, button { margin: 5px; font-size: 2.28rem; padding: 20px; width: 360px; }"
                  "button { background-color: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; }"
                  "button:hover { background-color: #45a049; }"
                  "form { display: flex; flex-direction: column; align-items: center; }"
                  ".restart-button { background-color: #f44336; color: white; border: none; border-radius: 5px; cursor: pointer; }"
                  ".restart-button:hover { background-color: #d32f2f; }"
                  "</style>"
                  "</head><body>"
                  "<h1>馍馍哒播报音箱配置页面</h1>"
                  "<form action='/save' method='POST'>"
                  "<label for='ssid'>无线网名称:</label>"
                  "<input type='text' id='ssid' name='ssid' value='" + ssid + "' placeholder='只能连接2.4G频段!!'><br>"
                  "<label for='password'>无线网密码:</label>"
                  "<input type='password' id='password' name='password' value='" + password + "' placeholder='请输入无线网密码'><br>"
                  "<label for='mqttTopic'>商家用户名:</label>"
                  "<input type='text' id='mqttTopic' name='mqttTopic' value='" + mqttTopic + "' placeholder='例如xiaoming'><br>"
                  "<input type='submit' value='保存'>"
                  "</form><br>"
                  "<label for='but'>音量最大为3.9最小为0.1:</label>"
                  "<button onclick='changeVolume(\"up\")'>音量加</button>"
                  "<button onclick='changeVolume(\"down\")'>音量减</button>"
                  "<button class='restart-button' onclick='restartESP()'>重启</button>"
                  "<button onclick=\"startOTA()\">固件更新</button>"
                  "<p>更新进度: <span id='progress'>0</span>%</p>"
                  "<script>"
                  "function changeVolume(dir) {"
                  "  fetch('/volume/' + dir).then(response => response.text()).then(data => alert(data));"
                  "}"
                  "function restartESP() {"
                  "  fetch('/restart').then(response => response.text()).then(data => alert(data));"
                  "}"
                  "function startOTA() {"
                  "  fetch('/ota').then(response => response.text()).then(data => {"
                  "    console.log(data);"
                  "  });"
                  "  const interval = setInterval(() => {"
                  "    fetch('/progress').then(response => response.text()).then(progress => {"
                  "      document.getElementById('progress').innerText = progress;"
                  "      if (progress == '100') {"
                  "        clearInterval(interval);"
                  "      }"
                  "    });"
                  "  }, 1000);"
                  "}"
                  "</script>"
                  "</body></html>";

    server.send(200, "text/html", html);
}
void handleVolumeUp() {
    if (volumeGain < 4.0) {
        volumeGain += 0.5;
        if (volumeGain >= 4.0) volumeGain = 3.9;
        saveVolumeSetting(); // 保存音量设置
        
        playFile("/ja.wav");

    }
    server.send(200, "text/plain", "当前音量 " + String(volumeGain));
}

void handleVolumeDown() {
    if (volumeGain > 0.1) {
        volumeGain -= 0.5;
        if (volumeGain < 0.1) volumeGain = 0.1;
        saveVolumeSetting(); // 保存音量设置
        playFile("/jj.wav");
    }
    server.send(200, "text/plain", "当前音量 " + String(volumeGain));
}
void loadVolumeSetting() {
    File file = SPIFFS.open("/volume.txt", "r");
    if (!file) {
        Serial.println("Failed to open volume file for reading");
        volumeGain = 1.0;  // 设置默认音量
        return;
    }
    
    if (file.available()) {
        volumeGain = file.parseFloat();
        // 检查解析后的值是否有效，如果无效则设置为默认值
        if (volumeGain == 0 && file.peek() == -1) {
            volumeGain = 1.0;
        }
    } else {
        volumeGain = 1.0;  // 文件为空，设置默认音量
    }
    
    file.close();
    Serial.println("Volume setting loaded.");
    Serial.print("Volume gain: ");
    Serial.println(volumeGain);
}
void saveVolumeSetting() {
    File file = SPIFFS.open("/volume.txt", "w");
    if (!file) {
        Serial.println("Failed to open volume file for writing");
        return;
    }
    file.println(volumeGain);
    file.close();
    Serial.println("Volume setting saved.");
}

void handleRestart() {
    server.send(200, "text/plain", "正在重启...");
    delay(1000);
    ESP.restart();
}

void handleSave() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    String mqttTopic = server.arg("mqttTopic");

    if (ssid.length() > 0 && password.length() > 0) {
        File file = SPIFFS.open("/config.txt", "w");
        if (file) {
            file.println(ssid);
            file.println(password);
            file.println(mqttTopic);
            file.close();
            server.send(200, "text/plain", "OK!");
            delay(3000);
            ESP.restart();  // 重启ESP32
        } else {
            server.send(500, "text/plain", "Failed to save configuration");
        }
    } else {
        server.send(400, "text/plain", "Invalid parameters");
    }
}

// Handle the path "/ota"
void handleOTA() {
  if (!isUpdating) {
    isUpdating = true;
    server.send(200, "text/plain", "OTA update started.");
    xTaskCreate(
      [] (void * parameter) -> void {
        checkForUpdate();
        isUpdating = false;
        vTaskDelete(NULL);
      },
      "OTA",
      8192,
      NULL,
      1,
      NULL
    );
  } else {
    server.send(200, "text/plain", "OTA update already in progress.");
  }
}
// Serve OTA progress
void handleProgress() {
  xSemaphoreTake(updateMutex, portMAX_DELAY);
  int progress = updateProgress;
  xSemaphoreGive(updateMutex);
  server.send(200, "text/plain", String(progress));
}
void checkForUpdate() {
  HTTPClient http;
  http.begin(firmwareUrl);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    if (contentLength > 0) {
      bool canBegin = Update.begin(contentLength);

      if (canBegin) {
        Serial.println("Begin OTA update");
        WiFiClient *client = http.getStreamPtr();
        size_t written = 0;
        const int chunkSize = 1024;

        while (written < contentLength) {
          size_t len = client->available();
          if (len > chunkSize) {
            len = chunkSize;
          }
          uint8_t buf[chunkSize];
          client->readBytes(buf, len);
          written += Update.write(buf, len);

          xSemaphoreTake(updateMutex, portMAX_DELAY);
          updateProgress = (written * 100) / contentLength;
          xSemaphoreGive(updateMutex);

          delay(100);
        }

        if (written == contentLength) {
          Serial.println("Written : " + String(written) + " successfully");
        } else {
          Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
        }

        if (Update.end()) {
          Serial.println("OTA done!");
          if (Update.isFinished()) {
            Serial.println("Update successfully completed. Rebooting.");
            xSemaphoreTake(updateMutex, portMAX_DELAY);
            updateProgress = 100;
            xSemaphoreGive(updateMutex);
            delay(2000); // Wait for the client to see 100% progress
            ESP.restart();
          } else {
            Serial.println("Update not finished? Something went wrong!");
          }
        } else {
          Serial.println("Error Occurred. Error #: " + String(Update.getError()));
        }
      } else {
        Serial.println("Not enough space to begin OTA");
      }
    } else {
      Serial.println("Content-Length is not defined or zero");
    }
  } else {
    Serial.println("HTTP Error code: " + String(httpCode));
  }
  http.end();
}
void stopServer() {
  server.stop();
  Serial.println("Web server stopped");
}