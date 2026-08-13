// Auto-generated from WEB_SRC/page-app.js by WEB_SRC/generate_embedded_web.py. Do not edit manually.
#pragma once
#include <Arduino.h>

static const char PAGE_APP_JS[] PROGMEM = R"RCWEB(
var app = document.getElementById('app');
var sensorLabels = {
  T1:'T1', T2:'T2', T3:'T3', dT:'dT',
  P:'Давление', L:'Уровень', F:'Проток',
  C:'Ток нагрузки', V:'Напряжение нагрузки'
};
var outputTitles = {
  CH1:'CH1', CH2:'CH2', CH3:'CH3', CH4:'Звонок', CH5:'Буззер'
};
var LIVE_STATE_POLL_MS = 1000;
var CAL_BASIC_POLL_MS = 500;
var MANUAL_RELAY_POLL_MS = 500;
var MANUAL_VISUAL_FEEDBACK_MS = 150;
var MANUAL_PENDING_HARD_TIMEOUT_MS = 8000;
var CONNECTION_STALE_MS = 5000;
var HEADER_CLOCK_STALE_MS = 3000;
var TIME_SYNC_RETRY_MS = 30000;
var SCHEMA_RETRY_MS = 1000;
var SCHEMA_RETRY_MAX = 3;
var SCHEMA_UNAVAILABLE_TEXT = 'Конфигурация не загружена. Обновите страницу.';
var NOTICE_AUTOHIDE_MS = 5000;
var NTFY_HTTP_PREFIX = 'http://ntfy.sh/';
var STATE_FETCH_ERROR_TEXT = 'Не удалось получить состояние устройства.';
var NOTICE_ALLOWED_VIEWS = {
  menu:true,
  sound:true,
  outputConfig:true,
  theme:true,
  notifications:true,
  diag:true,
  log:true,
  num:true,
  'cal-basic':true,
  'cal-expert':true
};
var noticeHideTimer = 0;
var CURRENT_CAL_EXPERT_UPLOT_JS_URL = '/uplot.js';
var CURRENT_CAL_EXPERT_UPLOT_CSS_URL = '/uplot.css';
var CURRENT_CAL_EXPERT_CHART_HEIGHT = 292;
var currentCalExpertUPlotLoadPromise = null;

/* ===== UI font config ===== */
var uiFontConfig = {
  homeMainLabel: 'clamp(14px,3.8vw,19px)',
  homeMainValue: 'clamp(22px,5.8vw,32px)',
  homeMainError: 'clamp(10px,2.6vw,13px)',
  homeStack: 'clamp(14px,4.1vw,22px)',
  homeStackSmall: '13px',
  homeStackPlaceholder: 'clamp(14px,3.8vw,19px)',
  homeStackPlaceholderSmall: '14px',
  homeHeaderGrid: 'clamp(16px,4.4vw,21px)',
  homeTopbarRowSize: 'clamp(92px,26vw,112px)'
};

function applyUiFontConfig(){
  var root = document.documentElement;
  if (!root || !root.style) return;
  root.style.setProperty('--rc-home-main-label-font', uiFontConfig.homeMainLabel);
  root.style.setProperty('--rc-home-main-value-font', uiFontConfig.homeMainValue);
  root.style.setProperty('--rc-home-main-error-font', uiFontConfig.homeMainError);
  root.style.setProperty('--rc-home-stack-font', uiFontConfig.homeStack);
  root.style.setProperty('--rc-home-stack-font-small', uiFontConfig.homeStackSmall);
  root.style.setProperty('--rc-home-stack-placeholder-font', uiFontConfig.homeStackPlaceholder);
  root.style.setProperty('--rc-home-stack-placeholder-font-small', uiFontConfig.homeStackPlaceholderSmall);
  root.style.setProperty('--rc-home-header-grid-font', uiFontConfig.homeHeaderGrid);
  root.style.setProperty('--rc-home-topbar-row-size', uiFontConfig.homeTopbarRowSize);
}

function ensureManualRelayPendingStyles(){
  if (byId('rc-manual-relay-pending-style')) return;
  var style = document.createElement('style');
  style.id = 'rc-manual-relay-pending-style';
  style.textContent = [
    ':root{--btn-pending-bg:#374151;--btn-pending-fg:#f9fafb;--btn-pending-spinner:#f9fafb;}',
    '.manual-toggle-btn .manual-btn-content{display:inline-flex;align-items:center;justify-content:center;gap:10px;}',
    '.manual-toggle-btn .manual-btn-spinner{display:none;width:13px;height:13px;border:2px solid rgba(248,250,252,.4);border-top-color:var(--btn-pending-spinner);border-radius:50%;flex:0 0 13px;animation:manual-btn-spinner-rotate .7s linear infinite;}',
    '.manual-toggle-btn.relay-btn.btn-pending{background:var(--btn-pending-bg);color:var(--btn-pending-fg);display:flex;align-items:center;justify-content:center;pointer-events:none;}',
    '.manual-toggle-btn.relay-btn.btn-pending:disabled{opacity:1;filter:none;}',
    '.manual-toggle-btn.relay-btn.btn-pending .manual-btn-spinner{display:inline-block;}',
    '@keyframes manual-btn-spinner-rotate{from{transform:rotate(0deg)}to{transform:rotate(360deg)}}'
  ].join('');
  document.head.appendChild(style);
}

function ensureUnifiedAlertOverlayStyles(){
  if (byId('rc-unified-alert-style')) return;
  var style = document.createElement('style');
  style.id = 'rc-unified-alert-style';
  style.textContent = [
    '.rc-unified-alert-overlay{position:fixed;top:calc(env(safe-area-inset-top,0px) + 92px);right:12px;left:12px;display:flex;justify-content:flex-end;pointer-events:none;z-index:1300;}',
    '.rc-unified-alert-card{pointer-events:none;width:min(420px,calc(100vw - 24px));max-height:min(46vh,420px);overflow:auto;padding:14px 14px 12px;border-radius:16px;background:rgba(8,10,18,.97);border:1px solid rgba(255,107,107,.45);box-shadow:0 18px 38px rgba(0,0,0,.46);color:#f8fafc;}',
    '.rc-unified-alert-title{font-weight:800;font-size:16px;line-height:1.2;margin:0 0 10px;}',
    '.rc-unified-alert-section + .rc-unified-alert-section{margin-top:12px;}',
    '.rc-unified-alert-section-title{font-size:12px;line-height:1.25;text-transform:uppercase;letter-spacing:.04em;color:#fca5a5;margin:0 0 8px;}',
    '.rc-unified-alert-lines{display:flex;flex-direction:column;gap:6px;}',
    '.rc-unified-alert-line{display:flex;align-items:flex-start;justify-content:space-between;gap:10px;padding:8px 10px;border-radius:12px;font-size:14px;line-height:1.3;}',
    '.rc-unified-alert-line-text{flex:1 1 auto;min-width:0;word-break:break-word;}',
    '.rc-unified-alert-line-unacked{background:rgba(127,29,29,.48);border:1px solid rgba(252,165,165,.38);color:#fee2e2;}',
    '.rc-unified-alert-line-acked{background:rgba(120,53,15,.32);border:1px solid rgba(251,191,36,.28);color:#fed7aa;}',
    '.rc-unified-alert-line-latched{background:rgba(69,26,3,.34);border:1px solid rgba(253,186,116,.28);color:#ffedd5;}',
    '.rc-unified-alert-ack-btn{pointer-events:auto;display:block;width:100%;margin-top:14px;padding:14px 16px;border:none;border-radius:12px;background:#2563eb;color:#fff;font-weight:700;font-size:18px;line-height:1.2;cursor:pointer}',
    '.rc-unified-alert-ack-btn:active{background:#1d4ed8}',
    '.rc-unified-alert-mark{flex:0 0 auto;font-size:11px;line-height:1.2;color:#fcd34d;white-space:nowrap;padding-top:2px;}',
    '@media (min-width: 720px){.rc-unified-alert-overlay{left:auto;max-width:420px;}}'
  ].join('');
  document.head.appendChild(style);
}

function ensureHeaderClockStyles(){
  if (byId('rc-header-clock-style')) return;
  var style = document.createElement('style');
  style.id = 'rc-header-clock-style';
  style.textContent = [
    '.home-topbar .wifi-left,.home-topbar .wifi-side{flex:1 1 0;min-width:0;}',
    '.home-topbar .wifi-left{align-items:flex-start;}',
    '.home-topbar .wifi-side{display:flex;flex-direction:column;align-items:flex-end;gap:2px;}',
    '.home-topbar .wifi-row{display:flex;align-items:center;gap:8px;}',
    '.home-topbar .wifi-side .clock{margin-top:0;font-family:\"Courier New\",monospace;font-size:18px;line-height:1.05;font-variant-numeric:tabular-nums;letter-spacing:.02em;white-space:nowrap;}',
    '.home-topbar .wifi-side .clock.stale{opacity:.4;}',
    '.home-topbar .ip{font-size:10.5px;}'
  ].join('');
  document.head.appendChild(style);
}

function ensureCalExpertStyles(){
  if (byId('rc-cal-expert-style')) return;
  var style = document.createElement('style');
  style.id = 'rc-cal-expert-style';
  style.textContent = [
    '.cal-expert-summary{display:flex;flex-wrap:wrap;justify-content:space-between;gap:12px;}',
    '.cal-expert-stat{flex:1 1 160px;min-width:0;}',
    '.cal-expert-stat strong{display:block;margin-top:4px;font-size:18px;line-height:1.2;}',
    '.cal-expert-coeffs{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px;margin-top:12px;}',
    '.cal-expert-coeff{padding:10px 12px;border-radius:14px;background:rgba(148,163,184,.10);border:1px solid rgba(148,163,184,.18);}',
    '.cal-expert-chart-card{padding:0;overflow:hidden;}',
    '.cal-expert-chart-head{display:flex;flex-wrap:wrap;align-items:flex-start;justify-content:space-between;gap:10px;padding:14px 16px 0;}',
    '.cal-expert-chart-title{margin:2px 0 0;font-size:18px;line-height:1.2;}',
    '.cal-expert-chart-caption{opacity:.82;}',
    '.cal-expert-chart-legend{display:flex;flex-wrap:wrap;gap:8px 10px;font-size:12px;line-height:1.25;}',
    '.cal-expert-chart-legend-item{display:inline-flex;align-items:center;gap:6px;}',
    '.cal-expert-chart-legend-swatch{display:inline-block;width:12px;height:12px;border-radius:999px;}',
    '.cal-expert-chart-shell{position:relative;padding:14px 16px 16px;}',
    '.cal-expert-chart-frame{position:relative;min-height:292px;border-radius:18px;overflow:hidden;border:1px solid rgba(148,163,184,.18);background:radial-gradient(circle at top,rgba(37,99,235,.18),transparent 52%),linear-gradient(180deg,rgba(15,23,42,.96),rgba(15,23,42,.82));}',
    '.cal-expert-chart-canvas{min-height:292px;}',
    '.cal-expert-chart-fallback{position:absolute;inset:0;display:flex;align-items:center;justify-content:center;padding:18px;text-align:center;background:rgba(15,23,42,.74);}',
    '.cal-expert-chart-note{max-width:420px;font-size:14px;line-height:1.45;color:#dbeafe;}',
    '.cal-expert-chart-note strong{display:block;margin-bottom:6px;font-size:15px;line-height:1.25;color:#f8fafc;}',
    '.cal-expert-chart-canvas .uplot{width:100%;border-radius:18px;overflow:hidden;background:transparent;color:#e2e8f0;}',
    '.cal-expert-chart-canvas .u-title{color:#f8fafc;}',
    '.cal-expert-chart-canvas .u-legend{display:none;}',
    '.cal-expert-table{margin-top:8px;border-radius:16px;overflow:hidden;border:1px solid rgba(148,163,184,.18);background:rgba(148,163,184,.06);}',
    '.cal-expert-table-head,.cal-expert-table-row{display:grid;grid-template-columns:56px minmax(96px,1fr) minmax(112px,1fr) 118px;gap:10px;align-items:center;padding:12px;}',
    '.cal-expert-table-head{background:rgba(148,163,184,.12);font-size:12px;font-weight:700;line-height:1.2;text-transform:uppercase;letter-spacing:.04em;}',
    '.cal-expert-table-row + .cal-expert-table-row{border-top:1px solid rgba(148,163,184,.18);}',
    '.cal-expert-table-cell{min-width:0;word-break:break-word;}',
    '.cal-expert-table-cell.action{display:flex;justify-content:flex-end;}',
    '.cal-expert-table-empty{padding:14px 12px;font-size:14px;line-height:1.4;opacity:.82;}',
    '@media (max-width:640px){',
    '.cal-expert-chart-head{padding:14px 14px 0;}',
    '.cal-expert-chart-shell{padding:12px 14px 14px;}',
    '.cal-expert-chart-frame,.cal-expert-chart-canvas{min-height:248px;}',
    '.cal-expert-table-head{display:none;}',
    '.cal-expert-table-row{grid-template-columns:1fr;gap:8px;}',
    '.cal-expert-table-cell[data-label]::before{content:attr(data-label) ": ";font-size:12px;font-weight:700;opacity:.72;}',
    '.cal-expert-table-cell.action{justify-content:flex-start;}',
    '.cal-expert-coeffs{grid-template-columns:1fr;}',
    '}'
  ].join('');
  document.head.appendChild(style);
}

function routeParts(){
  var h = location.hash || '#/home';
  if (h.indexOf('#/') !== 0) h = '#/home';
  return h.slice(2).split('/');
}

var state = {
  schema: null,
  config: null,
  live: null,
  lastState: null,
  pollTimer: null,
  pollBusy: false,
  error: '',
  notice: '',
  theme: localStorage.getItem('rc_theme') || 'dark',
  logLoaded: false,
  logEntries: [],
  timeAutoSyncSent: false,
  timeSyncInFlight: false,
  lastTimeSyncTryMs: 0,
  connectionLost: false,
  connectionOfflineSince: 0,
  connectionLastOkMs: 0,
  lastValidStateMs: 0,
  connectionLastError: '',
  stateFailCount: 0,
  homeScrollY: 0,
  homeTopBlockKey: '',
  notificationsCache: null,
  manualFeedback: {},
  manualPendingByCh: {},
  manualVisual: {},
  manualVisualTimers: {},
  relayErrorSeen: {},
  currentView: '',
  ackPending: false,
  currentCal: {
    a: 0.014622769114,
    b: -18.724550898204,
    calibrated: false,
    calDate: 0,
    zeroDate: 0,
    raw: null,
    amps: null,
    zeroSet: false,
    loaded: false,
    loading: false,
    x0: null,
    points: [],
    pointCount: 0,
    stateError: '',
    stateLoaded: false,
    stateLoading: false
  },
  currentCalBasicBusy: false,
  currentCalExpert: {
    ampsValue: '',
    addBusy: false,
    clearBusy: false,
    deleteIndex: -1,
    chart: null,
    chartError: '',
    chartRefreshQueued: false,
    chartResizeCleanup: null
  }
};

