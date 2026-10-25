// ============================================================
// BC-250 Dual-Pad Power Controller (ATX Hardware Safe Management)
// BC-250 듀얼패드 전원 제어기 (ATX 하드웨어 안전 관리 시스템)
// ============================================================

#include <Arduino.h>

// [필수] Classic BT 힙 메모리 자동 해제 방지 / [Required] Prevent auto-free of Classic BT heap memory
#if __has_include("esp32-hal-alloc-bt-classic-mem.h")
#include "esp32-hal-alloc-bt-classic-mem.h"
#endif

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>
#include <esp_mac.h>
#include <esp_ota_ops.h>      // [필수] OTA 및 부트로더 자동 롤백 제어 / [Required] OTA & Bootloader auto-rollback control
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <driver/gpio.h>

// Wi-Fi, WebServer, UDP, OTA, NVS 라이브러리 / Wi-Fi, WebServer, UDP, OTA, NVS Libraries
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <Update.h>
#include <Preferences.h>

// 사용자 정의 모듈 헤더 / Custom Module Headers
#include "config.h"
#include "web_ui.h"

// ============================================================
// ▼ [필수] 아두이노 조기 승인 차단 -> 순수 부트로더 롤백 활성화
// ▼ [Required] Block early Arduino confirmation -> Enable pure bootloader rollback
// ============================================================
extern "C" bool verifyRollbackLater(void) {
  return true;
}

// ============================================================
// ▼ [웹 시리얼 콘솔] 백그라운드 2KB 링 버퍼 및 전용 래퍼 클래스
// ▼ [Web Serial Console] Background 2KB Ring Buffer & Stream Wrapper
// ============================================================
static char webLogBuf[WEB_LOG_SIZE];
static size_t webLogHead = 0;
static bool webLogWrapped = false;
static portMUX_TYPE webLogMux = portMUX_INITIALIZER_UNLOCKED;

inline void appendWebLogChar(char c) {
  portENTER_CRITICAL(&webLogMux);
  webLogBuf[webLogHead] = c;
  webLogHead = (webLogHead + 1) % WEB_LOG_SIZE;
  if (webLogHead == 0) webLogWrapped = true;
  portEXIT_CRITICAL(&webLogMux);
}

class WebSerialLogger : public Stream {
private:
  HardwareSerial* _hw;
public:
  WebSerialLogger(HardwareSerial* hw) : _hw(hw) {}
  void begin(unsigned long baud) { _hw->begin(baud); }
  int available() override { return _hw->available(); }
  int read() override { return _hw->read(); }
  int peek() override { return _hw->peek(); }
  void flush() override { _hw->flush(); }
  size_t write(uint8_t c) override {
    _hw->write(c);
    appendWebLogChar((char)c);
    return 1;
  }
  size_t write(const uint8_t *buffer, size_t size) override {
    _hw->write(buffer, size);
    for (size_t i = 0; i < size; i++) appendWebLogChar((char)buffer[i]);
    return size;
  }
};

static HardwareSerial& realSerial = ::Serial;
static WebSerialLogger WebSerial(&realSerial);
#define LOG_PRINT(x)    WebSerial.print(x)
#define LOG_PRINTLN(x)  WebSerial.println(x)
#define LOG_PRINTF(...) WebSerial.printf(__VA_ARGS__)

String getWebLogs() {
  String res;
  if (!res.reserve(WEB_LOG_SIZE + 1)) return String("");

  portENTER_CRITICAL(&webLogMux);
  if (webLogWrapped) {
    size_t part1 = WEB_LOG_SIZE - webLogHead;
    res.concat(webLogBuf + webLogHead, part1);
    res.concat(webLogBuf, webLogHead);
  } else if (webLogHead > 0) {
    res.concat(webLogBuf, webLogHead);
  }
  portEXIT_CRITICAL(&webLogMux);
  return res;
}

void clearWebLogs() {
  portENTER_CRITICAL(&webLogMux);
  webLogHead = 0;
  webLogWrapped = false;
  portEXIT_CRITICAL(&webLogMux);
}

// ============================================================
// 0. 런타임 제어 변수
// 0. Runtime Control Variables
// ============================================================
static bool isSignatureMatched = false;
static uint8_t sigTailBuf[32];
static size_t sigTailLen = 0;

bool needRestartOnStandby = false;
volatile bool isOtaUpdating = false;
unsigned long otaLastActiveTime = 0;
static bool otaConfirmed = false;

// ============================================================
// 1. Wi-Fi SoftAP, WebServer & 삼중 WOL UDP
// 1. Wi-Fi SoftAP, WebServer & Triple WOL UDP
// ============================================================
String apPass = "";
bool needApReload = false;
unsigned long apReloadStartTime = 0;

WebServer server(80);
WiFiUDP wolUdp7;
WiFiUDP wolUdp9;
WiFiUDP wolUdp2304;
bool udpReady = false;
Preferences prefs;

// ============================================================
// ▼ 최근 감지된 패드 실시간 버퍼 (패시브 스니핑 수집)
// ▼ Real-Time Buffer for Recently Detected Pads (Passive Sniffing)
// ============================================================
String lastDetectedDs4Mac = "";
unsigned long lastDetectedDs4Time = 0;
String lastDetectedXboxMac = "";
unsigned long lastDetectedXboxTime = 0;

// ============================================================
// ▼ 무선 RF 모뎀 안전 전원 차단 후 클린 재부팅
// ▼ Safely Power Off Wireless RF Modem and Clean Reboot
// ============================================================
void safeRestart() {
  LOG_PRINTLN(F("[SYSTEM] 무선 모뎀 안전 종료 후 재부팅 진행... / Safely powering off RF modem and rebooting..."));
  server.close();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);
  ESP.restart();
}

// ============================================================
// 2. 설정 변수 및 NVS 스토리지 제어 (MAC 자동 포맷 정규화 탑재)
// 2. Configuration Variables & NVS Storage Control (Auto MAC Normalization)
// ============================================================
String targetDs4Mac  = DEFAULT_DS4_MAC;
String targetXboxMac = DEFAULT_XBOX_MAC;
String dongleMacStr  = DEFAULT_DONGLE_MAC;
uint8_t DONGLE_MAC[6] = { 0x00, 0x1A, 0x7D, 0xDA, 0x71, 0x13 };

