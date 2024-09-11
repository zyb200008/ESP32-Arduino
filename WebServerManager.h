#ifndef WEBSERVERMODULE_H
#define WEBSERVERMODULE_H

#include <WebServer.h>
#include <SPIFFS.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern WebServer server; // 声明 server 为外部变量
extern SemaphoreHandle_t updateMutex;


void initWebServer();
void handleRoot();
void handleSave();
void handleOTA();
void handleProgress();
void checkForUpdate();
void saveVolumeSetting();
void handleVolumeUp();
void handleVolumeDown();
void handleRestart();
void loadVolumeSetting();
void stopServer();
#endif