function esc(s){
  s = String(s == null ? '' : s);
  return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;')
          .replace(/"/g,'&quot;').replace(/'/g,'&#39;');
}
function byId(id){ return document.getElementById(id); }
function boolWord(v){ return v ? 'Да' : 'Нет'; }
function go(hash){ location.hash = hash; }

function api(url, options, cb){
  var xhr = new XMLHttpRequest();
  var finished = false;
  var method = (options && options.method) ? options.method : 'GET';
  var body = (options && options.body) ? options.body : null;
  var requestUrl = url;
  if (method === 'GET') {
    requestUrl += (requestUrl.indexOf('?') >= 0 ? '&' : '?') + '_ts=' + Date.now();
  }
  function finish(data){
    if (finished) return;
    finished = true;
    cb && cb(data || {});
  }
  xhr.open(method, requestUrl, true);
  xhr.timeout = 10000;
  xhr.setRequestHeader('Cache-Control', 'no-cache');
  xhr.setRequestHeader('Pragma', 'no-cache');
  if (options && options.headers) {
    for (var k in options.headers) {
      if (Object.prototype.hasOwnProperty.call(options.headers, k)) {
        xhr.setRequestHeader(k, options.headers[k]);
      }
    }
  }
  xhr.onreadystatechange = function(){
    if (xhr.readyState !== 4) return;
    var status = xhr.status || 0;
    if (status > 0) markConnectionOk();
    else markConnectionLost('network');
    var data = {};
    try { data = JSON.parse(xhr.responseText || '{}'); }
    catch (e) { data = { ok:false, error:'bad_json', raw:xhr.responseText || '' }; }
    data.__http = status;
    finish(data);
  };
  xhr.ontimeout = function(){ markConnectionLost('timeout'); finish({ ok:false, error:'network_timeout', __http:0 }); };
  xhr.onerror = function(){ markConnectionLost('error'); finish({ ok:false, error:'network_error', __http:0 }); };
  try { xhr.send(body); }
  catch (e) { markConnectionLost('send'); finish({ ok:false, error:'network_error', __http:0 }); }
}

function markConnectionOk(){
  state.connectionLastOkMs = Date.now();
  if (state.connectionLost) {
    state.connectionLost = false;
    state.connectionOfflineSince = 0;
    state.connectionLastError = '';
    clearError();
    updateConnectionOverlay();
  }
}
function markConnectionLost(reason){
  if (!state.connectionLost) state.connectionOfflineSince = Date.now();
  state.connectionLost = true;
  state.connectionLastError = reason || 'network';
  updateConnectionOverlay();
}
function connectionLostText(){
  var seconds = state.connectionOfflineSince ? Math.max(1, Math.floor((Date.now() - state.connectionOfflineSince) / 1000)) : 0;
  var tail = seconds ? (' ' + seconds + ' с.') : '';
  return 'Связь с контроллером прервана. Показаны последние полученные данные.' + tail;
}
function headerClockText(rawTime){
  var match = String(rawTime || '').match(/(\d{2}:\d{2}:\d{2})$/);
  return match ? match[1] : '--:--:--';
}
function isHeaderClockStale(){
  if (!state.lastValidStateMs) return true;
  return (Date.now() - state.lastValidStateMs) > HEADER_CLOCK_STALE_MS;
}
function updateHeaderClock(){
  var el = byId('hdrClock');
  if (!el) return;
  var s = state.lastState || {};
  el.textContent = headerClockText(s.time);
  if (isHeaderClockStale()) el.classList.add('stale');
  else el.classList.remove('stale');
}
function updateConnectionOverlay(){
  var existing = byId('connection-lost-overlay');
  if (existing && existing.parentNode) existing.parentNode.removeChild(existing);
  if (!state.connectionLost) return;
  var div = document.createElement('div');
  div.id = 'connection-lost-overlay';
  div.className = 'connection-lost-overlay';
  div.setAttribute('role', 'alert');
  div.textContent = connectionLostText();
  document.body.appendChild(div);
}
function updateConnectionWatchdog(){
  updateHeaderClock();
  if (document.hidden) return;
  var view = routeParts()[0];
  if (!isLiveStateRoute(view) && !isCurrentCalLiveRoute(view)) return;
  if (!state.lastValidStateMs) return;
  if (Date.now() - state.lastValidStateMs > CONNECTION_STALE_MS) {
    markConnectionLost('stale');
  } else {
    updateConnectionOverlay();
  }
}

function cancelNoticeHideTimer(){
  if (!noticeHideTimer) return;
  clearTimeout(noticeHideTimer);
  noticeHideTimer = 0;
}
function isNoticeAllowedView(view){
  return !!NOTICE_ALLOWED_VIEWS[(view || '').trim()];
}
function setNotice(text, autoHideMs){
  cancelNoticeHideTimer();
  state.notice = text || '';
  if (!state.notice || !(autoHideMs > 0)) return;
  noticeHideTimer = setTimeout(function(){
    state.notice = '';
    noticeHideTimer = 0;
    if (isNoticeAllowedView(state.currentView || routeParts()[0] || '')) render();
  }, autoHideMs);
}
function setSuccessNotice(text){ setNotice(text, NOTICE_AUTOHIDE_MS); }
function setResultNotice(res, successText, fallbackErrorText){
  if (res && res.ok) setSuccessNotice(successText);
  else setNotice((res && (res.error || res.err)) ? (res.error || res.err) : fallbackErrorText);
}
function clearNotice(){
  cancelNoticeHideTimer();
  state.notice = '';
}
function setError(text){ state.error = text || ''; }
function clearError(){ state.error = ''; }

function applyTheme(){
  if (state.theme === 'dark') document.documentElement.setAttribute('data-theme', 'dark');
  else document.documentElement.removeAttribute('data-theme');
}
function setTheme(name){
  state.theme = (name === 'dark') ? 'dark' : 'light';
  localStorage.setItem('rc_theme', state.theme);
  applyTheme();
  render();
}

function stopPoll(){
  if (state.pollTimer) clearInterval(state.pollTimer);
  state.pollTimer = null;
  state.pollBusy = false;
}
function startPoll(ms, fn){
  stopPoll();
  state.pollTimer = setInterval(function(){
    if (document.hidden) return;
    if (state.pollBusy) return;
    state.pollBusy = true;
    fn(function(){ state.pollBusy = false; });
  }, ms);
}
function isCalBasicRoute(view){
  return view === 'cal-basic';
}
function isCalExpertRoute(view){
  return view === 'cal-expert';
}
function isCurrentCalLiveRoute(view){
  return isCalBasicRoute(view) || isCalExpertRoute(view);
}
function isLiveStateRoute(view){
  return view === 'home' || view === 'manual' || view === 'sensor' || view === 'sensorCtrl'
      || view === 'sensorAlarm';
}
function startLiveStatePoll(view){
  if (!isLiveStateRoute(view)) return;
  var pollMs = (view === 'manual') ? MANUAL_RELAY_POLL_MS : LIVE_STATE_POLL_MS;
  startPoll(pollMs, function(done){
    loadLive(function(){
      if (routeParts()[0] === view) render();
      done();
    });
  });
}

function primaryIp(s){
  if (!s) return '-';
  return s.staIP || s.apIP || '-';
}
function wifiText(s){
  if (!s) return 'WiFi';
  if (s.staIP) return 'WiFi подключён';
  if (s.apRunning || s.apIP) return 'Точка доступа';
  return 'WiFi';
}
function wifiModeCode(s){
  var mode = String(s && s.wifiMode ? s.wifiMode : '').toLowerCase();
  if (mode === 'ap_only' || mode === 'sta_ap') return mode;
  if (s && s.staIP && (s.apRunning || s.apIP)) return 'sta_ap';
  if (s && (s.apRunning || s.apIP) && !s.staIP) return 'ap_only';
  return '';
}
function wifiModeLabel(s){
  var mode = wifiModeCode(s);
  if (mode === 'ap_only') return 'AP-only';
  if (mode === 'sta_ap') return 'STA+AP';
  return 'WiFi';
}
function wifiModeNote(s){
  var mode = wifiModeCode(s);
  if (mode === 'ap_only') return 'Только точка доступа';
  if (mode === 'sta_ap') return 'Домашняя сеть + точка доступа';
  return 'WiFi';
}
function wifiSignalRssi(s){
  if (!s) return NaN;
  if (s.staIP) {
    var staRssi = Number(typeof s.staRssi !== 'undefined' ? s.staRssi : s.rssi);
    return isFinite(staRssi) ? staRssi : NaN;
  }
  if (s.apRunning || s.apIP) {
    var apRssi = Number(typeof s.apRssi !== 'undefined' ? s.apRssi : s.rssi);
    return isFinite(apRssi) ? apRssi : NaN;
  }
  return NaN;
}
function wifiBarsVisible(s){
  var rssi = wifiSignalRssi(s);
  return isFinite(rssi) && rssi > -127;
}
function wifiBarsLevel(rssi){
  rssi = Number(rssi);
  if (!isFinite(rssi)) return 0;
  if (rssi >= -60) return 4;
  if (rssi >= -70) return 3;
  if (rssi >= -80) return 2;
  if (rssi >= -88) return 1;
  return 0;
}
function wifiBarsText(rssi){
  return ['▱▱▱▱','▰▱▱▱','▰▰▱▱','▰▰▰▱','▰▰▰▰'][wifiBarsLevel(rssi)] || '▱▱▱▱';
}
function headerBadgeList(s){
  var out = [];
  if (s && s.stopLatched) out.push('STOP');
  if (s && s.synced) out.push('время установлено');
  if (s && s.ch5Enabled) out.push('зуммер вкл.');
  if (s && s.ch4Enabled) out.push('звонок вкл.');
  return out;
}

function fixedAlarmLabel(idx){
  return ['Мин 1','Мин 2','Макс 1','Макс 2'][idx] || ('Уровень ' + (idx + 1));
}
function collectActiveAlarmItems(){
  var s = state.lastState || {};
  var source = Array.isArray(s.activeAlarmsAll) ? s.activeAlarmsAll : [];
  var seen = {};
  var items = [];
  for (var i = 0; i < source.length; i++) {
    var src = source[i] || {};
    var text = typeof src.text === 'string' ? src.text.trim() : '';
    if (!text) continue;
    var acked = !!src.acked;
    if (Object.prototype.hasOwnProperty.call(seen, text)) {
      if (!acked) items[seen[text]].acked = false;
      continue;
    }
    seen[text] = items.length;
    items.push({ text:text, acked:acked });
  }
  return items;
}
function collectActiveSensorLossLines(){
  var s = state.lastState || {};
  var sensors = Array.isArray(s.sensors) ? s.sensors : [];
  var seen = {};
  var lines = [];
  for (var i = 0; i < sensors.length; i++) {
    var sensor = sensors[i] || {};
    if (!sensor.enabled) continue;
    if (sensor.sensorErrorActive !== true && sensor.sensorLostUnacked !== true) continue;
    var line = typeof sensor.sensorLostNotice === 'string' ? sensor.sensorLostNotice.trim() : '';
    if (!line) continue;
    if (seen[line]) continue;
    seen[line] = true;
    var acked = (sensor.sensorLostUnacked !== true);
    lines.push({ text: line, acked: acked });
  }
  return lines;
}
function isAlarmSoundActive(){
  var ch4 = findOutput('CH4') || {};
  var ch5 = findOutput('CH5') || {};
  return !!(ch4.actual || ch5.actual);
}
function hasAnyAlert(){
  var s = state.lastState || {};
  var unacked = Number(s.unackedAlarmCount || 0);
  return unacked > 0 || isAlarmSoundActive();
}
function unifiedAlertOverlayHtml(activeItems, latchedLines){
  var html = '';
  var lines = '';
  var lossLines = '';
  html += '<div class="rc-unified-alert-card" role="status" aria-live="assertive">';
  html += '<div class="rc-unified-alert-title">Активные тревоги и ошибки</div>';
  for (var i = 0; i < activeItems.length; i++) {
    var item = activeItems[i];
    if (item.acked) continue;
    lines += '<div class="rc-unified-alert-line rc-unified-alert-line-unacked">';
    lines += '<div class="rc-unified-alert-line-text">' + esc(item.text) + '</div>';
    lines += '</div>';
  }
  if (lines) {
    html += '<section class="rc-unified-alert-section">';
    html += '<div class="rc-unified-alert-section-title">Тревоги</div>';
    html += '<div class="rc-unified-alert-lines">' + lines + '</div>';
    html += '</section>';
  }
  for (var li = 0; li < latchedLines.length; li++) {
    var lossItem = latchedLines[li] || {};
    var lossText = (typeof lossItem === 'string') ? lossItem : (lossItem.text || '');
    var lossAcked = (typeof lossItem === 'object') && lossItem.acked === true;
    var lossCls = 'rc-unified-alert-line rc-unified-alert-line-latched';
    if (lossAcked) lossCls += ' rc-unified-alert-line-acked';
    lossLines += '<div class="' + lossCls + '">';
    lossLines += '<div class="rc-unified-alert-line-text">' + esc(lossText) + '</div>';
    lossLines += '</div>';
  }
  if (lossLines) {
    html += '<section class="rc-unified-alert-section">';
    html += '<div class="rc-unified-alert-section-title">Ошибки датчиков</div>';
    html += '<div class="rc-unified-alert-lines">' + lossLines + '</div>';
    html += '</section>';
  }
  html += '<button type="button" class="rc-unified-alert-ack-btn" onclick="acknowledgeAlarms()">Квитировать</button>';
  html += '</div>';
  return html;
}
function updateUnifiedAlertOverlay(){
  var existing = byId('rc-unified-alert-overlay');
  if (existing && existing.parentNode) existing.parentNode.removeChild(existing);
  var activeItems = collectActiveAlarmItems();
  var latchedLines = collectActiveSensorLossLines();
  var s = state.lastState || {};
  var unacked = Number(s.unackedAlarmCount || 0);
  var visibleActiveItems = activeItems.filter(function(item){ return item && !item.acked; });
  var unackedLatched = latchedLines.filter(function(it){ return it && it.acked !== true; });
  if (!document.body || !(visibleActiveItems.length > 0 || unackedLatched.length > 0 || unacked > 0)) return;
  ensureUnifiedAlertOverlayStyles();
  var shell = document.createElement('div');
  shell.id = 'rc-unified-alert-overlay';
  shell.className = 'rc-unified-alert-overlay';
  shell.innerHTML = unifiedAlertOverlayHtml(visibleActiveItems, latchedLines);
  document.body.appendChild(shell);
}

function emptySchema(){
  return { sensorIds: [], outputIds: [], confirmationIds: [] };
}
function schemaUnavailable(){
  var s = state.schema;
  return !!(s && Array.isArray(s.sensorIds) && Array.isArray(s.outputIds) && !s.sensorIds.length && !s.outputIds.length);
}
function loadSchema(cb, attempt){
  if (state.schema) { cb(state.schema); return; }
  attempt = Number(attempt || 0);
  api('/api/v1/schema', null, function(res){
    if (res && (res.ok || res.sensorIds)) {
      state.schema = res;
      cb(state.schema);
      if (attempt > 0 && state.currentView) renderCurrentRoute();
      return;
    }
    if ((attempt + 1) < SCHEMA_RETRY_MAX) {
      setTimeout(function(){ loadSchema(cb, attempt + 1); }, SCHEMA_RETRY_MS);
      return;
    }
    state.schema = emptySchema();
    setNotice(SCHEMA_UNAVAILABLE_TEXT);
    cb(state.schema);
    renderCurrentRoute();
  });
}

// Шлюз видимости по профилю. Источник истины — schema с бэкенда.
// FULL/старый сервер: schema содержит все id → показываем всё.
// LIGHT: schema сужена → скрываем отсутствующие id.
// Если schema-массивы отсутствуют — не скрываем ничего.
// Если schema-массивы пусты — это безопасный fallback, ничего не показываем.
function sensorVisible(id){
  var s = state.schema;
  if (!s || !Array.isArray(s.sensorIds)) return true;
  return s.sensorIds.indexOf(id) !== -1;
}
function outputVisible(id){
  var s = state.schema;
  if (!s || !Array.isArray(s.outputIds)) return true;
  return s.outputIds.indexOf(id) !== -1;
}
function routeAllowed(route, param){
  if (route === 'sensor' && param) return sensorVisible(param);
  if (route === 'sensorAlarm' && param) return sensorVisible(param);
  if (route === 'sensorCtrl' && param) return sensorVisible(param);
  if (route === 'ctrlRule' && param) return sensorVisible(param);
  if (route === 'alarmPref' && param) return sensorVisible(param);
  if (route === 'cal-basic') return sensorVisible('C');
  if (route === 'cal-expert') return sensorVisible('C');
  return true;
}

function copyOwn(dst, src){
  if (!src) return dst;
  for (var key in src) {
    if (Object.prototype.hasOwnProperty.call(src, key)) dst[key] = src[key];
  }
  return dst;
}

function cloneObjectArray(arr){
  var out = [];
  arr = Array.isArray(arr) ? arr : [];
  for (var i = 0; i < arr.length; i++) {
    var item = arr[i];
    if (item && typeof item === 'object') {
      var cloned = {};
      copyOwn(cloned, item);
      out.push(cloned);
    } else {
      out.push(item);
    }
  }
  return out;
}

function mergeObjectArrayByIndex(baseArr, overlayArr){
  baseArr = Array.isArray(baseArr) ? baseArr : [];
  overlayArr = Array.isArray(overlayArr) ? overlayArr : [];
  var len = Math.max(baseArr.length, overlayArr.length);
  var out = [];
  for (var i = 0; i < len; i++) {
    var base = baseArr[i];
    var overlay = overlayArr[i];
    var merged = {};
    if (base && typeof base === 'object') copyOwn(merged, base);
    if (overlay && typeof overlay === 'object') copyOwn(merged, overlay);
    out.push(merged);
  }
  return out;
}

function mergeById(configArr, liveArr, mergeEntry){
  configArr = Array.isArray(configArr) ? configArr : [];
  liveArr = Array.isArray(liveArr) ? liveArr : [];
  var configById = {};
  var liveById = {};
  var orderedIds = [];
  var seen = {};
  var i;

  for (i = 0; i < configArr.length; i++) {
    var configItem = configArr[i];
    if (!configItem || !configItem.id) continue;
    configById[configItem.id] = configItem;
    orderedIds.push(configItem.id);
    seen[configItem.id] = true;
  }

  for (i = 0; i < liveArr.length; i++) {
    var liveItem = liveArr[i];
    if (!liveItem || !liveItem.id) continue;
    liveById[liveItem.id] = liveItem;
    if (!seen[liveItem.id]) {
      orderedIds.push(liveItem.id);
      seen[liveItem.id] = true;
    }
  }

  var out = [];
  for (i = 0; i < orderedIds.length; i++) {
    var id = orderedIds[i];
    out.push(mergeEntry(configById[id], liveById[id]));
  }
  return out;
}

function mergeSensorState(configSensor, liveSensor){
  var merged = {};
  copyOwn(merged, configSensor || {});
  copyOwn(merged, liveSensor || {});
  merged.alarms = mergeObjectArrayByIndex(configSensor && configSensor.alarms, liveSensor && liveSensor.alarms);
  merged.ctrl = cloneObjectArray(configSensor && configSensor.ctrl);
  return merged;
}

function mergeOutputState(configOutput, liveOutput){
  var merged = {};
  copyOwn(merged, configOutput || {});
  copyOwn(merged, liveOutput || {});
  return merged;
}

function rebuildMergedState(){
  var merged = {};
  copyOwn(merged, state.config || {});
  copyOwn(merged, state.live || {});
  merged.sensors = mergeById(
    state.config && state.config.sensors,
    state.live && state.live.sensors,
    mergeSensorState
  );
  merged.outputs = mergeById(
    state.config && state.config.outputs,
    state.live && state.live.outputs,
    mergeOutputState
  );
  return merged;
}

function loadConfig(cb, forceReload){
  if (state.config && !forceReload) {
    cb && cb(state.config);
    return;
  }
  api('/api/v1/config', null, function(res){
    if (res && res.sensors && res.outputs) {
      state.config = res;
      state.lastState = rebuildMergedState();
    }
    cb && cb(state.config);
  });
}

function loadLive(cb){
  api('/api/v1/live', null, function(res){
    if (res && res.sensors && res.outputs) {
      state.live = res;
      state.lastState = rebuildMergedState();
      state.stateFailCount = 0;
      state.lastValidStateMs = Date.now();
      clearError();
      syncManualRelayState();
      if (!res.synced) setTimeout(autoSyncTimeIfNeeded, 0);
    } else {
      state.stateFailCount += 1;
      if (res && (res.error || res.err)) {
        setError(res.error || res.err);
      } else if (state.stateFailCount >= 3) {
        setError(STATE_FETCH_ERROR_TEXT);
      }
    }
    cb && cb(state.lastState);
  });
}

function loadState(cb){
  if (state.config) {
    loadLive(cb);
    return;
  }
  loadConfig(function(){
    loadLive(cb);
  });
}

function reloadConfigAndState(cb){
  loadConfig(function(){
    loadLive(cb);
  }, true);
}

function loadLog(cb){
  api('/api/v1/log', null, function(res){
    var arr = [];
    if (res && res.entries && res.entries.length) arr = res.entries;
    else if (res && typeof res.length !== 'undefined') arr = res;
    arr = (arr || []).slice().map(function(e, i){
      return { item:e, __i:i };
    });
    // UI requirement: newest events first.  Firmware now returns this order,
    // and this client-side sort keeps the display correct even if an older
    // backend still returns chronological order.
    arr.sort(function(a, b){
      var am = Number(a && a.item && (typeof a.item.ms !== 'undefined' ? a.item.ms : a.item.absMs));
      var bm = Number(b && b.item && (typeof b.item.ms !== 'undefined' ? b.item.ms : b.item.absMs));
      if (!isNaN(am) && !isNaN(bm) && am !== bm) return bm - am;
      return a.__i - b.__i;
    });
    arr = arr.map(function(entry){ return entry.item; });
    state.logEntries = arr;
    state.logLoaded = true;
    cb && cb(state.logEntries);
  });
}

function autoSyncTimeIfNeeded(){
  var s = state.lastState || {};
  if (s.synced) return;
  var now = Date.now();
  if (state.timeSyncInFlight) return;
  if (state.lastTimeSyncTryMs && (now - state.lastTimeSyncTryMs < TIME_SYNC_RETRY_MS)) return;
  state.lastTimeSyncTryMs = now;
  state.timeAutoSyncSent = true;
  state.timeSyncInFlight = true;
  api('/api/v1/time/sync', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({
      unixTimeMs: Date.now(),
      millisAtSend: 0,
      tzOffsetMin: -new Date().getTimezoneOffset()
    })
  }, function(res){
    state.timeSyncInFlight = false;
    if (res && res.ok) {
      loadState(function(){ render(); });
    } else {
      state.timeAutoSyncSent = false;
    }
  });
}

function syncDeviceTime(){
  clearNotice();
  state.lastTimeSyncTryMs = Date.now();
  api('/api/v1/time/sync', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({
      unixTimeMs: Date.now(),
      millisAtSend: 0,
      tzOffsetMin: -new Date().getTimezoneOffset()
    })
  }, function(res){
    if (res && res.ok) state.timeAutoSyncSent = true;
    setResultNotice(res, 'время установлено', 'Не удалось синхронизировать время.');
    loadState(function(){ render(); });
  });
}

function clearLog(){
  clearNotice();
  api('/api/v1/log', { method:'DELETE' }, function(res){
    state.logLoaded = false;
    state.logEntries = [];
    if (res && res.ok) setSuccessNotice('Журнал очищен.');
    else setNotice('Не удалось очистить журнал.');
    renderLogPage();
  });
}

function downloadLog(){
  clearNotice();
  var link = document.createElement('a');
  link.href = '/api/v1/log/download';
  link.style.display = 'none';
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
}

function setMute(muted){
  clearNotice();
  api('/api/v1/mute', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({ muted: muted })
  }, function(res){
    setResultNotice(res, muted ? 'Звук отключён.' : 'Звук включён.', 'Не удалось изменить звук.');
    loadState(function(){ render(); });
  });
}
function testSound(){
  clearNotice();
  api('/api/v1/sound/test', { method:'POST' }, function(res){
    if (res && res.ok) {
      if (res.ch4 || res.ch5) setNotice('Проверка звука запущена', NOTICE_AUTOHIDE_MS);
      else setNotice('Включите звонок или зуммер, чтобы проверить звук', NOTICE_AUTOHIDE_MS);
      return;
    }
    setResultNotice(res, 'Проверка звука запущена', 'Не удалось запустить проверку звука.');
  });
}
function muteAll(){ setMute(true); }
function isAudibleAlarmActiveRaw(){
  var s = state.lastState || {};
  var unacked = Number(s.unackedAlarmCount || 0);
  return unacked > 0 || !!s.safetyAlarmActive;
}
function mergeAckStateFromResponse(res){
  if (!res || !res.ok) return;
  var s = state.lastState || {};
  s.activeAlarmCount = Number(res.activeAlarmCount || 0);
  s.unackedAlarmCount = Number(res.unackedAlarmCount || 0);
  if (Array.isArray(res.activeAlarmReasons)) s.activeAlarmReasons = res.activeAlarmReasons.slice();
  if (Array.isArray(res.activeAlarmsAll)) s.activeAlarmsAll = res.activeAlarmsAll.slice();
  if (typeof res.muted !== 'undefined') s.muted = !!res.muted;
  if (typeof res.ch4Enabled !== 'undefined') s.ch4Enabled = !!res.ch4Enabled;
  if (typeof res.ch5Enabled !== 'undefined') s.ch5Enabled = !!res.ch5Enabled;
  var sensors = s.sensors || [];
  for (var si = 0; si < sensors.length; si++) {
    var sensor = sensors[si] || {};
    sensor.sensorLostUnacked = false;
    var alarms = sensor.alarms || [];
    for (var ai = 0; ai < alarms.length; ai++) {
      if (alarms[ai]) alarms[ai].unacked = false;
    }
  }
  var outputs = s.outputs || [];
  for (var i = 0; i < outputs.length; i++) {
    var o = outputs[i];
    if (!o || !o.id) continue;
    if (o.id === 'CH4' && typeof res.ch4Actual !== 'undefined') {
      o.actual = !!res.ch4Actual;
      o.state = !!res.ch4Actual;
      o.requested = !!res.ch4Actual;
    }
    if (o.id === 'CH5' && typeof res.ch5Actual !== 'undefined') {
      o.actual = !!res.ch5Actual;
      o.state = !!res.ch5Actual;
      o.requested = !!res.ch5Actual;
    }
  }
  state.lastState = s;
}
function sendAckRequest(cb){
  var postOpts = {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:'{}'
  };
  function tryGetFallback(){
    api('/api/v1/ack', { method:'GET' }, function(res3){
      if (res3 && res3.ok) { cb(res3); return; }
      api('/api/v1/ack/', { method:'GET' }, cb);
    });
  }
  api('/api/v1/ack', postOpts, function(res){
    var status = Number((res && res.__http) || 0);
    if (res && res.ok) { cb(res); return; }
    if (status !== 404 && status !== 405) { cb(res); return; }
    api('/api/v1/ack/', postOpts, function(res2){
      var status2 = Number((res2 && res2.__http) || 0);
      if (res2 && res2.ok) { cb(res2); return; }
      if (status2 !== 404 && status2 !== 405) { cb(res2); return; }
      tryGetFallback();
    });
  });
}
function acknowledgeAlarms(){
  clearError();
  clearNotice();
  if (state.ackPending) return;
  state.ackPending = true;
  sendAckRequest(function(res){
    state.ackPending = false;
    if (!res || !res.ok) {
      loadState(function(){ render(); });
      return;
    }
    mergeAckStateFromResponse(res);
    render();
    setTimeout(function(){
      loadState(function(){ render(); });
    }, 120);
  });
}
function stopMainOutputs(cb){
  clearNotice();
  function finish(){
    loadState(function(){
      if (typeof cb === 'function') cb();
      else render();
    });
  }
  function legacyStop(){
    var ids = ['CH1','CH2','CH3'];
    var idx = 0;
    function step(){
      if (idx >= ids.length) {
        finish();
        return;
      }
      var id = ids[idx++];
      api('/api/v1/output/' + encodeURIComponent(id) + '/manual', {
        method:'POST',
        headers:{'Content-Type':'application/json'},
        body: JSON.stringify({ state:false })
      }, function(){ step(); });
    }
    step();
  }
  api('/api/v1/stop', { method:'POST' }, function(res){
    if (res && res.ok) {
      setNotice('STOP активен. Автоматика CH1–CH3 заблокирована до отмены стопа.');
      finish();
      return;
    }
    legacyStop();
  });
}
function releaseStopMainOutputs(cb){
  clearNotice();
  function releaseOk(res){
    return !!(res && res.ok && res.stopLatched === false);
  }
  function finish(res){
    loadState(function(){
      var released = releaseOk(res) || !(state.lastState && state.lastState.stopLatched);
      if (released) setNotice('STOP отменён. Автоматика CH1–CH3 снова разрешена.');
      else setNotice((res && (res.userMessage || res.error || res.err)) ? (res.userMessage || res.error || res.err) : 'Не удалось отменить STOP.');
      if (typeof cb === 'function') cb(res);
      else render();
    });
  }
  function tryLegacyRelease(prev){
    api('/api/v1/stop/release', { method:'POST' }, function(res){
      var status = Number((res && res.__http) || 0);
      if (releaseOk(res)) { finish(res); return; }
      if (status !== 404 && status !== 405) { finish(res); return; }
      api('/api/v1/stop/release/', { method:'POST' }, finish);
    });
  }
  // Primary path: reuse the known-good STOP endpoint and select release by query.
  // This avoids the nested /stop/release route that returned 404 on the device.
  api('/api/v1/stop?release=1', { method:'POST' }, function(res){
    var status = Number((res && res.__http) || 0);
    if (releaseOk(res)) { finish(res); return; }
    if (status === 404 || status === 405 || (res && res.ok && res.stopLatched === true)) {
      tryLegacyRelease(res);
      return;
    }
    finish(res);
  });
}
function showStopInfo(){ go('#/stopConfirm'); render(); }

function findSensor(id){
  var arr = (state.lastState && state.lastState.sensors) ? state.lastState.sensors : [];
  for (var i = 0; i < arr.length; i++) if (arr[i].id === id) return arr[i];
  return null;
}
function findOutput(id){
  var arr = (state.lastState && state.lastState.outputs) ? state.lastState.outputs : [];
  for (var i = 0; i < arr.length; i++) if (arr[i].id === id) return arr[i];
  return null;
}

function outputStateLabel(o){
  if (!o) return '—';
  return o.actual ? 'ВКЛ' : 'ВЫКЛ';
}

function formatValueWithUnit(num, unit, digits){
  if (num == null || isNaN(Number(num))) return '—';
  var d = typeof digits === 'number' ? digits : 1;
  return Number(num).toFixed(d) + (unit ? (' ' + unit) : '');
}

function sensorEnabledAlarms(s){
  var out = [];
  var arr = (s && s.alarms) ? s.alarms : [];
  for (var i = 0; i < arr.length; i++) {
    if (arr[i] && arr[i].enabled) out.push({ idx:i, item:arr[i] });
  }
  return out;
}
function sensorTriggeredAlarms(s){
  var out = [];
  var arr = sensorEnabledAlarms(s);
  for (var i = 0; i < arr.length; i++) if (arr[i].item.triggered) out.push(arr[i]);
  return out;
}
function sensorCtrlRules(s){
  var out = [];
  var arr = (s && s.ctrl) ? s.ctrl : [];
  for (var i = 0; i < arr.length; i++) {
    var rule = arr[i];
    if (!rule || !rule.enabled) continue;
    if (Number(rule.outIdx) > 2) continue;
    out.push(rule);
  }
  return out;
}
function ctrlLogicLabel(logic, sensorId){
  if (logic === 'cool' && (sensorId === 'L' || sensorId === 'F')) return 'включено';
  return logic === 'cool' ? 'охлаждение' : 'нагрев';
}