bool wolEnabled     = false;
String wifiSsid     = "";
String wifiPass     = "";

String getStaMacAddress() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[18];
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

String normalizeMac(String mac) {
  mac.trim();
  mac.toUpperCase();
  String clean = "";
  for (unsigned int i = 0; i < mac.length(); i++) {
    char c = mac[i];
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
      clean += c;
    }
  }
  if (clean.length() == 12) {
    String formatted = "";
    for (int i = 0; i < 12; i += 2) {
      formatted += clean.substring(i, i + 2);
      if (i < 10) formatted += ":";
    }
    return formatted;
  }
  return mac;
}

bool parseMacAddress(const String& macStr, uint8_t* bytes) {
  int values[6];
  if (sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2],
             &values[3], &values[4], &values[5]) == 6) {
    for (int i = 0; i < 6; i++) bytes[i] = (uint8_t)values[i];
    return true;
  }
  return false;
}

void loadSettings() {
  prefs.begin("bc250_cfg", true);
  targetDs4Mac  = prefs.getString("ds4_mac", DEFAULT_DS4_MAC);
  targetXboxMac = prefs.getString("xbox_mac", DEFAULT_XBOX_MAC);
  dongleMacStr  = prefs.getString("dongle_mac", DEFAULT_DONGLE_MAC);
  wolEnabled    = prefs.getBool("wol_en", false);
  wifiSsid      = prefs.getString("wifi_ssid", "");
  wifiPass      = prefs.getString("wifi_pass", "");
  apPass        = prefs.getString("ap_pass", "");
  prefs.end();

  targetDs4Mac = normalizeMac(targetDs4Mac);
  targetXboxMac = normalizeMac(targetXboxMac);
  dongleMacStr = normalizeMac(dongleMacStr);

  wifiSsid.trim();
  apPass.trim();

  if (!parseMacAddress(dongleMacStr, DONGLE_MAC)) {
    parseMacAddress(DEFAULT_DONGLE_MAC, DONGLE_MAC);
  }

  LOG_PRINTLN(F("[NVS] 비휘발성 설정 로드 완료 / Non-Volatile Settings Loaded Successfully:"));
  LOG_PRINTF(" - DS4/Sense MAC: %s\n", targetDs4Mac.c_str());
  LOG_PRINTF(" - Xbox MAC: %s\n", targetXboxMac.c_str());
  LOG_PRINTF(" - PC BT MAC: %s\n", dongleMacStr.c_str());
  LOG_PRINTF(" - AP 보안 / AP Security: %s (%s)\n", 
                (apPass.length() >= 8) ? "WPA2 암호화 / WPA2 Encrypted" : "비밀번호 미설정 (오픈) / Open Mode",
                "최대 2대 허용 / Max 2 Clients");
  LOG_PRINTF(" - WOL 활성화 / WOL Enabled: %s\n", wolEnabled ? "YES" : "NO");
}

void startWolStation() {
  if (wolEnabled && wifiSsid.length() > 0) {
    udpReady = false;
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.mode(WIFI_AP_STA);
      WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
      LOG_PRINTF("[WOL] 대기 모드 진입 -> 공유기 연결 시작 (SSID: %s) / Standby Mode -> Connecting to Router (SSID: %s)...\n", wifiSsid.c_str(), wifiSsid.c_str());
    }
  }
}

void stopWolStation() {
  wolUdp7.stop();
  wolUdp9.stop();
  wolUdp2304.stop();
  udpReady = false;

  if (WiFi.getMode() & WIFI_MODE_STA) {
    WiFi.disconnect(false, true);
    WiFi.mode(WIFI_AP);
    LOG_PRINTLN(F("[WIFI] 공유기 Wi-Fi(STA) 종료 -> 순수 AP 모드 전환 / Router Wi-Fi(STA) Stopped -> Switched to Pure AP Mode"));
  }
}

bool checkApAccess() {
  if (WiFi.status() == WL_CONNECTED && server.client().localIP() != WiFi.softAPIP()) {
    server.client().stop();
    return false;
  }
  return true;
}

// ============================================================
// 3. 상태 머신 열거형 및 제어 변수
// 3. State Machine Enum & Control Variables
// ============================================================
enum SystemState { STATE_STANDBY, STATE_BOOTING, STATE_RUNNING };
SystemState currentState = STATE_STANDBY;

bool xboxArmed = false;
unsigned long lastXboxSeenTime = 0;

unsigned long bootStartTime = 0;
bool tpmLowPending = false;
unsigned long tpmLowStartTime = 0;

int switchRawReading = HIGH;
int switchStableState = HIGH;
int lastStableSwitchState = HIGH;
unsigned long switchDebounceStartTime = 0;

bool switchWasPressed = false;
unsigned long switchPressStartTime = 0;
bool waitForSwitchRelease = false;

volatile bool powerTriggerPending = false;
const char* volatile triggerReason = nullptr;

BLEScan* pBLEScan = nullptr;

// ============================================================
// 4. 전원 제어 (오픈 드레인 하드웨어 보호)
// 4. Power Control (Open-Drain Hardware Protection)
// ============================================================
void startPowerSupply() {
  digitalWrite(PIN_PSON_OUT, LOW);
  LOG_PRINTLN(F("[POWER] PS_ON = LOW -> PSU ON (전원 켬 / Power ON)"));
}

void stopPowerSupply() {
  digitalWrite(PIN_PSON_OUT, HIGH);
  LOG_PRINTLN(F("[POWER] PS_ON = HIGH(Hi-Z) -> PSU OFF (전원 차단 / Power Shut Off)"));
}

// ============================================================
// 5. 무선 감시 제어 및 블루투스 안전 완전 킬
// 5. Wireless Sniffing & Safe Bluetooth Stack Teardown
// ============================================================
void bt_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

void startWirelessSniffing() {
  powerTriggerPending = false;
  triggerReason = nullptr;

  if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_ENABLED) {
    if (esp_bt_controller_enable(ESP_BT_MODE_BTDM) == ESP_OK) {
      esp_bluedroid_enable();
      esp_bt_gap_register_callback(bt_gap_callback);
    }
  }

  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);

  if (pBLEScan != nullptr) {
    pBLEScan->clearResults();
    pBLEScan->start(0, nullptr, false);
  }

  LOG_PRINTLN(F("[RADIO] Classic BT PageScan 및 Xbox BLE 동시 감시 활성화 / Classic BT PageScan & Xbox BLE Dual Sniffing Enabled"));
}

