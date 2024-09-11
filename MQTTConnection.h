#ifndef MQTTCONNECITON_H
#define MQTTCONNECITON_H
#include <WiFi.h>
#include <PubSubClient.h>

// 声明外部的 MQTT 客户端对象
extern PubSubClient mqttClient;

void initMQTT();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void publishMessage(const char* topic, const char* message);
String generateClientID();
#endif // MQTTMANAGER_H