function sensorStateInfo(s){
  if (!s) return { text:'Нет данных', cls:'off' };
  if (!s.enabled) return { text:'', cls:'off' };
  if (s.error && s.note) return { text:'Ограничение', cls:'warn' };
  if (s.error) return { text:'Ошибка', cls:'danger' };
  if (sensorTriggeredAlarms(s).length) return { text:'Тревога', cls:'danger' };
  if (sensorEnabledAlarms(s).length) return { text:'Контроль', cls:'ok' };
  return { text:'Норма', cls:'ok' };
}

function sensorAlarmSummary(s){
  if (!s || !s.enabled) return '';
  var enabled = sensorEnabledAlarms(s);
  if (!enabled.length) return 'Сигнализация: выкл';
  var parts = [];
  for (var i = 0; i < enabled.length; i++) {
    var item = enabled[i];
    parts.push(fixedAlarmLabel(item.idx) + (item.item.triggered ? ' !' : ''));
  }
  return 'Сигнализация: ' + parts.join(', ');
}

function sensorCtrlSummary(s){
  if (!s || !s.enabled) return '';
  var rules = sensorCtrlRules(s);
  if (!rules.length) return 'Управление: выкл';
  var parts = [];
  for (var i = 0; i < rules.length; i++) {
    var r = rules[i];
    var outId = 'CH' + (Number(r.outIdx) + 1);
    parts.push(outId + ' ' + ctrlLogicLabel(r.logic, s.id) + ' [' + Number(r.min).toFixed(1) + '; ' + Number(r.max).toFixed(1) + ']');
  }
  return 'Управление: ' + parts.join(' · ');
}

function sensorNoteSummary(s){
  if (!s || !s.enabled) return '';
  if (s.note) return s.note;
  if (s.hwLimited) return 'Ограничение по оборудованию';
  return '';
}

function outputConfirmSummary(o){
  if (!o || typeof o.confirmAvailable === 'undefined') return 'Подтверждение: —';
  if (!o.confirmAvailable) return 'Подтверждение: недоступно';
  if (o.pending) return 'Подтверждение: ожидание';
  if (o.timeout) return 'Подтверждение: нет, таймаут';
  if (o.mismatch && o.confirmActual) return 'Подтверждение: активно при выключенном выходе';
  if (o.confirmed) return o.actual ? 'Подтверждение: есть' : 'Подтверждение: снято';
  return 'Подтверждение: нет';
}
function relayDotClass(o){
  if (!o) return 'err';
  if (o.confirmAvailable) {
    if (o.pending) return 'pending';
    if (o.timeout || o.mismatch) return 'err';
    if (o.confirmed && o.actual) return 'ok';
    return 'err';
  }
  return o.actual ? 'ok' : 'err';
}

function relayFeedbackOn(o){
  if (!o) return false;
  if (typeof o.feedbackOn !== 'undefined') return !!o.feedbackOn;
  if (typeof o.confirmActual !== 'undefined') return !!o.confirmActual;
  if (typeof o.confirmedLevel !== 'undefined') return Number(o.confirmedLevel) === 1;
  if (typeof o.confirmed === 'number') return Number(o.confirmed) === 1;
  return !!o.actual;
}

function relayDisplayOn(o){
  if (!o) return false;
  if (typeof o.displayOn !== 'undefined') return !!o.displayOn;
  if (typeof o.actual !== 'undefined') return !!o.actual;
  if (typeof o.state !== 'undefined') return !!o.state;
  return relayFeedbackOn(o);
}

function relayCommandPending(o){
  if (!o) return false;
  return !!(o.relayPending || (o.pendingCmd && o.pendingCmd !== 'none'));
}

function relayCommandConfirmed(o){
  if (!o) return false;
  if (typeof o.confirmedBool !== 'undefined') return !!o.confirmedBool;
  if (typeof o.confirmed === 'number') return Number(o.confirmed) !== 0;
  if (typeof o.confirmedLevel !== 'undefined') return Number(o.confirmedLevel) !== 0;
  return !!o.confirmed;
}

function relayCommandMismatch(o){
  return !!(o && (o.confirmMismatch || o.mismatch));
}

function relayCommandTimeout(o){
  return !!(o && (o.confirmTimeout || o.timeout));
}

function relayCommandFaultLatched(o){
  return !!(o && (o.confirmFaultLatched || o.faultLatched));
}

function relayCommandFailed(o){
  var err = (o && o.relayError) ? String(o.relayError).toLowerCase() : '';
  return err === 'timeout' || err === 'blocked' ||
         relayCommandTimeout(o) || relayCommandMismatch(o) || relayCommandFaultLatched(o);
}

function relayCommandErrorSignature(o){
  if (!o) return '';
  var err = (o.relayError ? String(o.relayError).toLowerCase() : '');
  if (err !== 'timeout' && err !== 'blocked') return '';
  return err + ':' + String(Number(o.relayErrorMs || 0));
}

function relayTerminalBaseline(o){
  return {
    commandError: relayCommandErrorSignature(o),
    timeout: relayCommandTimeout(o),
    mismatch: relayCommandMismatch(o),
    faultLatched: relayCommandFaultLatched(o)
  };
}

function normalizeRelayCommand(cmd){
  cmd = String(cmd || '').toLowerCase();
  return (cmd === 'on' || cmd === 'off') ? cmd : '';
}

function manualPendingStateRaw(id){
  state.manualPendingByCh = state.manualPendingByCh || {};
  if (!id) return null;
  return state.manualPendingByCh[id] || null;
}

function clearManualPendingState(id){
  if (!id || !state.manualPendingByCh) return;
  delete state.manualPendingByCh[id];
}

function armManualPendingState(id, cmd, sentAt, baselineTerminal){
  var normalized = normalizeRelayCommand(cmd);
  if (!id || !normalized) return null;
  state.manualPendingByCh = state.manualPendingByCh || {};
  baselineTerminal = baselineTerminal || {};
  var ctx = {
    cmd: normalized,
    sentAt: Number(sentAt || Date.now()),
    armed: true,
    terminal: {
      commandError: String(baselineTerminal.commandError || ''),
      timeout: !!baselineTerminal.timeout,
      mismatch: !!baselineTerminal.mismatch,
      faultLatched: !!baselineTerminal.faultLatched
    }
  };
  state.manualPendingByCh[id] = ctx;
  return ctx;
}

function manualPendingHardTimedOut(ctx){
  return !!(ctx && ctx.armed && ctx.sentAt && (Date.now() - ctx.sentAt >= MANUAL_PENDING_HARD_TIMEOUT_MS));
}

function manualPendingCommand(o, ctx){
  ctx = ctx || manualPendingStateRaw(o && o.id);
  if (ctx && ctx.cmd) return normalizeRelayCommand(ctx.cmd);
  var remoteCmd = normalizeRelayCommand(o && o.pendingCmd);
  if (remoteCmd) return remoteCmd;
  if (o && typeof o.confirmExpected !== 'undefined') return o.confirmExpected ? 'on' : 'off';
  return '';
}

function manualPendingResolvedByActual(o, ctx){
  var cmd = manualPendingCommand(o, ctx);
  var actualOn = outputConfirmedOn(o);
  if (cmd === 'on') return actualOn === true;
  if (cmd === 'off') return actualOn === false;
  return false;
}

function manualPendingBaselineSignature(o){
  return relayTerminalBaseline(o);
}

function clearManualRelayErrorFields(o){
  if (!o) return;
  o.relayError = '';
  o.relayErrorMs = 0;
  o.relayErrorText = '';
}

function manualPendingTerminalForContext(o, ctx){
  if (!ctx || !ctx.armed || !relayCommandFailed(o)) return false;
  var baseline = ctx.terminal || {};
  var commandError = relayCommandErrorSignature(o);
  if (commandError && commandError !== String(baseline.commandError || '')) return true;
  if (relayCommandTimeout(o) && !baseline.timeout) return true;
  if (relayCommandMismatch(o) && !baseline.mismatch) return true;
  if (relayCommandFaultLatched(o) && !baseline.faultLatched) return true;
  return false;
}

function manualPendingContext(o){
  if (!o || !o.id) return null;
  state.manualPendingByCh = state.manualPendingByCh || {};
  var ctx = manualPendingStateRaw(o.id);
  if (ctx) {
    if (manualPendingResolvedByActual(o, ctx) || manualPendingTerminalForContext(o, ctx)) {
      clearManualPendingState(o.id);
      return null;
    }
    if (manualPendingHardTimedOut(ctx)) {
      clearManualPendingState(o.id);
      return null;
    }
    return ctx;
  }
  if (relayCommandPending(o) || o.commandActive === true) {
    var remoteCmd = manualPendingCommand(o);
    if (remoteCmd) return armManualPendingState(o.id, remoteCmd, Date.now(), manualPendingBaselineSignature(o));
  }
  return null;
}

function resetManualRelayUiState(){
  state.manualPendingByCh = {};
  state.manualFeedback = {};
  state.manualVisual = {};
  if (state.manualVisualTimers) {
    for (var id in state.manualVisualTimers) {
      if (!Object.prototype.hasOwnProperty.call(state.manualVisualTimers, id)) continue;
      clearTimeout(state.manualVisualTimers[id]);
    }
  }
  state.manualVisualTimers = {};
}

function classifyManualRelayButtonState(o, ctx){
  var actualOn = outputConfirmedOn(o);
  ctx = ctx || manualPendingContext(o);
  if (relayCommandFailed(o) && (!ctx || manualPendingTerminalForContext(o, ctx))) {
    clearManualPendingState(o && o.id);
    return 'off';
  }
  if (ctx && ctx.armed) return 'pending';
  return actualOn ? 'on' : 'off';
}

function manualButtonState(o, ctx){
  return classifyManualRelayButtonState(o, ctx);
}

function manualDesiredOn(o){
  return outputConfirmedOn(o);
}

function manualPhysicalOn(o){
  return outputConfirmedOn(o);
}
function isMainOutputId(id){
  return id === 'CH1' || id === 'CH2' || id === 'CH3';
}
function stopBlocksOutput(o){
  return !!(state.lastState && state.lastState.stopLatched && o && isMainOutputId(o.id));
}

function syncManualRelayState(){
  var outputs = (state.lastState && state.lastState.outputs) ? state.lastState.outputs : [];
  state.manualPendingByCh = state.manualPendingByCh || {};
  state.relayErrorSeen = state.relayErrorSeen || {};
  for (var i = 0; i < outputs.length; i++) {
    var o = outputs[i] || {};
    if (!o.id) continue;
    manualButtonState(o, manualPendingContext(o));

    if (o.relayError && o.relayErrorMs) {
      var key = o.id + ':' + o.relayError + ':' + o.relayErrorMs;
      if (!state.relayErrorSeen[key]) {
        state.relayErrorSeen[key] = true;
        if (o.relayError === 'timeout') setNotice('Таймаут подтверждения реле ' + o.id + '. Проверьте WER-сигнал.');
      }
    }
  }
}

function manualVisualState(id){
  if (!id || !state.manualVisual) return null;
  var item = state.manualVisual[id];
  if (!item) return null;
  if (item.until && item.until <= Date.now()) {
    delete state.manualVisual[id];
    return null;
  }
  return item;
}
function setManualVisualState(id, phase, durationMs){
  if (!id) return;
  state.manualVisual = state.manualVisual || {};
  state.manualVisualTimers = state.manualVisualTimers || {};
  if (state.manualVisualTimers[id]) {
    clearTimeout(state.manualVisualTimers[id]);
    delete state.manualVisualTimers[id];
  }
  if (!phase) {
    delete state.manualVisual[id];
    return;
  }
  var ms = Math.max(0, Number(durationMs || 0));
  state.manualVisual[id] = { phase: phase, until: Date.now() + ms };
  state.manualVisualTimers[id] = setTimeout(function(){
    if (state.manualVisual && state.manualVisual[id] && state.manualVisual[id].phase === phase) {
      delete state.manualVisual[id];
    }
    if (state.manualVisualTimers) delete state.manualVisualTimers[id];
    if (routeParts()[0] === 'manual') render();
  }, ms + 20);
}
function manualVisualPhase(o){
  var item = manualVisualState(o && o.id);
  return item ? item.phase : '';
}

function manualToggleLabel(o, btnState){
  return manualDesiredOn(o) ? 'Выключить' : 'Включить';
}

function manualToggleClass(o, btnState){
  btnState = btnState || manualButtonState(o);
  var cls = ' relay-btn btn-' + btnState;
  if (btnState === 'pending') cls += ' relay-pending';
  if (btnState !== 'pending' && manualVisualPhase(o) === 'blink') cls += ' btn-blink';
  if (manualVisualPhase(o) === 'error') cls += ' blink';
  return cls;
}

function manualToggleInnerHtml(o, btnState){
  return '<span class="manual-btn-content"><span class="manual-btn-label">' + esc(manualToggleLabel(o, btnState)) + '</span><span class="manual-btn-spinner" aria-hidden="true"></span></span>';
}

function manualDotClass(o){
  if (!o) return 'err';
  return manualPhysicalOn(o) ? 'ok' : 'err';
}

function manualStatusText(o, ctx){
  ctx = ctx || manualPendingStateRaw(o && o.id);
  if (o && o.relayErrorText && (!ctx || !ctx.armed || manualPendingTerminalForContext(o, ctx))) {
    return o.relayErrorText;
  }
  return manualPhysicalOn(o) ? 'включено' : 'выкл';
}

function manualMessageText(o){
  if (!o) return '';
  if (state.manualFeedback && state.manualFeedback[o.id]) return state.manualFeedback[o.id];
  if (o.relayError === 'timeout') return 'таймаут подтверждения команды';
  if (stopBlocksOutput(o)) return 'STOP активен: включение будет отклонено';
  if (o.operatorHoldOff) return 'автоматика заблокирована вручную';
  if (o.timeout || o.mismatch) return 'ошибка подтверждения';
  return '';
}


function renderMessages(extra){
  var blocks = [];
  var currentView = state.currentView || routeParts()[0] || '';
  if (state.error) blocks.push('<div class="error-box">' + esc(state.error) + '</div>');
  if (state.notice && isNoticeAllowedView(currentView)) blocks.push('<div class="notice-box">' + esc(state.notice) + '</div>');
  if (state.lastState && state.lastState.stopLatched) {
    blocks.push('<div class="error-box">STOP активен: CH1–CH3 выключены, автоматика заблокирована. <button class="btn light inline" onclick="releaseStopMainOutputs()">Отменить стоп</button></div>');
  }
  if (extra) blocks.push(extra);
  return blocks.join('');
}

function timeSyncBlock(){
  if (!state.lastState || state.lastState.synced) return '';
  return '<div class="notice-box">' +
    'Время устройства ещё не синхронизировано. Для журнала и событий лучше синхронизировать его со временем браузера.' +
    '<button class="btn notice-top-space" onclick="syncDeviceTime()">Синхронизировать время</button>' +
    '</div>';
}

function translateLogEventRu(ev){
  var s = String(ev || '');
  var m;
  var map = {
    'Time synchronized': 'Время синхронизировано',
    'Web server started': 'Веб-сервер запущен',
    'Flow lost: alarm + process stop': 'Потеря протока: авария и остановка процесса',
    'Flow alarm: no flow after CH2 confirmed': 'Авария протока: нет протока после подтверждения CH2',
    'Log cleared': 'Журнал очищен',
    'Alarms acknowledged': 'Аварии квитированы',
    'Storage: NVS recovered, saved settings reset': 'Хранилище: NVS восстановлено, сохранённые настройки сброшены',
    'WiFi: STA disconnected': 'Wi‑Fi: STA отключён',
    'AP password set': 'Пароль точки доступа задан',
    'AP password cleared': 'Пароль точки доступа очищен',
    'WiFi wizard completed': 'Мастер Wi‑Fi завершён',
    'WiFi wizard reopened': 'Мастер Wi‑Fi открыт снова'
  };
  if (map[s]) return map[s];
  m = s.match(/^Boot: firmware (.+)$/); if (m) return 'Запуск прошивки ' + m[1];
  m = s.match(/^Boot: reset reason (.+)$/); if (m) return 'Причина перезапуска: ' + m[1];
  m = s.match(/^Mode: (REAL|EMU)$/); if (m) return 'Режим: ' + (m[1] === 'EMU' ? 'эмуляция' : 'реальное оборудование');
  m = s.match(/^Mode: (.+)$/); if (m) return 'Режим: ' + m[1];
  m = s.match(/^Storage: unavailable - (.+)$/); if (m) return 'Хранилище недоступно: ' + m[1];
  m = s.match(/^Manual: (CH\d) (ON|OFF)( \[BLOCKED\])?$/);
  if (m) return 'Ручная команда: ' + m[1] + ' ' + (m[2] === 'ON' ? 'включить' : 'выключить') + (m[3] ? ' (заблокировано автоматикой)' : '');
  m = s.match(/^(CH\d) (ON|OFF)$/); if (m) return 'Реле ' + m[1] + ' ' + (m[2] === 'ON' ? 'включено' : 'выключено');
  m = s.match(/^(T1|T2|T3|dT|P|L|F|C|V) alarm FIRED$/); if (m) return 'Сработала авария датчика ' + m[1];
  m = s.match(/^(T1|T2|T3|dT|P|L|F|C|V) alarm cleared$/); if (m) return 'Авария датчика ' + m[1] + ' снята';
  m = s.match(/^(T1|T2|T3|dT|P|L|F|C|V) alarm changed$/); if (m) return 'Изменилось состояние аварии датчика ' + m[1];
  m = s.match(/^(T1|T2|T3|dT|P|L|F|C|V) connected$/); if (m) return 'Датчик ' + m[1] + ' подключён';
  m = s.match(/^(T1|T2|T3|dT|P|L|F|C|V) disconnected$/); if (m) return 'Датчик ' + m[1] + ' отключён';
  m = s.match(/^(T1|T2|T3|dT|P|L|F|C|V) ERROR$/); if (m) return 'Ошибка датчика ' + m[1];
  m = s.match(/^(T1|T2|T3|dT|P|L|F|C|V) ERROR cleared$/); if (m) return 'Ошибка датчика ' + m[1] + ' снята';
  m = s.match(/^WiFi: STA connected - (.+) IP=(.+)$/); if (m) return 'Подключение к Wi‑Fi: ' + m[1] + ', IP ' + m[2];
  m = s.match(/^Notify failed: (.+)$/); if (m) return 'Ошибка отправки уведомления: ' + m[1];
  m = s.match(/^EMU scenario: (.+)$/); if (m) return 'Сценарий эмуляции: ' + m[1];
  m = s.match(/^(WER_CH\d) timeout: output ON but confirmation missing$/); if (m) return m[1] + ': нет подтверждения при включённом выходе';
  m = s.match(/^(WER_CH\d) mismatch: confirmation active while output OFF$/); if (m) return m[1] + ': вход подтверждения активен при выключенном выходе';
  m = s.match(/^(WER_CH\d) timeout: output OFF$/); if (m) return m[1] + ': таймаут подтверждения, выход выключен';
  m = s.match(/^(WER_CH\d) restored$/); if (m) return m[1] + ': подтверждение снова в норме';
  m = s.match(/^Stop pressed: CH1-CH3 OFF$/); if (m) return 'Нажата кнопка СТОП';
  m = s.match(/^Stop pressed: CH1-CH3 inhibited$/); if (m) return 'Нажата кнопка STOP: CH1–CH3 выключены, автоматика заблокирована';
  m = s.match(/^Stop released: automation enabled$/); if (m) return 'STOP отменён: автоматика CH1–CH3 снова разрешена';
  m = s.match(/^(WER_CH\d) timeout: process stop$/); if (m) return m[1] + ': нет подтверждения, процесс остановлен';
  m = s.match(/^(WER_CH\d) restored after timeout$/); if (m) return m[1] + ': восстановлено после таймаута';
  return s;
}

function renderHeader(){
  var s = state.lastState || {};
  var chips = headerBadgeList(s);
  var html = '';
  html += '<div class="topbar">';
  html += '<div class="topline">';
  html += '<strong>' + esc(s.time || 'Время неизвестно') + '</strong>';
  html += '<div class="wifi-mini"><span class="wifi-mode">' + esc(wifiModeLabel(s)) + '</span>' + (wifiBarsVisible(s) ? '<span class="wifi-bars">' + esc(wifiBarsText(wifiSignalRssi(s))) + '</span>' : '') + '</div>';
  html += '</div>';
  html += '<div class="meta compact">IP: ' + esc(primaryIp(s)) + '</div>';
  if (chips.length) {
    html += '<div class="meta-row compact">';
    for (var i = 0; i < chips.length; i++) html += '<span class="badge ok">' + esc(chips[i]) + '</span>';
    html += '</div>';
  }
  html += '</div>';
  return html;
}

function renderMenu(){
  stopPoll();
  var html = '<div class="app"><div class="panel"><h2>Меню</h2>';
  html += renderMessages('');
  if (schemaUnavailable()) html += '<div class="error-box">' + esc(SCHEMA_UNAVAILABLE_TEXT) + '</div>';
  html += '<button class="btn" onclick="location.href=\'/wifi\'">Конфигурация WiFi</button>';
  html += '<button class="btn" onclick="go(\'#/sound\')">Конфигурация звука</button>';
  html += '<button class="btn" onclick="go(\'#/outputConfig\')">Конфигурация выходов CH1–CH3</button>';
  if (sensorVisible('C')) html += '<button class="btn" onclick="go(\'#/cal-basic\');render();">Датчик тока</button>';
  html += '<button class="btn" onclick="go(\'#/notifications\')">Уведомления</button>';
  html += '<button class="btn" onclick="go(\'#/log\')">Лог</button>';
  html += '<button class="btn light" onclick="go(\'#/home\')">Назад</button>';
  html += '</div></div>';
  app.innerHTML = html;
}

function renderCalBasic(){
  stopPoll();
  var cal = ensureCurrentCalState();
  var sensor = findSensor('C') || { enabled:false };
  var busy = !!state.currentCalBasicBusy;
  var html = '<div class="app"><div class="panel"><h2>Калибровка нуля</h2>';
  html += '<div id="calBasicMessages">' + renderMessages('') + '</div>';
  html += '<div class="field"><label>Дата установки нуля</label><div id="calBasicZeroDateValue">' + esc(formatUnixDateTime(cal.zeroDate)) + '</div></div>';
  if (!sensor.enabled) {
    html += '<div class="error-box">Датчик тока отключён. Включите его в настройках, чтобы выполнить калибровку.</div>';
  }
  html += '<div class="field rule-card">';
  html += '<div class="small">Текущие данные</div>';
  html += '<div id="calBasicAmpsValue" style="font-size:28px;font-weight:700;line-height:1.2">Нагрузка: ' + esc(tplCurrentFromApi(cal.amps)) + '</div>';
  html += '<div id="calBasicRawValue" class="mono" style="font-size:18px;margin-top:6px">Raw: ' + esc(tplCurrentRawIntValue(cal.raw)) + '</div>';
  html += '</div>';
  html += '<div class="field"><div>Отключите нагрузку и нажмите кнопку</div></div>';
  html += '<div class="inline-actions">';
  html += '<button class="btn" onclick="submitCalBasicZero()"'
       + ((!sensor.enabled || busy) ? ' disabled' : '')
       + (busy ? ' aria-busy="true"' : '')
       + '>' + esc(busy ? 'Устанавливаем...' : 'Установить ноль') + '</button>';
  html += '<button class="btn secondary" onclick="go(\'#/cal-expert\');render();">Экспертная калибровка</button>';
  html += '</div>';
  html += '<button class="btn light" onclick="go(\'#/menu\');render();">Назад</button>';
  html += '</div></div>';
  app.innerHTML = html;
  updateCalBasicLiveFields();
  updateCalBasicStatusFields();
  if (!document.hidden) {
    loadCurrentCalLive(function(calState, res){
      if (!isCalBasicRoute(routeParts()[0])) return;
      updateCalBasicLiveFields();
      updateCurrentCalRouteMessages('calBasicMessages');
      if (!currentCalApiSuccess(res)) render();
    });
    loadCurrentCalStatus(function(){
      if (!isCalBasicRoute(routeParts()[0])) return;
      updateCalBasicStatusFields();
      updateCurrentCalRouteMessages('calBasicMessages');
    });
  }
  startCalBasicPoll();
}