void stopWirelessSniffing() {
  if (pBLEScan != nullptr) {
    pBLEScan->stop();
    pBLEScan->clearResults();
  }

  if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) {
    esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
    esp_bluedroid_disable();
  }
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
    esp_bt_controller_disable();
  }
}

void killBluetoothStack() {
  stopWirelessSniffing();
  pBLEScan = nullptr;

  if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_INITIALIZED) {
    esp_bluedroid_deinit();
  }
  if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED) {
    esp_bt_controller_deinit();
  }

  LOG_PRINTF("[RADIO] 블루투스 스택 안전 완전 종료 완료 / Bluetooth Stack Safely Terminated -> Free Heap: %u KB\n", ESP.getFreeHeap() / 1024);
}

// ============================================================
// 6. 콜백 함수 및 WOL 매직 패킷 검증 (패시브 스니핑 탑재)
// 6. Callbacks & WOL Magic Packet Verification (Passive Sniffing)
// ============================================================
void bt_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  if (isOtaUpdating || currentState != STATE_STANDBY || powerTriggerPending) return;

  char bda_str[18] = {0};

  if (event == ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT) {
    sprintf(bda_str, "%02X:%02X:%02X:%02X:%02X:%02X",
            param->acl_conn_cmpl_stat.bda[0], param->acl_conn_cmpl_stat.bda[1], param->acl_conn_cmpl_stat.bda[2],
            param->acl_conn_cmpl_stat.bda[3], param->acl_conn_cmpl_stat.bda[4], param->acl_conn_cmpl_stat.bda[5]);
    LOG_PRINTF("[GAP-EVT 16] ACL 연결 감지 / ACL Connection Detected! MAC: %s\n", bda_str);

    lastDetectedDs4Mac = String(bda_str);
    lastDetectedDs4Time = millis();
  }

  if (bda_str[0] != '\0' && targetDs4Mac.equalsIgnoreCase(bda_str)) {
    triggerReason = "DualShock 4 / DualSense 연결 감지 / Connection Detected";
    powerTriggerPending = true;
  }
}

class XboxSnifferCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    if (isOtaUpdating || currentState != STATE_STANDBY || powerTriggerPending) return;

    String currentMac = advertisedDevice.getAddress().toString().c_str();
    currentMac.toUpperCase();

    bool isGamepadAdv = false;
    if (advertisedDevice.haveName()) {
      String devName = advertisedDevice.getName().c_str();
      if (devName.indexOf("Xbox") >= 0 || devName.indexOf("Controller") >= 0) {
        isGamepadAdv = true;
      }
    }
    if (advertisedDevice.haveAppearance() && advertisedDevice.getAppearance() == 0x03C4) {
      isGamepadAdv = true;
    }
    if (isGamepadAdv) {
      lastDetectedXboxMac = currentMac;
      lastDetectedXboxTime = millis();
    }

    if (currentMac.equalsIgnoreCase(targetXboxMac)) {
      lastXboxSeenTime = millis();
      lastDetectedXboxMac = currentMac;
      lastDetectedXboxTime = millis();

      if (xboxArmed) {
        triggerReason = "Xbox Controller 감지 / Controller Detected";
        powerTriggerPending = true;
      }
    }
  }
};

bool checkMagicPacket(const uint8_t* buf, int len, const uint8_t* targetMac) {
  if (len < 102) return false;
  for (int i = 0; i <= len - 102; i++) {
    bool headerMatch = true;
    for (int h = 0; h < 6; h++) {
      if (buf[i + h] != 0xFF) { headerMatch = false; break; }
    }
    if (!headerMatch) continue;

    bool macMatch = true;
    for (int rep = 0; rep < 16; rep++) {
      for (int m = 0; m < 6; m++) {
        if (buf[i + 6 + rep * 6 + m] != targetMac[m]) {
          macMatch = false;
          break;
        }
      }
      if (!macMatch) break;
    }
    if (macMatch) return true;
  }
  return false;
}

bool processUdpSocket(WiFiUDP &udp, uint16_t port, const uint8_t* myMac) {
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
    uint8_t packetBuf[256];
    int len = udp.read(packetBuf, sizeof(packetBuf));

    if (checkMagicPacket(packetBuf, len, myMac)) {
      LOG_PRINTF("[WOL] 매직 패킷 검증 일치 (수신 포트: %u번) / Magic Packet Verified (Port: %u)\n", port, port);
      triggerReason = "WOL 매직 패킷 감지 / Magic Packet Detected";
      return true;
    }
  }
  return false;
}

// ============================================================
// 7. 상태 전환 시퀀스 / 7. State Transition Sequences
// ============================================================
void startBootSequence() {
  const char* reason = triggerReason;
  const char* defaultReason = "물리 버튼 / Physical Button";
  LOG_PRINTF("\n[STATE] 부팅 시퀀스 시작 (트리거 원인: %s) / Boot Sequence Started (Trigger: %s)\n", 
                reason ? reason : defaultReason, reason ? reason : defaultReason);

  powerTriggerPending = false;
  triggerReason = nullptr;

  startPowerSupply();
  bootStartTime = millis();
  switchWasPressed = false;
  waitForSwitchRelease = (switchStableState == LOW);

  stopWirelessSniffing();

  currentState = STATE_BOOTING;
  LOG_PRINTLN(F("[STATE] -> BOOTING (TPM 응답 신호 감시 중... / Monitoring TPM response signal...)"));
}

void enterStandby() {
  stopPowerSupply();
  currentState = STATE_STANDBY;

  tpmLowPending = false;
  switchWasPressed = false;
  waitForSwitchRelease = (switchStableState == LOW);

  xboxArmed = false;
  lastXboxSeenTime = millis();

  if (needRestartOnStandby) {
    needRestartOnStandby = false;
    LOG_PRINTLN(F("[SYSTEM] PC 종료 감지 -> 보류된 변경사항 적용을 위해 안전 재부팅... / PC Shutdown Detected -> Safe rebooting to apply pending changes..."));
    delay(600);
    safeRestart();
  }

  startWirelessSniffing();
  startWolStation();

  LOG_PRINTLN(F("[STATE] -> STANDBY (대기 모드 진입 / Entering Standby Mode)"));
}

