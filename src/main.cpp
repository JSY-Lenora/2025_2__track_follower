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
// ================= 運動控制參數 =================
int BASE_SPEED = 180;  // 基準直進速度 (0-255)
int MAX_SPEED = 255;   // 單側馬達最高速度限制
int MIN_SPEED = -150;  // 允許的最大反轉速度 (用於急彎產生原地旋轉力矩)

// 控制理論參數 (Kp 與 Kd 需依實際摩擦力與車體慣性進行實測調校)
float Kp = 3.5;  // 比例常數：決定轉向的敏銳度
float Kd = 15.0; // 微分常數：決定抑制蛇行震盪的阻尼強度

// 系統狀態變數
int lastError = 0;
enum VehicleState { STATE_SAFE, STATE_AUTONOMOUS, STATE_OTHER };
VehicleState currentState = STATE_SAFE;

// 定義五路感測器輸入腳位 (由左至右)
const int S1 = 13; const int S2 = 14; const int S3 = 32; 
const int S4 = 34; const int S5 = 35;

// 定義 ASL 控制腳位 (假設初始對應，稍後可依實際亮燈顏色調整)
const int PIN_R = 16;
const int PIN_G = 17;
const int PIN_B = 18;

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

void updateASL(VehicleState state) {
  if (currentState == state) return; 
  currentState = state;
  switch (state) {
    case STATE_SAFE:       setLED(false, true, false); break; // 綠
    case STATE_AUTONOMOUS: setLED(true, false, false); break; // 紅
    case STATE_OTHER:      setLED(false, false, true); break; // 藍
  }
}

void setMotor(int speedLeft, int speedRight) {
  // 限制輸出範圍以保護硬體並確保 PWM 合法性
  speedLeft = constrain(speedLeft, MIN_SPEED, MAX_SPEED);
  speedRight = constrain(speedRight, MIN_SPEED, MAX_SPEED);

  if (speedLeft >= 0) {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); analogWrite(ENA, speedLeft);
  } else {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); analogWrite(ENA, -speedLeft);
  }

  if (speedRight >= 0) {
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); analogWrite(ENB, speedRight);
  } else {
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); analogWrite(ENB, -speedRight);
  }
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

  pinMode(S1, INPUT); pinMode(S2, INPUT); pinMode(S3, INPUT);
  pinMode(S4, INPUT); pinMode(S5, INPUT);

  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  
  updateASL(STATE_SAFE);
  setMotor(0, 0);
  delay(3000);
}

void loop() {
// 1. 讀取感測器狀態 (假設 1 為壓到黑線)
  int s[5] = {digitalRead(S1), digitalRead(S2), digitalRead(S3), digitalRead(S4), digitalRead(S5)};
  
  // 2. 空間權重定義
  int weights[5] = {-20, -10, 0, 10, 20};
  
  int activeSensors = 0;
  long weightedSum = 0;
  int currentError = 0;

  // 3. 計算加權平均誤差
  for (int i = 0; i < 5; i++) {
    if (s[i] == 1) {
      weightedSum += weights[i];
      activeSensors++;
    }
  }

  if (activeSensors > 0) {
    currentError = weightedSum / activeSensors;
    lastError = currentError; // 記憶最後一次的誤差方向
    updateASL(STATE_AUTONOMOUS); // 進入自主導航 (紅燈)
  } else {
    // 記憶尋線機制：若完全脫離黑線，根據最後的誤差方向給予極端值，迫使車體原地旋轉找回路線
    if (lastError > 0) {
      currentError = 30; // 強烈右轉
    } else if (lastError < 0) {
      currentError = -30; // 強烈左轉
    } else {
      currentError = 0; 
    }
    
    // 若極端迷失，亮起藍燈 (實務上若車體一直找不到線，這裡會被觸發)
    updateASL(STATE_OTHER); 
  }

  // 4. PD 控制器運算
  int pTerm = Kp * currentError;
  int dTerm = Kd * (currentError - lastError);
  int correction = pTerm + dTerm;

  // 5. 動力混合與輸出 (差速映射)
  // 左轉時 correction 為負，右轉時 correction 為正
  int speedLeft = BASE_SPEED + correction;
  int speedRight = BASE_SPEED - correction;

  setMotor(speedLeft, speedRight);
  
  // 6. 更新離散狀態
  lastError = currentError;
}