function renderCalExpert(){
  stopPoll();
  destroyCurrentCalExpertChart();
  ensureCalExpertStyles();
  var cal = ensureCurrentCalState();
  var expert = ensureCurrentCalExpertState();
  var sensor = findSensor('C') || { enabled:false };
  var inputError = currentCalExpertInputError(expert.ampsValue);
  var html = '<div class="app"><div class="panel"><h2>Экспертная калибровка</h2>';
  html += '<div id="calExpertMessages">' + renderMessages('') + '</div>';
  if (!sensor.enabled) {
    html += '<div class="error-box">Датчик тока отключён. Live-данные и таблица доступны только для просмотра, добавление точек заблокировано.</div>';
  }
  html += '<div class="field rule-card">';
  html += '<div class="small">Текущие данные</div>';
  html += '<div id="calExpertAmpsValue" style="font-size:28px;font-weight:700;line-height:1.2">Нагрузка: ' + esc(tplCurrentFromApi(cal.amps)) + '</div>';
  html += '<div id="calExpertRawValue" class="mono" style="font-size:18px;margin-top:6px">Raw: ' + esc(tplCurrentRawIntValue(cal.raw)) + '</div>';
  html += '</div>';
  html += '<div class="field rule-card">';
  html += '<div class="cal-expert-summary">';
  html += '<div class="cal-expert-stat"><div class="small">Точек</div><strong id="calExpertCountValue">' + esc(currentCalExpertCountText()) + '</strong></div>';
  html += '</div>';
  html += '<div id="calExpertLinearValues">' + currentCalExpertLinearHtml() + '</div>';
  html += '<div class="small" style="margin-top:10px;line-height:1.4">Модель линейная: прямая проходит через ноль (x0), а точки задают наклон по методу наименьших квадратов. Чем больше точек в рабочем диапазоне — тем точнее наклон.</div>';
  html += '</div>';
  html += currentCalExpertChartCardHtml();
  html += '<div class="field"><label>Известный ток (A)</label><input id="calExpertAmpsInput" class="mono" type="text" inputmode="decimal" value="'
       + esc(expert.ampsValue || '')
       + '" oninput="onCurrentCalExpertAmpsInput(this.value)"></div>';
  html += '<div id="calExpertInputError" class="small">' + esc(inputError) + '</div>';
  html += '<div class="small">Raw берётся автоматически из текущего live-значения датчика.</div>';
  html += '<div class="inline-actions">';
  html += '<button type="button" id="calExpertAddBtn" class="btn secondary" onclick="submitCurrentCalExpertPoint()"'
       + (currentCalExpertAddDisabled(sensor.enabled) ? ' disabled' : '')
       + (expert.addBusy ? ' aria-busy="true"' : '')
       + '>' + esc(expert.addBusy ? 'Добавляем...' : 'Добавить точку') + '</button>';
  html += '<button type="button" id="calExpertClearBtn" class="btn danger" onclick="clearCurrentCalExpertPoints()"'
       + (currentCalExpertClearDisabled() ? ' disabled' : '')
       + (expert.clearBusy ? ' aria-busy="true"' : '')
       + '>' + esc(expert.clearBusy ? 'Очищаем...' : 'Очистить все') + '</button>';
  html += '</div>';
  html += '<div id="calExpertPointsTable">' + currentCalExpertPointsTableHtml() + '</div>';
  html += '<button type="button" class="btn light" onclick="go(\'#/cal-basic\');render();">Назад</button>';
  html += '</div></div>';
  app.innerHTML = html;
  updateCalExpertLiveFields();
  updateCurrentCalExpertUi();
  loadCurrentCalExpertChartAssets().then(function(){
    var current = ensureCurrentCalExpertState();
    current.chartError = '';
    if (isCalExpertRoute(routeParts()[0])) {
      updateCurrentCalRouteMessages('calExpertMessages');
      scheduleCurrentCalExpertChartRefresh();
    }
  }).catch(function(err){
    var current = ensureCurrentCalExpertState();
    current.chartError = (err && err.message) ? err.message : 'uPlot не удалось загрузить.';
    if (isCalExpertRoute(routeParts()[0])) {
      updateCurrentCalRouteMessages('calExpertMessages');
      updateCurrentCalExpertUi();
    }
  });
  if (!document.hidden) {
    loadCurrentCalLive(function(calState, res){
      if (!isCalExpertRoute(routeParts()[0])) return;
      updateCalExpertLiveFields();
      updateCurrentCalRouteMessages('calExpertMessages');
      if (!currentCalApiSuccess(res)) render();
    });
    loadCurrentCalStateData(function(){
      if (!isCalExpertRoute(routeParts()[0])) return;
      updateCurrentCalRouteMessages('calExpertMessages');
      updateCurrentCalExpertUi();
    });
  }
  startCalExpertPoll();
}

function renderSound(){
  stopPoll();
  var s = state.lastState || {};
  var soundVisible = outputVisible('CH4') || outputVisible('CH5');
  var html = '<div class="app"><div class="panel"><h2>Конфигурация звука</h2>';
  html += renderMessages('');
  html += '<div class="small">CH4 - внешний звонок. CH5 - встроенный зуммер.</div>';
  if (!soundVisible) html += '<div class="error-box">' + esc(SCHEMA_UNAVAILABLE_TEXT) + '</div>';
  if (outputVisible('CH4')) html += '<label style="display:flex;gap:8px;align-items:center;margin-top:12px"><input id="snd_ch4_en" type="checkbox"' + (s.ch4Enabled ? ' checked' : '') + '><span>CH4: звонок включён</span></label>';
  if (outputVisible('CH5')) html += '<label style="display:flex;gap:8px;align-items:center;margin-top:12px"><input id="snd_ch5_en" type="checkbox"' + (s.ch5Enabled ? ' checked' : '') + '><span>CH5: зуммер включён</span></label>';
  html += '<button class="btn light" onclick="testSound()">Проверить звук</button>';
  html += '<div class="small">Квитирование выполняется кнопкой на главной странице и отключает только текущий звук тревоги. Настройки звука ниже определяют, какие звуковые каналы доступны системе.</div>';
  html += '<button class="btn" onclick="saveSoundConfig()">Сохранить настройки звука</button>';
  html += '<button class="btn light" onclick="go(\'#/menu\')">Назад</button>';
  html += '</div></div>';
  app.innerHTML = html;
}

function saveSoundConfig(){
  clearNotice();
  var payload = {};
  if (outputVisible('CH4')) payload.ch4Enabled = !!(byId('snd_ch4_en') && byId('snd_ch4_en').checked);
  if (outputVisible('CH5')) payload.ch5Enabled = !!(byId('snd_ch5_en') && byId('snd_ch5_en').checked);
  api('/api/v1/output/config', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify(payload)
  }, function(res){
    if (res && res.ok) {
      var prev = state.lastState || {};
      if (!prev.ch5Enabled && payload.ch5Enabled) setSuccessNotice('зуммер вкл.');
      else if (!prev.ch4Enabled && payload.ch4Enabled) setSuccessNotice('звонок вкл.');
      else setSuccessNotice('Настройки звука сохранены.');
    } else {
      setNotice((res && (res.error || res.err)) ? (res.error || res.err) : 'Не удалось сохранить настройки звука.');
    }
    loadState(function(){ render(); });
  });
}

function outputConfigModeValue(mode){
  mode = String(mode == null ? '' : mode).toLowerCase();
  return mode === 'cool' ? 'cool' : 'heat';
}
function outputConfigOutIdx(id){
  var m = String(id == null ? '' : id).match(/^CH(\d+)$/);
  if (!m) return -1;
  var idx = Number(m[1]) - 1;
  return (idx >= 0 && idx < 3) ? idx : -1;
}
function effectiveOutputConfigMode(id){
  var outIdx = outputConfigOutIdx(id);
  if (outIdx < 0) return '';
  var sensors = (state.lastState && state.lastState.sensors) ? state.lastState.sensors : [];
  var fallback = '';
  for (var si = 0; si < sensors.length; si++) {
    var sensor = sensors[si] || {};
    var rules = sensor.ctrl || [];
    for (var ri = 0; ri < rules.length; ri++) {
      var rule = rules[ri];
      if (!rule || Number(rule.outIdx) !== outIdx) continue;
      var mode = outputConfigModeValue(rule.logic);
      if (rule.enabled) return mode;
      if (!fallback) fallback = mode;
    }
  }
  return fallback;
}
function renderOutputConfigView(cfg){
  stopPoll();
  cfg = cfg || {};
  var outputs = cfg.outputs || [];
  var visibleOutputIds = ['CH1','CH2','CH3'].filter(outputVisible);
  function modeOf(id){
    for (var i = 0; i < outputs.length; i++) {
      if (outputs[i].id === id) return outputConfigModeValue(outputs[i].mode);
    }
    var effective = effectiveOutputConfigMode(id);
    if (effective) return effective;
    return 'heat';
  }
  var html = '<div class="app"><div class="panel"><h2>Конфигурация выходов CH1–CH3</h2>';
  html += renderMessages('');
  html += '<div class="small">CH4 и CH5 вынесены в раздел «Конфигурация звука».</div>';
  if (!visibleOutputIds.length) html += '<div class="error-box">' + esc(SCHEMA_UNAVAILABLE_TEXT) + '</div>';
  visibleOutputIds.forEach(function(id){
    html += '<div class="field rule-card">';
    html += '<div class="rule-title">' + esc(id) + '</div>';
    html += '<label>Логика</label>';
    html += '<select id="out_mode_' + id + '">';
    html += '<option value="heat">Нагрев</option>';
    html += '<option value="cool">Охлаждение</option>';
    html += '</select></div>';
  });
  html += '<button class="btn" onclick="saveOutputConfig()">Сохранить</button>';
  html += '<button class="btn light" onclick="go(\'#/menu\')">Назад</button>';
  html += '</div></div>';
  app.innerHTML = html;
  visibleOutputIds.forEach(function(id){
    var el = byId('out_mode_' + id);
    if (el) el.value = modeOf(id);
  });
}
function renderOutputConfig(){
  stopPoll();
  api('/api/v1/output/config', null, function(res){
    var list = null;
    if (Array.isArray(res)) {
      list = res;
    } else if (res && Array.isArray(res.outputs)) {
      list = res.outputs;
    }
    var ok = !!(list && list.length);
    var cfg = ok ? { outputs: list } : {
      outputs: (state.lastState && state.lastState.outputs) ? state.lastState.outputs : []
    };
    if (!ok) {
      setNotice('Не удалось загрузить конфигурацию выходов. Показываю текущий режим по состоянию устройства.');
    }
    renderOutputConfigView(cfg);
  });
}

function saveOutputConfig(){
  clearNotice();
  var payload = {
    outputs: ['CH1','CH2','CH3'].filter(outputVisible).map(function(id){
      return { id:id, mode:outputConfigModeValue((byId('out_mode_' + id) && byId('out_mode_' + id).value) || 'heat') };
    })
  };
  api('/api/v1/output/config', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify(payload)
  }, function(res){
    var ok = !!(res && res.ok);
    if (ok) setSuccessNotice('Конфигурация выходов сохранена.');
    else setNotice((res && (res.error || res.err)) ? (res.error || res.err) : 'Не удалось сохранить конфигурацию выходов.');
    if (!ok) {
      renderOutputConfig();
      return;
    }
    reloadConfigAndState(function(){
      renderOutputConfigView((res && res.outputs) ? res : payload);
    });
  });
}

function renderTheme(){
  stopPoll();
  var html = '<div class="app"><div class="panel"><h2>Тема</h2>';
  html += renderMessages('');
  html += '<button class="btn" onclick="setTheme(\'light\')">Светлая тема</button>';
  html += '<button class="btn secondary" onclick="setTheme(\'dark\')">Тёмная тема</button>';
  html += '<button class="btn light" onclick="go(\'#/menu\')">Назад</button>';
  html += '</div></div>';
  app.innerHTML = html;
}

function notifyHttpMessage(){
  return 'Введите имя подписки.';
}
function normalizeNotifyUrl(url){
  return String(url == null ? '' : url).trim();
}
function normalizeNotifyTopic(topic){
  return String(topic == null ? '' : topic).trim().replace(/^\/+|\/+$/g, '');
}
function notifyTopicFromUrl(url){
  url = normalizeNotifyUrl(url);
  if (!url) return '';
  var topic = url;
  if (topic.indexOf(NTFY_HTTP_PREFIX) === 0) {
    topic = topic.slice(NTFY_HTTP_PREFIX.length);
  } else {
    topic = topic.replace(/^https?:\/\/[^/]+\//, '');
    topic = topic.replace(/^\/+/, '');
  }
  try { return decodeURIComponent(topic); }
  catch (e) { return topic; }
}
function notifyUrlFromTopic(topic){
  topic = normalizeNotifyTopic(topic);
  if (!topic) return '';
  return NTFY_HTTP_PREFIX + encodeURIComponent(topic);
}
function validateNotifyUrl(url){
  url = normalizeNotifyUrl(url);
  if (!url) return '';
  return url.indexOf('http://') === 0 ? '' : notifyHttpMessage();
}

function renderNotifications(){
  stopPoll();
  api('/api/v1/notify/config', null, function(res){
    state.notificationsCache = res || {};
    var cfg = state.notificationsCache;
    var topic = notifyTopicFromUrl(cfg.url || '');
    var html = '<div class="app"><div class="panel"><h2>Уведомления ntfy.sh</h2>';
    html += renderMessages('');
    html += '<div class="small">Введите имя своей подписки. Постарайтесь, чтобы имя было уникальным, иначе уведомления могут получать другие люди. Например: буква и номер телефона.</div>';
    html += '<label style="display:flex;gap:8px;align-items:center;margin-top:12px"><input id="notify_enabled" type="checkbox"' + (cfg.enabled ? ' checked' : '') + '><span>Уведомления включены</span></label>';
    html += '<div class="field"><label>Имя подписки</label><div class="notify-topic-row"><span class="notify-prefix">http://ntfy.sh/</span><input id="notify_topic" class="mono notify-topic-input" placeholder="a79001234567" value="' + esc(topic) + '"></div></div>';
    html += '<div class="inline-actions">';
    html += '<button class="btn" onclick="saveNotifyConfig()">Сохранить</button>';
    html += '<button class="btn secondary" onclick="sendNotifyTest()">Тест</button>';
    html += '</div>';
    html += '<button class="btn light" onclick="go(\'#/menu\')">Назад</button>';
    html += '</div></div>';
    app.innerHTML = html;
  });
}

function saveNotifyConfig(){
  clearNotice();
  var topic = normalizeNotifyTopic((byId('notify_topic') && byId('notify_topic').value) || '');
  var payload = {
    enabled: !!(byId('notify_enabled') && byId('notify_enabled').checked),
    url: notifyUrlFromTopic(topic)
  };
  if (payload.enabled && !payload.url) {
    setNotice(notifyHttpMessage());
    return;
  }
  var urlError = validateNotifyUrl(payload.url);
  if (urlError) {
    setNotice(urlError);
    return;
  }
  api('/api/v1/notify/config', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify(payload)
  }, function(res){
    setResultNotice(res, 'Настройки уведомлений сохранены.', 'Не удалось сохранить настройки уведомлений.');
    renderNotifications();
  });
}

function sendNotifyTest(){
  clearNotice();
  var topic = normalizeNotifyTopic((byId('notify_topic') && byId('notify_topic').value) || '');
  var payload = {
    url: notifyUrlFromTopic(topic)
  };
  if (!payload.url) {
    setNotice(notifyHttpMessage());
    return;
  }
  var urlError = validateNotifyUrl(payload.url);
  if (urlError) {
    setNotice(urlError);
    return;
  }
  api('/api/v1/notify/test', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify(payload)
  }, function(res){
    setResultNotice(res, 'Тестовое уведомление отправлено.', 'Не удалось отправить тестовое уведомление.');
    renderNotifications();
  });
}

function renderDiag(){
  stopPoll();
  api('/api/v1/diag', null, function(res){
    var html = '<div class="app"><div class="panel"><h2>Диагностика</h2>';
    html += renderMessages('');
    if (res && res.hardwareLimitations && res.hardwareLimitations.length) {
      html += '<div class="small">Ограничения:</div><ul>';
      for (var i = 0; i < res.hardwareLimitations.length; i++) html += '<li>' + esc(res.hardwareLimitations[i]) + '</li>';
      html += '</ul>';
    }
    html += '<pre style="white-space:pre-wrap;font-size:14px">' + esc(JSON.stringify(res || {}, null, 2)) + '</pre>';
    html += '<button class="btn light" onclick="go(\'#/menu\')">Назад</button>';
    html += '</div></div>';
    app.innerHTML = html;
  });
}

function logTimeText(e){
  var t = e.t || e.time || '';
  if (typeof e.ms !== 'undefined' && t && t.indexOf('.') < 0) {
    var m = Math.abs(Number(e.ms) || 0) % 1000;
    t += '.' + ('00' + m).slice(-3);
  }
  return t;
}

function renderLogPage(){
  stopPoll();
  var html = '<div class="app log-app"><div class="panel log-panel"><h2>Лог</h2>';
  html += renderMessages('');
  html += '<div class="log-toolbar">';
  html += '<button class="btn secondary" onclick="loadLog(function(){ renderLogPage(); })">' + (state.logLoaded ? 'Обновить' : 'Загрузить лог') + '</button>';
  html += '<button class="btn secondary" onclick="downloadLog()">Скачать журнал</button>';
  if (state.logLoaded) html += '<button class="btn danger" onclick="clearLog()">Очистить журнал</button>';
  html += '<button class="btn light" onclick="go(\'#/menu\')">Назад</button>';
  html += '</div>';
  html += '<div class="log-list">';
  if (!state.logLoaded) {
    html += '<div class="small">Журнал загружается только по запросу, чтобы не нагружать устройство.</div>';
  } else {
    if (!state.logEntries.length) html += '<div class="small">Журнал пуст.</div>';
    for (var i = 0; i < state.logEntries.length; i++) {
      var e = state.logEntries[i];
      var t = logTimeText(e);
      var ev = e.e || e.event || '';
      html += '<div class="field rule-card">';
      html += '<div style="font-size:16px;color:var(--muted)">' + esc(t) + '</div>';
      html += '<div style="font-size:18px;font-weight:700;margin-top:4px">' + esc(translateLogEventRu(ev)) + '</div>';
      if (typeof e.T1 !== 'undefined' || typeof e.T2 !== 'undefined' || typeof e.T3 !== 'undefined' || typeof e.dT !== 'undefined') {
        html += '<div class="small" style="margin-top:6px">';
        html += 'T1=' + esc(e.T1) + ' · T2=' + esc(e.T2) + ' · T3=' + esc(e.T3) + ' · dT=' + esc(e.dT);
        html += '</div>';
      }
      html += '</div>';
    }
  }
  html += '</div>';
  html += '</div></div>';
  app.innerHTML = html;
}

function mergeManualOutputState(id, res){
  var s = state.lastState || {};
  var outputs = s.outputs || [];
  var acceptsPendingState = !!(res && (
    res.accepted === true ||
    res.pending === true ||
    res.relayPending === true ||
    normalizeRelayCommand(res.pendingCmd)
  ));
  for (var i = 0; i < outputs.length; i++) {
    var o = outputs[i];
    if (!o || o.id !== id) continue;
    if (typeof res.manualWant !== 'undefined') o.manualWant = !!res.manualWant;
    if (typeof res.requested !== 'undefined') o.requested = !!res.requested;
    if (typeof res.state !== 'undefined') {
      o.state = !!res.state;
      o.actual = !!res.state;
      if (typeof res.displayOn === 'undefined') o.displayOn = !!res.state;
    }
    if (typeof res.actual !== 'undefined') {
      o.actual = !!res.actual;
      o.state = !!res.actual;
      if (typeof res.displayOn === 'undefined') o.displayOn = !!res.actual;
    }
    if (typeof res.displayOn !== 'undefined') o.displayOn = !!res.displayOn;
    if (typeof res.forbidden !== 'undefined') o.forbidden = !!res.forbidden;
    if (typeof res.forbidMask !== 'undefined') o.forbidMask = Number(res.forbidMask || 0);
    if (typeof res.wantOnMask !== 'undefined') o.wantOnMask = Number(res.wantOnMask || 0);
    if (typeof res.operatorHoldOff !== 'undefined') o.operatorHoldOff = !!res.operatorHoldOff;
    if (typeof res.feedbackOn !== 'undefined') o.feedbackOn = !!res.feedbackOn;
    if (typeof res.confirmActual !== 'undefined') o.confirmActual = !!res.confirmActual;
    if (typeof res.confirmAvailable !== 'undefined') o.confirmAvailable = !!res.confirmAvailable;
    if (typeof res.confirmed !== 'undefined') {
      if (typeof res.confirmed === 'number') {
        o.confirmed = Number(res.confirmed) !== 0;
        o.confirmedBool = Number(res.confirmed) !== 0;
        o.confirmedLevel = Number(res.confirmed);
        o.feedbackOn = Number(res.confirmed) === 1;
      } else {
        o.confirmed = !!res.confirmed;
        o.confirmedBool = !!res.confirmed;
      }
    }
    if (typeof res.confirmedBool !== 'undefined') o.confirmedBool = !!res.confirmedBool;
    if (typeof res.confirmMismatch !== 'undefined') o.confirmMismatch = !!res.confirmMismatch;
    if (typeof res.confirmTimeout !== 'undefined') o.confirmTimeout = !!res.confirmTimeout;
    if (typeof res.confirmFaultLatched !== 'undefined') o.confirmFaultLatched = !!res.confirmFaultLatched;
    if (typeof res.confirmExpected !== 'undefined') o.confirmExpected = !!res.confirmExpected;
    if (typeof res.commandActive !== 'undefined') o.commandActive = !!res.commandActive;
    if (typeof res.pending !== 'undefined') o.relayPending = !!res.pending;
    if (typeof res.relayPending !== 'undefined') o.relayPending = !!res.relayPending;
    if (typeof res.pendingCmd !== 'undefined') o.pendingCmd = res.pendingCmd;
    if (typeof res.relayError !== 'undefined') o.relayError = res.relayError || '';
    else if (acceptsPendingState) o.relayError = '';
    if (typeof res.relayErrorMs !== 'undefined') o.relayErrorMs = Number(res.relayErrorMs || 0);
    else if (acceptsPendingState) o.relayErrorMs = 0;
    if (typeof res.relayErrorText !== 'undefined') o.relayErrorText = res.relayErrorText || '';
    else if ((typeof res.relayError !== 'undefined' && !res.relayError) || acceptsPendingState) o.relayErrorText = '';
    break;
  }
  if (typeof res.stopLatched !== 'undefined') s.stopLatched = !!res.stopLatched;
  state.lastState = s;
}