// ============================================================
// 8. 스위치 디바운스 및 긴급 강제종료
// 8. Switch Debounce & Emergency Hard Power-Cut
// ============================================================
void updateSwitch() {
  int raw = digitalRead(PIN_SWITCH_IN);

  if (raw != switchRawReading) {
    switchRawReading = raw;
    switchDebounceStartTime = millis();
  }

  if ((millis() - switchDebounceStartTime) >= SWITCH_DEBOUNCE_TIME) {
    if (switchStableState != switchRawReading) {
      lastStableSwitchState = switchStableState;
      switchStableState = switchRawReading;

      if (switchStableState == HIGH) {
        if (waitForSwitchRelease) waitForSwitchRelease = false;
      }
    }
  }
}

bool checkEmergencyLongPress() {
  if (waitForSwitchRelease || currentState != STATE_RUNNING) return false;

  if (switchStableState == LOW) {
    if (!switchWasPressed) {
      switchWasPressed = true;
      switchPressStartTime = millis();
    } else if ((millis() - switchPressStartTime) >= LONG_PRESS_TIME) {
      LOG_PRINTLN(F("[EMERGENCY] 스위치 5초 롱프레스 감지 -> 강제 전원 차단! / Switch 5s long-press detected -> Forced Power Cutoff!"));
      switchWasPressed = false;
      stopPowerSupply();
      delay(800);
      safeRestart();
      return true;
    }
  } else {
    switchWasPressed = false;
  }
  return false;
}

// ============================================================
// 9. 터미널 명령 처리 및 페일세이프 자가진단
// 9. Terminal Command Processor & Failsafe Diagnostics
// ============================================================
void executeCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;
  LOG_PRINTF("\n>> [CMD 수신 / CMD Received] %s\n", cmd.c_str());

  if (cmd.equalsIgnoreCase("status") || cmd.equalsIgnoreCase("s")) {
    unsigned long upSec = millis() / 1000UL;
    unsigned int upDays = upSec / 86400;
    unsigned int upHours = (upSec % 86400) / 3600;
    unsigned int upMinutes = (upSec % 3600) / 60;
    unsigned int upSeconds = upSec % 60;

    LOG_PRINTLN(F("=========================================="));
    LOG_PRINTLN(F("[시스템 종합 진단 보고 / System Diagnostic Report]"));
    LOG_PRINTLN(F("=========================================="));
    LOG_PRINTF(" - 가동 시간 / Uptime    : %ud %02uh %02um %02us\n", upDays, upHours, upMinutes, upSeconds);
    LOG_PRINTF(" - 제어 상태 / State     : %s\n", 
                  (currentState == STATE_STANDBY) ? "STANDBY (대기 / Standby)" :
                  (currentState == STATE_BOOTING) ? "BOOTING (부팅 중 / Booting)" : "RUNNING (정상 구동 중 / Running)");
    LOG_PRINTF(" - TPM 감지 / TPM Sense  : %s\n", (digitalRead(PIN_TPM_IN) == HIGH) ? "HIGH (3.3V / PC ON)" : "LOW (0V / PC OFF)");
    LOG_PRINTF(" - PS_ON 핀 / PS_ON Pin  : %s\n", (digitalRead(PIN_PSON_OUT) == LOW) ? "LOW (PSU ON / 도통)" : "HIGH (PSU OFF / Hi-Z)");
    LOG_PRINTF(" - 잔여 힙 / Free Heap   : %u KB\n", ESP.getFreeHeap() / 1024);
    LOG_PRINTLN(F("=========================================="));
  } 
  else if (cmd.equalsIgnoreCase("reboot") || cmd.equalsIgnoreCase("r")) {
    delay(500);
    safeRestart();
  } 
  else if (cmd.equalsIgnoreCase("clear") || cmd.equalsIgnoreCase("c")) {
    clearWebLogs();
    LOG_PRINTLN(F("[CMD] 웹 콘솔 로그 버퍼 초기화 완료 / Web console log buffer cleared"));
  } 
  else if (cmd.equalsIgnoreCase("help") || cmd.equalsIgnoreCase("h") || cmd.equalsIgnoreCase("?")) {
    LOG_PRINTLN(F("명령어 / Commands: status(s), reboot(r), clear(c), help(h)"));
  }
}

bool runSelfDiagnostics() {
  if (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) return false;
  if (digitalRead(PIN_PSON_OUT) == LOW && currentState == STATE_STANDBY) return false;
  return true;
}

void rollbackAndReboot(const char* reason) {
  LOG_PRINTF("\n[ROLLBACK] 치명적 오류 (%s) -> 이전 펌웨어로 자동 롤백 재부팅! / Critical error (%s) -> Auto-rollback to previous firmware!\n", reason, reason);
  delay(100);
  esp_ota_mark_app_invalid_rollback_and_reboot();
}

