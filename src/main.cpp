#include <Arduino.h>

void setup() {
  Serial.begin(115200); 
  delay(1000);
  Serial.println("ESP32 初始化完成，準備就緒！");
}

void loop() {
  Serial.println("循跡車大腦運作中...");
  delay(2000); 
}