function setManual(id, on){
  clearNotice();
  state.manualFeedback = state.manualFeedback || {};
  if (manualButtonState(findOutput(id) || { id:id }) === 'pending') return;

  var cmd = (on === 'on' || on === true || on === 1 || on === 'true') ? 'on' : 'off';
  var currentOutput = findOutput(id);
  var baselineTerminal = manualPendingBaselineSignature(currentOutput);
  state.manualFeedback[id] = '';
  clearManualRelayErrorFields(currentOutput);
  armManualPendingState(id, cmd, Date.now(), baselineTerminal);
  setManualVisualState(id, 'blink', MANUAL_VISUAL_FEEDBACK_MS);
  render();

  api('/api/v1/relay/' + encodeURIComponent(id) + '/command', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({ cmd:cmd })
  }, function(res){
    mergeManualOutputState(id, res || {});
    if (!res || res.__http === 0) {
      clearManualPendingState(id);
      state.manualFeedback[id] = 'ошибка связи';
      setNotice('Ошибка связи с контроллером.');
      setManualVisualState(id, 'error', MANUAL_VISUAL_FEEDBACK_MS);
    } else if (res.accepted === false) {
      if (res.reason === 'duplicate' || res.reason === 'busy' || res.pending === true || res.relayPending === true) {
        armManualPendingState(id, manualPendingCommand(res, { cmd:cmd }) || cmd, Date.now(), manualPendingBaselineSignature(res));
      } else {
        clearManualPendingState(id);
      }
      state.manualFeedback[id] = res.userMessage || res.detail || res.reason || 'команда отклонена';
      setNotice(state.manualFeedback[id]);
      setManualVisualState(id, 'error', MANUAL_VISUAL_FEEDBACK_MS);
    } else if (!res.ok && res.accepted !== true) {
      clearManualPendingState(id);
      state.manualFeedback[id] = (res.error || res.err || 'Не удалось отправить команду реле.');
      setNotice(state.manualFeedback[id]);
      setManualVisualState(id, 'error', MANUAL_VISUAL_FEEDBACK_MS);
    } else {
      armManualPendingState(id, manualPendingCommand(res, { cmd:cmd }) || cmd, Date.now(), manualPendingBaselineSignature(res));
      state.manualFeedback[id] = '';
      clearNotice();
    }
    syncManualRelayState();
    render();
    setTimeout(function(){
      loadState(function(){ render(); });
    }, MANUAL_VISUAL_FEEDBACK_MS + 80);
  });
}

function renderManual(){
  ensureManualRelayPendingStyles();
  var ids = ['CH1','CH2','CH3'].filter(outputVisible);
  var html = '<div class="app"><div class="panel"><h2>Ручное управление реле</h2>';
  html += renderMessages('');
  if (!ids.length) html += '<div class="error-box">' + esc(SCHEMA_UNAVAILABLE_TEXT) + '</div>';
  for (var i = 0; i < ids.length; i++) {
    var id = ids[i];
    var o = findOutput(id) || { id:id };
    var ctx = manualPendingContext(o);
    var on = manualPhysicalOn(o);
    var desiredOn = manualDesiredOn(o);
    var btnState = manualButtonState(o, ctx);
    var pending = (btnState === 'pending');
    var nextCmd = desiredOn ? 'off' : 'on';
    html += '<div class="rule-card manual-card">';
    html += '<div class="manual-row">';
    html += '<div><div class="rule-title">' + esc(outputTitles[o.id] || o.id) + ' (' + esc(o.id) + ')</div><div class="manual-status-text">Статус: ' + esc(manualStatusText(o, ctx)) + '</div></div>';
    html += '<div class="manual-indicator"><span class="led ' + (on ? 'green' : 'off') + '"></span></div>';
    html += '</div>';
    html += '<button type="button" class="btn manual-toggle-btn' + manualToggleClass(o, btnState) + '" data-btn-state="' + btnState + '" data-next-cmd="' + nextCmd + '" aria-busy="' + (pending ? 'true' : 'false') + '" ' + (pending ? 'disabled ' : '') + 'onclick="setManual(\'' + esc(o.id) + '\',\'' + nextCmd + '\')">' + manualToggleInnerHtml(o, btnState) + '</button>';
    html += '</div>';
  }
  html += '<button class="btn light" onclick="go(\'#/home\')">Назад</button>';
  html += '</div></div>';
  app.innerHTML = html;
  startLiveStatePoll('manual');
}


function renderStopConfirm(){
  stopPoll();
  var html = [];
  html.push('<div class="tpl-screen"><div class="phone tpl-page">');
  html.push(tplHeader());
  html.push('<main class="content">');
  html.push('<section class="hero">Вы уверены, что хотите остановить CH1, CH2 и CH3? Автоматика будет заблокирована до нажатия «Отменить стоп».</section>');
  html.push('<div class="confirm-actions">');
  html.push('<button type="button" class="cell btn" onclick="go(\'#/home\');render();">Отмена</button>');
  html.push('<button type="button" class="cell btn red" onclick="stopMainOutputs(function(){ go(\'#/home\'); render(); });">STOP</button>');
  html.push('</div>');
  html.push('</main></div></div>');
  app.innerHTML = html.join('');
}
function prepareCurrentRoute(view){
  var nextView = view || '';
  var prev = state.currentView || '';
  if (prev === 'manual' && nextView !== 'manual') resetManualRelayUiState();
  if (isCurrentCalLiveRoute(prev) && !isCurrentCalLiveRoute(nextView)) stopPoll();
  if (isCalExpertRoute(prev) && !isCalExpertRoute(nextView)) destroyCurrentCalExpertChart();
  if (!isCalExpertRoute(prev) && isCalExpertRoute(nextView)) invalidateCurrentCalStateData();
  state.currentView = nextView;
}

function renderCurrentRoute(){
  var r = routeParts();
  var view = r[0];
  var arg = r[1] ? decodeURIComponent(r[1]) : '';
  var arg2 = r[2] ? decodeURIComponent(r[2]) : '';
  if (!routeAllowed(view, arg)) {
    if (location.hash !== '#/home') { go('#/home'); return; }
  }
  prepareCurrentRoute(view);
  if (view === 'home') return renderHome();
  if (view === 'menu') return renderMenu();
  if (view === 'sound') return renderSound();
  if (view === 'outputConfig') return renderOutputConfig();
  if (view === 'cal-basic') return renderCalBasic();
  if (view === 'cal-expert') return renderCalExpert();
  if (view === 'theme') return renderTheme();
  if (view === 'notifications') return renderNotifications();
  if (view === 'diag') return renderDiag();
  if (view === 'log') return renderLogPage();
  if (view === 'manual') return renderManual();
  if (view === 'stopConfirm') return renderStopConfirm();
  if (view === 'sensor' && arg) return renderSensor(arg);
  if (view === 'sensorAlarm' && arg) return renderSensorAlarm(arg);
  if (view === 'sensorCtrl' && arg) return renderSensorCtrl(arg);
  if (view === 'ctrlRule' && arg) return renderCtrlRule(arg, Number(arg2 || 0));
  if (view === 'alarmPref' && arg) return renderAlarmPref(arg, arg2 || 'al1');
  if (view === 'num') return renderNumberInput();
  go('#/home');
}

function render(){
  if (!state.lastState) {
    loadState(function(){
      if (state.lastState) render();
      else {
        app.innerHTML = '<div class="app"><div class="panel"><h2>Нет связи с контроллером</h2><div class="error-box">' + esc(connectionLostText()) + '</div></div></div>';
        updateConnectionOverlay();
      }
    });
    return;
  }
  renderCurrentRoute();
  updateHeaderClock();
  updateUnifiedAlertOverlay();
  updateConnectionOverlay();
}

window.addEventListener('hashchange', function(){ setTimeout(render, 0); });
document.addEventListener('visibilitychange', function(){
  var view = routeParts()[0];
  if (document.hidden) {
    if (isCurrentCalLiveRoute(view)) stopPoll();
    return;
  }
  if (isCalExpertRoute(view)) invalidateCurrentCalStateData();
  if (isCurrentCalLiveRoute(view) || isLiveStateRoute(view)) render();
});

window.addEventListener('online', function(){ loadState(function(){ render(); }); });
window.addEventListener('offline', function(){ markConnectionLost('browser_offline'); });
setInterval(function(){
  updateConnectionWatchdog();
}, 1000);

applyUiFontConfig();
applyTheme();
loadSchema(function(){
  loadConfig(function(){
    loadLive(function(){
      autoSyncTimeIfNeeded();
      render();
    });
  });
});


/* ===== Template-based UI overrides ===== */
state.numEdit = null;
var tplHomeOrder = ['T1','T2','T3','dT','P','L','F','C'];
var tplLabels = {
  T1:'T1', T2:'T2', T3:'T3', dT:'dT',
  P:'давление', L:'уровень', F:'проток',
  C:'ток нагрузки', V:'напряжение нагрузки'
};
var CURRENT_SENSOR_CAL_A_DEFAULT = 0.014622769114;
var CURRENT_SENSOR_CAL_B_DEFAULT = -18.724550898204;

function tplSensorLabel(id){
  return tplLabels[id] || sensorLabels[id] || id;
}
function tplHomeSensorLabel(id){
  return id === 'dT' ? 'dT (T2-T1)' : tplSensorLabel(id);
}
function tplHomeSensorErrorText(sensor){
  if (!sensor || !sensor.enabled) return '';
  if (sensor.sensorErrorSticky === true) return 'Ошибка датчика';
  if (sensor.sensorErrorLatched === true) return 'Ошибка датчика';
  return '';
}
function tplComma2(num){
  var n = Number(num);
  if (!isFinite(n)) return '—';
  return n.toFixed(2).replace('.', ',');
}
function tplParseComma(text){
  var normalized = String(text || '').replace(/\s+/g, '').replace(',', '.');
  return Number(normalized);
}

function ensureCurrentCalState(){
  if (!state.currentCal) {
    state.currentCal = {
      a: CURRENT_SENSOR_CAL_A_DEFAULT,
      b: CURRENT_SENSOR_CAL_B_DEFAULT,
      calibrated: false,
      calDate: 0,
      zeroDate: 0,
      raw: null,
      amps: null,
      zeroSet: false,
      loaded: false,
      loading: false,
      x0: null,
      points: [],
      pointCount: 0,
      stateError: '',
      stateLoaded: false,
      stateLoading: false
    };
  }
  if (typeof state.currentCal.calDate === 'undefined') state.currentCal.calDate = 0;
  if (typeof state.currentCal.zeroDate === 'undefined') state.currentCal.zeroDate = 0;
  if (typeof state.currentCal.raw === 'undefined') state.currentCal.raw = null;
  if (typeof state.currentCal.amps === 'undefined') state.currentCal.amps = null;
  if (typeof state.currentCal.zeroSet === 'undefined') state.currentCal.zeroSet = false;
  if (typeof state.currentCal.x0 === 'undefined') state.currentCal.x0 = null;
  if (!Array.isArray(state.currentCal.points)) state.currentCal.points = [];
  if (typeof state.currentCal.pointCount === 'undefined') state.currentCal.pointCount = state.currentCal.points.length;
  if (typeof state.currentCal.stateError === 'undefined') state.currentCal.stateError = '';
  if (typeof state.currentCal.stateLoaded === 'undefined') state.currentCal.stateLoaded = false;
  if (typeof state.currentCal.stateLoading === 'undefined') state.currentCal.stateLoading = false;
  return state.currentCal;
}
function ensureCurrentCalExpertState(){
  if (!state.currentCalExpert) {
    state.currentCalExpert = {
      ampsValue: '',
      addBusy: false,
      clearBusy: false,
      deleteIndex: -1,
      chart: null,
      chartError: '',
      chartRefreshQueued: false,
      chartResizeCleanup: null
    };
  }
  if (typeof state.currentCalExpert.ampsValue === 'undefined') state.currentCalExpert.ampsValue = '';
  if (typeof state.currentCalExpert.addBusy === 'undefined') state.currentCalExpert.addBusy = false;
  if (typeof state.currentCalExpert.clearBusy === 'undefined') state.currentCalExpert.clearBusy = false;
  if (typeof state.currentCalExpert.deleteIndex === 'undefined') state.currentCalExpert.deleteIndex = -1;
  if (typeof state.currentCalExpert.chart === 'undefined') state.currentCalExpert.chart = null;
  if (typeof state.currentCalExpert.chartError === 'undefined') state.currentCalExpert.chartError = '';
  if (typeof state.currentCalExpert.chartRefreshQueued === 'undefined') state.currentCalExpert.chartRefreshQueued = false;
  if (typeof state.currentCalExpert.chartResizeCleanup === 'undefined') state.currentCalExpert.chartResizeCleanup = null;
  return state.currentCalExpert;
}
function invalidateCurrentCalStateData(){
  var cal = ensureCurrentCalState();
  cal.stateError = '';
  cal.stateLoaded = false;
  cal.stateLoading = false;
}
function currentCalA(){
  var cal = ensureCurrentCalState();
  var a = Number(cal.a);
  if (!isFinite(a) || a === 0) return CURRENT_SENSOR_CAL_A_DEFAULT;
  return a;
}
function currentCalB(){
  var cal = ensureCurrentCalState();
  var b = Number(cal.b);
  if (!isFinite(b)) return CURRENT_SENSOR_CAL_B_DEFAULT;
  return b;
}
function loadCurrentCalStatus(cb, forceReload){
  var cal = ensureCurrentCalState();
  if (cal.loading) {
    if (cb) cb(cal);
    return;
  }
  if (cal.loaded && !forceReload) {
    if (cb) cb(cal);
    return;
  }
  cal.loading = true;
  api('/api/v1/calibrate/status', null, function(res){
    cal.loading = false;
    if (currentCalApiSuccess(res)) applyCurrentCalStatusPayload(res);
    else cal.loaded = false;
    if (cb) cb(cal);
  });
}
function currentCalApiSuccess(res){
  if (!res) return false;
  if (res.ok === false) return false;
  if (res.error || res.err) return false;
  var http = Number(res.__http || 0);
  if (http && (http < 200 || http >= 300)) return false;
  return true;
}
function applyCurrentCalLivePayload(res){
  var cal = ensureCurrentCalState();
  cal.raw = (res && res.raw !== null && typeof res.raw !== 'undefined') ? Number(res.raw) : null;
  cal.amps = (res && res.amps !== null && typeof res.amps !== 'undefined') ? Number(res.amps) : null;
  return cal;
}
function applyCurrentCalStatusPayload(res){
  var cal = ensureCurrentCalState();
  var pointsSrc = [];
  var points = [];
  var count = 0;
  applyCurrentCalLivePayload(res);
  pointsSrc = (res && Array.isArray(res.points)) ? res.points : ((res && Array.isArray(res.calPoints)) ? res.calPoints : []);
  for (var i = 0; i < pointsSrc.length; i++) {
    var point = pointsSrc[i] || {};
    points.push({
      raw: (point.raw !== null && typeof point.raw !== 'undefined') ? Number(point.raw) : null,
      amps: (point.amps !== null && typeof point.amps !== 'undefined') ? Number(point.amps) : null
    });
  }
  count = Number(res && (typeof res.count !== 'undefined' ? res.count : res.calPointCount));
  if (res && res.a !== null && typeof res.a !== 'undefined') cal.a = Number(res.a);
  else cal.a = CURRENT_SENSOR_CAL_A_DEFAULT;
  if (res && res.b !== null && typeof res.b !== 'undefined') cal.b = Number(res.b);
  else cal.b = CURRENT_SENSOR_CAL_B_DEFAULT;
  cal.x0 = (res && res.x0 !== null && typeof res.x0 !== 'undefined') ? Number(res.x0) : null;
  cal.zeroSet = (res && typeof res.zeroSet !== 'undefined') ? !!res.zeroSet : (cal.x0 !== null);
  cal.points = points;
  cal.pointCount = (isFinite(count) && count >= 0) ? count : points.length;
  cal.calibrated = !!(res && res.calibrated);
  var zeroDate = Number(res && res.zeroDate) || 0;
  var calDate = Number(res && res.calDate) || 0;
  cal.zeroDate = zeroDate || calDate || 0;
  cal.calDate = calDate || zeroDate || 0;
  cal.loaded = true;
  cal.loading = false;
  cal.stateError = '';
  cal.stateLoaded = true;
  cal.stateLoading = false;
  clearError();
  return cal;
}
function applyCurrentCalStatePayload(res){
  return applyCurrentCalStatusPayload(res);
}
function loadCurrentCalStateData(cb, forceReload){
  var cal = ensureCurrentCalState();
  if (cal.stateLoading) {
    if (cb) cb(cal, null);
    return;
  }
  if (cal.stateLoaded && !forceReload) {
    if (cb) cb(cal, null);
    return;
  }
  cal.stateLoading = true;
  api('/api/v1/calibrate/status', null, function(res){
    cal.stateLoading = false;
    if (currentCalApiSuccess(res)) {
      applyCurrentCalStatePayload(res);
    } else {
      cal.stateLoaded = false;
      cal.stateError = (res && (res.error || res.err)) ? String(res.error || res.err) : 'Не удалось получить точки калибровки.';
    }
    if (cb) cb(cal, res);
  });
}