// ============================================================
// 10. 웹서버 결과 페이지 응답기
// 10. WebServer Result Page Responder
// ============================================================
void sendResultPage(const char* titleKo, const char* titleEn,
                    const char* bodyKo, const char* bodyEn,
                    const char* color = "#4caf50", int refreshSec = 0) {
  String html;
  html.reserve(1200);
  html += F("<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
  if (refreshSec > 0) html += "<meta http-equiv='refresh' content='" + String(refreshSec) + ";url=/'>";
  html += F("<style>body{background:#121212;color:#fff;text-align:center;padding:45px 16px;font-family:-apple-system,sans-serif;margin:0;}"
            ".box{display:none;max-width:480px;margin:0 auto;line-height:1.65;font-size:14.5px;color:#ccc;}"
            "h3{margin:0 0 16px;font-size:19px;}strong{color:#fff;}a{color:#2196F3;text-decoration:none;font-weight:bold;}"
            "</style></head><body>");
  html += "<div id='box-ko' class='box'><h3 style='color:" + String(color) + ";'>" + String(titleKo) + "</h3><p>" + String(bodyKo) + "</p>";
  if (refreshSec <= 0) html += F("<p style='margin-top:22px;'><a href='/'>돌아가기</a></p>");
  html += "</div>";
  html += "<div id='box-en' class='box'><h3 style='color:" + String(color) + ";'>" + String(titleEn) + "</h3><p>" + String(bodyEn) + "</p>";
  if (refreshSec <= 0) html += F("<p style='margin-top:22px;'><a href='/'>Back to Main</a></p>");
  html += "</div>";
  html += F("<script>var l=localStorage.getItem('bc250_lang')||'ko';var el=document.getElementById('box-'+l)||document.getElementById('box-ko');if(el)el.style.display='block';</script></body></html>");
  server.send(200, "text/html; charset=utf-8", html);
}

// ============================================================
// 11. 웹서버 요청 핸들러
// 11. WebServer Request Handlers
// ============================================================
void handleRoot() {
  if (!checkApAccess()) return;

  size_t totalLen = strlen_P(INDEX_HTML);
  server.setContentLength(totalLen);
  server.send(200, "text/html; charset=utf-8", "");

  const char* ptr = INDEX_HTML;
  size_t remaining = totalLen;
  const size_t CHUNK_SIZE = 1024;

  while (remaining > 0) {
    size_t toSend = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
    server.sendContent_P(ptr, toSend);
    ptr += toSend;
    remaining -= toSend;
    yield();
  }
}

void handleApiStatus() {
  if (!checkApAccess()) return;
  if (isOtaUpdating) { server.send(204); return; }

  const char* statusStr = (currentState == STATE_STANDBY) ? "STANDBY" :
                          (currentState == STATE_BOOTING) ? "BOOTING" : "PC RUNNING";
  int uiRawState = (currentState == STATE_STANDBY) ? 0 :
                   (currentState == STATE_BOOTING) ? 1 : 2;
  const char* tpmStr = (digitalRead(PIN_TPM_IN) == HIGH) ? "TPM: 3.3V (PC ON)" : "TPM: 0V (PC OFF)";

  char sensorStr[64];
  snprintf(sensorStr, sizeof(sensorStr), "%s | Free Heap: %u KB", tpmStr, ESP.getFreeHeap() / 1024);

  char wifiStatusText[64] = "DISABLED";
  const char* wifiColor = "#888";

  if (wolEnabled) {
    if (currentState == STATE_RUNNING) {
      snprintf(wifiStatusText, sizeof(wifiStatusText), "OFF");
      wifiColor = "#9E9E9E";
    } else if (WiFi.status() == WL_CONNECTED) {
      snprintf(wifiStatusText, sizeof(wifiStatusText), "CONNECTED (%s)", WiFi.localIP().toString().c_str());
      wifiColor = "#4CAF50";
    } else {
      snprintf(wifiStatusText, sizeof(wifiStatusText), "CONNECTING");
      wifiColor = "#ff9800";
    }
  }

  char jsonBuf[768];
  snprintf(jsonBuf, sizeof(jsonBuf),
    "{\"status\":\"%s\",\"sensor\":\"%s\",\"raw_state\":%d,\"ds4\":\"%s\",\"xbox\":\"%s\",\"dongle\":\"%s\",\"esp_mac\":\"%s\",\"wol_en\":%s,\"wifi_status\":\"%s\",\"wifi_color\":\"%s\",\"ssid\":\"%s\",\"wifi_pass_set\":%s,\"ap_pass_set\":%s}",
    statusStr, sensorStr, uiRawState, targetDs4Mac.c_str(), targetXboxMac.c_str(), dongleMacStr.c_str(),
    getStaMacAddress().c_str(), wolEnabled ? "true" : "false", wifiStatusText, wifiColor,
    wifiSsid.c_str(), (wifiPass.length() > 0 ? "true" : "false"), (apPass.length() >= 8 ? "true" : "false")
  );

  server.send(200, "application/json", jsonBuf);
}

void handleScan() {
  if (!checkApAccess()) return;
  if (isOtaUpdating) { server.send(204); return; }

  if (pBLEScan != nullptr) {
    pBLEScan->stop();
    delay(20);
  }

  wifi_mode_t prevMode = WiFi.getMode();
  if (!(prevMode & WIFI_MODE_STA)) {
    WiFi.mode(WIFI_AP_STA);
    delay(30);
  }

  int n = WiFi.scanNetworks(false, false);
  String json = "[";
  json.reserve(1024);

  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"enc\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
  }
  json += "]";
  WiFi.scanDelete();

  if (prevMode == WIFI_MODE_AP) WiFi.mode(WIFI_AP);
  server.send(200, "application/json", json);

  if (pBLEScan != nullptr && currentState == STATE_STANDBY && !isOtaUpdating) {
    pBLEScan->start(0, nullptr, false);
  }
}

