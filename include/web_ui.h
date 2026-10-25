#pragma once
#include <Arduino.h>

// ============================================================
// 대시보드 웹 UI 프론트엔드 (HTML/CSS/JS)
// Dashboard Web UI Frontend (HTML/CSS/JS)
// ============================================================
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ko">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>BC-250 Power Controller</title>
  <style>
    * { box-sizing: border-box; }
    body { 
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; 
      background: #121212; 
      color: #e0e0e0; 
      margin: 0; 
      padding: 8px; 
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
    }
    .container { 
      width: 100%; 
      max-width: 520px; 
      background: #1c1c1c; 
      border-radius: 10px; 
      padding: 12px 14px 14px; 
      box-shadow: 0 8px 30px rgba(0,0,0,0.6); 
      border: 1px solid #2d2d2d;
    }
    .top-header { 
      display: flex; 
      justify-content: space-between; 
      align-items: center; 
      margin-bottom: 11px;
      gap: 8px;
    }
    .header-left {
      display: flex;
      flex-direction: column;
      min-width: 0;
    }
    h2 { 
      margin: 0; 
      color: #E2E8F0; 
      font-size: 18.5px; 
      font-weight: 700; 
      letter-spacing: -0.2px;
      white-space: nowrap;
    }
    @media (min-width: 440px) {
      h2 { font-size: 22px; letter-spacing: 0.2px; }
    }
    .ver-sub {
      font-size: 11px;
      color: #94A3B8;
      font-family: monospace;
      font-weight: 600;
      margin-top: 2px;
      white-space: nowrap;
    }
    .lang-wrap {
      display: inline-flex;
      align-items: center;
      gap: 5px;
      background: #262626;
      border: 1px solid #444;
      border-radius: 5px;
      padding: 0 6px;
      height: 27px;
      flex-shrink: 0;
    }
    .lang-icon {
      width: 13px;
      height: 13px;
      color: #94A3B8;
      flex-shrink: 0;
    }
    .lang-select {
      height: 100%;
      background: transparent;
      color: #ddd;
      border: none;
      font-size: 11.5px;
      padding: 0;
      cursor: pointer;
      outline: none;
    }
    .lang-select option {
      background-color: #222222;
      color: #ffffff;
    }
    .card { background: #262626; border-radius: 7px; padding: 7px 11px 9px; margin-bottom: 7px; border: 1px solid #333; }
    .card h3 { margin: 0 0 5px 0; font-size: 14px; font-weight: 600; line-height: 1.2; }
    .status-card-inner {
      display: flex;
      flex-direction: column;
      gap: 10px;
    }
    .status-info-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .status-badge { padding: 4px 8px; border-radius: 4px; font-weight: bold; background: #388E3C; color: #fff; font-size: 12px; }
    .status-info { font-size: 11.5px; color: #aaa; margin-top: 3px; }
    .btn-power-ctl {
      width: 100%;
      height: 44px;
      margin: 0;
      padding: 0 16px;
      font-size: 16px;
      font-weight: 700;
      border-radius: 8px;
      border: 2px solid transparent;
      cursor: pointer;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
      white-space: nowrap;
      flex-shrink: 0;
      transition: all 0.2s ease;
      box-shadow: 0 3px 8px rgba(0,0,0,0.4);
    }
    .btn-power-ctl:active { transform: scale(0.97); }
    .btn-power-ctl.pwr-on { background: #1976D2; border-color: #42A5F5; color: #fff; }
    .btn-power-ctl.pwr-on:hover { background: #1565C0; }
    .btn-power-ctl.pwr-off { background: #D32F2F; border-color: #EF5350; color: #fff; }
    .btn-power-ctl.pwr-off:hover { background: #C62828; }
    .btn-power-ctl.pwr-booting { background: #333; border-color: #555; color: #888; cursor: not-allowed; box-shadow: none; }
    .pwr-svg { width: 20px; height: 20px; stroke: currentColor; stroke-width: 2.8; fill: none; stroke-linecap: round; stroke-linejoin: round; }
    @media (min-width: 440px) {
      .status-card-inner {
        flex-direction: row;
        justify-content: space-between;
        align-items: center;
        min-height: 48px;
      }
      .btn-power-ctl {
        width: auto !important;
        min-height: 42px;
        padding: 0 16px;
        font-size: 16px;
      }
    }
    .svg-icon { stroke: currentColor; fill: none; stroke-width: 2; stroke-linecap: round; stroke-linejoin: round; }
    .notice-box { 
      background: #2a2000; 
      border-left: 3px solid #ffb300; 
      padding: 5px 8px; 
      font-size: 11.5px; 
      color: #ffd54f; 
      line-height: 1.35; 
      margin-bottom: 5px; 
      border-radius: 0 4px 4px 0; 
      display: flex;
      align-items: flex-start;
      gap: 6px;
    }
    .notice-icon { width: 15px; height: 15px; flex-shrink: 0; margin-top: 1px; color: #ffb300; }
    .notice-content { flex: 1; }
    .warn-box { 
      background: #331400; 
      border-left: 3px solid #ff9800; 
      padding: 5px 8px; 
      font-size: 11.5px; 
      color: #ffb74d; 
      line-height: 1.35; 
      margin-bottom: 5px; 
      border-radius: 0 4px 4px 0; 
      display: flex;
      align-items: flex-start;
      gap: 6px;
    }
    label { display: block; margin: 4px 0 2px; font-size: 12px; color: #bbb; }
    input[type="text"], input[type="password"] { width: 100%; height: 32px; padding: 3px 8px; background: #141414; border: 1px solid #444; border-radius: 5px; color: #fff; font-family: monospace; font-size: 13.5px; }
    input[type="text"]:focus, input[type="password"]:focus { border-color: #4CAF50; outline: none; }
    .pass-wrap { position: relative; width: 100%; display: flex; align-items: center; }
    .pass-wrap input { padding-right: 36px; }
    .pass-icon-btn { 
      position: absolute; 
      right: 8px; 
      background: none; 
      border: none; 
      padding: 0; 
      width: 22px; 
      height: 22px; 
      cursor: pointer; 
      color: #777; 
      display: flex; 
      align-items: center; 
      justify-content: center; 
      transition: color 0.2s; 
      outline: none;
    }
    .pass-icon-btn:hover { color: #4CAF50; }
    .pass-icon-btn svg { width: 18px; height: 18px; }
    .btn-sub-action {
      display: inline-flex;
      align-items: center;
      gap: 5px;
      cursor: pointer;
      border-radius: 4px;
      font-size: 11.5px;
      font-weight: 600;
      white-space: nowrap;
      flex-shrink: 0;
      transition: all 0.2s;
    }
    .btn-sniff-action { border: 1px solid #1976D2; background: #0d2238; color: #64B5F6; }
    .btn-sniff-action:hover { background: #153252; border-color: #42A5F5; }
    .btn-scan-action { border: 1px solid #00ACC1; background: #00363a; color: #4DD0E1; }
    .btn-scan-action:hover { background: #004d40; border-color: #26C6DA; color: #E0F7FA; }
    .btn-apply-mac {
      width: auto;
      height: 24px;
      padding: 0 10px;
      font-size: 11px;
      font-weight: 600;
      background: #153252;
      border: 1px solid #2196F3;
      color: #FFFFFF;
      border-radius: 4px;
      cursor: pointer;
      white-space: nowrap;
      flex-shrink: 0;
      transition: background 0.2s, border-color 0.2s;
    }
    .btn-apply-mac:hover {
      background: #1d4572;
      border-color: #64B5F6;
    }
    .btn-file-select {
      border: 1px solid #555;
      background: #333;
      color: #fff;
      padding: 5px 11px;
      border-radius: 5px;
      font-size: 12px;
      font-weight: 600;
      cursor: pointer;
      transition: background 0.2s;
      white-space: nowrap;
    }
    .btn-file-select:hover { background: #444; }
    button.wol-save-btn {
      width: 100%; 
      height: 34px; 
      border: none; 
      border-radius: 5px; 
      color: #fff; 
      font-size: 13.5px; 
      font-weight: bold; 
      cursor: pointer; 
      margin-top: 6px; 
      background: #00897B; 
      transition: background 0.2s;
    }
    button.wol-save-btn:hover { background: #00796B; }
    button.save-btn { width: 100%; height: 34px; border: none; border-radius: 5px; color: #fff; font-size: 13.5px; font-weight: bold; cursor: pointer; margin-top: 5px; background: #4CAF50; transition: opacity 0.2s; }
    button.ota-btn { width: 100%; height: 34px; border: none; border-radius: 5px; color: #fff; font-size: 13.5px; font-weight: bold; cursor: pointer; margin-top: 5px; background: #E91E63; }
    button.ota-btn:disabled { background: #555 !important; cursor: not-allowed; }
    .progress-box { display: none; margin-top: 5px; }
    .progress-track { background: #141414; border-radius: 4px; height: 12px; width: 100%; overflow: hidden; border: 1px solid #444; }
    .progress-bar { background: linear-gradient(90deg, #E91E63, #ff4081); height: 100%; width: 0%; transition: width 0.15s ease; }
    .progress-bar.flashing { animation: pulse 1s infinite alternate; background: #00E676; }
    @keyframes pulse { from { opacity: 0.6; } to { opacity: 1; } }
    .progress-info { font-size: 11px; color: #bbb; text-align: center; margin-top: 3px; line-height: 1.3; }
  </style>
</head>
<body>
  <div class="container">
    <div class="top-header">
      <div class="header-left">
        <h2 data-i18n="title">BC-250 전원 제어기</h2>
        <div class="ver-sub">Ver.1.0</div>
      </div>
      <div class="lang-wrap">
        <svg class="svg-icon lang-icon" viewBox="0 0 24 24">
          <circle cx="12" cy="12" r="10"></circle>
          <line x1="2" y1="12" x2="22" y2="12"></line>
          <path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"></path>
        </svg>
        <select id="lang_select" class="lang-select" onchange="setLanguage(this.value)">
          <option value="ko">한국어</option>
          <option value="en">English</option>
        </select>
      </div>
    </div>
    
    <div class="card">
      <div class="status-card-inner">
        <div>
          <div class="status-info-row">
            <span style="font-size:13.5px; font-weight:bold; white-space:nowrap;" data-i18n="pc_status_card">PC 전원 상태</span>
            <span id="ui_status" class="status-badge">...</span>
          </div>
          <div id="ui_sensor" class="status-info">TPM: ... | Free Heap: ... KB</div>
        </div>
        <button type="button" id="btn_power_ctl" class="btn-power-ctl pwr-on" onclick="handlePowerCtl()" style="display:none;">
          <svg class="pwr-svg" viewBox="0 0 24 24"><path d="M12 2v10M18.36 6.64A9 9 0 1 1 5.64 6.64"/></svg>
          <span id="lbl_power_ctl">전원 켜기</span>
        </button>
      </div>
    </div>

    <div class="card">
      <h3 style="color:#FFB74D;" data-i18n="ap_sec_card">전원 제어기 Wi-Fi AP 접속 보안</h3>
      <div id="ap_warn" class="warn-box" style="display:none;" data-i18n="ap_warn_text"></div>
      <form action="/save_ap" method="POST">
        <label data-i18n="lbl_new_ap_pass">접속 비밀번호 변경 (8자 이상, 공백 저장 시 암호 없는 공개 모드)</label>
        <div style="display:flex; gap:6px;">
          <div class="pass-wrap" style="flex:1;">
            <input type="password" id="ui_new_ap_pass" name="new_ap_pass" minlength="8" placeholder="저장됨">
            <button type="button" id="icon_ap_pass" class="pass-icon-btn" onclick="togglePass('ui_new_ap_pass', 'icon_ap_pass')" title="비밀번호 표시/숨김">
              <svg class="svg-icon" viewBox="0 0 24 24"><path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line></svg>
            </button>
          </div>
          <button type="submit" class="save-btn" style="width:auto; height:32px; padding:0 12px; margin:0; background:#FF9800; white-space:nowrap;" data-i18n="btn_change_ap">비밀번호 변경</button>
        </div>
      </form>
    </div>

    <div class="card">
      <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:4px;">
        <h3 style="margin:0;" data-i18n="mac_card">Controller MAC 주소 설정</h3>
        <button type="button" id="btn_sniff_pad" class="btn-sub-action btn-sniff-action" onclick="sniffPadSignal()" style="height:26px; padding:0 9px;">
          <svg class="svg-icon" style="width:13px; height:13px;" viewBox="0 0 24 24"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>
          <span data-i18n="btn_sniff_pad">컨트롤러 신호 찾기</span>
        </button>
      </div>
      <div id="sniff_status_box" style="display:none; background:#112233; border:1px solid #1976D2; border-radius:5px; padding:7px 9px; margin-bottom:5px; font-size:11.5px; color:#90CAF9; line-height:1.45;"></div>
      
      <div class="notice-box">
        <svg class="svg-icon notice-icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"></circle><line x1="12" y1="16" x2="12" y2="12"></line><line x1="12" y1="8" x2="12.01" y2="8"></line></svg>
        <div class="notice-content" data-i18n="mac_notice"></div>
      </div>

      <form action="/save" method="POST">
        <label data-i18n="lbl_ds4">DualShock 4 / DualSense MAC (Classic BT)</label>
        <input type="text" id="ui_ds4_mac" name="ds4_mac" onblur="formatMacInput(this)" placeholder="예: ACFD93D6F441 또는 AC:FD:93:D6:F4:41" required>
        <label data-i18n="lbl_xbox">Xbox Controller MAC (BLE)</label>
        <input type="text" id="ui_xbox_mac" name="xbox_mac" onblur="formatMacInput(this)" placeholder="예: 9CAA1B86C560 또는 9C:AA:1B:86:C5:60" required>
        <label data-i18n="lbl_dongle">PC 블루투스 MAC 복제</label>
        <input type="text" id="ui_dongle_mac" name="dongle_mac" onblur="formatMacInput(this)" placeholder="예: 001A7DDA7113 또는 00:1A:7D:DA:71:13" required>
        <button type="submit" class="save-btn" data-i18n="btn_save_mac">설정 저장</button>
      </form>
    </div>

    <div class="card">
      <h3 data-i18n="wol_card">WOL 원격 부팅 설정</h3>
      <form action="/save_wol" method="POST">
        <div style="display:flex; align-items:center; margin: 2px 0 5px;">
          <input type="checkbox" id="ui_wol_en" name="wol_enabled" value="1" onchange="toggleWolSection(this.checked)" style="width:15px; height:15px; cursor:pointer;">
          <label for="ui_wol_en" data-i18n="lbl_wol_en" style="margin:0 0 0 5px; font-size:13px; font-weight:bold; cursor:pointer; color:#fff;">WOL 기능 활성화</label>
        </div>
        <div id="wol-section" style="display:none;">
          <div class="notice-box">
            <svg class="svg-icon notice-icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"></circle><line x1="12" y1="16" x2="12" y2="12"></line><line x1="12" y1="8" x2="12.01" y2="8"></line></svg>
            <div class="notice-content" data-i18n="wol_notice"></div>
          </div>

          <div class="status-info" style="margin: 3px 0 5px;"><span data-i18n="lbl_wifi_state">공유기 연결 상태: </span><strong id="ui_wifi_status">...</strong></div>
          <label data-i18n="lbl_esp_mac">WOL 수신 MAC (ESP32 Wi-Fi)</label>
          <input type="text" id="ui_esp_mac" readonly style="background:#181818; color:#4CAF50; font-weight:bold; cursor:default; border-color:#333;">
          <label data-i18n="lbl_ssid">와이파이 선택</label>
          <div style="display:flex; gap:5px; margin-bottom:3px;">
            <select id="wifi_select" style="flex:1; height:32px; background:#141414; border:1px solid #444; border-radius:5px; color:#fff; font-size:12.5px; padding:0 6px;" onchange="handleSsidSelect(this)">
              <option value="" data-i18n="opt_sel_ssid">[와이파이 선택]</option>
              <option value="__DIRECT__" data-i18n="opt_direct_ssid">[직접 입력]</option>
            </select>
            <button type="button" id="scan_btn" class="btn-sub-action btn-scan-action" onclick="scanWifi()" style="height:32px; padding:0 10px; white-space:nowrap;">
              <svg class="svg-icon" style="width:13px; height:13px;" viewBox="0 0 24 24"><polyline points="23 4 23 10 17 10"></polyline><polyline points="1 20 1 14 7 14"></polyline><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path></svg>
              <span data-i18n="btn_scan">검색</span>
            </button>
          </div>
          <input type="text" id="wifi_ssid_input" name="wifi_ssid" placeholder="Wi-Fi SSID">
          <label data-i18n="lbl_pass">와이파이 비밀번호</label>
          <div class="pass-wrap">
            <input type="password" id="ui_wifi_pass" name="wifi_pass" placeholder="저장됨">
            <button type="button" id="icon_wifi_pass" class="pass-icon-btn" onclick="togglePass('ui_wifi_pass', 'icon_wifi_pass')" title="비밀번호 표시/숨김">
              <svg class="svg-icon" viewBox="0 0 24 24"><path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line></svg>
            </button>
          </div>
        </div>
        <button type="submit" class="wol-save-btn" data-i18n="btn_save_wol">WOL 설정 저장 (즉시 적용)</button>
      </form>
    </div>

    <div class="card">
      <div style="display:flex; justify-content:space-between; align-items:center;">
        <h3 style="margin:0; color:#64B5F6;" data-i18n="term_card">웹 시리얼 모니터 콘솔</h3>
        <label style="display:flex; align-items:center; gap:5px; cursor:pointer; margin:0; font-size:12.5px; font-weight:bold; color:#4CAF50;">
          <input type="checkbox" id="chk_terminal" onchange="toggleTerminal(this.checked)" style="width:16px; height:16px; cursor:pointer;">
          <span data-i18n="lbl_term_en">터미널 켜기</span>
        </label>
      </div>
      <div id="terminal_box" style="display:none; margin-top:7px;">
        <div id="term_log" style="background:#0a0a0a; border:1px solid #333; border-radius:5px; padding:7px; font-family:monospace; font-size:11px; color:#00E676; height:180px; overflow-y:auto; white-space:pre-wrap; word-break:break-all;"></div>
        <div style="display:flex; gap:6px; margin-top:6px;">
          <input type="text" id="term_input" placeholder="명령어 입력 (예: status, reboot, help)" style="flex:1; height:32px; background:#141414; border:1px solid #444; border-radius:5px; color:#fff; font-family:monospace; font-size:12.5px; padding:0 8px;" onkeydown="if(event.key==='Enter') sendTermCmd()">
          <button type="button" onclick="sendTermCmd()" style="width:auto; height:32px; margin:0; padding:0 14px; background:#2196F3; color:#ffffff; border:none; outline:none; font-size:12px; font-weight:600; border-radius:5px; cursor:pointer;" data-i18n="btn_send">전송</button>
        </div>
      </div>
    </div>

    <div class="card" style="margin-bottom:0;">
      <h3 style="color:#ff77a9;" data-i18n="ota_card">펌웨어 무선 업데이트</h3>
      <div class="notice-box">
        <svg class="svg-icon notice-icon" viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"></circle><line x1="12" y1="16" x2="12" y2="12"></line><line x1="12" y1="8" x2="12.01" y2="8"></line></svg>
        <div class="notice-content" data-i18n="ota_notice"></div>
      </div>
      <form id="ota-form" onsubmit="uploadFirmware(event)">
        <label data-i18n="lbl_bin_file">바이너리 파일 (.bin)</label>
        
        <div style="display:flex; align-items:center; gap:8px; margin:5px 0 6px;">
          <input type="file" id="fw_file" name="update" accept=".bin" style="display:none;" onchange="updateFileName(this)" required>
          <label for="fw_file" id="btn_file_label" class="btn-file-select" data-i18n="lbl_choose_file">파일 선택</label>
          <span id="file_name_text" style="font-size:12px; color:#888; font-family:monospace;" data-i18n="txt_no_file">선택된 파일 없음</span>
        </div>

        <button type="submit" id="ota-btn" class="ota-btn" data-i18n="btn_ota_standby">업로드 시작</button>
      </form>
      <div id="progress-box" class="progress-box">
        <div class="progress-track"><div id="progress-bar" class="progress-bar"></div></div>
        <div id="progress-info" class="progress-info">0% (0 / 0 KB)</div>
      </div>
    </div>
  </div>

  <script>
    var isFirstLoad = true, lastStatusData = null, currentLang = localStorage.getItem('bc250_lang') || 'ko';
    var termInterval = null, statusInterval = null, isUploading = false, sniffTimer = null;
    var sniffState = { status: 'idle', ds4: '', xbox: '' };

    var SVG_EYE = '<svg class="svg-icon" viewBox="0 0 24 24"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>';
    var SVG_EYE_OFF = '<svg class="svg-icon" viewBox="0 0 24 24"><path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line></svg>';
    var SVG_TOWER = '<svg class="svg-icon" style="width:14px; height:14px; vertical-align:-2px; margin-right:4px; color:#64B5F6;" viewBox="0 0 24 24"><path d="M4.93 4.93a10 10 0 0 1 14.14 0"></path><path d="M7.76 7.76a6 6 0 0 1 8.48 0"></path><circle cx="12" cy="12" r="2"></circle><path d="M12 14v7"></path></svg>';
    var SVG_WARN = '<svg class="svg-icon" style="width:14px; height:14px; vertical-align:-2px; margin-right:4px; color:#ffb74d;" viewBox="0 0 24 24"><path d="m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3Z"></path><line x1="12" y1="9" x2="12" y2="13"></line><line x1="12" y1="17" x2="12.01" y2="17"></line></svg>';

    var I18N = {
      ko: {
        title: "BC-250 전원 제어기", pc_status_card: "PC 전원 상태", 
        ap_sec_card: "전원 제어기 Wi-Fi AP 접속 보안",
        ap_warn_text: SVG_WARN + "<strong>보안 경고:</strong> 현재 비밀번호가 없습니다. 외부 접속 차단을 위해 8자 이상의 비밀번호를 설정해주세요.",
        lbl_new_ap_pass: "접속 비밀번호 변경 (8자 이상, 공백 저장 시 암호 없는 공개 모드)",
        ph_pass_set: "저장됨", ph_pass_open: "암호를 입력하세요 (공개 모드)", ph_pass_empty: "암호를 입력하세요",
        btn_change_ap: "비밀번호 변경",
        btn_pwr_on: "전원 켜기",
        btn_pwr_booting: "부팅 중...",
        btn_pwr_off: "전원 차단",
        confirm_pwr_off: "⚠️ 경고: PC 강제 전원 차단\n\nOS 정상 종료 절차 없이 즉시 전원이 차단됩니다.\n저장되지 않은 데이터가 손실될 수 있습니다.\n\n정말 전원을 차단하시겠습니까?",
        mac_card: "Controller MAC 주소 설정",
        btn_sniff_pad: "컨트롤러 신호 찾기",
        sniff_wait: SVG_TOWER + "<strong>컨트롤러 신호 감시 중 (10초)...</strong><br>등록할 패드의 전원 버튼(PS / Xbox)을 눌러주세요.",
        sniff_found_hdr: SVG_TOWER + "<strong>감지된 신호:</strong>",
        sniff_none: SVG_WARN + "감지된 신호가 없습니다. (패드 전원을 켤 때 신호가 포착됩니다)",
        btn_apply_mac: "적용",
        mac_notice: "<strong>전원 제어기 재부팅 안내:</strong><br>• <strong>PC STANDBY:</strong> 전원 제어기 즉시 재부팅 적용<br>• <strong>PC RUNNING:</strong> PC 종료 시 전원 제어기 자동 재부팅 적용",
        lbl_ds4: "DualShock 4 / DualSense MAC (Classic BT)", lbl_xbox: "Xbox Controller MAC (BLE)", lbl_dongle: "PC 블루투스 MAC 복제", btn_save_mac: "설정 저장",
        wol_card: "WOL 원격 부팅 설정", lbl_wol_en: "WOL 기능 활성화",
        wol_notice: "<strong>Wi-Fi 동작 안내:</strong><br>• <strong>PC 대기(STANDBY):</strong> WOL 패킷 수신을 위해 공유기에 자동 연결됩니다.<br>• <strong>PC 구동(RUNNING):</strong> 공유기 연결이 자동으로 해제됩니다.",
        lbl_wifi_state: "공유기 연결 상태: ", lbl_esp_mac: "WOL 수신 MAC (ESP32 Wi-Fi)", lbl_ssid: "와이파이 선택",
        opt_sel_ssid: "[와이파이 선택]", opt_direct_ssid: "[직접 입력]", btn_scan: "검색", btn_scanning: "검색 중...", lbl_pass: "와이파이 비밀번호",
        btn_save_wol: "WOL 설정 저장 (즉시 적용)",
        term_card: "웹 시리얼 모니터 콘솔", lbl_term_en: "터미널 켜기", btn_send: "전송",
        ota_card: "펌웨어 무선 업데이트",
        ota_notice: "PC 사용 중 업로드 시 PC 꺼짐 방지를 위해 <strong>전원 제어기 재부팅을 보류</strong>하며, <strong>PC 정상 종료 시 전원 제어기가 재부팅되어 적용</strong>됩니다.",
        lbl_bin_file: "바이너리 파일 (.bin)", 
        lbl_choose_file: "파일 선택", 
        txt_no_file: "선택된 파일 없음",
        btn_ota_standby: "업로드 시작 (즉시 재부팅 적용)", btn_ota_running: "업로드 시작 (PC 종료 시 적용)",
        btn_ota_booting: "PC 부팅 진행 중 (잠시 대기)", btn_ota_uploading: "전송 중...", btn_ota_flashing: "플래시 기록 및 검증 중...",
        btn_ota_prep: "메모리 확보 및 플래시 사전 소거 중...",
        ota_flash_notice: "전송 완료! 플래시 기록 및 검증 중입니다.<br><strong style='color:#00E676;'>완료 화면이 나타날 때까지 잠시만 기다려주세요...</strong>",
        wifi_disabled: "WOL 비활성화됨", wifi_running_off: "PC 동작 중 Wi-Fi 꺼짐", wifi_connecting: "연결 시도 중...", wifi_connected: "연결됨", cur_setting: "(현재 설정)"
      },
      en: {
        title: "BC-250 Power Controller", pc_status_card: "PC Power Status",
        ap_sec_card: "Controller Wi-Fi AP Security",
        ap_warn_text: SVG_WARN + "<strong>Security Warning:</strong> No password set! Set a password (8+ chars) to prevent unauthorized access.",
        lbl_new_ap_pass: "Change AP Password (8+ chars, empty for open mode)",
        ph_pass_set: "Saved", ph_pass_open: "Enter password (open mode)", ph_pass_empty: "Enter password",
        btn_change_ap: "Change Password",
        btn_pwr_on: "Power On",
        btn_pwr_booting: "Booting...",
        btn_pwr_off: "Shut Off",
        confirm_pwr_off: "⚠️ Warning: Forced PC Power Cutoff\n\nPower will be cut immediately without normal OS shutdown.\nUnsaved data may be lost.\n\nAre you sure you want to cut off power?",
        mac_card: "Controller MAC Address Settings",
        btn_sniff_pad: "Scan for Controller",
        sniff_wait: SVG_TOWER + "<strong>Listening for Signal (10s)...</strong><br>Press the power button (PS / Xbox) on your controller.",
        sniff_found_hdr: SVG_TOWER + "<strong>Detected Signal:</strong>",
        sniff_none: SVG_WARN + "No signal detected. (Turn on the pad to broadcast signal)",
        btn_apply_mac: "Apply",
        mac_notice: "<strong>Controller Reboot Info:</strong><br>• <strong>PC STANDBY:</strong> Controller reboots immediately<br>• <strong>PC RUNNING:</strong> Controller reboots after PC shuts down",
        lbl_ds4: "DualShock 4 / DualSense MAC (Classic BT)", lbl_xbox: "Xbox Controller MAC (BLE)", lbl_dongle: "Cloned PC Bluetooth MAC", btn_save_mac: "Save Settings",
        wol_card: "WOL Remote Boot Settings", lbl_wol_en: "Enable WOL Function",
        wol_notice: "<strong>Wi-Fi Operation Info:</strong><br>• <strong>PC STANDBY:</strong> Automatically connects to router to listen for WOL packets.<br>• <strong>PC RUNNING:</strong> Router connection is automatically disconnected.",
        lbl_wifi_state: "Router Connection: ", lbl_esp_mac: "WOL Target MAC (ESP32 Wi-Fi)", lbl_ssid: "Select Wi-Fi",
        opt_sel_ssid: "[Select Wi-Fi SSID]", opt_direct_ssid: "[Direct Input]", btn_scan: "Scan", btn_scanning: "Scanning...", lbl_pass: "Wi-Fi Password",
        btn_save_wol: "Save WOL Settings (Apply Immediately)",
        term_card: "Web Serial Console", lbl_term_en: "Enable Terminal", btn_send: "Send",
        ota_card: "Firmware OTA Update",
        ota_notice: "When uploaded while PC is in use, <strong>reboot is deferred</strong> to prevent PC shutdown, and <strong>applied automatically upon PC shutdown</strong>.",
        lbl_bin_file: "Binary File (.bin)", 
        lbl_choose_file: "Choose File", 
        txt_no_file: "No file chosen",
        btn_ota_standby: "Start Upload (Reboot Immediately)", btn_ota_running: "Start Upload (Apply on PC Shutdown)",
        btn_ota_booting: "PC Booting (Please wait)", btn_ota_uploading: "Uploading...", btn_ota_flashing: "Writing to Flash & Verifying...",
        btn_ota_prep: "Securing memory & pre-erasing flash...",
        ota_flash_notice: "Upload complete! Writing to flash & verifying.<br><strong style='color:#00E676;'>Please wait until the completion screen appears...</strong>",
        wifi_disabled: "WOL Disabled", wifi_running_off: "Wi-Fi off while PC is running", wifi_connecting: "Connecting...", wifi_connected: "Connected", cur_setting: "(Current)"
      }
    };

    function renderSniffBox() {
      var box = document.getElementById('sniff_status_box');
      if (!box || sniffState.status === 'idle') return;
      var dict = I18N[currentLang] || I18N['ko'];

      if (sniffState.status === 'waiting') {
        box.innerHTML = dict.sniff_wait;
      } else if (sniffState.status === 'found') {
        var html = '<div style="margin-bottom:6px;">' + dict.sniff_found_hdr + '</div>';
        if (sniffState.ds4) {
          html += '<div style="display:flex; justify-content:space-between; align-items:center; margin-top:5px; gap:8px;">' +
                  '<span style="white-space:nowrap; overflow:hidden; text-overflow:ellipsis;">• Classic BT: <strong style="color:#00E676; font-family:monospace;">' + sniffState.ds4 + '</strong></span>' +
                  '<button type="button" class="btn-apply-mac" onclick="document.getElementById(\'ui_ds4_mac\').value=\'' + sniffState.ds4 + '\'">' + dict.btn_apply_mac + '</button>' +
                  '</div>';
        }
        if (sniffState.xbox) {
          html += '<div style="display:flex; justify-content:space-between; align-items:center; margin-top:5px; gap:8px;">' +
                  '<span style="white-space:nowrap; overflow:hidden; text-overflow:ellipsis;">• BLE (Xbox): <strong style="color:#00E676; font-family:monospace;">' + sniffState.xbox + '</strong></span>' +
                  '<button type="button" class="btn-apply-mac" onclick="document.getElementById(\'ui_xbox_mac\').value=\'' + sniffState.xbox + '\'">' + dict.btn_apply_mac + '</button>' +
                  '</div>';
        }
        box.innerHTML = html;
      } else if (sniffState.status === 'none') {
        box.innerHTML = dict.sniff_none;
      }
    }

    function setLanguage(lang) {
      currentLang = lang; localStorage.setItem('bc250_lang', lang); document.getElementById('lang_select').value = lang;
      var dict = I18N[lang] || I18N['ko'];
      document.querySelectorAll('[data-i18n]').forEach(function(el) { var key = el.getAttribute('data-i18n'); if (dict[key]) el.innerHTML = dict[key]; });

      var curOpt = document.getElementById("opt_cur_wifi");
      if (curOpt && lastStatusData && lastStatusData.ssid) {
        curOpt.text = lastStatusData.ssid + " " + dict.cur_setting;
      }

      var fwFile = document.getElementById("fw_file");
      var textSpan = document.getElementById("file_name_text");
      if (textSpan && (!fwFile.files || fwFile.files.length === 0)) {
        textSpan.innerText = dict.txt_no_file;
        textSpan.style.color = "#888";
      }

      renderSniffBox();
      if (lastStatusData) applyStatusData(lastStatusData);
    }

    function togglePass(inputId, btnId) {
      var input = document.getElementById(inputId);
      var btn = document.getElementById(btnId);
      if (input.type === "password") {
        input.type = "text";
        btn.innerHTML = SVG_EYE;
        btn.style.color = "#4CAF50";
      } else {
        input.type = "password";
        btn.innerHTML = SVG_EYE_OFF;
        btn.style.color = "#777";
      }
    }

    function updateFileName(input) {
      var textSpan = document.getElementById("file_name_text");
      var dict = I18N[currentLang] || I18N['ko'];
      if (input.files && input.files.length > 0) {
        textSpan.innerText = input.files[0].name;
        textSpan.style.color = "#00E676";
      } else {
        textSpan.innerText = dict.txt_no_file;
        textSpan.style.color = "#888";
      }
    }

    function handlePowerCtl() {
      if (!lastStatusData) return;
      var state = lastStatusData.raw_state;
      var dict = I18N[currentLang] || I18N['ko'];

      if (state === 0) {
        fetch('/api/power_ctl?action=on', { method: 'POST' }).then(function() { setTimeout(fetchStatus, 300); });
      } else if (state === 2) {
        if (confirm(dict.confirm_pwr_off)) {
          fetch('/api/power_ctl?action=off', { method: 'POST' }).then(function() { setTimeout(fetchStatus, 1500); });
        }
      }
    }

    function sniffPadSignal() {
      var box = document.getElementById('sniff_status_box');
      box.style.display = 'block';
      sniffState = { status: 'waiting', ds4: '', xbox: '' };
      renderSniffBox();
      
      var count = 0;
      if (sniffTimer) clearInterval(sniffTimer);
      sniffTimer = setInterval(function() {
        count++;
        fetch('/api/last_detected_pads').then(function(r){ return r.json(); }).then(function(d){
          var found = false;
          var ds4Mac = '', xboxMac = '';
          if (d.ds4 && d.ds4.length === 17) { found = true; ds4Mac = d.ds4; }
          if (d.xbox && d.xbox.length === 17) { found = true; xboxMac = d.xbox; }

          if (found) {
            sniffState = { status: 'found', ds4: ds4Mac, xbox: xboxMac };
            renderSniffBox();
          } else if (count >= 10) {
            clearInterval(sniffTimer);
            sniffState = { status: 'none', ds4: '', xbox: '' };
            renderSniffBox();
          }
        }).catch(function(){});

        if (count >= 10) clearInterval(sniffTimer);
      }, 1000);
    }

    function applyStatusData(data) {
      if (!data) return;
      lastStatusData = data; var dict = I18N[currentLang] || I18N['ko'];
      document.getElementById("ui_status").innerText = data.status; document.getElementById("ui_sensor").innerText = data.sensor;

      var pwrBtn = document.getElementById("btn_power_ctl");
      var pwrLbl = document.getElementById("lbl_power_ctl");
      if (pwrBtn) {
        pwrBtn.style.display = "inline-flex";
        if (data.raw_state === 0) {
          pwrBtn.className = "btn-power-ctl pwr-on";
          pwrLbl.innerText = dict.btn_pwr_on;
          pwrBtn.disabled = false;
        } else if (data.raw_state === 1) {
          pwrBtn.className = "btn-power-ctl pwr-booting";
          pwrLbl.innerText = dict.btn_pwr_booting;
          pwrBtn.disabled = true;
        } else if (data.raw_state === 2) {
          pwrBtn.className = "btn-power-ctl pwr-off";
          pwrLbl.innerText = dict.btn_pwr_off;
          pwrBtn.disabled = false;
        }
      }

      var apWarn = document.getElementById("ap_warn");
      apWarn.style.display = data.ap_pass_set ? "none" : "flex";

      var apPassInput = document.getElementById("ui_new_ap_pass");
      if (apPassInput) apPassInput.placeholder = data.ap_pass_set ? dict.ph_pass_set : dict.ph_pass_open;
      var staPassInput = document.getElementById("ui_wifi_pass");
      if (staPassInput) staPassInput.placeholder = data.wifi_pass_set ? dict.ph_pass_set : dict.ph_pass_empty;

      var wifiText = data.wifi_status;
      if (wifiText.indexOf("PC 동작 중 Wi-Fi 꺼짐") !== -1 || wifiText.indexOf("OFF") !== -1) wifiText = dict.wifi_running_off;
      else if (wifiText.indexOf("연결됨") !== -1 || wifiText.indexOf("CONNECTED") !== -1) {
        var match = wifiText.match(/\((.*?)\)/);
        wifiText = dict.wifi_connected + (match ? " (" + match[1] + ")" : "");
      } else if (wifiText.indexOf("연결 시도 중") !== -1 || wifiText.indexOf("CONNECTING") !== -1) wifiText = dict.wifi_connecting;
      else if (wifiText.indexOf("WOL 비활성화됨") !== -1 || wifiText.indexOf("DISABLED") !== -1) wifiText = dict.wifi_disabled;
      var statusSpan = document.getElementById("ui_wifi_status"); statusSpan.innerText = wifiText; statusSpan.style.color = data.wifi_color;

      var otaBtn = document.getElementById("ota-btn"); var fwFile = document.getElementById("fw_file");
      if (data.raw_state === 1) { otaBtn.disabled = true; otaBtn.innerText = dict.btn_ota_booting; fwFile.disabled = true; }
      else if (data.raw_state === 2) { otaBtn.disabled = false; otaBtn.innerText = dict.btn_ota_running; fwFile.disabled = false; }
      else { otaBtn.disabled = false; otaBtn.innerText = dict.btn_ota_standby; fwFile.disabled = false; }

      if (isFirstLoad) {
        document.getElementById("ui_ds4_mac").value = data.ds4; document.getElementById("ui_xbox_mac").value = data.xbox;
        document.getElementById("ui_dongle_mac").value = data.dongle; document.getElementById("ui_esp_mac").value = data.esp_mac;

        document.getElementById("ui_wol_en").checked = data.wol_en; toggleWolSection(data.wol_en);
        var sel = document.getElementById("wifi_select"); var sInput = document.getElementById("wifi_ssid_input");
        if (data.ssid && data.ssid.length > 0) {
          var curOpt = document.getElementById("opt_cur_wifi");
          if (!curOpt) {
            curOpt = document.createElement("option");
            curOpt.id = "opt_cur_wifi";
            curOpt.value = data.ssid;
            sel.insertBefore(curOpt, sel.firstChild.nextSibling);
          }
          curOpt.text = data.ssid + " " + dict.cur_setting;
          curOpt.selected = true;
          sInput.value = data.ssid;
          sInput.style.display = "none";
        } else {
          sInput.style.display = "block";
        }
        
        document.getElementById("ui_new_ap_pass").value = "";
        document.getElementById("ui_wifi_pass").value = "";
        isFirstLoad = false;
      } else {
        var curOpt = document.getElementById("opt_cur_wifi");
        if (curOpt && data.ssid) {
          curOpt.text = data.ssid + " " + dict.cur_setting;
        }
      }
    }

    function fetchStatus() { 
      if (isUploading) return; 
      fetch('/api/status').then(function(r){ return (r.status === 200) ? r.json() : null; })
        .then(function(d){ if (d) applyStatusData(d); }).catch(function(){}); 
    }
    
    window.onload = function() { 
      setLanguage(currentLang); 
      fetchStatus(); 
      statusInterval = setInterval(fetchStatus, 2000); 
    };

    function toggleWolSection(checked) { document.getElementById("wol-section").style.display = checked ? "block" : "none"; }
    function handleSsidSelect(select) {
      var input = document.getElementById("wifi_ssid_input");
      if (select.value === "__DIRECT__") { input.value = ""; input.style.display = "block"; input.focus(); }
      else if (select.value !== "") { input.value = select.value; input.style.display = "none"; }
    }

    function scanWifi() {
      var dict = I18N[currentLang] || I18N['ko']; var sel = document.getElementById("wifi_select"); var btn = document.getElementById("scan_btn");
      btn.innerHTML = '<svg class="svg-icon" style="width:13px; height:13px;" viewBox="0 0 24 24"><polyline points="23 4 23 10 17 10"></polyline><polyline points="1 20 1 14 7 14"></polyline><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path></svg> <span>' + dict.btn_scanning + '</span>';
      btn.disabled = true;

      fetch('/scan').then(function(res){ return res.json(); }).then(function(data){
        sel.innerHTML = '<option value="">' + dict.opt_sel_ssid + '</option>';
        data.forEach(function(item) {
          var opt = document.createElement("option"); opt.value = item.ssid; var lock = item.enc ? " 🔒" : "";
          opt.text = item.ssid + " (📶 " + item.rssi + "dBm" + lock + ")"; sel.appendChild(opt);
        });
        var directOpt = document.createElement("option"); directOpt.value = "__DIRECT__"; directOpt.text = dict.opt_direct_ssid; sel.appendChild(directOpt);
        btn.innerHTML = '<svg class="svg-icon" style="width:13px; height:13px;" viewBox="0 0 24 24"><polyline points="23 4 23 10 17 10"></polyline><polyline points="1 20 1 14 7 14"></polyline><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path></svg> <span>' + dict.btn_scan + '</span>';
        btn.disabled = false;
      }).catch(function(){ 
        btn.innerHTML = '<svg class="svg-icon" style="width:13px; height:13px;" viewBox="0 0 24 24"><polyline points="23 4 23 10 17 10"></polyline><polyline points="1 20 1 14 7 14"></polyline><path d="M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15"></path></svg> <span>' + dict.btn_scan + '</span>';
        btn.disabled = false; 
      });
    }

    function toggleTerminal(enable) {
      var box = document.getElementById('terminal_box');
      box.style.display = enable ? 'block' : 'none';
      if (enable) {
        fetchLogs();
        if (!termInterval) termInterval = setInterval(fetchLogs, 1500);
      } else {
        if (termInterval) { clearInterval(termInterval); termInterval = null; }
      }
    }

    function fetchLogs() {
      if (isUploading) return; 
      fetch('/api/logs').then(function(r){ return (r.status === 200) ? r.text() : null; }).then(function(txt){
        if (txt === null) return;
        var el = document.getElementById('term_log');
        var wasAtBottom = (el.scrollHeight - el.scrollTop - el.clientHeight) < 40;
        el.innerText = txt;
        if (wasAtBottom) el.scrollTop = el.scrollHeight;
      }).catch(function(){});
    }

    function sendTermCmd() {
      var input = document.getElementById('term_input');
      var cmd = input.value.trim();
      if (!cmd) return;
      fetch('/api/send_cmd?cmd=' + encodeURIComponent(cmd), { method: 'POST' })
        .then(function(){ input.value = ''; setTimeout(fetchLogs, 250); });
    }

    function formatMacInput(input) {
      let val = input.value.replace(/[^0-9A-Fa-f]/g, '').toUpperCase();
      if (val.length === 12) {
        let formatted = '';
        for (let i = 0; i < 12; i += 2) {
          formatted += val.substring(i, i + 2);
          if (i < 10) formatted += ':';
        }
        input.value = formatted;
      }
    }

    function uploadFirmware(event) {
      event.preventDefault(); 
      var file = document.getElementById("fw_file").files[0]; 
      if (!file) return;

      isUploading = true; 
      if (termInterval) { clearInterval(termInterval); termInterval = null; }
      if (statusInterval) { clearInterval(statusInterval); statusInterval = null; }

      var chkTerm = document.getElementById('chk_terminal');
      if (chkTerm) chkTerm.checked = false;
      var termBox = document.getElementById('terminal_box');
      if (termBox) termBox.style.display = 'none';

      var dict = I18N[currentLang] || I18N['ko']; 
      var btn = document.getElementById("ota-btn"); 
      var box = document.getElementById("progress-box"); 
      var bar = document.getElementById("progress-bar"); 
      var info = document.getElementById("progress-info");

      btn.disabled = true; 
      btn.innerText = dict.btn_ota_prep;

      fetch('/api/prepare_ota', { method: 'POST' }).then(function(res) {
        btn.innerText = dict.btn_ota_uploading;

        var formData = new FormData(); 
        formData.append("update", file);
        
        var xhr = new XMLHttpRequest(); 
        xhr.open("POST", "/update?size=" + file.size, true);
        xhr.timeout = 180000;
        
        box.style.display = "block"; 
        bar.style.width = "0%"; 

        xhr.upload.onprogress = function(e) {
          if (e.lengthComputable) {
            var percent = Math.round((e.loaded / e.total) * 100);
            if (percent < 100) { 
              bar.style.width = percent + "%"; 
              info.innerText = percent + "% (" + Math.round(e.loaded/1024) + " / " + Math.round(e.total/1024) + " KB)"; 
            } else { 
              bar.style.width = "100%"; 
              bar.className = "progress-bar flashing"; 
              btn.innerText = dict.btn_ota_flashing; 
              info.innerHTML = dict.ota_flash_notice; 
            }
          }
        };
        xhr.onload = function() { 
          document.open(); 
          document.write(xhr.responseText); 
          document.close(); 
        };
        xhr.onerror = function() { 
          alert("Upload Failed!"); 
          isUploading = false; btn.disabled = false; btn.innerText = dict.btn_ota_standby; 
        };
        xhr.ontimeout = function() {
          alert("Upload Timeout!");
          isUploading = false; btn.disabled = false; btn.innerText = dict.btn_ota_standby;
        };
        xhr.send(formData);
      }).catch(function() {
        alert("Pre-OTA Memory Allocation Failed!");
        isUploading = false; btn.disabled = false;
      });
    }
  </script>
</body>
</html>
)rawliteral";