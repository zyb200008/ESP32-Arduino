#ifndef WIFICONNECTION_H
#define WIFICONNECTION_H
#include "WebServerManager.h" // 包含新的Web模块
#include "MQTTConnection.h" 

void initWiFi();
void reconnectWiFi();
void startAPMode();
void stopAPMode();
void handleWiFiReconnection();
void setWiFiConfigured(bool configured);
bool isWiFiConfigured();
void SpeakIP(String ipStr);
void error1();
#endif