void handleSaveAp() {
  if (!checkApAccess()) return;
  if (server.hasArg("new_ap_pass")) {
    String newPass = server.arg("new_ap_pass");
    newPass.trim();

    if (newPass.length() > 0 && newPass.length() < 8) {
      sendResultPage("비밀번호 오류", "Password Error",
                     "Wi-Fi 비밀번호는 8자 이상이어야 합니다.<br>(공백으로 두시면 암호 없는 공개 모드로 동작합니다)",
                     "Wi-Fi password must be at least 8 characters.<br>(Leave blank for open public mode)",
                     "#ff5252");
      return;
    }

    prefs.begin("bc250_cfg", false);
    prefs.putString("ap_pass", newPass);
    prefs.end();
    apPass = newPass;

    needApReload = true;
    apReloadStartTime = millis();

    sendResultPage("Wi-Fi 접속 설정 적용 중", "Applying Wi-Fi AP Settings",
                   "새 비밀번호가 저장되었습니다!<br><br>"
                   "잠시 후 <strong>PC 구동에 영향 없이 Wi-Fi만 재시작</strong>됩니다.<br>"
                   "Wi-Fi 연결이 끊어지면 <strong>새 설정으로 다시 연결</strong>해 주세요.",
                   "New password saved!<br><br>"
                   "Wi-Fi will restart shortly <strong>without affecting PC power</strong>.<br>"
                   "Please reconnect to Wi-Fi with the <strong>new settings</strong>.",
                   "#4caf50");
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleSave() {
  if (!checkApAccess()) return;

  if (server.hasArg("ds4_mac") && server.hasArg("xbox_mac") && server.hasArg("dongle_mac")) {
    String ds4 = normalizeMac(server.arg("ds4_mac"));
    String xbox = normalizeMac(server.arg("xbox_mac"));
    String dongle = normalizeMac(server.arg("dongle_mac"));

    prefs.begin("bc250_cfg", false);
    prefs.putString("ds4_mac", ds4);
    prefs.putString("xbox_mac", xbox);
    prefs.putString("dongle_mac", dongle);
    prefs.end();

    if (currentState != STATE_STANDBY || digitalRead(PIN_TPM_IN) == HIGH) {
      needRestartOnStandby = true;
      sendResultPage("설정 저장 완료 (적용 예약됨)", "Settings Saved (Scheduled)",
                     "PC 구동 유지를 위해 <strong>전원 제어기를 지금 재부팅하지 않습니다.</strong><br><br>"
                     "<strong>PC를 정상 종료하시면 전원 제어기가 자동으로 재부팅되어 새 설정이 적용됩니다.</strong>",
                     "To keep the PC running, <strong>the controller will not reboot now.</strong><br><br>"
                     "<strong>When the PC shuts down, the controller will automatically reboot to apply the new settings.</strong>");
    } else {
      sendResultPage("설정이 저장되었습니다", "Settings Saved",
                     "1초 후 전원 제어기가 재부팅됩니다...",
                     "The controller will reboot in 1 second...");
      delay(1000);
      safeRestart();
    }
  } else {
    server.send(400, "text/plain", "Bad Request");
  }
}

void handleSaveWol() {
  if (!checkApAccess()) return;

  bool newWolEn = server.hasArg("wol_enabled");
  String newSsid = server.hasArg("wifi_ssid") ? server.arg("wifi_ssid") : "";
  String newPass = server.hasArg("wifi_pass") ? server.arg("wifi_pass") : "";
  newSsid.trim();
  newPass.trim();

  prefs.begin("bc250_cfg", false);
  prefs.putBool("wol_en", newWolEn);
  prefs.putString("wifi_ssid", newSsid);
  if (newPass.length() > 0) {
    prefs.putString("wifi_pass", newPass);
    wifiPass = newPass;
  }
  prefs.end();

  wolEnabled = newWolEn;
  wifiSsid = newSsid;

  if (currentState == STATE_STANDBY && !isOtaUpdating) {
    if (wolEnabled) startWolStation();
    else stopWolStation();
  }

  sendResultPage("WOL 설정이 즉시 반영되었습니다!", "WOL Settings Applied!",
                 "재부팅 없이 바로 적용되었습니다.<br>잠시 후 메인 화면으로 돌아갑니다...",
                 "Applied immediately without rebooting.<br>Returning to main screen shortly...",
                 "#4caf50", 1);
}

void handleUpdateUpload() {
  if (!checkApAccess()) return;
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    server.client().setTimeout(15000);

    if (currentState == STATE_BOOTING) {
      Update.abort();
      isOtaUpdating = false;
      return;
    }

    isOtaUpdating = true;
    otaLastActiveTime = millis();
    isSignatureMatched = false;
    sigTailLen = 0; 

    if (!upload.filename.endsWith(".bin")) {
      Update.abort();
      isOtaUpdating = false;
      return;
    }

    killBluetoothStack();
    wolUdp7.stop();
    wolUdp9.stop();
    wolUdp2304.stop();
    udpReady = false;
    WiFi.disconnect(false, true);
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);

    size_t updateSize = UPDATE_SIZE_UNKNOWN;
    if (server.hasArg("size")) {
      updateSize = (size_t)server.arg("size").toInt();
    }

    if (!Update.begin(updateSize)) {
      Update.printError(realSerial);
      isOtaUpdating = false;
    }
  }
  else if (upload.status == UPLOAD_FILE_WRITE) {
    otaLastActiveTime = millis();

    if (!Update.hasError()) {
      if (upload.totalSize == 0 && upload.buf[0] != 0xE9) {
        Update.abort();
        isOtaUpdating = false;
        return;
      }

      if (!isSignatureMatched) {
        if (sigTailLen > 0) {
          uint8_t seamBuf[64];
          size_t headLen = (upload.currentSize < 32) ? upload.currentSize : 32;
          memcpy(seamBuf, sigTailBuf, sigTailLen);
          memcpy(seamBuf + sigTailLen, upload.buf, headLen);
          if (memmem(seamBuf, sigTailLen + headLen, PROJECT_SIGNATURE, strlen(PROJECT_SIGNATURE)) != NULL) {
            isSignatureMatched = true;
          }
        }

        if (!isSignatureMatched && memmem(upload.buf, upload.currentSize, PROJECT_SIGNATURE, strlen(PROJECT_SIGNATURE)) != NULL) {
          isSignatureMatched = true;
        }

        if (upload.currentSize >= 32) {
          memcpy(sigTailBuf, upload.buf + (upload.currentSize - 32), 32);
          sigTailLen = 32;
        } else {
          memcpy(sigTailBuf, upload.buf, upload.currentSize);
          sigTailLen = upload.currentSize;
        }
      }

      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(realSerial);
      }
      yield();
    }
  }
  else if (upload.status == UPLOAD_FILE_END) {
    otaLastActiveTime = millis();

    if (!isSignatureMatched) {
      Update.abort();
      isOtaUpdating = false;
      return;
    }

    if (!Update.end(true)) {
      Update.printError(realSerial);
      isOtaUpdating = false;
    }
  }
}

void handleUpdateEnd() {
  if (!checkApAccess()) return;
  server.sendHeader("Connection", "close");
  bool error = Update.hasError();

  if (error) {
    isOtaUpdating = false;
    sendResultPage("업데이트 실패!", "Update Failed!",
                   "서명 불일치 또는 손상된 파일입니다.",
                   "Invalid signature or corrupted binary file.",
                   "#ff5252");
    if (currentState == STATE_STANDBY) {
      delay(1000);
      safeRestart();
    }
    return;
  }

  if (currentState == STATE_RUNNING || digitalRead(PIN_TPM_IN) == HIGH) {
    needRestartOnStandby = true;
    isOtaUpdating = false;
    sendResultPage("펌웨어 기록 완료 (예약됨)", "Firmware Update Saved (Scheduled)",
                   "새 펌웨어가 플래시에 저장되었습니다.<br>"
                   "PC 구동 유지를 위해 <strong>전원 제어기를 지금 재부팅하지 않습니다.</strong><br><br>"
                   "<strong>PC를 정상 종료하시면 전원 제어기가 자동으로 재부팅되어 새 설정이 적용됩니다.</strong>",
                   "New firmware written to flash.<br>Controller reboot scheduled upon normal PC shutdown.");
  } else {
    sendResultPage("업데이트 성공!", "Update Success!",
                   "1초 후 전원 제어기가 안전 재부팅되어 적용됩니다...",
                   "Rebooting safely in 1 second to apply new firmware...");
    delay(1000);
    safeRestart();
  }
}

