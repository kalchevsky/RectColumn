// Auto-generated from OUT/app.css. Do not edit manually.
#pragma once
#include <Arduino.h>

static const char PAGE_APP_CSS[] PROGMEM = R"RCWEB(
:root{
  --bg:#f2f3f5;
  --card:#ffffff;
  --sheet:#e6e8eb;
  --line:#cfd6de;
  --text:#111827;
  --muted:#4b5563;
  --ok:#166534;
  --warn:#92400e;
  --danger:#b91c1c;
  --accent:#2563eb;
  --nav-bg:#ffffff;
  --input-bg:#ffffff;
  --notice-bg:#eff6ff;
  --notice-text:#1d4ed8;
  --error-bg:#fff7ed;
  --error-text:#9a3412;
  --rc-home-main-label-font:clamp(14px,3.8vw,19px);
  --rc-home-main-value-font:clamp(22px,5.8vw,32px);
  --rc-home-main-error-font:clamp(10px,2.6vw,13px);
  --rc-home-stack-font:clamp(14px,4.1vw,22px);
  --rc-home-stack-font-small:13px;
  --rc-home-stack-placeholder-font:clamp(14px,3.8vw,19px);
  --rc-home-stack-placeholder-font-small:14px;
  --rc-home-header-grid-font:clamp(16px,4.4vw,21px);
  --rc-home-topbar-row-size:clamp(92px,26vw,112px);
}
:root[data-theme="dark"]{
  --bg:#111827;
  --card:#1f2937;
  --sheet:#18212e;
  --line:#374151;
  --text:#f9fafb;
  --muted:#cbd5e1;
  --ok:#86efac;
  --warn:#fbbf24;
  --danger:#fca5a5;
  --accent:#60a5fa;
  --nav-bg:#111827;
  --input-bg:#111827;
  --notice-bg:#0f172a;
  --notice-text:#bfdbfe;
  --error-bg:#3f1d1d;
  --error-text:#fecaca;
}
*{box-sizing:border-box}
html,body{background:var(--bg)}
body{margin:0;font-family:Arial,sans-serif;background:var(--bg);color:var(--text)}
.app{max-width:560px;margin:0 auto;padding:12px 12px 96px}
.topbar,.sheet,.menu-sheet,.error-box,.notice-box{background:var(--card);border:1px solid var(--line);border-radius:16px}
.topbar{padding:10px 12px;margin-bottom:10px}
.topline{display:flex;justify-content:space-between;gap:8px;align-items:center}
.topline strong{font-size:21px;line-height:1.1}
.wifi-mini{display:flex;align-items:center;gap:8px;font-size:13px;color:var(--muted)}
.wifi-mode{font-weight:700;letter-spacing:.02em}
.wifi-bars{font-family:monospace;font-size:15px;line-height:1}
.meta{font-size:13px;color:var(--muted);margin-top:4px}
.meta.compact{white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.meta-row{display:flex;gap:6px;flex-wrap:wrap;margin-top:8px}
.meta-row.compact{margin-top:6px}
.badge{display:inline-block;padding:4px 8px;border-radius:999px;border:1px solid var(--line);font-size:11px;background:rgba(0,0,0,.04)}
.badge.ok{color:var(--ok)}
.badge.warn{color:var(--warn)}
.badge.danger{color:var(--danger)}
.sheet{background:var(--sheet);padding:8px;display:flex;flex-direction:column;gap:8px}
.sensor-row{display:grid;grid-template-columns:minmax(0,1fr) auto auto;gap:10px;align-items:center;padding:14px;border:1px solid var(--line);border-radius:14px;background:var(--card);width:100%;text-align:left;color:var(--text)}
.sensor-row.disabled{opacity:.9}
.sensor-main{min-width:0}
.sensor-name{font-size:20px;font-weight:700}
.sensor-sub{font-size:14px;color:var(--muted);margin-top:4px;line-height:1.35}
.sensor-config{display:flex;gap:6px;flex-wrap:wrap;margin-top:8px}
.cfg-pill{display:inline-block;padding:4px 8px;border-radius:999px;border:1px solid var(--line);font-size:12px;background:rgba(255,255,255,.65)}
.sensor-value{font-size:24px;font-weight:700;min-width:88px;text-align:right}
.sensor-value.blank{color:transparent}
.sensor-state{font-size:16px;padding:6px 10px;border-radius:999px;border:1px solid var(--line);min-width:88px;text-align:center}
.sensor-state.ok{color:var(--ok)}
.sensor-state.warn{color:var(--warn)}
.sensor-state.danger{color:var(--danger)}
.sensor-state.off{color:var(--muted)}
.clickable{cursor:pointer}
.relay-strip{display:flex;align-items:center;gap:10px;width:100%;padding:12px;border:1px solid var(--line);border-radius:14px;background:var(--card);color:var(--text);margin:0 0 10px;text-align:left}
.relay-strip-title{font-size:16px;font-weight:700;white-space:nowrap}
.relay-chip{display:inline-flex;align-items:center;gap:6px;padding:4px 8px;border:1px solid var(--line);border-radius:999px;background:rgba(255,255,255,.7)}
.relay-chip-name{font-size:14px;font-weight:700}
.relay-dot{display:inline-block;width:12px;height:12px;border-radius:50%;background:#b91c1c;flex:0 0 12px}
.relay-dot.ok{background:#16a34a}
.relay-dot.err{background:#dc2626}
.relay-dot.pending{background:#9ca3af}
.bottom-nav{position:fixed;left:0;right:0;bottom:0;background:var(--nav-bg);border-top:1px solid var(--line);padding:10px 12px calc(10px + env(safe-area-inset-bottom,0px))}
.bottom-nav-inner{max-width:560px;margin:0 auto;display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px}
.bottom-btn{font-size:20px;font-weight:700;padding:16px;border-radius:14px;border:none;background:var(--accent);color:#fff}
.bottom-btn.stop{background:#b91c1c}
.bottom-btn.menu{background:#475569}
.panel{background:var(--card);border:1px solid var(--line);border-radius:16px;padding:16px;margin:12px 0}
.panel h2{font-size:24px;margin:0 0 12px}
.field{margin:14px 0}
.field label{display:block;font-size:17px;font-weight:700;margin-bottom:6px}
.field input,.field select,.field textarea{width:100%;font-size:18px;padding:14px;border-radius:12px;border:1px solid var(--line);background:var(--input-bg);color:var(--text)}
.field textarea{min-height:100px;resize:vertical}
.btn{display:block;width:100%;font-size:20px;font-weight:700;padding:16px;border:none;border-radius:14px;background:var(--accent);color:#fff;margin:10px 0}
.btn.secondary{background:#475569}
.btn.light{background:#e5e7eb;color:#111827}
:root[data-theme="dark"] .btn.light{background:#374151;color:#f9fafb}
.btn.danger{background:#b91c1c}
.btn.inline{display:inline-flex;width:auto;margin:8px 0 0;padding:8px 12px;font-size:14px}

@keyframes btn-blink-anim{0%{opacity:1}30%{opacity:.3}70%{opacity:.3}100%{opacity:1}}
.btn-blink{animation:btn-blink-anim 150ms ease-in-out}
.manual-toggle-btn.relay-btn.btn-off{background:#1a6fc4;color:#fff}
.manual-toggle-btn.relay-btn.btn-on{background:#c42b1a;color:#fff}
.manual-toggle-btn.relay-pending:disabled{opacity:1;filter:none}
.manual-toggle-btn.relay-pending{cursor:wait}
.btn:disabled,.btn.disabled{opacity:.55;pointer-events:none}
.error-box{padding:14px;font-size:18px;line-height:1.45;margin:12px 0;background:var(--error-bg);color:var(--error-text)}
.notice-box{padding:10px 12px;font-size:14px;line-height:1.35;margin:10px 0;background:var(--notice-bg);color:var(--notice-text)}
.small{font-size:15px;color:var(--muted);line-height:1.45}
.hidden{display:none}
pre{background:var(--card);color:var(--text)}
.info-list{display:flex;flex-direction:column;gap:8px;margin:12px 0}
.info-line{padding:10px 12px;border:1px solid var(--line);border-radius:12px;background:rgba(255,255,255,.35)}
.rule-card{padding:12px;border:1px solid var(--line);border-radius:14px;margin:12px 0;background:rgba(255,255,255,.35)}
.rule-title{font-size:18px;font-weight:700;margin-bottom:8px}
.inline-actions{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.mono{font-family:monospace}
.notice-top-space{margin-top:8px}
.manual-card{padding:12px;border:1px solid var(--line);border-radius:14px;margin:12px 0;background:rgba(255,255,255,.35)}
.manual-row{display:flex;justify-content:space-between;align-items:center;gap:12px}
.manual-title{font-size:18px;font-weight:700}
.manual-sub{font-size:12px;color:var(--muted);margin-top:2px}
.manual-indicator{display:flex;align-items:center;gap:8px}
.manual-status-text{font-size:14px;color:var(--muted);margin-top:4px}
.manual-msg{margin-top:10px;font-size:14px;color:var(--danger)}
.manual-toggle-btn{margin-top:12px;width:100%}
.manual-toggle-btn.pending{opacity:.72}
.manual-toggle-btn.blink{animation:rcBlink .2s linear 1}
.log-app{display:flex;flex-direction:column;height:100svh;height:100dvh;min-height:100svh;min-height:100dvh;overflow:hidden;padding-bottom:calc(12px + env(safe-area-inset-bottom,0px))}
.log-panel{display:flex;flex-direction:column;flex:1 1 auto;min-height:0;overflow:hidden}
.connection-lost-overlay{position:fixed;left:12px;right:12px;top:12px;z-index:10000;padding:12px 14px;border-radius:14px;background:var(--error-bg);color:var(--error-text);border:1px solid var(--danger);box-shadow:0 8px 24px rgba(0,0,0,.32);font-weight:700;text-align:center;line-height:1.35;pointer-events:none}
.log-toolbar{position:relative;z-index:2;background:var(--card);padding:4px 0 8px;flex:0 0 auto}
.log-toolbar .btn{margin:10px 0 0}
.log-list{flex:1 1 auto;min-height:0;overflow-y:auto;overflow-x:hidden;-webkit-overflow-scrolling:touch;overscroll-behavior:contain;touch-action:pan-y;padding-right:2px}
.notify-topic-row{display:flex;align-items:center;gap:8px}
.notify-prefix{display:inline-flex;align-items:center;justify-content:center;padding:14px 12px;border:1px solid var(--line);border-radius:12px;background:rgba(255,255,255,.55);color:var(--muted);white-space:nowrap}
.notify-topic-input{flex:1 1 auto;min-width:0}
@media (max-width:480px){
  .sensor-row{grid-template-columns:minmax(0,1fr)}
  .sensor-value,.sensor-state{min-width:0;text-align:left}
  .inline-actions{grid-template-columns:1fr}
  .relay-strip{flex-wrap:wrap}
  .manual-row{align-items:flex-start;flex-direction:row}
  .notify-topic-row{flex-direction:column;align-items:stretch}
  .notify-prefix{text-align:center}
}


/* ===== Template screens ===== */
.tpl-screen{
  --bg:#050505;
  --panel:#111214;
  --panel2:#181a1d;
  --line:#2a2d31;
  --text:#f3f4f6;
  --muted:#949aa3;
  --green:#52d86a;
  --red:#ff5a64;
  --yellow:#ffd84d;
  --blue:#5ab7ff;
  --disabled:#5c6068;
  --shadow:0 8px 20px rgba(0,0,0,.28);
  --radius:12px;
  --row-h:46px;
  --key-h:62px;
}
.tpl-screen *{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
.tpl-screen .phone{width:100%;max-width:430px;margin:0 auto;min-height:100vh;background:var(--bg);color:var(--text)}
.tpl-screen .phone.tpl-home{display:grid;grid-template-rows:auto 1fr auto;overflow:hidden}
.tpl-screen .phone.tpl-page{display:grid;grid-template-rows:auto 1fr;overflow:hidden}
.tpl-screen .topbar{background:#090a0b;border-bottom:1px solid var(--line);padding:10px 12px 8px;position:relative;z-index:5}
.tpl-screen .wifi-line{display:flex;align-items:center;justify-content:space-between;gap:10px;min-height:24px}
.tpl-screen .ip{font-size:14px;color:#eef1f4;letter-spacing:.2px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.tpl-screen .wifi-side{display:flex;align-items:center;gap:8px;color:var(--muted);font-size:12px;white-space:nowrap}
.tpl-screen .wifi-bars{display:flex;align-items:flex-end;gap:2px;height:14px}
.tpl-screen .wifi-bars span{display:block;width:3px;border-radius:2px;background:#35393f}
.tpl-screen .wifi-bars span:nth-child(1){height:5px}
.tpl-screen .wifi-bars span:nth-child(2){height:8px}
.tpl-screen .wifi-bars span:nth-child(3){height:11px}
.tpl-screen .wifi-bars span:nth-child(4){height:14px;background:#35393f}
.tpl-screen .wifi-bars span.on{background:var(--green)}
.tpl-screen .wifi-note{margin-top:4px;font-size:11px;color:var(--muted)}
.tpl-screen .content{padding:10px 8px 14px;display:flex;flex-direction:column;gap:8px;overflow:auto}
.tpl-screen .content.num{padding:16px 10px;gap:12px;overflow:visible}
.tpl-screen .sensor-list{display:flex;flex-direction:column;gap:6px;min-height:0}
.tpl-screen .row{display:grid;grid-template-columns:1.6fr 0.7fr 0.7fr;gap:6px;min-height:0}
.tpl-screen .btn{background:linear-gradient(180deg,var(--panel),var(--panel2));border:1px solid var(--line);border-radius:var(--radius);box-shadow:var(--shadow);height:var(--row-h);min-height:var(--row-h);max-height:var(--row-h);display:flex;align-items:center;justify-content:center;color:var(--text);text-decoration:none;user-select:none;overflow:hidden;font-size:15px}
.tpl-screen .btn-main{justify-content:space-between;padding:8px 10px;gap:8px}
.tpl-screen .left{min-width:0;display:flex;flex-direction:column;justify-content:center;gap:2px;flex:1}
.tpl-screen .label{font-size:15px;line-height:1.2;color:#eef1f4}
.tpl-screen .btn-main .label{font-size:14px;line-height:1.05;white-space:nowrap;overflow:visible;text-overflow:clip}
.tpl-screen .val{font-size:20px;line-height:1;font-weight:400;white-space:nowrap;margin-left:auto;flex-shrink:0}
.tpl-screen .val.red{color:var(--red)}
.tpl-screen .val.blue{color:#8de7ff}
.tpl-screen .val.yellow{color:var(--yellow)}
.tpl-screen .val.green{color:#62ff62}
.tpl-screen .btn-small{flex-direction:column;gap:2px;padding:6px 4px;text-align:center}
.tpl-screen .btn-small .t1{font-size:13px;line-height:1;color:#f0f2f5}
.tpl-screen .disabled{color:var(--disabled);background:linear-gradient(180deg,#0d0e10,#121316);border-color:#24272c;box-shadow:none;pointer-events:none}
.tpl-screen .disabled .t1,.tpl-screen .disabled .label,.tpl-screen .disabled .val{color:var(--disabled)}
.tpl-screen .relay-box{display:grid;grid-template-columns:repeat(3,1fr);gap:6px}
.tpl-screen .relay{min-height:50px;background:linear-gradient(180deg,var(--panel),var(--panel2));border:1px solid var(--line);border-radius:var(--radius);box-shadow:var(--shadow);display:flex;flex-direction:column;align-items:center;justify-content:center;gap:3px;text-align:center;text-decoration:none;color:var(--text)}
.tpl-screen .relay .name{font-size:12px;color:#dfe3e8;line-height:1}
.tpl-screen .relay .state{font-size:13px;line-height:1;color:var(--muted)}
.tpl-screen .relay.active{border-color:rgba(82,216,106,.45)}
.tpl-screen .relay.active .state{color:var(--green)}
.tpl-screen .relay.alert{border-color:rgba(255,90,100,.45)}
.tpl-screen .relay.alert .state{color:var(--red)}
.tpl-screen .bottom{border-top:1px solid var(--line);background:#090a0b;padding:8px 8px calc(8px + env(safe-area-inset-bottom));display:grid;grid-template-columns:repeat(3,1fr);gap:6px}
.tpl-screen .nav-btn{background:#111214;border:1px solid var(--line);color:#d5d8dc;border-radius:12px;min-height:46px;padding:4px;display:flex;flex-direction:column;justify-content:center;align-items:center;gap:3px;font-size:10px;text-align:center;text-decoration:none}
.tpl-screen .nav-btn.active{background:#181b1f;border-color:#4a4f56;color:#fff}
.tpl-screen .icon{font-size:15px;line-height:1}
.tpl-screen .hero{min-height:56px;background:linear-gradient(180deg,var(--panel),var(--panel2));border:1px solid var(--line);border-radius:var(--radius);box-shadow:var(--shadow);padding:14px 12px;display:flex;align-items:center;justify-content:center;gap:10px;font-size:18px;color:#eef1f4;text-align:center}
.tpl-screen .hero .name{font-size:26px;line-height:1;color:#eef1f4}
.tpl-screen .hero .value{font-size:30px;line-height:1;color:var(--yellow);white-space:nowrap}
.tpl-screen .grid{display:grid;grid-template-columns:1.15fr 1fr;gap:8px}
.tpl-screen .cell{min-height:54px;background:linear-gradient(180deg,var(--panel),var(--panel2));border:1px solid var(--line);border-radius:var(--radius);box-shadow:var(--shadow);display:flex;align-items:center;justify-content:center;text-align:center;padding:10px 12px;color:var(--text);text-decoration:none}
.tpl-screen .cell.label{justify-content:flex-start;text-align:left;color:#eef1f4;font-size:15px;line-height:1.2}
.tpl-screen .cell.btn.red{border-color:rgba(255,90,100,.35);color:#ffd9dc}
.tpl-screen .cell.btn.green{border-color:rgba(82,216,106,.35);color:#dfffe4}
.tpl-screen .note{color:var(--muted);font-size:11px;line-height:1.25;padding:2px 2px 0;text-align:center}
.tpl-screen .head-row,.tpl-screen .ctrl-row,.tpl-screen .alarm-row{display:grid;grid-template-columns:1fr 1.2fr 1fr;gap:8px}
.tpl-screen .head{min-height:38px;font-size:13px;color:var(--muted)}
.tpl-screen .head .head-center{display:flex;align-items:center;justify-content:center;gap:8px;width:100%}
.tpl-screen .head .head-id{font-size:13px;line-height:1;color:#eef1f4}
.tpl-screen .head .head-value{font-size:24px;line-height:1;color:var(--yellow)}
.tpl-screen .minmax,.tpl-screen .value{font-size:20px;line-height:1;white-space:nowrap}
.tpl-screen .center{display:flex;flex-direction:column;gap:4px;align-items:center;justify-content:center;line-height:1;font-size:18px}
.tpl-screen .ch,.tpl-screen .al-name{font-size:18px;color:#eef1f4}
.tpl-screen .state{font-size:12px}
.tpl-screen .state.on{color:var(--green)}
.tpl-screen .state.off{color:var(--red)}
.tpl-screen .state.mode{color:var(--green)}
.tpl-screen .value.alert{color:var(--red)}
.tpl-screen .value.ok{color:var(--green)}
.tpl-screen .al-name.ok{color:var(--green)}
.tpl-screen .al-name.alert{color:var(--red)}
.tpl-screen .empty{color:var(--muted);opacity:.35}
.tpl-screen .back{margin-top:6px;min-height:54px;background:linear-gradient(180deg,var(--panel),var(--panel2));border:1px solid rgba(82,216,106,.35);border-radius:var(--radius);box-shadow:var(--shadow);display:flex;align-items:center;justify-content:center;text-align:center;padding:10px 12px;text-decoration:none;color:#dfffe4;font-size:15px}
.tpl-screen .display{background:linear-gradient(180deg,var(--panel),var(--panel2));border:1px solid var(--line);border-radius:var(--radius);box-shadow:var(--shadow);padding:16px}
.tpl-screen .display .label{font-size:12px;color:var(--muted)}
.tpl-screen .display input{width:100%;font-size:36px;margin-top:6px;background:transparent;border:none;outline:none;color:var(--yellow);text-align:right;letter-spacing:1px}
.tpl-screen .save{margin-top:10px;height:56px;display:flex;align-items:center;justify-content:center;border-radius:var(--radius);border:1px solid rgba(82,216,106,.35);background:linear-gradient(180deg,var(--panel),var(--panel2));color:#dfffe4;font-size:18px;text-decoration:none;box-shadow:var(--shadow)}
.tpl-screen .hint{font-size:11px;color:var(--muted);text-align:center}
.tpl-screen .notice-box,.tpl-screen .error-box{margin:0 0 8px}
.active-alarm-overlay{position:fixed;inset:0;display:flex;align-items:center;justify-content:center;padding:18px;pointer-events:none;z-index:9999}
.active-alarm-card{width:min(100%,380px);background:rgba(9,10,12,.94);border:2px solid rgba(255,90,100,.58);border-radius:16px;box-shadow:0 16px 40px rgba(0,0,0,.46);padding:16px 14px;text-align:center}.tpl-screen .content.home-content>.active-alarm-card{width:calc(100% - 16px);max-width:none;margin:8px;box-sizing:border-box}
.active-alarm-title{font-size:20px;line-height:1.1;font-weight:800;color:#ff5a64}
.active-alarm-lines{margin-top:10px;display:flex;flex-direction:column;gap:8px}
.active-alarm-line{font-size:24px;line-height:1.08;font-weight:800;color:#f3f4f6}
@media (max-width:390px){
  .active-alarm-card{padding:14px 12px}
  .active-alarm-title{font-size:18px}
  .active-alarm-line{font-size:21px}
}
@media (max-width:390px){
  .tpl-screen{--row-h:44px}
  .tpl-screen .row{grid-template-columns:1.8fr 0.6fr 0.6fr}
  .tpl-screen .btn-main .label{font-size:12px}
  .tpl-screen .val,.tpl-screen .minmax,.tpl-screen .value{font-size:18px}
  .tpl-screen .btn-small .t1{font-size:12px}
  .tpl-screen .hero{font-size:16px}
  .tpl-screen .head .head-value{font-size:20px}
  .tpl-screen .grid{grid-template-columns:1fr 1fr}
}


/* ===== main screen based on 01.0-main-screen.html ===== */
.tpl-screen.tpl-home-theme-light{
  --bg:#f3f4f6;
  --panel:#ffffff;
  --panel2:#f7f8fa;
  --line:#d7dbe2;
  --text:#111827;
  --muted:#5b6470;
  --green:#138a50;
  --red:#d93025;
  --yellow:#8a6a00;
  --blue:#1a73e8;
  --disabled:#a0a7b1;
  --shadow:0 8px 20px rgba(0,0,0,.08);
}
.tpl-screen.tpl-home-theme-dark{
  --bg:#050505;
  --panel:#111214;
  --panel2:#181a1d;
  --line:#2a2d31;
  --text:#f3f4f6;
  --muted:#949aa3;
  --green:#52d86a;
  --red:#ff5a64;
  --yellow:#ffd84d;
  --blue:#5ab7ff;
  --disabled:#5c6068;
  --shadow:0 8px 20px rgba(0,0,0,.28);
}
.tpl-screen.home-fixed-layout{height:100svh;height:100dvh;min-height:100svh;min-height:100dvh;overflow:hidden}
.tpl-screen.home-fixed-layout .phone.tpl-home-main{height:100%;min-height:0;max-height:none;overflow:hidden}
.tpl-screen.home-fixed-layout .home-topbar,.tpl-screen.home-fixed-layout .home-bottom{position:relative;top:auto;bottom:auto}
.tpl-screen.home-fixed-layout .content.home-content{flex:1 1 0;min-height:0;overflow-y:auto;overflow-x:hidden;-webkit-overflow-scrolling:touch;overscroll-behavior:contain}
.tpl-screen .phone.tpl-home-main{display:flex;flex-direction:column;overflow:hidden;height:100svh;height:100dvh;min-height:100svh;min-height:100dvh;max-height:100dvh}
.tpl-screen .content.home-content{padding:0;overflow-y:auto;overflow-x:hidden;display:block;min-height:0;flex:1 1 auto;width:100%;-webkit-overflow-scrolling:touch;overscroll-behavior:contain;scrollbar-width:none;-ms-overflow-style:none}
.tpl-screen .content.home-content::-webkit-scrollbar{width:0;height:0;display:none}
.tpl-screen .sensor-list.compact{min-height:0}
.tpl-screen .sensor-list.compact.home-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:8px;padding:8px;box-sizing:border-box;width:100%;align-content:start;overflow-x:hidden}
.tpl-screen .sensor-list.compact.home-grid > .btn{width:100%;min-width:0;height:auto !important;min-height:0 !important;max-height:none !important;aspect-ratio:1 / 1;display:flex;margin:0 !important}
.tpl-screen .btn.home-main{align-items:stretch;justify-content:space-between;flex-direction:column;padding:6px 5px 4px;overflow:hidden;text-align:center;gap:2px}
.tpl-screen .btn.home-main .sensor-label{font-size:var(--rc-home-main-label-font,clamp(14px,3.8vw,19px));line-height:1.05;color:#eef1f4;white-space:normal;word-break:break-word;overflow-wrap:anywhere;text-align:center;width:100%;display:flex;align-items:flex-start;justify-content:center;flex:0 0 auto;min-height:2.15em;font-weight:700}
.tpl-screen .btn.home-main .sensor-value{font-size:var(--rc-home-main-value-font,clamp(22px,5.8vw,32px));line-height:1.0;color:var(--yellow);white-space:normal;text-align:center;width:100%;margin-top:0;display:flex;align-items:center;justify-content:center;flex:1 1 auto;min-height:0;font-weight:700}
.tpl-screen .btn.home-main .sensor-error{font-size:var(--rc-home-main-error-font,clamp(10px,2.6vw,13px));line-height:1.05;color:var(--red);white-space:normal;word-break:break-word;overflow-wrap:anywhere;text-align:center;width:100%;display:flex;align-items:flex-end;justify-content:center;flex:0 0 auto;min-height:2.1em;padding-bottom:1px;font-weight:700}
.tpl-screen .btn.home-main .sensor-error.empty{visibility:hidden}
.tpl-screen.tpl-home-theme-light .topbar{background:#ffffff;border-bottom-color:var(--line)}
.tpl-screen.tpl-home-theme-light .ip{color:var(--text)}
.tpl-screen.tpl-home-theme-light .wifi-side,.tpl-screen.tpl-home-theme-light .wifi-note{color:var(--muted)}
.tpl-screen.tpl-home-theme-light .wifi-bars span{background:#c9cfd8}
.tpl-screen.tpl-home-theme-light .btn.home-main .sensor-label{color:var(--text)}
.tpl-screen .btn.home-stack{align-items:center;justify-content:center;flex-direction:column;padding:4px 3px;gap:1px;overflow:hidden;text-align:center}
.tpl-screen .btn.home-stack.disabled{pointer-events:none}
.tpl-screen .stack-line{font-size:var(--rc-home-stack-font,clamp(13px,3.8vw,20px));line-height:1.04;width:100%;text-align:center;color:var(--text);white-space:normal;overflow-wrap:anywhere;word-break:break-word;display:flex;align-items:center;justify-content:center;min-height:0;font-weight:700}
.tpl-screen .stack-line.good{color:var(--green)}
.tpl-screen .stack-line.bad{color:var(--red)}
.tpl-screen .stack-line.empty{visibility:hidden}
.tpl-screen .stack-placeholder{font-size:var(--rc-home-stack-placeholder-font,clamp(13px,3.5vw,18px));line-height:1;color:var(--muted);width:100%;text-align:center;margin:auto;font-weight:700}
.tpl-screen .home-status-zone{display:grid;grid-template-columns:1fr 1fr;gap:6px;align-items:stretch}
.tpl-screen .status-column{display:flex;flex-direction:column;gap:4px}
.tpl-screen .status-pill{min-height:20px;background:linear-gradient(180deg,var(--panel),var(--panel2));border:1px solid var(--line);border-radius:10px;box-shadow:var(--shadow);display:flex;align-items:center;justify-content:space-between;padding:4px 6px;gap:6px}
.tpl-screen .status-pill-label{font-size:10px;line-height:1;color:var(--text)}
.tpl-screen .status-pill-value{font-size:10px;line-height:1;color:var(--muted)}
.tpl-screen .status-pill.on .status-pill-value{color:var(--green)}
.tpl-screen .status-pill.alert .status-pill-value{color:var(--red)}
.tpl-screen .topbar{padding:10px 12px 8px}
.tpl-screen .wifi-left{display:flex;flex-direction:column;align-items:flex-start;justify-content:center;min-width:0;flex:0 1 auto}
.tpl-screen .wifi-note{margin-top:4px;font-size:11px;color:var(--muted)}
.tpl-screen .wifi-note.inline{margin-top:2px}
.tpl-screen .home-topbar{position:sticky;top:0;z-index:20;flex:0 0 auto;padding:2px 12px}
.tpl-screen .home-topbar .wifi-line{align-items:stretch;height:var(--rc-home-topbar-row-size,clamp(92px,26vw,112px));min-height:var(--rc-home-topbar-row-size,clamp(92px,26vw,112px))}
.tpl-screen .home-topbar .wifi-left,.tpl-screen .home-topbar .wifi-side{align-self:center}
.tpl-screen .home-header-manual-wrap{display:flex;justify-content:center;align-items:stretch;align-self:stretch;flex:0 0 calc(var(--rc-home-topbar-row-size,clamp(92px,26vw,112px))*1.302);width:calc(var(--rc-home-topbar-row-size,clamp(92px,26vw,112px))*1.302);max-width:calc(var(--rc-home-topbar-row-size,clamp(92px,26vw,112px))*1.302);min-width:0;min-height:0;aspect-ratio:auto;padding:0;margin:0 2px}
.tpl-screen .home-header-manual-btn{width:100%;max-width:none;height:100%;min-height:0;max-height:none;padding:8px;margin:0;background:linear-gradient(180deg,var(--panel),var(--panel2));border:1px solid var(--line);border-radius:14px;box-shadow:var(--shadow);color:var(--text);text-decoration:none;display:flex;align-items:center;justify-content:center}
.tpl-screen .home-header-manual-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:6px 8px;width:100%;height:100%;font-size:var(--rc-home-header-grid-font,clamp(16px,4.4vw,21px));line-height:1.05;text-align:center;color:var(--text);align-content:center}
.tpl-screen .home-header-item{display:inline-flex;align-items:center;justify-content:center;gap:6px;min-height:22px;min-width:0;font-weight:700;white-space:nowrap}
.tpl-screen .home-header-item-sound{display:inline-flex;align-items:center}
.tpl-screen .home-header-sound-dots{display:inline-flex;flex-direction:column;align-items:center;justify-content:center;gap:2px;min-width:8px;margin-left:4px;line-height:0}
.tpl-screen .home-header-sound-dot{display:block;width:8px;height:8px;border-radius:50%;background:transparent;flex:0 0 8px}
.tpl-screen .home-header-sound-dot.bell.on{background:var(--green)}
.tpl-screen .home-header-sound-dot.buzzer.on{background:var(--yellow)}
.tpl-screen .home-header-dot{display:inline-block;width:8px;height:8px;border-radius:50%;background:var(--green);flex:0 0 8px}
.tpl-screen .home-bottom{border-top:1px solid var(--line);background:#090a0b;padding:8px 8px calc(8px + env(safe-area-inset-bottom));display:grid;grid-template-columns:repeat(3,1fr);gap:6px;position:sticky;bottom:0;z-index:20;flex:0 0 auto}
.tpl-screen.tpl-home-theme-light .home-bottom{background:#ffffff}
.tpl-screen .home-action-btn{min-height:52px;border:1px solid var(--line);border-radius:12px;background:linear-gradient(180deg,var(--panel),var(--panel2));box-shadow:var(--shadow);color:var(--text);font-size:14px;font-weight:700;display:flex;align-items:center;justify-content:center;text-align:center}
.tpl-screen .home-action-btn.stop{border-color:rgba(255,90,100,.35);color:#ffd9dc}
.tpl-screen .home-action-btn.alert{color:var(--red);border-color:rgba(255,90,100,.45)}
.tpl-screen .home-action-btn.blink{animation:rcBlink 1s linear infinite}
.tpl-screen.tpl-home-theme-light .home-action-btn.stop{color:var(--red)}
.tpl-screen .home-action-btn.pending{opacity:.72}
.tpl-screen .sensor-list.compact.home-grid > .btn.home-stack:nth-child(3n-1) .stack-line,
.tpl-screen .sensor-list.compact.home-grid > .btn.home-stack:nth-child(3n) .stack-line{font-size:clamp(19.6px,5.74vw,30.8px)}
.tpl-screen .sensor-list.compact.home-grid > .btn.home-stack:nth-child(3n-1) .stack-placeholder,
.tpl-screen .sensor-list.compact.home-grid > .btn.home-stack:nth-child(3n) .stack-placeholder{font-size:clamp(19.6px,5.32vw,26.6px)}
@media (max-width:390px){
  .tpl-screen .sensor-list.compact.home-grid{gap:6px;padding:6px}
  .tpl-screen .btn.home-main{padding:5px 4px 4px}
  .tpl-screen .btn.home-main .sensor-label{font-size:var(--rc-home-main-label-font,14px);min-height:2.05em}
  .tpl-screen .btn.home-main .sensor-value{font-size:var(--rc-home-main-value-font,22px)}
  .tpl-screen .btn.home-main .sensor-error{font-size:var(--rc-home-main-error-font,11px);min-height:2em}
  .tpl-screen .stack-line{font-size:var(--rc-home-stack-font-small,13px)}
  .tpl-screen .stack-placeholder{font-size:var(--rc-home-stack-placeholder-font-small,14px)}
  .tpl-screen .sensor-list.compact.home-grid > .btn.home-stack:nth-child(3n-1) .stack-line,
  .tpl-screen .sensor-list.compact.home-grid > .btn.home-stack:nth-child(3n) .stack-line{font-size:18.2px}
  .tpl-screen .sensor-list.compact.home-grid > .btn.home-stack:nth-child(3n-1) .stack-placeholder,
  .tpl-screen .sensor-list.compact.home-grid > .btn.home-stack:nth-child(3n) .stack-placeholder{font-size:19.6px}
  .tpl-screen .home-header-manual-wrap{padding:0;margin:0 2px;flex-basis:calc(var(--rc-home-topbar-row-size,clamp(92px,26vw,112px))*1.302);width:calc(var(--rc-home-topbar-row-size,clamp(92px,26vw,112px))*1.302);max-width:calc(var(--rc-home-topbar-row-size,clamp(92px,26vw,112px))*1.302)}
  .tpl-screen .home-header-manual-btn{height:100%;min-height:0;padding:6px;margin:0}
  .tpl-screen .home-header-manual-grid{gap:4px 6px}
}


@keyframes rcBlink{0%,100%{opacity:1}50%{opacity:.35}}
)RCWEB";

static const size_t PAGE_APP_CSS_LEN = sizeof(PAGE_APP_CSS) - 1;
