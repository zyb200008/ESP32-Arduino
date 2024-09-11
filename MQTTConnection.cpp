
#include "MQTTConnection.h"
#include "WiFiConnection.h"  // 确保包含 WiFiConnection 以确保 WiFi 已连接
#include "AudioFunctions.h"
#include "Globals.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

const int mqttPort = 1883;    // 替换为你的MQTT服务器端口
const char* mqttServer = "";  //替换为你的服务器地址

void initMQTT() {
    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(mqttCallback);

}

void reconnectMQTT() {
    // Loop until we're reconnected
    
    String ESPID = generateClientID();
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (mqttClient.connect(ESPID.c_str())) {
            File file = SPIFFS.open("/config.txt", "r");
            if (!file) {
                Serial.println("Failed to open config file");
                startAPMode();
                return;
            }
            String ssid = file.readStringUntil('\n'); ssid.trim();
            String password = file.readStringUntil('\n'); password.trim();
            String mqttTopic = file.readStringUntil('\n'); mqttTopic.trim();
            mqttClient.subscribe(mqttTopic.c_str());
            Serial.println("connected");
            
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 2 seconds");
            num++;
            // Wait 2 seconds before retrying
            delay(2000);
        }
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message = "";

    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println();
    Serial.println(message);
    playNumberSequence(message);
}

void publishMessage(const char* topic, const char* message) {
    if (mqttClient.connected()) {
        mqttClient.publish(topic, message);
    } else {
        Serial.println("MQTT Client not connected");
    }
}
String generateClientID() {
    String clientId = "arduino-";
    clientId += String(ESP.getEfuseMac()); // 使用芯片ID
    clientId += String(random(0xffff), HEX); // 添加随机数
    return clientId;
}