// ============================================================
// 12. setup (하드웨어 초기화 및 엔드포인트 바인딩)
// 12. Setup (Hardware Initialization & Endpoint Binding)
// ============================================================
void setup() {
  pinMode(PIN_TPM_IN, INPUT);
  pinMode(PIN_SWITCH_IN, INPUT_PULLUP);
  bool pcAlreadyRunning = (digitalRead(PIN_TPM_IN) == HIGH);

  gpio_set_level((gpio_num_t)PIN_PSON_OUT, pcAlreadyRunning ? 0 : 1);
  pinMode(PIN_PSON_OUT, OUTPUT_OPEN_DRAIN);

  Serial.begin(115200);
  delay(1000);

  LOG_PRINTLN(F("=========================================="));
  LOG_PRINTF (" BC-250 Dual-Pad Controller %s\n", FIRMWARE_VERSION);
  LOG_PRINTLN(F("=========================================="));

  const esp_partition_t *runningPart = esp_ota_get_running_partition();
  esp_ota_img_states_t otaState;
  if (runningPart && esp_ota_get_state_partition(runningPart, &otaState) == ESP_OK) {
    if (otaState == ESP_OTA_IMG_PENDING_VERIFY) {
      LOG_PRINTLN(F("[OTA] 새 펌웨어 검증 대기 중 (15초 후 영구 확정) / Pending firmware verification (Confirmed after 15s)"));
    }
  }

  WiFi.persistent(false);
  loadSettings();

  if (apPass.length() >= 8) {
    WiFi.softAP(DEFAULT_AP_SSID, apPass.c_str(), 1, 0, AP_MAX_CLIENTS);
  } else {
    WiFi.softAP(DEFAULT_AP_SSID, NULL, 1, 0, AP_MAX_CLIENTS);
  }

  // 웹 원격 전원 켜기(Power On) / 차단(Shut Off) 엔드포인트 / Web Remote Power On / Shut Off Endpoint
  server.on("/api/power_ctl", HTTP_POST, []() {
    if (!checkApAccess()) return;
    if (!server.hasArg("action")) {
      server.send(400, "text/plain", "Missing action");
      return;
    }
    String act = server.arg("action");
    if (act.equalsIgnoreCase("on")) {
      if (currentState == STATE_STANDBY) {
        triggerReason = "웹 원격 제어 (Power On) / Web Remote Control (Power On)";
        startBootSequence();
        server.send(200, "text/plain", "OK");
      } else {
        server.send(409, "text/plain", "Not in STANDBY");
      }
    } else if (act.equalsIgnoreCase("off")) {
      if (currentState == STATE_RUNNING) {
        LOG_PRINTLN(F("[WEB] 웹 원격 전원 차단(Shut Off) 수신 -> 전원 즉시 차단 / Web Power Shut Off received -> Cutting power immediately"));
        server.send(200, "text/plain", "OK");
        delay(200);
        stopPowerSupply();
        delay(800);
        safeRestart();
      } else {
        server.send(409, "text/plain", "Not in RUNNING");
      }
    } else {
      server.send(400, "text/plain", "Invalid action");
    }
  });

  server.on("/api/prepare_ota", HTTP_POST, []() {
    if (!checkApAccess()) return;
    isOtaUpdating = true;
    otaLastActiveTime = millis();
    killBluetoothStack();
    stopWolStation();
    WiFi.setSleep(false);
    server.send(200, "text/plain", "READY");
  });

  server.on("/api/last_detected_pads", HTTP_GET, []() {
    if (!checkApAccess()) return;
    String ds4 = (lastDetectedDs4Time > 0 && (millis() - lastDetectedDs4Time <= 15000UL)) ? lastDetectedDs4Mac : "";
    String xbox = (lastDetectedXboxTime > 0 && (millis() - lastDetectedXboxTime <= 15000UL)) ? lastDetectedXboxMac : "";
    char json[128];
    snprintf(json, sizeof(json), "{\"ds4\":\"%s\",\"xbox\":\"%s\"}", ds4.c_str(), xbox.c_str());
    server.send(200, "application/json", json);
  });

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/save_ap", HTTP_POST, handleSaveAp);
  server.on("/save_wol", HTTP_POST, handleSaveWol);
  server.on("/update", HTTP_POST, handleUpdateEnd, handleUpdateUpload);

  server.on("/api/logs", HTTP_GET, []() {
    if (!checkApAccess()) return;
    if (isOtaUpdating) { server.send(204); return; } 
    server.send(200, "text/plain; charset=utf-8", getWebLogs());
  });
  server.on("/api/send_cmd", HTTP_POST, []() {
    if (!checkApAccess()) return;
    if (server.hasArg("cmd")) {
      executeCommand(server.arg("cmd"));
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Missing cmd");
    }
  });
  server.on("/api/clear_logs", HTTP_POST, []() {
    if (!checkApAccess()) return;
    clearWebLogs();
    server.send(200, "text/plain", "OK");
  });

  server.onNotFound([]() {
    if (!checkApAccess()) return;
    server.send(404, "text/plain", "Not Found");
  });
  server.begin();

  esp_err_t macErr = esp_iface_mac_addr_set(DONGLE_MAC, ESP_MAC_BT);
  LOG_PRINTF("[RADIO] PC BT MAC 복제 결과 / PC BT MAC Clone Result: %s\n", esp_err_to_name(macErr));

  esp_bt_controller_config_t btCfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  if (esp_bt_controller_init(&btCfg) == ESP_OK) {
    if (esp_bt_controller_enable(ESP_BT_MODE_BTDM) == ESP_OK) {
      esp_bluedroid_init();
      esp_bluedroid_enable();
      esp_bt_gap_register_callback(bt_gap_callback);
    }
  }

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new XboxSnifferCallbacks());
  pBLEScan->setActiveScan(false);
  pBLEScan->setInterval(80);
  pBLEScan->setWindow(60);

  if (digitalRead(PIN_TPM_IN) == HIGH) {
    startPowerSupply();
    currentState = STATE_RUNNING;
    stopWolStation();
    killBluetoothStack();
  } else {
    enterStandby();
  }

  if (runningPart && esp_ota_get_state_partition(runningPart, &otaState) == ESP_OK) {
    if (otaState == ESP_OTA_IMG_PENDING_VERIFY) {
      if (!runSelfDiagnostics()) {
        rollbackAndReboot("초기화 자가진단 실패 / Initialization Self-Diagnostics Failed");
      }
    }
  }
}

