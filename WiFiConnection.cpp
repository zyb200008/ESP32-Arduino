#include "WiFiConnection.h"
#include <WiFi.h>
#include <SPIFFS.h>
#include "AudioFunctions.h"
#include "Globals.h"

const char* apSSID = "ESP32-AP";
const char* apPassword = "";

unsigned long lastReconnectAttempt = 0;
const unsigned long quickReconnectInterval = 3000; 
const unsigned long slowReconnectInterval = 5000; // 5秒
bool isInSlowReconnectMode = false;
int quickReconnectAttempts = 0;
const int maxQuickAttempts = 2; // 连接失败两次后进入AP模式
bool wifiConfigured = false;
String UserSSID = "";
String UserPWD = "";
int Maxreturncount = 0;
bool flag = false;
bool cflag = true;
int num = 0;
void initWiFi() {
    isWiFiConfigured();
    File file = SPIFFS.open("/config.txt", "r");

    if (!file) {
        Serial.println("Config file not found. Starting AP mode.");
        startAPMode();
        error1();
        return;
    }
    String ssid = file.readStringUntil('\n'); ssid.trim();
    String password = file.readStringUntil('\n'); password.trim();
    file.close();

    if (ssid.isEmpty() || password.isEmpty()) {
        Serial.println("SSID or password is empty. Starting AP mode.");
        startAPMode();
        error1();
        cflag = !cflag;
        return;
    }
    UserSSID = ssid;
    UserPWD = password;
    WiFi.begin(ssid.c_str(), password.c_str());
    int counter = 0;
    int maxAttempts = 60;
    while (WiFi.status() != WL_CONNECTED && counter < maxAttempts) {
        Serial.print(".");
        delay(500);
        counter++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        stopAPMode();
        Serial.println("Connected to WiFi");
        if(!wifiConfigured){
          setWiFiConfigured(true); // 标记WiFi配置成功
        }
        if(!wifiConfigured){
          SpeakIP(WiFi.localIP().toString());
        }
    } else {
        if (wifiConfigured) {
            // 曾经成功连接过，进入无限重连模式
            isInSlowReconnectMode = true;
        } else {
            startAPMode();
            error1();
            cflag = !cflag;
        }
    }
}

void reconnectWiFi() {
    WiFi.begin(UserSSID.c_str(), UserPWD.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 60) {
        Serial.print("1");
        delay(500);
        attempts++;
    }
    Serial.println("");
    Maxreturncount++;
    if(Maxreturncount >= 5){
      ESP.restart();
    }
    if (WiFi.status() == WL_CONNECTED) {
        stopAPMode();
        Serial.println("Reconnected to WiFi");
        quickReconnectAttempts = 0; // 重置快速重连尝试次数
        if(!wifiConfigured){
          SpeakIP(WiFi.localIP().toString());
        }
    } else {
        if (++quickReconnectAttempts >= maxQuickAttempts && !wifiConfigured) {
            isInSlowReconnectMode = true;
            startAPMode();
            error1();
            delay(60000); // 等待1分钟
            isInSlowReconnectMode = false; // 重置慢速重连模式
        }
    }
}

void startAPMode() {
    WiFi.softAP(apSSID, apPassword);
    Serial.println("Entered AP mode");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    initWebServer(); // 启动Web服务器

}

void stopAPMode() {
    if (WiFi.getMode() & WIFI_AP) {
        WiFi.softAPdisconnect(true);
        Serial.println("AP mode stopped");
    }
}

void handleWiFiReconnection() {
    unsigned long currentMillis = millis();
    unsigned long reconnectInterval = isInSlowReconnectMode ? slowReconnectInterval : quickReconnectInterval;
    if(cflag){
      if (WiFi.status() != WL_CONNECTED && currentMillis - lastReconnectAttempt >= reconnectInterval) {
          WiFi.disconnect(true);
          lastReconnectAttempt = currentMillis;
          reconnectWiFi();
      }else{
          if(currentMillis < 300000 && !flag){
            server.handleClient(); // 处理客户端请求
          }else{
            if(!flag){
              stopServer();
              flag = !flag;
            }
          }
          if (!mqttClient.connected()) {
            if(num < 30){
              reconnectMQTT();
            }else{
              WiFi.disconnect(true);
              num = 0;
            }
              
          }
          mqttClient.loop(); // 处理 MQTT 客户端
      }
    }else{
      server.handleClient(); //对于AP模式需开启
    }

}

void setWiFiConfigured(bool configured) {
    wifiConfigured = configured;
    File file = SPIFFS.open("/wifi_flag.txt", "w");
    if (file) {
        file.print(configured ? "1" : "0");
        file.close();
    }
}

bool isWiFiConfigured() {
    if (!wifiConfigured) {
        File file = SPIFFS.open("/wifi_flag.txt", "r");
        if (file) {
            String flag = file.readStringUntil('\n'); flag.trim();
            file.close();
            wifiConfigured = (flag == "1");
        }
    }
    return wifiConfigured;
}
void SpeakIP(String ipStr){
  for (int i = 0; i < ipStr.length(); i++) {
      Serial.println(ipStr[i]);
      if(ipStr[i] == '.'){
          playFile("/d.wav");
      }else{
          playDigit(ipStr[i] - '0'); // 逐字符播放音频
      }
  }
  initWebServer(); // 启动Web服务器
}
void error1(){
  playFile("/error2.wav");
}