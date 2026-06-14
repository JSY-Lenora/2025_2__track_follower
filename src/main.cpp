#include <Arduino.h>

// ================= 硬體腳位定義 =================
const int ENA = 25; const int IN1 = 26; const int IN2 = 27; 
const int ENB = 22; const int IN3 = 21; const int IN4 = 19; 
const int S1 = 13; const int S2 = 14; const int S3 = 32; 
const int S4 = 34; const int S5 = 35; 
const int PIN_R = 16; const int PIN_G = 17; const int PIN_B = 18; 
const bool COMMON_ANODE = false; 
const int PIN_START = 4;

// ================= 運動控制與硬體補償參數 =================
int BASE_SPEED = 125;  // 已調降基礎直進速度
int MAX_SPEED = 175;   // 同步調降最大速度上限，避免過度補償
int MIN_SPEED = -120;  // 調整反轉極限

// 硬體非對稱性補償係數 (Motor Trim)
// 針對右輪過快的現象，對右側 PWM 輸出進行 30% 的衰減
const float LEFT_TRIM = 0.75;
const float RIGHT_TRIM = 1.0;
// 不知道為什麼這邊反了，但反著調就好

// 控制理論參數 (調降基礎速度後，系統慣性改變，可能需要重新微調 Kp)
float Kp = 8.1;
float Kd = 12.0;
const int LOST_ERROR = 50;

int lastError = 0;
bool isStarted = false; // 系統啟動狀態鎖
enum VehicleState { STATE_SAFE, STATE_AUTONOMOUS, STATE_OTHER };
VehicleState currentState = STATE_OTHER;

// --- 新增以下三個時間與狀態追蹤變數 ---
unsigned long lostStartTime = 0; // 記錄失去路線的瞬間時間
bool isLost = false;             // 標記目前是否處於迷失自救狀態
bool isDisqualified = false;     // 系統失格鎖定標籤
// --------------------------------------

// PWM 狀態濾波器
int currentLeftPWM = -999;
int currentRightPWM = -999;

// ================= 基礎封裝函式 =================
void setLED(bool r, bool g, bool b) {
  if (COMMON_ANODE) {
    digitalWrite(PIN_R, !r); digitalWrite(PIN_G, !g); digitalWrite(PIN_B, !b);
  } else {
    digitalWrite(PIN_R, r); digitalWrite(PIN_G, g); digitalWrite(PIN_B, b);
  }
}

void updateASL(VehicleState state) {
  if (currentState == state) return; 
  currentState = state;
  switch (state) {
    case STATE_SAFE:       setLED(false, true, false); break; 
    case STATE_AUTONOMOUS: setLED(true, false, false); break; 
    case STATE_OTHER:      setLED(false, false, true); break; 
  }
}

void setMotor(int speedLeft, int speedRight) {
  // 1. 注入硬體補償係數 (Hardware Trim)
  speedLeft = speedLeft * LEFT_TRIM;
  speedRight = speedRight * RIGHT_TRIM;

  // 2. 限制輸出範圍
  speedLeft = constrain(speedLeft, MIN_SPEED, MAX_SPEED);
  speedRight = constrain(speedRight, MIN_SPEED, MAX_SPEED);

  // 3. PWM 狀態濾波器 (防護 ESP32 Timer)
  if (speedLeft == currentLeftPWM && speedRight == currentRightPWM) {
    return; 
  }

  // 4. 硬體訊號輸出
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

  currentLeftPWM = speedLeft;
  currentRightPWM = speedRight;
}

void setup() {
  Serial.begin(115200);
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(S1, INPUT); pinMode(S2, INPUT); pinMode(S3, INPUT);
  pinMode(S4, INPUT); pinMode(S5, INPUT);
  pinMode(PIN_R, OUTPUT); pinMode(PIN_G, OUTPUT); pinMode(PIN_B, OUTPUT);
  pinMode(PIN_START, INPUT_PULLUP);

  updateASL(STATE_SAFE);
  setMotor(0, 0);
}

void loop() {

  if (isDisqualified) {
    updateASL(STATE_OTHER); // 恆亮藍燈
    setMotor(0, 0);         // 徹底關閉馬達
    
    // 偵測物理開關是否被「關閉」(拉高至 HIGH)
    if (digitalRead(PIN_START) == HIGH) {
      isDisqualified = false; // 解除失格鎖定
      isStarted = false;      // 回到未啟動狀態
      delay(500); // 簡單防彈跳
    }
    return; // 強制中斷，絕對不執行任何循跡演算法
  }

  // 階段一：等待啟動訊號
  if (!isStarted) {
    updateASL(STATE_SAFE); // 恆亮綠燈
    setMotor(0, 0);
    
    // 偵測開關是否被「開啟」(拉低至 LOW)
    if (digitalRead(PIN_START) == LOW) {
      delay(2000); 
      isStarted = true; 
      isLost = false; // 每次重新起跑時，確保迷失狀態被重置
    }
    return; 
  }

  int s[5] = {digitalRead(S1), digitalRead(S2), digitalRead(S3), digitalRead(S4), digitalRead(S5)};
  int weights[5] = {-25, -10, 0, 10, 25};
  
  int activeSensors = 0;
  long weightedSum = 0;
  int currentError = 0;

  for (int i = 0; i < 5; i++) {
    if (s[i] == 1) { 
      weightedSum += weights[i];
      activeSensors++;
    }
  }

  if (activeSensors > 0) {
    currentError = weightedSum / activeSensors;
    updateASL(STATE_AUTONOMOUS); // 正常導航中，維持紅燈
    isLost = false;              // 只要壓到線，立刻解除迷失狀態與計時
  } else {
    // 進入迷失狀態
    if (!isLost) {
      isLost = true;
      lostStartTime = millis(); 
    }

    if (millis() - lostStartTime >= 5000) {
      isDisqualified = true; 
      return; 
    }

    updateASL(STATE_AUTONOMOUS); 
    
    // 使用您提議的常數來決定轉向方向
    if (lastError > 0) currentError = LOST_ERROR; 
    else if (lastError < 0) currentError = -LOST_ERROR; 
    else currentError = 0; 
  }

  // === 演算法防護核心 ===
  int pTerm = Kp * currentError;
  int dTerm = 0; // 預設 D-Term 為 0

  if (activeSensors > 0) {
    // 只有在「正常壓線」狀態下，才計算 D-Term (PD 控制)
    dTerm = Kd * (currentError - lastError);
  } else {
    // 在「迷失自救」狀態下，強制阻斷 D-Term (退化為純 P 控制的暴力拉回)
    // 這樣可以徹底消滅 Derivative Kick 和 0 阻尼帶來的數學異常
    dTerm = 0; 
  }

  int correction = pTerm + dTerm;

  int speedLeft = BASE_SPEED + correction;
  int speedRight = BASE_SPEED - correction;

  setMotor(speedLeft, speedRight);
  
  lastError = currentError; 
  
  delay(5); 
}