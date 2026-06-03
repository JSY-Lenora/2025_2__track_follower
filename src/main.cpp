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

// 定義五路感測器輸入腳位 (由左至右)
const int SENSOR_L2 = 13; // 極左 S1
const int SENSOR_L1 = 14; // 中左 S2
const int SENSOR_M  = 32; // 中央 S3
const int SENSOR_R1 = 34; // 中右 S4
const int SENSOR_R2 = 35; // 極右 S5

// 定義 ASL 控制腳位 (假設初始對應，稍後可依實際亮燈顏色調整)
const int PIN_R = 16;
const int PIN_G = 17;
const int PIN_B = 18;

// 定義 ASL 狀態列舉
enum VehicleState {
  STATE_SAFE,       // 安全狀態 (綠)
  STATE_AUTONOMOUS, // 自主導航狀態 (紅)
  STATE_OTHER       // 其他狀態 (藍)
};

const bool COMMON_ANODE = false;

void setLED(bool r, bool g, bool b) {
  if (COMMON_ANODE) {
    digitalWrite(PIN_R, !r);
    digitalWrite(PIN_G, !g);
    digitalWrite(PIN_B, !b);
  } else {
    digitalWrite(PIN_R, r);
    digitalWrite(PIN_G, g);
    digitalWrite(PIN_B, b);
  }
}

void turnOffASL() {
  setLED(false, false, false);
}

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

  pinMode(SENSOR_L2, INPUT);
  pinMode(SENSOR_L1, INPUT);
  pinMode(SENSOR_M, INPUT);
  pinMode(SENSOR_R1, INPUT);
  pinMode(SENSOR_R2, INPUT);
  
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  
  // 初始狀態關閉所有燈號
  turnOffASL();
  Serial.println("ASL 系統初始化完成，開始測試狀態切換。");
  delay(2000);
}

void updateASL(VehicleState state) {
  switch (state) {
    case STATE_SAFE:
      setLED(false, true, false); // 綠燈恆亮
      Serial.println("目前狀態：安全 (預期亮綠燈)");
      break;
    case STATE_AUTONOMOUS:
      setLED(true, false, false); // 紅燈恆亮
      Serial.println("目前狀態：自主導航 (預期亮紅燈)");
      break;
    case STATE_OTHER:
      setLED(false, false, true); // 藍燈恆亮
      Serial.println("目前狀態：其他 (預期亮藍燈)");
      break;
  }
}

void loop() {
  //updateASL(STATE_SAFE);
  //delay(2000);
  
  //updateASL(STATE_AUTONOMOUS);
  //delay(2000);
  
  //updateASL(STATE_OTHER);
  //delay(2000);
}