#include <Arduino.h>

// 定義左馬達控制腳位
const int ENA = 25; 
const int IN1 = 26; 
const int IN2 = 27; 

// 定義右馬達控制腳位
const int ENB = 22; 
const int IN3 = 21; 
const int IN4 = 19; 

// 定義測試用的基礎轉速 (PWM範圍: 0-255)
// 150 約為 60% 功率，足以克服靜摩擦力啟動 TT 馬達
const int BASE_SPEED = 125; 

void setup() {
  Serial.begin(115200);
  
  // 設定腳位為輸出模式
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // 初始狀態確保馬達完全停止
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  
  Serial.println("動力系統初始化完成，準備執行動作序列。");
  delay(3000); // 留出 3 秒空檔，讓你將車體放在空曠處或架空
}

void loop() {
  // 狀態 1：雙輪正轉 (車體前進)
  // L298N 邏輯：IN1(H), IN2(L) 為一側正轉；IN3(H), IN4(L) 為另一側正轉
  Serial.println("測試：前進");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, BASE_SPEED);
  analogWrite(ENB, BASE_SPEED);
  delay(2000);

  // 狀態 2：緊急停止 (煞車)
  // 將兩端電壓差歸零
  Serial.println("測試：停止");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  delay(1000);

  // 狀態 3：雙輪反轉 (車體後退)
  // 電流反向：IN1(L), IN2(H) ; IN3(L), IN4(H)
  Serial.println("測試：後退");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, BASE_SPEED);
  analogWrite(ENB, BASE_SPEED);
  delay(2000);

  // 狀態 4：再次停止
  Serial.println("測試：停止");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  delay(3000);
}