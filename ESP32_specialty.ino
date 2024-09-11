#include <WiFi.h>
#include <SPIFFS.h>
#include "WiFiConnection.h"
#include "WebServerManager.h" 
#include "MQTTConnection.h" 
#include "Globals.h"
//外部模块
#include <FS.h>
#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"
AudioGeneratorWAV *wav = nullptr;
AudioFileSourceSPIFFS *file = nullptr;
AudioOutputI2S *out = nullptr; // 使用带有 DAC 的 I2S 输出
float volumeGain = 1.0;  // Default gain/volume level

SemaphoreHandle_t updateMutex;

void setup() {
    Serial.begin(115200);
    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    out = new AudioOutputI2S(); // 使用带有 DAC 的 I2S 输出
    loadVolumeSetting();
    initWiFi();
    initMQTT();
    out->SetGain(volumeGain); // 设置音量
    updateMutex = xSemaphoreCreateMutex();

}

void loop() {
    handleWiFiReconnection();
    delay(20);
    // 其他业务逻辑
}




void playFile(const char *filename) {
  if (wav != nullptr && wav->isRunning()) {
    // 如果有正在运行的音频，立即停止并清理资源
    wav->stop();
    delete wav;
    delete file;
  }

  // 设置音频源和生成器
  file = new AudioFileSourceSPIFFS(filename);
  wav = new AudioGeneratorWAV();

  // 尝试开始播放
  if (!wav->begin(file, out)) {
    Serial.println("Failed to begin WAV");
    while (1) delay(10); // 持续延迟以指示错误
  }

  // 设置正常音量
  out->SetGain(volumeGain);

  // 播放文件，直到结束
  while (wav->isRunning()) {
    wav->loop(); // 确保循环以保持播放
  }
}
void playNumberSequence(String numStr) {
    playFile("/mr.wav"); // 开始播放音频的标志
    int dotIndex = numStr.indexOf('.');
    String intPart = numStr.substring(0, dotIndex);
    String decimalPart = numStr.substring(dotIndex + 1);

    // 处理整数部分
    int len = intPart.length();
    if (len == 2 && intPart.charAt(0) != '1') { // 单独处理20到99之间的数字
        int tenDigit = intPart.charAt(0) - '0'; // 十位数字
        int oneDigit = intPart.charAt(1) - '0'; // 个位数字
        playDigit(tenDigit);
        playFile("/10.wav"); // 播放“十”的音频
        if (oneDigit > 0) { // 如果个位数不为0，则播放个位数的音频
            playDigit(oneDigit);
        }
    } else {
        if (len > 0) {
          boolean allZeros = true; // 检查整数部分是否全为0
          for (int i = 0; i < len; i++) {
              int digit = intPart.charAt(i) - '0';
              if (digit != 0) allZeros = false; // 如果存在非零数字，则整数部分非全0

              // 特殊处理10到19之间的数字
              if (len == 2 && intPart.charAt(0) == '1' && i == 0) {
                  playFile("/10.wav");
                  if (digit > 0) { // 对于11到19的情况，播放个位数字
                      continue; // 跳过这次循环，因为十位数已经处理
                  }
              }

              // 读出每一位数字及其单位
              if (digit > 0 || (digit == 0 && i < len - 1 && intPart.charAt(i + 1) != '0')) { // 修正逻辑，避免末尾是0的情况被错误处理
                  playDigit(digit);
              }

              // 确定并播放单位
              if (i < len - 1) { // 除了个位数，其它位都有可能有单位
                  int remainLen = len - i - 1; // 剩余长度决定单位
                  switch (remainLen) {
                      case 1: // 十位
                          if (digit != 0 && len != 2) { // 避免重复读出十位数，且不在10到19之间
                              playFile("/10.wav");
                          }
                          break;
                      case 2: // 百位
                          if(digit != 0){
                            playFile("/b.wav");
                            break;
                          }
                      case 3: // 千位
                          if(digit != 0){
                            playFile("/q.wav");
                            break;
                          }
                      case 4: // 万位
                          if(digit != 0){
                            playFile("/w.wav");
                            break;
                          }
                      // 根据需要添加更多的单位处理
                  }
              }
          }
          if (allZeros) playFile("/0.wav"); // 整数部分全为0
       } else {
          // 整数部分为0
          playFile("/0.wav");
      }
  }


    // 处理小数部分
    if (!decimalPart.equals("0") && !decimalPart.equals("00")) {
        playFile("/d.wav"); // 小数点
        for (int i = 0; i < decimalPart.length(); i++) {
            int digit = decimalPart.charAt(i) - '0';
            playDigit(digit);
        }
    }

    playFile("/y.wav"); // 结束播放音频的标志
}
void playDigit(int digit) {
  // 根据digit值播放对应的音频文件
  String filename = "/" + String(digit) + ".wav";
  playFile(filename.c_str());
}