function currentRawFromPercent(percent){
  var n = Number(percent);
  if (!isFinite(n)) return NaN;
  if (n < 0) n = 0;
  if (n > 100) n = 100;
  return (n * 4095) / 100;
}
function currentPercentFromRaw(raw){
  var n = Number(raw);
  if (!isFinite(n)) return NaN;
  if (n < 0) n = 0;
  if (n > 4095) n = 4095;
  return (n * 100) / 4095;
}
function ampsFromRaw(raw){
  var n = Number(raw);
  if (!isFinite(n)) return NaN;
  if (n < 0) n = 0;
  if (n > 4095) n = 4095;
  return (currentCalA() * n) + currentCalB();
}
function ampsFromPercent(percent){
  var raw = currentRawFromPercent(percent);
  if (!isFinite(raw)) return NaN;
  return ampsFromRaw(raw);
}
function percentFromAmps(amps){
  var n = Number(amps);
  if (!isFinite(n)) return NaN;
  return (((n - currentCalB()) / currentCalA()) * 100) / 4095;
}
function tplCurrentValue(amps){
  if (amps === null || typeof amps === 'undefined') return '—';
  var n = Number(amps);
  if (!isFinite(n)) return '—';
  return tplComma2(n) + ' А';
}
function tplCurrentFromApi(amps){
  if (amps === null || typeof amps === 'undefined') return '—';
  var value = Number(amps);
  if (!isFinite(value)) return '—';
  return tplCurrentValue(value);
}
function tplCurrentFromPercent(percent){
  var amps = ampsFromPercent(percent);
  if (!isFinite(amps)) return '—';
  return tplCurrentValue(amps);
}
function tplCurrentPercentFromRaw(raw){
  var percent = currentPercentFromRaw(raw);
  if (!isFinite(percent)) return '—';
  return tplComma2(percent) + ' %';
}
function tplCurrentRawValue(raw){
  if (raw === null || typeof raw === 'undefined') return '—';
  var n = Number(raw);
  if (!isFinite(n)) return '—';
  return tplComma2(n);
}
function tplCurrentRawIntValue(raw){
  if (raw === null || typeof raw === 'undefined') return '—';
  var n = Number(raw);
  if (!isFinite(n)) return '—';
  return String(Math.round(n));
}
function currentCalKnownValueError(value){
  var text = String(value == null ? '' : value).trim();
  if (!text) return 'Введите ток от 0 до 20 A';
  var amps = tplParseComma(text);
  if (!isFinite(amps) || amps < 0 || amps > 20) return 'Введите ток от 0 до 20 A';
  return '';
}
function currentCalPointAddErrorText(res){
  var type = String(res && res.type ? res.type : '');
  if (type === 'bad_amps' || type === 'bad_value') return 'Введите ток от 0 до 20 A';
  if (type === 'sensor_unavailable') return 'Нет актуального значения датчика тока';
  if (type === 'too_many_points') return 'Максимум 10 точек калибровки';
  if (type === 'duplicate_raw') return 'Точка с таким raw уже существует';
  if (type === 'non_monotonic') return 'Точка нарушает монотонность зависимости raw(ток). Проверьте значения.';
  if (type === 'point_add_failed') return 'Не удалось добавить точку калибровки';
  if (type === 'storage') return 'Не удалось сохранить калибровку тока';
  if (res && (res.error === 'network_error' || res.error === 'network_timeout' || res.error === 'bad_json')) {
    return 'Не удалось добавить точку. Проверьте связь с контроллером и повторите попытку.';
  }
  if (res && (res.error || res.err)) return String(res.error || res.err);
  return 'Не удалось добавить точку. Повторите попытку.';
}
function currentCalPointDeleteErrorText(res){
  var type = String(res && res.type ? res.type : '');
  if (type === 'bad_index') return 'Точка с таким номером не найдена';
  if (type === 'storage') return 'Не удалось сохранить калибровку тока';
  if (res && (res.error === 'network_error' || res.error === 'network_timeout' || res.error === 'bad_json')) {
    return 'Не удалось удалить точку. Проверьте связь с контроллером и повторите попытку.';
  }
  if (res && (res.error || res.err)) return String(res.error || res.err);
  return 'Не удалось удалить точку. Повторите попытку.';
}
function currentCalClearErrorText(res){
  var type = String(res && res.type ? res.type : '');
  if (type === 'storage') return 'Не удалось сохранить калибровку тока';
  if (res && (res.error === 'network_error' || res.error === 'network_timeout' || res.error === 'bad_json')) {
    return 'Не удалось очистить точки. Проверьте связь с контроллером и повторите попытку.';
  }
  if (res && (res.error || res.err)) return String(res.error || res.err);
  return 'Не удалось очистить точки калибровки. Повторите попытку.';
}
function currentCalStateErrorHtml(){
  var cal = ensureCurrentCalState();
  if (!cal.stateError) return '';
  return '<div class="error-box">' + esc(cal.stateError) + '</div>';
}
function currentCalExpertChartErrorHtml(){
  var expert = ensureCurrentCalExpertState();
  if (!expert.chartError) return '';
  return '<div class="notice-box">График временно недоступен. ' + esc(expert.chartError) + '</div>';
}
function currentCalExpertBusy(){
  var expert = ensureCurrentCalExpertState();
  return !!(expert.addBusy || expert.clearBusy || expert.deleteIndex >= 0);
}
function currentCalExpertCountText(){
  var cal = ensureCurrentCalState();
  if (!cal.stateLoaded) return '... / 10';
  return String(Math.max(0, Number(cal.pointCount) || 0)) + ' / 10';
}
function currentCalExpertInputError(value){
  var cal = ensureCurrentCalState();
  var text = String(value == null ? '' : value).trim();
  if (cal.stateLoaded && Number(cal.pointCount) >= 10) return 'Достигнут лимит 10 точек. Удалите одну из существующих, чтобы добавить новую.';
  if (!text) return '';
  return currentCalKnownValueError(text);
}
function currentCalExpertAddDisabled(sensorEnabled){
  var expert = ensureCurrentCalExpertState();
  var cal = ensureCurrentCalState();
  return !sensorEnabled || expert.addBusy || expert.clearBusy || expert.deleteIndex >= 0
      || cal.stateLoading || !cal.stateLoaded || Number(cal.pointCount) >= 10
      || !!currentCalExpertInputError(expert.ampsValue);
}
function currentCalExpertClearDisabled(){
  var cal = ensureCurrentCalState();
  return currentCalExpertBusy() || !cal.stateLoaded || !(Number(cal.pointCount) > 0);
}
function currentCalExpertLinearHtml(){
  var cal = ensureCurrentCalState();
  if (!cal.stateLoaded) {
    return '<div class="small" style="margin-top:12px">Загрузка параметров калибровки...</div>';
  }
  var x0Text = (cal.x0 !== null && typeof cal.x0 !== 'undefined' && isFinite(Number(cal.x0)))
      ? tplCurrentRawIntValue(cal.x0)
      : 'не задан';
  return '<div class="cal-expert-coeffs">'
      + '<div class="cal-expert-coeff"><div class="small">a</div><div class="mono">' + esc(String(Number(cal.a))) + '</div></div>'
      + '<div class="cal-expert-coeff"><div class="small">b</div><div class="mono">' + esc(String(Number(cal.b))) + '</div></div>'
      + '<div class="cal-expert-coeff"><div class="small">Ноль (x0), raw</div><div class="mono">' + esc(String(x0Text)) + '</div></div>'
      + '</div>';
}
function currentCalExpertPointsTableHtml(){
  var cal = ensureCurrentCalState();
  var expert = ensureCurrentCalExpertState();
  var points = Array.isArray(cal.points) ? cal.points : [];
  var html = '';
  if (!cal.stateLoaded) {
    return '<div class="cal-expert-table"><div class="cal-expert-table-empty">Загрузка точек калибровки...</div></div>';
  }
  if (!points.length) {
    return '<div class="cal-expert-table"><div class="cal-expert-table-empty">Точки калибровки пока не добавлены.</div></div>';
  }
  html += '<div class="cal-expert-table">';
  html += '<div class="cal-expert-table-head"><div class="cal-expert-table-cell">№</div><div class="cal-expert-table-cell">Raw</div><div class="cal-expert-table-cell">Ток (A)</div><div class="cal-expert-table-cell">Действие</div></div>';
  for (var i = 0; i < points.length; i++) {
    var point = points[i] || {};
    var deleting = expert.deleteIndex === i;
    html += '<div class="cal-expert-table-row">';
    html += '<div class="cal-expert-table-cell" data-label="№">' + esc(String(i + 1)) + '</div>';
    html += '<div class="cal-expert-table-cell mono" data-label="Raw">' + esc(tplCurrentRawIntValue(point.raw)) + '</div>';
    html += '<div class="cal-expert-table-cell mono" data-label="Ток (A)">' + esc(tplCurrentFromApi(point.amps)) + '</div>';
    html += '<div class="cal-expert-table-cell action" data-label="Действие"><button type="button" class="btn danger" onclick="deleteCurrentCalExpertPoint(' + i + ')"'
         + (currentCalExpertBusy() ? ' disabled' : '')
         + (deleting ? ' aria-busy="true"' : '')
         + '>' + esc(deleting ? 'Удаляем...' : 'Удалить') + '</button></div>';
    html += '</div>';
  }
  html += '</div>';
  return html;
}
function currentCalExpertChartStatusHtml(){
  var cal = ensureCurrentCalState();
  var expert = ensureCurrentCalExpertState();
  if (expert.chartError) {
    return '<div class="cal-expert-chart-note"><strong>График временно недоступен</strong>' + esc(expert.chartError) + '</div>';
  }
  if (cal.stateLoading && !cal.stateLoaded) {
    return '<div class="cal-expert-chart-note"><strong>Подготавливаем график</strong>Загружаем текущее состояние калибровки и список точек.</div>';
  }
  if (!cal.stateLoaded) {
    return '<div class="cal-expert-chart-note"><strong>График ждёт данные</strong>' + esc(cal.stateError || 'Не удалось получить точки калибровки.') + '</div>';
  }
  if (!window.uPlot) {
    return '<div class="cal-expert-chart-note"><strong>Подготавливаем график</strong>Загружаем локальный модуль uPlot. Таблица точек уже доступна ниже.</div>';
  }
  return '';
}
function currentCalExpertChartCardHtml(){
  var fallback = currentCalExpertChartStatusHtml();
  return '<div class="field rule-card cal-expert-chart-card">'
      + '<div class="cal-expert-chart-head">'
      + '<div><div class="small">График калибровки</div>'
      + '<h3 class="cal-expert-chart-title">Ток 0..20 A -> Raw</h3>'
      + '<div class="small cal-expert-chart-caption">Кривая строится по текущему режиму, точки и live-значение накладываются поверх.</div></div>'
      + '<div class="cal-expert-chart-legend">'
      + '<span class="cal-expert-chart-legend-item"><span class="cal-expert-chart-legend-swatch" style="background:#2563eb"></span>Кривая</span>'
      + '<span class="cal-expert-chart-legend-item"><span class="cal-expert-chart-legend-swatch" style="background:#b91c1c"></span>Точки</span>'
      + '<span class="cal-expert-chart-legend-item"><span class="cal-expert-chart-legend-swatch" style="background:#166534"></span>Текущее</span>'
      + '</div></div>'
      + '<div class="cal-expert-chart-shell"><div class="cal-expert-chart-frame">'
      + '<div id="calExpertChart" class="cal-expert-chart-canvas"></div>'
      + '<div id="calExpertChartFallback" class="cal-expert-chart-fallback"' + (fallback ? '' : ' style="display:none"') + '>'
      + fallback
      + '</div></div></div></div>';
}
function currentCalExpertSortedPoints(cal){
  var points = Array.isArray(cal && cal.points) ? cal.points : [];
  var list = [];
  for (var i = 0; i < points.length; i++) {
    var point = points[i] || {};
    var raw = Number(point.raw);
    var amps = Number(point.amps);
    if (!isFinite(raw) || !isFinite(amps)) continue;
    list.push({ raw: raw, amps: amps });
  }
  list.sort(function(a, b){
    if (a.amps === b.amps) return a.raw - b.raw;
    return a.amps - b.amps;
  });
  return list;
}
function currentCalExpertRawFromLinear(cal, amps){
  var a = Number(cal && cal.a);
  var b = Number(cal && cal.b);
  if (!isFinite(a) || a === 0) a = currentCalA();
  if (!isFinite(b)) b = currentCalB();
  if (!isFinite(a) || a === 0) return NaN;
  return (Number(amps) - b) / a;
}
function currentCalExpertClampRaw(raw){
  var value = Number(raw);
  if (!isFinite(value)) return NaN;
  if (value < 0) return 0;
  if (value > 4095) return 4095;
  return value;
}
function currentCalExpertRawFromAmps(cal, amps, points){
  var raw = currentCalExpertRawFromLinear(cal, amps);
  return currentCalExpertClampRaw(raw);
}
function currentCalExpertBuildChartModel(){
  var cal = ensureCurrentCalState();
  if (!cal.stateLoaded) return null;
  var points = currentCalExpertSortedPoints(cal);
  var xValues = [];
  var xSeen = {};
  var pointMap = {};
  var liveAmps = Number(cal.amps);
  var liveRaw = currentCalExpertClampRaw(cal.raw);
  function xKey(value){
    return Number(value).toFixed(6);
  }
  function clampAmps(value){
    var n = Number(value);
    if (!isFinite(n)) return NaN;
    if (n < 0) return 0;
    if (n > 20) return 20;
    return n;
  }
  function addX(value){
    var x = clampAmps(value);
    if (!isFinite(x)) return;
    var key = xKey(x);
    if (xSeen[key]) return;
    xSeen[key] = true;
    xValues.push(x);
  }
  addX(0);
  addX(20);
  for (var j = 0; j < points.length; j++) {
    var point = points[j];
    var pointX = clampAmps(point.amps);
    if (!isFinite(pointX)) continue;
    addX(pointX);
    pointMap[xKey(pointX)] = currentCalExpertClampRaw(point.raw);
  }
  var liveX = clampAmps(liveAmps);
  if (isFinite(liveX) && isFinite(liveRaw)) addX(liveX);
  xValues.sort(function(a, b){ return a - b; });
  if (!xValues.length) return null;
  var curve = [];
  var pointSeries = [];
  var liveSeries = [];
  var yValues = [];
  for (var k = 0; k < xValues.length; k++) {
    var x = xValues[k];
    var key = xKey(x);
    var curveRaw = currentCalExpertRawFromAmps(cal, x, points);
    curve.push(isFinite(curveRaw) ? curveRaw : null);
    if (isFinite(curveRaw)) yValues.push(curveRaw);
    var pointRaw = pointMap.hasOwnProperty(key) ? pointMap[key] : null;
    pointSeries.push(isFinite(pointRaw) ? pointRaw : null);
    if (isFinite(pointRaw)) yValues.push(pointRaw);
    var livePointRaw = (isFinite(liveX) && isFinite(liveRaw) && key === xKey(liveX)) ? liveRaw : null;
    liveSeries.push(isFinite(livePointRaw) ? livePointRaw : null);
    if (isFinite(livePointRaw)) yValues.push(livePointRaw);
  }
  if (!yValues.length) return null;
  var yMin = yValues[0];
  var yMax = yValues[0];
  for (var m = 1; m < yValues.length; m++) {
    if (yValues[m] < yMin) yMin = yValues[m];
    if (yValues[m] > yMax) yMax = yValues[m];
  }
  var pad = (yMax - yMin) * 0.12;
  if (!isFinite(pad) || pad < 40) pad = 40;
  yMin = currentCalExpertClampRaw(yMin - pad);
  yMax = currentCalExpertClampRaw(yMax + pad);
  if (!isFinite(yMin)) yMin = 0;
  if (!isFinite(yMax)) yMax = 4095;
  if (yMax <= yMin) {
    yMin = currentCalExpertClampRaw(yMin - 40);
    yMax = currentCalExpertClampRaw(yMax + 40);
  }
  if (yMax <= yMin) {
    yMin = 0;
    yMax = 4095;
  }
  return {
    data: [xValues, curve, pointSeries, liveSeries],
    yMin: yMin,
    yMax: yMax
  };
}
function currentCalExpertChartTextColor(){
  if (typeof getComputedStyle === 'function' && typeof document !== 'undefined' && document.documentElement) {
    var muted = getComputedStyle(document.documentElement).getPropertyValue('--muted');
    if (muted) {
      muted = String(muted).trim();
      if (muted) return muted;
    }
  }
  return '#64748b';
}
function currentCalExpertLoadCss(url, id){
  return new Promise(function(resolve, reject){
    if (byId(id)) {
      resolve(true);
      return;
    }
    var link = document.createElement('link');
    link.id = id;
    link.rel = 'stylesheet';
    link.href = url;
    link.onload = function(){ resolve(true); };
    link.onerror = function(){
      if (link.parentNode) link.parentNode.removeChild(link);
      reject(new Error('Не удалось загрузить локальный стиль uPlot.'));
    };
    document.head.appendChild(link);
  });
}
function currentCalExpertLoadScript(url, id){
  return new Promise(function(resolve, reject){
    if (window.uPlot) {
      resolve(window.uPlot);
      return;
    }
    var existing = byId(id);
    if (existing) {
      existing.addEventListener('load', function(){ resolve(window.uPlot); }, { once:true });
      existing.addEventListener('error', function(){ reject(new Error('Не удалось загрузить локальный скрипт uPlot.')); }, { once:true });
      return;
    }
    var script = document.createElement('script');
    script.id = id;
    script.src = url;
    script.async = true;
    script.onload = function(){
      if (!window.uPlot) {
        reject(new Error('Локальный uPlot загружен некорректно.'));
        return;
      }
      resolve(window.uPlot);
    };
    script.onerror = function(){
      if (script.parentNode) script.parentNode.removeChild(script);
      reject(new Error('Не удалось загрузить локальный скрипт uPlot.'));
    };
    document.head.appendChild(script);
  });
}
function loadCurrentCalExpertChartAssets(){
  if (window.uPlot) return Promise.resolve(window.uPlot);
  if (currentCalExpertUPlotLoadPromise) return currentCalExpertUPlotLoadPromise;
  currentCalExpertUPlotLoadPromise = currentCalExpertLoadCss(CURRENT_CAL_EXPERT_UPLOT_CSS_URL, 'rc-uplot-css').then(function(){
    return currentCalExpertLoadScript(CURRENT_CAL_EXPERT_UPLOT_JS_URL, 'rc-uplot-js');
  }).then(function(lib){
    if (!window.uPlot) throw new Error('Локальный uPlot недоступен после загрузки.');
    return lib;
  }).catch(function(err){
    currentCalExpertUPlotLoadPromise = null;
    throw err;
  });
  return currentCalExpertUPlotLoadPromise;
}
function destroyCurrentCalExpertChart(){
  var expert = ensureCurrentCalExpertState();
  if (expert.chartResizeCleanup) {
    try { expert.chartResizeCleanup(); } catch (e) {}
    expert.chartResizeCleanup = null;
  }
  if (expert.chart && typeof expert.chart.destroy === 'function') {
    try { expert.chart.destroy(); } catch (e2) {}
  }
  expert.chart = null;
  expert.chartRefreshQueued = false;
  var node = byId('calExpertChart');
  if (node) node.innerHTML = '';
}
function currentCalExpertEnsureChartResizeBinding(){
  var expert = ensureCurrentCalExpertState();
  if (expert.chartResizeCleanup) return;
  var node = byId('calExpertChart');
  if (!node) return;
  var frame = node.parentNode;
  var onResize = function(){
    if (isCalExpertRoute(routeParts()[0])) scheduleCurrentCalExpertChartRefresh();
  };
  var cleanup = [];
  window.addEventListener('resize', onResize);
  cleanup.push(function(){ window.removeEventListener('resize', onResize); });
  if (typeof ResizeObserver !== 'undefined' && frame) {
    var observer = new ResizeObserver(onResize);
    observer.observe(frame);
    cleanup.push(function(){ observer.disconnect(); });
  }
  expert.chartResizeCleanup = function(){
    for (var i = 0; i < cleanup.length; i++) {
      try { cleanup[i](); } catch (e) {}
    }
  };
}
function currentCalExpertRefreshChart(){
  var expert = ensureCurrentCalExpertState();
  expert.chartRefreshQueued = false;
  if (!isCalExpertRoute(routeParts()[0])) return;
  var chartNode = byId('calExpertChart');
  var fallbackNode = byId('calExpertChartFallback');
  if (!chartNode) return;
  var fallbackHtml = currentCalExpertChartStatusHtml();
  if (fallbackNode) {
    fallbackNode.innerHTML = fallbackHtml;
    fallbackNode.style.display = fallbackHtml ? 'flex' : 'none';
  }
  if (fallbackHtml) {
    if (expert.chart && typeof expert.chart.destroy === 'function') {
      try { expert.chart.destroy(); } catch (e) {}
      expert.chart = null;
    }
    return;
  }
  if (!window.uPlot) return;
  var model = currentCalExpertBuildChartModel();
  if (!model) {
    if (fallbackNode) {
      fallbackNode.innerHTML = '<div class="cal-expert-chart-note"><strong>График ждёт данные</strong>Пока недостаточно корректных значений для построения кривой.</div>';
      fallbackNode.style.display = 'flex';
    }
    if (expert.chart && typeof expert.chart.destroy === 'function') {
      try { expert.chart.destroy(); } catch (e2) {}
      expert.chart = null;
    }
    return;
  }
  if (fallbackNode) {
    fallbackNode.innerHTML = '';
    fallbackNode.style.display = 'none';
  }
  var width = chartNode.clientWidth || (chartNode.parentNode && chartNode.parentNode.clientWidth) || 320;
  width = Math.max(280, Math.round(width));
  var height = CURRENT_CAL_EXPERT_CHART_HEIGHT;
  var textColor = currentCalExpertChartTextColor();
  if (!expert.chart) {
    chartNode.innerHTML = '';
    expert.chart = new uPlot({
      width: width,
      height: height,
      padding: [14, 12, 10, 12],
      legend: { show: false },
      cursor: { drag: { x: false, y: false } },
      scales: {
        x: { time: false, auto: false, min: 0, max: 20 },
        y: { auto: false, min: model.yMin, max: model.yMax }
      },
      axes: [
        {
          stroke: textColor,
          grid: { stroke: '#cfd6de', width: 1 },
          values: function(u, values){
            var out = [];
            for (var i = 0; i < values.length; i++) out.push(tplComma2(values[i]));
            return out;
          }
        },
        {
          stroke: textColor,
          grid: { stroke: '#cfd6de', width: 1 },
          values: function(u, values){
            var out = [];
            for (var i = 0; i < values.length; i++) out.push(String(Math.round(values[i])));
            return out;
          }
        }
      ],
      series: [
        {},
        {
          label: 'Curve',
          stroke: '#2563eb',
          width: 2,
          spanGaps: true,
          points: { show: false }
        },
        {
          label: 'Points',
          stroke: 'rgba(0,0,0,0)',
          width: 0,
          spanGaps: true,
          points: {
            show: true,
            size: 8,
            width: 2,
            stroke: '#f8fafc',
            fill: '#b91c1c'
          }
        },
        {
          label: 'Live',
          stroke: 'rgba(0,0,0,0)',
          width: 0,
          spanGaps: true,
          points: {
            show: true,
            size: 10,
            width: 3,
            stroke: '#0f172a',
            fill: '#166534'
          }
        }
      ]
    }, model.data, chartNode);
    currentCalExpertEnsureChartResizeBinding();
  } else {
    expert.chart.setSize({ width: width, height: height });
    expert.chart.setScale('x', { min: 0, max: 20 });
    expert.chart.setScale('y', { min: model.yMin, max: model.yMax });
    expert.chart.setData(model.data);
  }
}
function scheduleCurrentCalExpertChartRefresh(){
  var expert = ensureCurrentCalExpertState();
  if (expert.chartRefreshQueued) return;
  expert.chartRefreshQueued = true;
  if (typeof requestAnimationFrame === 'function') {
    requestAnimationFrame(currentCalExpertRefreshChart);
  } else {
    setTimeout(currentCalExpertRefreshChart, 16);
  }
}
function tplValueText(sensor, blankWhenDisabled){
  if (!sensor) return blankWhenDisabled ? '' : '—';
  if (!sensor.enabled) return blankWhenDisabled ? '' : '—';
  if (sensor.error || sensor.value == null) return '—';
  if (sensor.id === 'L') return (typeof sensor.circuitOpen === 'boolean' ? sensor.circuitOpen : (Number(sensor.value) <= 0.5)) ? 'MAX!' : 'OK';
  if (sensor.id === 'F') return flowAlarmVisible(sensor) ? 'Нет протока!' : 'OK';
  if (sensor.id === 'C') return tplCurrentFromApi(sensor.amps);
  return tplComma2(sensor.value);
}
function tplHomeValueText(sensor){
  if (!sensor) return '';
  if (!sensor.enabled) return '';
  if (sensor.id === 'C') {
    if (sensor.error) return '—';
    return tplCurrentFromApi(sensor.amps);
  }
  return tplValueText(sensor, true) || '';
}
function pad2(num){
  num = Number(num) || 0;
  return num < 10 ? ('0' + num) : String(num);
}
function formatUnixDateTime(unixSec){
  var sec = Number(unixSec);
  if (!isFinite(sec) || sec <= 0) return '—';
  var d = new Date(sec * 1000);
  if (!isFinite(d.getTime())) return '—';
  return pad2(d.getDate()) + '.' + pad2(d.getMonth() + 1) + '.' + d.getFullYear()
      + ' ' + pad2(d.getHours()) + ':' + pad2(d.getMinutes()) + ':' + pad2(d.getSeconds());
}
function updateCurrentCalRouteMessages(targetId){
  var node = byId(targetId);
  if (!node) return;
  var extra = currentCalStateErrorHtml();
  if (targetId === 'calExpertMessages') extra += currentCalExpertChartErrorHtml();
  node.innerHTML = renderMessages(extra);
}
function refreshCurrentCalExpertRoute(options){
  if (!isCalExpertRoute(routeParts()[0])) return;
  updateCurrentCalRouteMessages('calExpertMessages');
  updateCurrentCalExpertUi(options);
}
function loadCurrentCalLive(cb){
  var cal = ensureCurrentCalState();
  api('/api/v1/calibrate/live', null, function(res){
    if (currentCalApiSuccess(res)) {
      applyCurrentCalLivePayload(res);
      state.lastValidStateMs = Date.now();
      clearError();
    } else if (res && (res.error || res.err)) {
      setError(res.error || res.err);
    } else {
      setError('Не удалось получить live-данные калибровки.');
    }
    if (cb) cb(cal, res);
  });
}
function updateCalBasicLiveFields(){
  var cal = ensureCurrentCalState();
  var ampsValue = byId('calBasicAmpsValue');
  var rawValue = byId('calBasicRawValue');
  if (ampsValue) ampsValue.textContent = 'Нагрузка: ' + tplCurrentFromApi(cal.amps);
  if (rawValue) rawValue.textContent = 'Raw: ' + tplCurrentRawIntValue(cal.raw);
}
function updateCalBasicStatusFields(){
  var cal = ensureCurrentCalState();
  var zeroDateValue = byId('calBasicZeroDateValue');
  if (zeroDateValue) zeroDateValue.textContent = formatUnixDateTime(cal.zeroDate);
}
function updateCalExpertLiveFields(){
  var cal = ensureCurrentCalState();
  var ampsValue = byId('calExpertAmpsValue');
  var rawValue = byId('calExpertRawValue');
  if (ampsValue) ampsValue.textContent = 'Нагрузка: ' + tplCurrentFromApi(cal.amps);
  if (rawValue) rawValue.textContent = 'Raw: ' + tplCurrentRawIntValue(cal.raw);
  scheduleCurrentCalExpertChartRefresh();
}
function updateCurrentCalExpertUi(options){
  options = options || {};
  var cal = ensureCurrentCalState();
  var expert = ensureCurrentCalExpertState();
  var sensor = findSensor('C') || { enabled:false };
  var countValue = byId('calExpertCountValue');
  var linearValues = byId('calExpertLinearValues');
  var inputError = byId('calExpertInputError');
  var addBtn = byId('calExpertAddBtn');
  var clearBtn = byId('calExpertClearBtn');
  var tableWrap = byId('calExpertPointsTable');
  var chartFallback = byId('calExpertChartFallback');
  if (countValue) countValue.textContent = currentCalExpertCountText();
  if (linearValues) linearValues.innerHTML = currentCalExpertLinearHtml();
  if (inputError && !options.keepInputError) inputError.textContent = currentCalExpertInputError(expert.ampsValue);
  if (addBtn) {
    addBtn.disabled = currentCalExpertAddDisabled(sensor.enabled);
    addBtn.textContent = expert.addBusy ? 'Добавляем...' : 'Добавить точку';
    if (expert.addBusy) addBtn.setAttribute('aria-busy', 'true');
    else addBtn.removeAttribute('aria-busy');
  }
  if (clearBtn) {
    clearBtn.disabled = currentCalExpertClearDisabled();
    clearBtn.textContent = expert.clearBusy ? 'Очищаем...' : 'Очистить все';
    if (expert.clearBusy) clearBtn.setAttribute('aria-busy', 'true');
    else clearBtn.removeAttribute('aria-busy');
  }
  if (tableWrap) tableWrap.innerHTML = currentCalExpertPointsTableHtml();
  if (chartFallback) {
    var fallbackHtml = currentCalExpertChartStatusHtml();
    chartFallback.innerHTML = fallbackHtml;
    chartFallback.style.display = fallbackHtml ? 'flex' : 'none';
  }
  scheduleCurrentCalExpertChartRefresh();
}
function startCalBasicPoll(){
  startPoll(CAL_BASIC_POLL_MS, function(done){
    loadCurrentCalLive(function(calState, res){
      if (isCalBasicRoute(routeParts()[0])) {
        updateCalBasicLiveFields();
        updateCurrentCalRouteMessages('calBasicMessages');
        if (!currentCalApiSuccess(res)) render();
      }
      done();
    });
  });
}
function startCalExpertPoll(){
  startPoll(CAL_BASIC_POLL_MS, function(done){
    loadCurrentCalLive(function(calState, res){
      if (isCalExpertRoute(routeParts()[0])) {
        updateCalExpertLiveFields();
        updateCurrentCalRouteMessages('calExpertMessages');
        if (!currentCalApiSuccess(res)) render();
      }
      done();
    });
  });
}
function onCurrentCalExpertAmpsInput(value){
  var expert = ensureCurrentCalExpertState();
  var input = byId('calExpertAmpsInput');
  expert.ampsValue = String(value == null ? '' : value);
  if (input && input.value !== expert.ampsValue) input.value = expert.ampsValue;
  if (isCalExpertRoute(routeParts()[0])) updateCurrentCalExpertUi();
}
function submitCalBasicZero(){
  if (state.currentCalBasicBusy) return;
  state.currentCalBasicBusy = true;
  clearNotice();
  if (isCalBasicRoute(routeParts()[0])) render();
  api('/api/v1/calibrate/zero', { method:'POST' }, function(res){
    state.currentCalBasicBusy = false;
    if (currentCalApiSuccess(res)) {
      applyCurrentCalStatePayload(res);
      setSuccessNotice('Ноль установлен');
      loadCurrentCalLive(function(){
        if (isCalBasicRoute(routeParts()[0])) render();
      });
      return;
    }
    setNotice((res && (res.error || res.err)) ? (res.error || res.err) : 'Не удалось установить ноль. Повторите попытку.');
    if (isCalBasicRoute(routeParts()[0])) render();
  });
}
function submitCurrentCalExpertPoint(){
  var expert = ensureCurrentCalExpertState();
  var cal = ensureCurrentCalState();
  var sensor = findSensor('C') || { enabled:false };
  if (currentCalExpertAddDisabled(sensor.enabled)) {
    if (Number(cal.pointCount) >= 10) setNotice('Максимум 10 точек калибровки');
    else {
      var inputError = currentCalExpertInputError(expert.ampsValue);
      if (inputError) setNotice(inputError);
    }
    if (isCalExpertRoute(routeParts()[0])) render();
    return;
  }
  expert.addBusy = true;
  clearNotice();
  if (isCalExpertRoute(routeParts()[0])) render();
  api('/api/v1/calibrate/point', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({ value: tplParseComma(expert.ampsValue) })
  }, function(res){
    expert.addBusy = false;
    if (currentCalApiSuccess(res)) {
      expert.ampsValue = '';
      applyCurrentCalStatePayload(res);
      setSuccessNotice('Точка добавлена');
      loadCurrentCalLive(function(){
        if (isCalExpertRoute(routeParts()[0])) render();
      });
      return;
    }
    setNotice(currentCalPointAddErrorText(res));
    if (isCalExpertRoute(routeParts()[0])) render();
  });
}
function deleteCurrentCalExpertPoint(index){
  var expert = ensureCurrentCalExpertState();
  var cal = ensureCurrentCalState();
  var idx = Number(index);
  if (!cal.stateLoaded || currentCalExpertBusy() || !isFinite(idx) || idx < 0) return;
  expert.deleteIndex = idx;
  clearNotice();
  refreshCurrentCalExpertRoute({ keepInputError:true });
  api('/api/v1/calibrate/point/delete', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({ index: Math.floor(idx) })
  }, function(res){
    expert.deleteIndex = -1;
    if (currentCalApiSuccess(res)) {
      applyCurrentCalStatePayload(res);
      setSuccessNotice('Точка удалена');
      refreshCurrentCalExpertRoute({ keepInputError:true });
      loadCurrentCalStateData(function(){
        refreshCurrentCalExpertRoute({ keepInputError:true });
      }, true);
      return;
    }
    setNotice(currentCalPointDeleteErrorText(res));
    refreshCurrentCalExpertRoute({ keepInputError:true });
  });
}
function clearCurrentCalExpertPoints(){
  var expert = ensureCurrentCalExpertState();
  var cal = ensureCurrentCalState();
  if (currentCalExpertClearDisabled()) return;
  if (!window.confirm('Удалить все точки калибровки?')) return;
  expert.clearBusy = true;
  clearNotice();
  refreshCurrentCalExpertRoute({ keepInputError:true });
  api('/api/v1/calibrate/clear', { method:'POST' }, function(res){
    expert.clearBusy = false;
    if (currentCalApiSuccess(res)) {
      applyCurrentCalStatePayload(res);
      setSuccessNotice('Точки калибровки очищены');
      refreshCurrentCalExpertRoute({ keepInputError:true });
      loadCurrentCalStateData(function(){
        refreshCurrentCalExpertRoute({ keepInputError:true });
      }, true);
      return;
    }
    setNotice(currentCalClearErrorText(res));
    refreshCurrentCalExpertRoute({ keepInputError:true });
  });
}
function tplHomeValueClass(sensor){
  if (!sensor || !sensor.enabled) return '';
  return ' yellow';
}
function tplHeader(opts){
  opts = opts || {};
  var s = state.lastState || {};
  ensureHeaderClockStyles();
  var html = '';
  html += '<header class="topbar' + (opts.home ? ' home-topbar' : '') + '">';
  html += '<div class="wifi-line">';
  html += '<div class="wifi-left"><div class="ip">IP: ' + esc(primaryIp(s)) + '</div><div class="wifi-note inline">' + esc(wifiModeNote(s)) + '</div></div>';
  if (opts.home) html += headerManualButtonHtml(true);
  html += '<div class="wifi-side"><div class="wifi-row"><span>Wi‑Fi</span>';
  if (wifiBarsVisible(s)) {
    html += '<div class="wifi-bars" aria-label="Уровень WiFi">';
    var level = wifiBarsLevel(wifiSignalRssi(s));
    for (var i = 0; i < 4; i++) {
      html += '<span' + (i < level ? ' class="on"' : '') + '></span>';
    }
    html += '</div>';
  }
  html += '</div>';
  if (opts.home) html += '<div class="clock' + (isHeaderClockStale() ? ' stale' : '') + '" id="hdrClock">' + esc(headerClockText(s.time)) + '</div>';
  html += '</div></div>';
  html += '</header>';
  return html;
}
function tplMessages(){
  return renderMessages('');
}
function isToggleOnlySensor(id){ return id === 'L' || id === 'F'; }
function isVirtualSensor(id){ return id === 'dT'; }
function isCtrlDisabledSensor(id){ return id === 'C' || id === 'V'; }
function isAlarmToggleOnlySensor(id){ return id === 'L' || id === 'F' || id === 'V'; }
function sensorSupportsAlarmDelay(id){ return id === 'T1' || id === 'T2' || id === 'T3' || id === 'P' || id === 'L' || id === 'F' || id === 'C' || id === 'dT'; }
function sensorSupportsCtrlDelay(id){ return id === 'T1' || id === 'T2' || id === 'T3' || id === 'P' || id === 'L' || id === 'F' || id === 'dT'; }
function sensorDelaySeconds(sensor, key){
  if (!sensor) return 0;
  var ms = Number(sensor[key] || 0);
  if (!isFinite(ms) || ms < 0) ms = 0;
  return ms / 1000;
}
function getCtrlRuleUi(sensor, outIdx){
  var rule = (sensor && sensor.ctrl && sensor.ctrl[outIdx]) ? sensor.ctrl[outIdx] : null;
  return {
    enabled: !!(rule && rule.enabled),
    logic: rule && rule.logic ? rule.logic : 'heat',
    min: rule && isFinite(Number(rule.min)) ? Number(rule.min) : 0,
    max: rule && isFinite(Number(rule.max)) ? Number(rule.max) : 0,
    outIdx: outIdx
  };
}
function getAlarmUi(sensorId){
  var sensor = findSensor(sensorId);
  var alarms = sensor && sensor.alarms ? sensor.alarms : [];
  var a0 = alarms[0] || { enabled:false, threshold:0, triggered:false };
  var a1 = alarms[1] || { enabled:false, threshold:0, triggered:false };
  var a2 = alarms[2] || { enabled:false, threshold:0, triggered:false };
  var a3 = alarms[3] || { enabled:false, threshold:0, triggered:false };
  if (sensorId === 'C') {
    return {
      cMinOnly: true,
      toggleOnly: false,
      toggle: {
        enabled: !!a0.enabled,
        threshold: isFinite(Number(a0.threshold)) ? Number(a0.threshold) : 0,
        triggered: !!a0.triggered
      },
      al1: {
        enabled: !!a0.enabled,
        min: isFinite(Number(a0.threshold)) ? Number(a0.threshold) : 0,
        max: null,
        minTriggered: !!a0.triggered,
        maxTriggered: false
      },
      al2: {
        enabled: false,
        min: null,
        max: null,
        minTriggered: false,
        maxTriggered: false
      }
    };
  }
  return {
    cMinOnly: false,
    toggleOnly: isAlarmToggleOnlySensor(sensorId),
    toggle: {
      enabled: !!a0.enabled,
      threshold: isFinite(Number(a0.threshold)) ? Number(a0.threshold) : 0,
      triggered: !!a0.triggered
    },
    al1: {
      enabled: !!(a0.enabled || a2.enabled),
      min: isFinite(Number(a0.threshold)) ? Number(a0.threshold) : 0,
      max: isFinite(Number(a2.threshold)) ? Number(a2.threshold) : 0,
      minTriggered: !!a0.triggered,
      maxTriggered: !!a2.triggered
    },
    al2: {
      enabled: !!(a1.enabled || a3.enabled),
      min: isFinite(Number(a1.threshold)) ? Number(a1.threshold) : 0,
      max: isFinite(Number(a3.threshold)) ? Number(a3.threshold) : 0,
      minTriggered: !!a1.triggered,
      maxTriggered: !!a3.triggered
    }
  };
}
function anyProjectAlarmActive(){
  var sensors = (state.lastState && state.lastState.sensors) ? state.lastState.sensors : [];
  for (var i = 0; i < sensors.length; i++) {
    if (sensorTriggeredAlarms(sensors[i]).length) return true;
  }
  return false;
}
function outputConfirmedOn(o){
  return relayFeedbackOn(o);
}
function relayOutputActive(outputId){
  var o = findOutput(outputId);
  if (!o) return false;
  return outputConfirmedOn(o);
}
function relayCard(name, active, text){
  var cls = 'relay';
  if (active && text === 'alarm') cls += ' alert';
  else if (active) cls += ' active';
  var safeText = active ? text : '0';
  return '<a class="' + cls + '" href="#/manual"><div class="name">' + esc(name) + '</div><div class="state">' + esc(safeText) + '</div></a>';
}
function ctrlLogicLabel(logic, sensorId){
  if (logic === 'cool' && (sensorId === 'L' || sensorId === 'F')) return 'включено';
  return logic === 'cool' ? 'охлаждение' : 'нагрев';
}
function sensorHeaderCenter(sensor){
  if (!sensor) return '<div class="head-center"><span class="head-id">—</span></div>';
  return '<div class="head-center"><span class="head-id">' + esc(sensor.id || '') + '</span><span class="head-value">' + esc(tplValueText(sensor, false)) + '</span></div>';
}
function sensorNumericInRange(sensor, min, max){
  if (!sensor || !sensor.enabled || sensor.error || sensor.value == null || sensor.present === false) return false;
  var v = Number(sensor.value);
  var mn = Number(min);
  var mx = Number(max);
  if (!isFinite(v) || !isFinite(mn) || !isFinite(mx)) return false;
  return v >= mn && v <= mx;
}
function sensorToggleAlarmTriggered(sensor){
  if (!sensor || !sensor.enabled || sensor.error || sensor.present === false) return false;
  return sensorTriggeredAlarms(sensor).length > 0;
}
function flowRawAlarmCondition(sensor){
  if (!sensor || sensor.id !== 'F') return false;
  if (!sensor.enabled || sensor.error || sensor.value == null || sensor.present === false) return false;
  return Number(sensor.value) <= 0.5;
}
function flowAlarmVisible(sensor){
  if (!sensor || sensor.id !== 'F') return false;
  if (!sensor.enabled || sensor.error || sensor.present === false) return false;
  return (typeof sensor.flowNoFlow === 'boolean') ? sensor.flowNoFlow : false;
}
function sensorDiscreteOk(sensor){
  if (!sensor || !sensor.enabled || sensor.error || sensor.value == null || sensor.present === false) return false;
  var contactClosed = Number(sensor.value) > 0.5;
  if (sensor.id === 'L') return contactClosed;
  if (sensor.id === 'F') return !flowAlarmVisible(sensor);
  return true;
}
function homeCtrlStack(sensor){
  var html = [];
  var anyVisible = false;
  if (!sensor || !sensor.enabled || isCtrlDisabledSensor(sensor.id)) {
    return '<div class="stack-placeholder">CH</div>';
  }
  for (var outIdx = 0; outIdx < 3; outIdx++) {
    var outId = 'CH' + (outIdx + 1);
    if (!outputVisible(outId)) continue;
    var rule = getCtrlRuleUi(sensor, outIdx);
    if (!rule.enabled) {
      html.push('<div class="stack-line empty">&nbsp;</div>');
      continue;
    }
    anyVisible = true;
    var ok = isToggleOnlySensor(sensor.id) ? sensorDiscreteOk(sensor) : sensorNumericInRange(sensor, rule.min, rule.max);
    html.push('<div class="stack-line ' + (ok ? 'good' : 'bad') + '">CH' + (outIdx + 1) + '</div>');
  }
  if (!anyVisible) return '<div class="stack-placeholder">CH</div>';
  return html.join('');
}
function homeAlarmStack(sensor){
  var html = [];
  var anyVisible = false;
  if (!sensor || !sensor.enabled) {
    return '<div class="stack-placeholder">AL</div>';
  }
  var alarms = (sensor && sensor.alarms) ? sensor.alarms : [];
  if (sensor.id === 'L' || sensor.id === 'F' || sensor.id === 'V') {
    var enabled = false;
    var triggered = false;
    for (var ti = 0; ti < alarms.length; ti++) {
      if (alarms[ti] && alarms[ti].enabled) enabled = true;
      if (alarms[ti] && alarms[ti].enabled && alarms[ti].triggered) triggered = true;
    }
    if (!enabled) return '<div class="stack-placeholder">AL</div>';
    return '<div class="stack-line ' + (triggered ? 'bad' : 'good') + '">AL</div>';
  }
  if (sensor.id === 'C') {
    var a0 = alarms[0] || null;
    if (!a0 || !a0.enabled) return '<div class="stack-placeholder">AL</div>';
    html.push('<div class="stack-line ' + (a0.triggered ? 'bad' : 'good') + '">ALmin</div>');
    html.push('<div class="stack-line empty">&nbsp;</div>');
    html.push('<div class="stack-line empty">&nbsp;</div>');
    html.push('<div class="stack-line empty">&nbsp;</div>');
    return html.join('');
  }
  var ui = getAlarmUi(sensor.id);
  var order = [
    { enabled:ui.al1.enabled, triggered:ui.al1.maxTriggered, label:'ALmax1' },
    { enabled:ui.al2.enabled, triggered:ui.al2.maxTriggered, label:'ALmax2' },
    { enabled:ui.al1.enabled, triggered:ui.al1.minTriggered, label:'ALmin1' },
    { enabled:ui.al2.enabled, triggered:ui.al2.minTriggered, label:'ALmin2' }
  ];
  for (var i = 0; i < order.length; i++) {
    var item = order[i];
    if (!item.enabled) {
      html.push('<div class="stack-line empty">&nbsp;</div>');
      continue;
    }
    anyVisible = true;
    html.push('<div class="stack-line ' + (item.triggered ? 'bad' : 'good') + '">' + item.label + '</div>');
  }
  if (!anyVisible) return '<div class="stack-placeholder">AL</div>';
  return html.join('');
}
function statusPill(label, text, cls){
  return '<div class="status-pill ' + cls + '"><span class="status-pill-label">' + esc(label) + '</span><span class="status-pill-value">' + esc(text) + '</span></div>';
}
function relayStatusPill(id){
  var o = findOutput(id) || {};
  var on = outputConfirmedOn(o);
  return statusPill(id, on ? 'ВКЛ' : 'ВЫКЛ', on ? 'on' : 'off');
}
function bellStatusPill(){
  var s = state.lastState || {};
  var o = findOutput('CH4') || {};
  if (o.actual) return statusPill('Звонок', 'АКТ', 'alert');
  return statusPill('Звонок', (s.ch4Enabled && !s.muted) ? 'ГОТОВ' : 'ВЫКЛ', (s.ch4Enabled && !s.muted) ? 'on' : 'off');
}
function buzzerStatusPill(){
  var s = state.lastState || {};
  var o = findOutput('CH5') || {};
  if (o.actual) return statusPill('Буззер', 'АКТ', 'alert');
  return statusPill('Буззер', (s.ch5Enabled && !s.muted) ? 'ГОТОВ' : 'ВЫКЛ', (s.ch5Enabled && !s.muted) ? 'on' : 'off');
}