// ============================================================
// 13. loop (메인 루프 & 전면 다기능 버튼 제어)
// 13. Loop (Main Loop & Front Multi-Function Button Control)
// ============================================================
void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    executeCommand(cmd);
  }

  server.handleClient();

  if (needApReload && (millis() - apReloadStartTime >= 1000UL)) {
    needApReload = false;
    WiFi.softAPdisconnect(true);
    delay(100);

    if (apPass.length() >= 8) {
      WiFi.softAP(DEFAULT_AP_SSID, apPass.c_str(), 1, 0, AP_MAX_CLIENTS);
    } else {
      WiFi.softAP(DEFAULT_AP_SSID, NULL, 1, 0, AP_MAX_CLIENTS);
    }
  }

  if (!otaConfirmed && millis() > 15000UL) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    if (running && esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
      if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
        if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
          otaConfirmed = true;
          LOG_PRINTLN(F("[OTA] 15초 안정 구동 확인 -> 펌웨어 영구 확정 / 15s stable operation verified -> Firmware confirmed permanently"));
        }
      } else {
        otaConfirmed = true;
      }
    }
  }

  if (isOtaUpdating) {
    if ((millis() - otaLastActiveTime) > 90000UL) {
      isOtaUpdating = false;
      delay(500);
      safeRestart();
    }
    delay(2);
    return;
  }

  updateSwitch();
  const bool tpmLevel = (digitalRead(PIN_TPM_IN) == HIGH);

  if (currentState == STATE_STANDBY) {
    static unsigned long lastBleCleanTime = 0;
    if (millis() - lastBleCleanTime >= 30000UL) {
      lastBleCleanTime = millis();
      if (pBLEScan != nullptr) pBLEScan->clearResults();
    }

    if (!xboxArmed && ((millis() - lastXboxSeenTime) >= XBOX_SILENCE_TIME)) {
      xboxArmed = true;
      LOG_PRINTLN(F("[BLE] 엑박 패드 침묵 확인 -> 트리거 무장 완료 / Xbox controller silence verified -> Trigger armed"));
    }

    if (wolEnabled && !powerTriggerPending) {
      if (WiFi.status() == WL_CONNECTED) {
        if (!udpReady) {
          wolUdp7.stop(); wolUdp9.stop(); wolUdp2304.stop();
          wolUdp7.begin(7); wolUdp9.begin(9); wolUdp2304.begin(2304);
          udpReady = true;
          LOG_PRINTF("[WOL] 소켓 가동 완료 / Sockets Ready (IP: %s)\n", WiFi.localIP().toString().c_str());
        }
      } else {
        udpReady = false;
      }

      if (udpReady) {
        uint8_t myMac[6];
        esp_read_mac(myMac, ESP_MAC_WIFI_STA);
        if (processUdpSocket(wolUdp7, 7, myMac) ||
            processUdpSocket(wolUdp9, 9, myMac) ||
            processUdpSocket(wolUdp2304, 2304, myMac)) {
          powerTriggerPending = true;
        }
      }
    }

    if (!waitForSwitchRelease) {
      if (switchStableState == LOW) {
        if (!switchWasPressed) {
          switchWasPressed = true;
          switchPressStartTime = millis();
        } else if ((millis() - switchPressStartTime) >= WIFI_RESET_HOLD_TIME) {
          LOG_PRINTLN(F("\n[RESET] 전면 버튼 3초 롱프레스 감지 -> Wi-Fi 비밀번호 초기화! / Front button 3s hold -> Wi-Fi password reset!"));
          prefs.begin("bc250_cfg", false);
          prefs.remove("ap_pass");
          prefs.end();
          apPass = "";

          needApReload = true;
          apReloadStartTime = millis();
          switchWasPressed = false;
          waitForSwitchRelease = true;
        }
      } else {
        if (switchWasPressed) {
          switchWasPressed = false;
          triggerReason = "물리 버튼 / Physical Button";
          startBootSequence();
        }
      }
    }

    if (powerTriggerPending) {
      startBootSequence();
    }
  }
  else if (currentState == STATE_BOOTING) {
    if (tpmLevel) {
      LOG_PRINTLN(F("[TPM] 3.3V 확인 -> PC 정상 구동(RUNNING) 진입 / 3.3V confirmed -> PC RUNNING entered"));
      currentState = STATE_RUNNING;
      switchWasPressed = false;
      waitForSwitchRelease = (switchStableState == LOW);
      tpmLowPending = false;
      stopWolStation();
      killBluetoothStack();
    } else if ((millis() - bootStartTime) >= BOOT_TIMEOUT) {
      LOG_PRINTLN(F("[ERROR] TPM 응답 타임아웃 -> 대기 모드 복귀 / TPM response timeout -> Returning to STANDBY"));
      enterStandby();
    }
  }
  else if (currentState == STATE_RUNNING) {
    if (!tpmLevel) {
      if (!tpmLowPending) {
        tpmLowPending = true;
        tpmLowStartTime = millis();
      } else if ((millis() - tpmLowStartTime) >= TPM_OFF_CONFIRM_TIME) {
        if (digitalRead(PIN_TPM_IN) == LOW) {
          LOG_PRINTLN(F("[TPM] LOW 신호 확정 -> PC 정상 종료 감지 -> 안전 재부팅 / LOW confirmed -> PC Shutdown detected -> Safe reboot"));
          stopPowerSupply();
          delay(800);
          safeRestart();
        } else {
          tpmLowPending = false;
        }
      }
    } else {
      tpmLowPending = false;
    }

    checkEmergencyLongPress();
  }

  delay(2);
}