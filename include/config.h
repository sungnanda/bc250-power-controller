#pragma once
#include <Arduino.h>

// ============================================================
// 0. 펌웨어 버전 및 보안 식별 시그니처
// 0. Firmware Version & Project Identification Signature
// ============================================================
#define FIRMWARE_VERSION    "Ver.1.0"
static const char PROJECT_SIGNATURE[] __attribute__((used)) = "@@BC250_DUAL_PAD_TRIGGER_V1@@";

// ============================================================
// 1. 하드웨어 핀 맵 (GPIO)
// 1. Hardware Pin Map (GPIO)
// ============================================================
#define PIN_TPM_IN          14   // BC-250 TPM 헤더 3.3V 감지 (PC 구동 감지) / BC-250 TPM Header 3.3V Detection (PC Power Sense)
#define PIN_PSON_OUT        16   // ATX 파워서플라이 24핀 PS_ON (오픈 드레인) / ATX PSU 24-Pin PS_ON (Open-Drain)
#define PIN_SWITCH_IN       17   // 케이스 전면 모멘터리 버튼 (내부 풀업) / Case Front Momentary Button (Internal Pull-Up)

// ============================================================
// 2. 기본 무선 통신 설정 (NVS 공장 출하 기본값)
// 2. Default Wireless Configuration (NVS Factory Defaults)
// ============================================================
#define DEFAULT_AP_SSID     "BC250-PowerController"
#define AP_MAX_CLIENTS      2

// 기본 페어링 MAC 주소 / Default Pairing MAC Addresses
#define DEFAULT_DS4_MAC     "AA:BB:CC:DD:EE:FF"
#define DEFAULT_XBOX_MAC    "11:22:33:44:55:66"
#define DEFAULT_DONGLE_MAC  "00:00:00:00:00:00"

// ============================================================
// 3. 상태 머신 타이밍 상수 (단위: ms)
// 3. State Machine Timing Constants (Unit: ms)
// ============================================================
const unsigned long BOOT_TIMEOUT             = 15000UL; // 부팅 시도 시 TPM 3.3V 대기 한계 / Boot Timeout Waiting for TPM 3.3V
const unsigned long TPM_OFF_CONFIRM_TIME     = 500UL;   // PC 정상 셧다운 확정 디바운스 / PC Shutdown Confirmation Debounce
const unsigned long LONG_PRESS_TIME          = 5000UL;  // 전면 버튼 5초: 강제 전원 차단 / Front Button 5s: Emergency Power Cut
const unsigned long WIFI_RESET_HOLD_TIME     = 3000UL;  // 전면 버튼 3초: AP 암호 초기화 / Front Button 3s: AP Password Reset
const unsigned long SWITCH_DEBOUNCE_TIME     = 30UL;    // 스위치 채터링 방지 디바운스 / Switch Debounce Time
const unsigned long XBOX_SILENCE_TIME        = 4000UL;  // 엑박 패드 트리거 무장 침묵 시간 / Xbox Controller Silence Armed Time

// ============================================================
// 4. 버퍼 및 시스템 규격
// 4. Buffer & System Specifications
// ============================================================
#define WEB_LOG_SIZE        2048                        // 웹 콘솔 링 버퍼 크기 / Web Console Ring Buffer Size