function headerManualItem(label, active){
  return '<span class="home-header-item">' + esc(label) + (active ? '<span class="home-header-dot"></span>' : '') + '</span>';
}
function headerSoundItem(bellOn, buzzerOn){
  var dots = '';
  if (outputVisible('CH4')) dots += '<span class="home-header-sound-dot bell' + (bellOn ? ' on' : '') + '"></span>';
  if (outputVisible('CH5')) dots += '<span class="home-header-sound-dot buzzer' + (buzzerOn ? ' on' : '') + '"></span>';
  return '<span class="home-header-item home-header-item-sound">' + esc('Звук') + '<span class="home-header-sound-dots">' + dots + '</span></span>';
}
function headerManualButtonHtml(inline){
  if (schemaUnavailable()) return '';
  var s = state.lastState || {};
  var ch1 = outputConfirmedOn(findOutput('CH1') || {});
  var ch2 = outputConfirmedOn(findOutput('CH2') || {});
  var ch3 = outputConfirmedOn(findOutput('CH3') || {});
  var bellOn = outputVisible('CH4') ? !!(s.ch4Enabled || ((findOutput('CH4') || {}).actual)) : false;
  var buzzerOn = outputVisible('CH5') ? !!(s.ch5Enabled || ((findOutput('CH5') || {}).actual)) : false;
  var wrapCls = 'home-header-manual-wrap' + (inline ? ' inline' : '');
  var btnCls = 'btn home-header-manual-btn' + (inline ? ' inline' : '');
  var gridCls = 'home-header-manual-grid' + (inline ? ' inline' : '');
  var html = '';
  html += '<div class="' + wrapCls + '">';
  html += '<a class="' + btnCls + '" href="#/manual" onclick="go(\'#/manual\'); return false;">';
  html += '<div class="' + gridCls + '">';
  if (outputVisible('CH1')) html += headerManualItem('CH1', ch1);
  if (outputVisible('CH2')) html += headerManualItem('CH2', ch2);
  if (outputVisible('CH3')) html += headerManualItem('CH3', ch3);
  if (outputVisible('CH4') || outputVisible('CH5')) html += headerSoundItem(bellOn, buzzerOn);
  html += '</div></a></div>';
  return html;
}

function isAudibleAlarmActive(){
  return isAudibleAlarmActiveRaw();
}

function openNumEditor(ctx){
  state.numEdit = ctx;
  clearError();
  go('#/num');
  render();
}
function editSensorPeriod(id){
  var s = findSensor(id) || {};
  openNumEditor({
    mode: 'period',
    sensorId: id,
    title: 'Введите значение (формат XX,XX)',
    value: tplComma2((Number(s.periodMs || 1000) / 1000)),
    returnHash: '#/sensor/' + encodeURIComponent(id)
  });
}
function editCtrlThreshold(id, outIdx, which){
  var rule = getCtrlRuleUi(findSensor(id), outIdx);
  openNumEditor({
    mode: 'ctrl-' + which,
    sensorId: id,
    outIdx: outIdx,
    title: 'Введите значение (формат XX,XX)',
    value: tplComma2(which === 'min' ? rule.min : rule.max),
    returnHash: '#/sensorCtrl/' + encodeURIComponent(id)
  });
}
function editAlarmThreshold(id, key, which){
  var ui = getAlarmUi(id);
  var value = 0;
  if (ui.cMinOnly) value = ui.al1.min;
  else if (ui.toggleOnly) value = ui.toggle.threshold;
  else if (key === 'al1') value = (which === 'min') ? ui.al1.min : ui.al1.max;
  else value = (which === 'min') ? ui.al2.min : ui.al2.max;
  var title = 'Введите значение (формат XX,XX)';
  if (id === 'C') {
    value = ampsFromPercent(value);
    title = 'Введите ток, А (формат XX,XX)';
  }
  openNumEditor({
    mode: 'alarm-' + key + '-' + which,
    sensorId: id,
    title: title,
    value: tplComma2(value),
    returnHash: '#/sensorAlarm/' + encodeURIComponent(id)
  });
}
function toggleSensorEnabled(id){
  var s = findSensor(id);
  if (!s) return;
  clearError();
  clearNotice();
  api('/api/v1/sensor/' + encodeURIComponent(id) + '/config', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({ enabled: !s.enabled, periodMs: Number(s.periodMs || 1000) })
  }, function(res){
    setResultNotice(res, 'Датчик ' + tplSensorLabel(id) + ' ' + (!s.enabled ? 'включён.' : 'отключён.'), 'Не удалось изменить статус датчика.');
    reloadConfigAndState(function(){ render(); });
  });
}
function toggleCtrlRule(id, outIdx){
  var rule = getCtrlRuleUi(findSensor(id), outIdx);
  clearError();
  clearNotice();
  api('/api/v1/sensor/' + encodeURIComponent(id) + '/ctrl', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({ outIdx: outIdx, enabled: !rule.enabled, logic: rule.logic, min: rule.min, max: rule.max })
  }, function(res){
    setResultNotice(res, 'Управление ' + tplSensorLabel(id) + ' → CH' + (outIdx + 1) + ' ' + (!rule.enabled ? 'включено.' : 'отключено.'), 'Не удалось изменить правило управления.');
    reloadConfigAndState(function(){ render(); });
  });
}
function postAlarm(id, idx, enabled, threshold, isMax, cb){
  api('/api/v1/sensor/' + encodeURIComponent(id) + '/alarm', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({ idx: idx, enabled: enabled, threshold: threshold, isMax: isMax })
  }, cb);
}
function toggleAlarmPref(id, key){
  var ui = getAlarmUi(id);
  clearError();
  clearNotice();
  if (ui.cMinOnly) {
    var nextC = !ui.al1.enabled;
    postAlarm(id, 0, nextC, ui.al1.min, false, function(res){
      setResultNotice(res, 'ALmin ' + (nextC ? 'включена.' : 'выключена.'), 'Не удалось изменить ALmin.');
      reloadConfigAndState(function(){ render(); });
    });
    return;
  }
  if (ui.toggleOnly) {
    var next = !ui.toggle.enabled;
    postAlarm(id, 0, next, ui.toggle.threshold, false, function(res){
      setResultNotice(res, 'Сигнализация ' + (next ? 'включена.' : 'выключена.'), 'Не удалось изменить сигнализацию.');
      reloadConfigAndState(function(){ render(); });
    });
    return;
  }
  if (key === 'al1') {
    var next1 = !ui.al1.enabled;
    postAlarm(id, 0, next1, ui.al1.min, false, function(res1){
      if (!(res1 && res1.ok)) {
        setNotice((res1 && (res1.error || res1.err)) ? (res1.error || res1.err) : 'Не удалось изменить AL1.');
        reloadConfigAndState(function(){ render(); });
        return;
      }
      postAlarm(id, 2, next1, ui.al1.max, true, function(res2){
        setResultNotice(res2, 'AL1 ' + (next1 ? 'включена.' : 'выключена.'), 'Не удалось изменить AL1.');
        reloadConfigAndState(function(){ render(); });
      });
    });
  } else {
    var next2 = !ui.al2.enabled;
    postAlarm(id, 1, next2, ui.al2.min, false, function(res1){
      if (!(res1 && res1.ok)) {
        setNotice((res1 && (res1.error || res1.err)) ? (res1.error || res1.err) : 'Не удалось изменить AL2.');
        reloadConfigAndState(function(){ render(); });
        return;
      }
      postAlarm(id, 3, next2, ui.al2.max, true, function(res2){
        setResultNotice(res2, 'AL2 ' + (next2 ? 'включена.' : 'выключена.'), 'Не удалось изменить AL2.');
        reloadConfigAndState(function(){ render(); });
      });
    });
  }
}
function saveNumEditor(){
  var ctx = state.numEdit;
  if (!ctx) { go('#/home'); render(); return; }
  var input = byId('tpl-num-input');
  var raw = input && input.value ? String(input.value).trim() : '';
  var normalized = raw.replace(/\s+/g, '').replace('.', ',');
  if (!/^-?\d{1,4}([,]\d{1,2})?$/.test(normalized)) {
    setNotice('Введите значение в формате XX,XX');
    renderNumberInput();
    return;
  }
  var value = tplParseComma(normalized);
  if (!isFinite(value)) {
    setNotice('Некорректное значение.');
    renderNumberInput();
    return;
  }
  value = Math.round(value * 100) / 100;
  clearError();
  clearNotice();

  if (ctx.mode === 'period') {
    var sensor = findSensor(ctx.sensorId);
    api('/api/v1/sensor/' + encodeURIComponent(ctx.sensorId) + '/config', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ enabled: !!(sensor && sensor.enabled), periodMs: Math.round(value * 1000) })
    }, function(res){
      state.numEdit = null;
      setResultNotice(res, 'Период опроса сохранён.', 'Не удалось сохранить значение.');
      reloadConfigAndState(function(){ go(ctx.returnHash || '#/sensor/' + encodeURIComponent(ctx.sensorId)); render(); });
    });
    return;
  }

  if (ctx.mode === 'delay-alarm' || ctx.mode === 'delay-ctrl') {
    var sensorCfg = findSensor(ctx.sensorId);
    var payloadCfg = {
      enabled: !!(sensorCfg && sensorCfg.enabled),
      periodMs: Number((sensorCfg && sensorCfg.periodMs) || 1000),
      alarmDelayMs: Number((sensorCfg && sensorCfg.alarmDelayMs) || 0),
      ctrlDelayMs: Number((sensorCfg && sensorCfg.ctrlDelayMs) || 0)
    };
    if (ctx.mode === 'delay-alarm') payloadCfg.alarmDelayMs = Math.max(0, Math.round(value * 1000));
    else payloadCfg.ctrlDelayMs = Math.max(0, Math.round(value * 1000));
    api('/api/v1/sensor/' + encodeURIComponent(ctx.sensorId) + '/config', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify(payloadCfg)
    }, function(res){
      state.numEdit = null;
      setResultNotice(res, 'Значение сохранено.', 'Не удалось сохранить значение.');
      reloadConfigAndState(function(){ go(ctx.returnHash || '#/sensor/' + encodeURIComponent(ctx.sensorId)); render(); });
    });
    return;
  }

  if (ctx.mode === 'ctrl-min' || ctx.mode === 'ctrl-max') {
    var rule = getCtrlRuleUi(findSensor(ctx.sensorId), ctx.outIdx);
    var min = rule.min;
    var max = rule.max;
    if (ctx.mode === 'ctrl-min') min = value; else max = value;
    if (min > max) {
      setNotice('MIN не должен быть больше MAX.');
      renderNumberInput();
      return;
    }
    api('/api/v1/sensor/' + encodeURIComponent(ctx.sensorId) + '/ctrl', {
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ outIdx: ctx.outIdx, enabled: rule.enabled, logic: rule.logic, min: min, max: max })
    }, function(res){
      state.numEdit = null;
      setResultNotice(res, 'Значение сохранено.', 'Не удалось сохранить значение.');
      reloadConfigAndState(function(){ go(ctx.returnHash || '#/sensorCtrl/' + encodeURIComponent(ctx.sensorId)); render(); });
    });
    return;
  }

  if (ctx.mode.indexOf('alarm-') === 0) {
    var ui = getAlarmUi(ctx.sensorId);
    var idx = 0;
    var enabled = false;
    var isMax = false;
    if (ui.cMinOnly) {
      idx = 0; enabled = ui.al1.enabled; isMax = false;
    } else if (ui.toggleOnly) {
      idx = 0;
      enabled = ui.toggle.enabled;
      isMax = false;
    } else if (ctx.mode === 'alarm-al1-min') {
      idx = 0; enabled = ui.al1.enabled; isMax = false;
    } else if (ctx.mode === 'alarm-al1-max') {
      idx = 2; enabled = ui.al1.enabled; isMax = true;
    } else if (ctx.mode === 'alarm-al2-min') {
      idx = 1; enabled = ui.al2.enabled; isMax = false;
    } else if (ctx.mode === 'alarm-al2-max') {
      idx = 3; enabled = ui.al2.enabled; isMax = true;
    }
    if (ctx.sensorId === 'C') {
      value = percentFromAmps(value);
    }
    postAlarm(ctx.sensorId, idx, enabled, value, isMax, function(res){
      state.numEdit = null;
      setResultNotice(res, 'Значение сохранено.', 'Не удалось сохранить значение.');
      reloadConfigAndState(function(){ go(ctx.returnHash || '#/sensorAlarm/' + encodeURIComponent(ctx.sensorId)); render(); });
    });
    return;
  }
}
function renderNumberInput(){
  stopPoll();
  var ctx = state.numEdit;
  if (!ctx) { go('#/home'); return; }
  var html = [];
  html.push('<div class="tpl-screen"><div class="phone tpl-page">');
  html.push('<main class="content num">');
  html.push(tplMessages());
  html.push('<div class="display"><div class="label">' + esc(ctx.title || 'Введите значение (формат XX,XX)') + '</div>');
  html.push('<input id="tpl-num-input" type="text" value="' + esc(ctx.value || '00,00') + '" inputmode="decimal" placeholder="00,00"></div>');
  html.push('<a class="save" href="#" onclick="event.preventDefault();saveNumEditor()">Сохранить!</a>');
  html.push('<div class="hint">Можно вводить 10, 10,5, 10,50 или 10.50. Сохранение выполняется в формате XX,XX.</div>');
  html.push('</main></div></div>');
  app.innerHTML = html.join('');
  var inp = byId('tpl-num-input');
  if (inp) {
    try {
      inp.focus();
      inp.setSelectionRange(inp.value.length, inp.value.length);
    } catch (e) {}
  }
}

function renderHome(){
  stopPoll();
  var html = [];
  var themeCls = state.theme === 'dark' ? 'tpl-home-theme-dark' : 'tpl-home-theme-light';
  var ackActive = isAudibleAlarmActive();
  var stopActive = !!(state.lastState && state.lastState.stopLatched);
  var homeTopBlockKey = stopActive ? 'stop' : '';
  var visibleSensorCount = 0;
  if (state.homeTopBlockKey !== homeTopBlockKey) {
    state.homeTopBlockKey = homeTopBlockKey;
    state.homeScrollY = 0;
  }
  html.push('<div class="tpl-screen home-fixed-layout ' + themeCls + '"><div class="phone tpl-home-main">');
  html.push(tplHeader({home:true}));
  html.push('<main id="home-scroll" class="content home-content">');
  if (stopActive) html.push('<section class="hero">STOP активен: CH1–CH3 выключены, автоматика заблокирована до отмены стопа.</section>');
  html.push('<section class="sensor-list compact home-grid">');
  for (var i = 0; i < tplHomeOrder.length; i++) {
    var id = tplHomeOrder[i];
    if (!sensorVisible(id)) continue;
    visibleSensorCount++;
    var sensor = findSensor(id) || { id:id, enabled:false, value:null };
    var ctrlDisabled = (id === 'C' || id === 'V');
    var sensorErrorText = tplHomeSensorErrorText(sensor);
    var homeValueText = tplHomeValueText(sensor);
    html.push('<a class="btn home-main home-grid-btn" href="#/sensor/' + encodeURIComponent(id) + '">');
    html.push('<div class="sensor-label">' + esc(tplHomeSensorLabel(id)) + '</div>');
    html.push('<div class="sensor-value">' + esc(homeValueText || '') + '</div>');
    html.push('<div class="sensor-error' + (sensorErrorText ? '' : ' empty') + '">' + esc(sensorErrorText || ' ') + '</div>');
    html.push('</a>');
    html.push('<a class="btn home-stack home-grid-btn' + (ctrlDisabled ? ' disabled' : '') + '" href="' + (ctrlDisabled ? '#' : '#/sensorCtrl/' + encodeURIComponent(id)) + '">' + homeCtrlStack(sensor) + '</a>');
    html.push('<a class="btn home-stack home-grid-btn" href="#/sensorAlarm/' + encodeURIComponent(id) + '">' + homeAlarmStack(sensor) + '</a>');
  }
  if (!visibleSensorCount) html.push('<div class="error-box" style="grid-column:1/-1;">' + esc(SCHEMA_UNAVAILABLE_TEXT) + '</div>');
  html.push('</section>');
  html.push('</main>');
  html.push('<nav class="home-bottom">');
  html.push('<button type="button" id="home-ack-btn" class="home-action-btn' + (ackActive ? ' alert blink' : '') + (state.ackPending ? ' pending' : '') + '">Квитирование</button>');
  html.push('<button type="button" class="home-action-btn stop' + (stopActive ? ' alert blink' : '') + '" onclick="' + (stopActive ? 'releaseStopMainOutputs()' : 'showStopInfo()') + '">' + (stopActive ? 'Отменить стоп' : 'Стоп') + '</button>');
  html.push('<button type="button" class="home-action-btn" onclick="go(\'#/menu\');render();">Настройки</button>');
  html.push('</nav>');
  html.push('</div></div>');
  app.innerHTML = html.join('');
  var homeScroll = byId('home-scroll');
  if (homeScroll) {
    homeScroll.scrollTop = state.homeScrollY || 0;
    homeScroll.addEventListener('scroll', function(){ state.homeScrollY = homeScroll.scrollTop || 0; }, { passive:true });
  }
  var headerManualBtn = byId('home-header-manual-btn');
  if (headerManualBtn) {
    headerManualBtn.addEventListener('click', function(ev){
      if (ev) ev.preventDefault();
      go('#/manual');
      render();
    });
  }
  var ackBtn = byId('home-ack-btn');
  if (ackBtn) {
    ackBtn.addEventListener('click', function(ev){
      if (ev) ev.preventDefault();
      acknowledgeAlarms();
    });
  }
  startLiveStatePoll('home');
}

function renderSensorConfigGrid(id, s){
  var html = [];
  html.push('<section class="grid">');
  html.push('<div class="cell"><div class="label">Статус: «' + esc(s.enabled ? 'Включено' : 'Выключено') + '»</div></div>');
  html.push('<a class="cell btn ' + (s.enabled ? 'red' : 'green') + '" href="#" onclick="event.preventDefault();toggleSensorEnabled(\'' + esc(id) + '\')">' + esc(s.enabled ? 'Отключить!' : 'Включить!') + '</a>');
  if (!isVirtualSensor(id)) {
    html.push('<div class="cell"><div class="label">Период опроса: ' + esc(tplComma2(Number(s.periodMs || 1000) / 1000)) + ' сек.</div></div>');
    html.push('<a class="cell btn" href="#" onclick="event.preventDefault();editSensorPeriod(\'' + esc(id) + '\')">Установить другой</a>');
  }
  if (sensorSupportsAlarmDelay(id)) {
    html.push('<div class="cell"><div class="label">Задержка срабатывания сигнализации: ' + esc(tplComma2(sensorDelaySeconds(s, "alarmDelayMs"))) + ' сек.</div></div>');
    html.push('<a class="cell btn" href="#" onclick="event.preventDefault();editSensorDelay(\'' + esc(id) + '\',\'alarm\')">Установить другой</a>');
  }
  if (sensorSupportsCtrlDelay(id)) {
    html.push('<div class="cell"><div class="label">Задержка срабатывания управления: ' + esc(tplComma2(sensorDelaySeconds(s, "ctrlDelayMs"))) + ' сек.</div></div>');
    html.push('<a class="cell btn" href="#" onclick="event.preventDefault();editSensorDelay(\'' + esc(id) + '\',\'ctrl\')">Установить другой</a>');
  }
  html.push('</section>');
  return html.join('');
}

function renderSensor(id){
  var s = findSensor(id);
  if (!s) { go('#/home'); return; }
  stopPoll();
  var html = [];
  html.push('<div class="tpl-screen"><div class="phone tpl-page">');
  html.push(tplHeader());
  html.push('<main class="content">');
  if (s.note && id !== 'C') html.push('<div class="error-box">' + esc(s.note) + '</div>');
  html.push(renderMessages(''));
  html.push('<section class="hero"><div class="name">' + esc(tplSensorLabel(id)) + '</div><div class="value">' + esc(tplValueText(s, false)) + '</div></section>');
  html.push(renderSensorConfigGrid(id, s));
  html.push('<a class="cell btn green" href="#/home">Вернуться на стартовую страницу</a>');
  html.push('</main></div></div>');
  app.innerHTML = html.join('');
  startLiveStatePoll('sensor');
}

function editSensorDelay(id, kind){
  var s = findSensor(id) || {};
  var key = kind === 'ctrl' ? 'ctrlDelayMs' : 'alarmDelayMs';
  var title = kind === 'ctrl' ? 'Введите задержку управления (сек.)' : 'Введите задержку сигнализации (сек.)';
  openNumEditor({
    mode: kind === 'ctrl' ? 'delay-ctrl' : 'delay-alarm',
    sensorId: id,
    title: title,
    value: tplComma2((Number(s[key] || 0) / 1000)),
    returnHash: '#/sensor/' + encodeURIComponent(id)
  });
}

function renderSensorCtrl(id){
  var s = findSensor(id);
  if (!s) { go('#/home'); return; }
  stopPoll();
  var html = [];
  html.push('<div class="tpl-screen"><div class="phone tpl-page">');
  html.push(tplHeader());
  html.push('<main class="content">');
  html.push(renderMessages(''));
  html.push('<section class="head-row">');
  html.push('<div class="cell head">MIN</div>');
  html.push('<div class="cell head">' + sensorHeaderCenter(s) + '</div>');
  html.push('<div class="cell head">MAX</div>');
  html.push('</section>');
  for (var outIdx = 0; outIdx < 3; outIdx++) {
    var outId = 'CH' + (outIdx + 1);
    if (!outputVisible(outId)) continue;
    var rule = getCtrlRuleUi(s, outIdx);
    html.push('<section class="ctrl-row">');
    if (isCtrlDisabledSensor(id) || isToggleOnlySensor(id)) html.push('<div class="cell empty">—</div>');
    else html.push('<a class="cell minmax" href="#" onclick="event.preventDefault();editCtrlThreshold(\'' + esc(id) + '\',' + outIdx + ',\'min\')">' + esc(tplComma2(rule.min)) + '</a>');
    if (isCtrlDisabledSensor(id)) html.push('<div class="cell empty">—</div>');
    else html.push('<a class="cell" href="#" onclick="event.preventDefault();toggleCtrlRule(\'' + esc(id) + '\',' + outIdx + ')"><div class="center"><div class="ch">CH' + (outIdx + 1) + '</div><div class="state ' + (rule.enabled ? 'mode' : 'off') + '">' + esc(rule.enabled ? ctrlLogicLabel(rule.logic, id) : 'отключено') + '</div></div></a>');
    if (isCtrlDisabledSensor(id) || isToggleOnlySensor(id)) html.push('<div class="cell empty">—</div>');
    else html.push('<a class="cell minmax" href="#" onclick="event.preventDefault();editCtrlThreshold(\'' + esc(id) + '\',' + outIdx + ',\'max\')">' + esc(tplComma2(rule.max)) + '</a>');
    html.push('</section>');
  }
  html.push('<a class="back" href="#/home">Вернуться на стартовую страницу</a>');
  html.push('</main></div></div>');
  app.innerHTML = html.join('');
  startLiveStatePoll('sensorCtrl');
}

function renderCtrlRule(id, outIdx){
  go('#/sensorCtrl/' + encodeURIComponent(id));
  render();
}

function alarmValueClass(enabled, triggered){
  if (!enabled) return "";
  return triggered ? " alert" : " ok";
}
function alarmNameClass(enabled, triggered){
  if (!enabled) return "";
  return triggered ? " alert" : " ok";
}
function renderSensorAlarm(id){
  var s = findSensor(id);
  if (!s) { go('#/home'); return; }
  stopPoll();
  if (id === 'C') {
    var cal = ensureCurrentCalState();
    if (!cal.loaded && !cal.loading && !document.hidden) {
      loadCurrentCalStatus(function(){
        if ((routeParts()[0] || '') === 'sensorAlarm' && decodeURIComponent(routeParts()[1] || '') === 'C') render();
      });
    }
  }
  var ui = getAlarmUi(id);
  var html = [];
  html.push('<div class="tpl-screen"><div class="phone tpl-page">');
  html.push(tplHeader());
  html.push('<main class="content">');
  html.push(renderMessages(''));
  html.push('<section class="head-row">');
  html.push('<div class="cell head">MIN</div>');
  html.push('<div class="cell head">' + sensorHeaderCenter(s) + '</div>');
  html.push('<div class="cell head">MAX</div>');
  html.push('</section>');

  if (ui.cMinOnly) {
    html.push('<section class="alarm-row">');
    html.push('<a class="cell value' + alarmValueClass(ui.al1.enabled, ui.al1.minTriggered) + '" href="#" onclick="event.preventDefault();editAlarmThreshold(\'' + esc(id) + '\',\'al1\',\'min\')">' + esc(tplCurrentFromPercent(ui.al1.min)) + '</a>');
    html.push('<a class="cell center" href="#" onclick="event.preventDefault();toggleAlarmPref(\'' + esc(id) + '\',\'al1\')"><div class="al-name' + alarmNameClass(ui.al1.enabled, ui.al1.minTriggered) + '">ALmin</div></a>');
    html.push('<div class="cell empty">—</div>');
    html.push('</section>');

    html.push('<section class="alarm-row">');
    html.push('<div class="cell empty">—</div>');
    html.push('<div class="cell empty">—</div>');
    html.push('<div class="cell empty">—</div>');
    html.push('</section>');
  } else if (ui.toggleOnly) {
    var toggleCls = alarmNameClass(ui.toggle.enabled, ui.toggle.triggered);
    html.push('<section class="alarm-row">');
    html.push('<div class="cell empty">—</div>');
    html.push('<a class="cell center" href="#" onclick="event.preventDefault();toggleAlarmPref(\'' + esc(id) + '\',\'al\')"><div class="al-name' + toggleCls + '">AL</div></a>');
    html.push('<div class="cell empty">—</div>');
    html.push('</section>');
  } else {
    var al1Triggered = !!(ui.al1.minTriggered || ui.al1.maxTriggered);
    var al2Triggered = !!(ui.al2.minTriggered || ui.al2.maxTriggered);
    html.push('<section class="alarm-row">');
    html.push('<a class="cell value' + alarmValueClass(ui.al1.enabled, ui.al1.minTriggered) + '" href="#" onclick="event.preventDefault();editAlarmThreshold(\'' + esc(id) + '\',\'al1\',\'min\')">' + esc(tplComma2(ui.al1.min)) + '</a>');
    html.push('<a class="cell center" href="#" onclick="event.preventDefault();toggleAlarmPref(\'' + esc(id) + '\',\'al1\')"><div class="al-name' + alarmNameClass(ui.al1.enabled, al1Triggered) + '">AL1</div></a>');
    html.push('<a class="cell value' + alarmValueClass(ui.al1.enabled, ui.al1.maxTriggered) + '" href="#" onclick="event.preventDefault();editAlarmThreshold(\'' + esc(id) + '\',\'al1\',\'max\')">' + esc(tplComma2(ui.al1.max)) + '</a>');
    html.push('</section>');

    html.push('<section class="alarm-row">');
    html.push('<a class="cell value' + alarmValueClass(ui.al2.enabled, ui.al2.minTriggered) + '" href="#" onclick="event.preventDefault();editAlarmThreshold(\'' + esc(id) + '\',\'al2\',\'min\')">' + esc(tplComma2(ui.al2.min)) + '</a>');
    html.push('<a class="cell center" href="#" onclick="event.preventDefault();toggleAlarmPref(\'' + esc(id) + '\',\'al2\')"><div class="al-name' + alarmNameClass(ui.al2.enabled, al2Triggered) + '">AL2</div></a>');
    html.push('<a class="cell value' + alarmValueClass(ui.al2.enabled, ui.al2.maxTriggered) + '" href="#" onclick="event.preventDefault();editAlarmThreshold(\'' + esc(id) + '\',\'al2\',\'max\')">' + esc(tplComma2(ui.al2.max)) + '</a>');
    html.push('</section>');
  }
  html.push('<a class="back" href="#/home">Вернуться на стартовую страницу</a>');
  html.push('</main></div></div>');
  app.innerHTML = html.join('');
  startLiveStatePoll('sensorAlarm');
}

function renderAlarmPref(id, key){
  go('#/sensorAlarm/' + encodeURIComponent(id));
  render();
}


function renderStopConfirm(){
  stopPoll();
  var html = [];
  html.push('<div class="tpl-screen"><div class="phone tpl-page">');
  html.push(tplHeader());
  html.push('<main class="content">');
  html.push('<section class="hero">Вы уверены, что хотите остановить CH1, CH2 и CH3? Автоматика будет заблокирована до нажатия «Отменить стоп».</section>');
  html.push('<div class="confirm-actions">');
  html.push('<button type="button" class="cell btn" onclick="go(\'#/home\');render();">Отмена</button>');
  html.push('<button type="button" class="cell btn red" onclick="stopMainOutputs(function(){ go(\'#/home\'); render(); });">STOP</button>');
  html.push('</div>');
  html.push('</main></div></div>');
  app.innerHTML = html.join('');
}
function renderCurrentRoute(){
  var r = routeParts();
  var view = r[0];
  var arg = r[1] ? decodeURIComponent(r[1]) : '';
  var arg2 = r[2] ? decodeURIComponent(r[2]) : '';
  if (!routeAllowed(view, arg)) {
    if (location.hash !== '#/home') { go('#/home'); return; }
  }
  prepareCurrentRoute(view);
  if (view === 'home') return renderHome();
  if (view === 'menu') return renderMenu();
  if (view === 'sound') return renderSound();
  if (view === 'outputConfig') return renderOutputConfig();
  if (view === 'cal-basic') return renderCalBasic();
  if (view === 'cal-expert') return renderCalExpert();
  if (view === 'theme') return renderTheme();
  if (view === 'notifications') return renderNotifications();
  if (view === 'diag') return renderDiag();
  if (view === 'log') return renderLogPage();
  if (view === 'manual') return renderManual();
  if (view === 'stopConfirm') return renderStopConfirm();
  if (view === 'sensor' && arg) return renderSensor(arg);
  if (view === 'sensorAlarm' && arg) return renderSensorAlarm(arg);
  if (view === 'sensorCtrl' && arg) return renderSensorCtrl(arg);
  if (view === 'ctrlRule' && arg) return renderCtrlRule(arg, Number(arg2 || 0));
  if (view === 'alarmPref' && arg) return renderAlarmPref(arg, arg2 || 'al1');
  if (view === 'num') return renderNumberInput();
  go('#/home');
}
)RCWEB";

static const size_t PAGE_APP_JS_LEN = sizeof(PAGE_APP_JS) - 1;
