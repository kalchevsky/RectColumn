#!/usr/bin/env python3
"""
Пассивный мониторинг устройства 1.6.32-hotfix1.

Скрипт НЕ управляет устройством, только наблюдает:
  - снимок diag (heap, stopLatched, notify*)
  - снимок журнала
  - ждёт N секунд
  - повторный снимок
  - сравнивает: появились ли WER/ошибки, просел ли heap, выросли ли dropped

Запуск:
    python test_passive_244.py --host 192.168.10.244 --duration 60

Сценарий теста хотфикса:
    1. Запусти скрипт с --duration 60
    2. В первые 5 секунд нажми STOP на устройстве (или активируй из UI)
    3. Скрипт через 60s покажет, появились ли в журнале WER-события
"""

import argparse
import json
import sys
import time
from typing import Any, Optional

try:
    import requests
except ImportError:
    print("ERROR: pip install requests", file=sys.stderr)
    sys.exit(2)


GREEN = "\033[32m"
RED = "\033[31m"
YELLOW = "\033[33m"
CYAN = "\033[36m"
GRAY = "\033[90m"
RESET = "\033[0m"


def http_get_json(url: str, timeout: float = 5.0) -> Any:
    r = requests.get(url, timeout=timeout)
    r.raise_for_status()
    return r.json()


def fmt_entry(entry: dict) -> str:
    """Красиво напечатать запись журнала."""
    t = entry.get("t", "?")
    e = entry.get("e", "")
    return f"[{t}] {e}"


def is_suspicious(entry: dict) -> bool:
    """
    Запись в журнале похожа на ложное срабатывание защиты канала.
    Маркеры — на основе известного формата прошивки:
      - "WER_CH" в тексте
      - "STUCK_ON" в тексте
      - "застрял" / "не отключ" в русском тексте
      - "TIMEOUT" канала
    """
    txt = str(entry.get("e", "")).upper()
    txt_lower = str(entry.get("e", "")).lower()
    if "WER_CH" in txt:
        return True
    if "STUCK_ON" in txt or "STUCK ON" in txt:
        return True
    if "TIMEOUT" in txt and "CH" in txt:
        return True
    # Русские маркеры — на всякий случай
    for marker in ("застрял", "не отключ", "залип", "не выключ"):
        if marker in txt_lower:
            return True
    return False


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--host", default="192.168.10.244")
    p.add_argument("--duration", type=int, default=60,
                   help="сколько секунд наблюдать (по умолчанию 60)")
    p.add_argument("--show-all-new", action="store_true",
                   help="показать ВСЕ новые записи журнала, а не только подозрительные")
    args = p.parse_args()

    base = f"http://{args.host}"
    print(f"{CYAN}Пассивный мониторинг {base}{RESET}")
    print(f"Длительность наблюдения: {args.duration} секунд\n")

    # -------- СНИМОК ДО --------
    print(f"{CYAN}=== СНИМОК ДО ==={RESET}")
    try:
        diag_before = http_get_json(f"{base}/api/v1/diag")
    except Exception as ex:
        print(f"{RED}Не удалось получить /api/v1/diag: {ex}{RESET}")
        return 2

    try:
        log_before = http_get_json(f"{base}/api/v1/log")
        if not isinstance(log_before, list):
            log_before = []
    except Exception as ex:
        print(f"{YELLOW}Не удалось получить /api/v1/log: {ex}{RESET}")
        log_before = []

    heap_before = diag_before.get("freeHeap")
    min_heap_before = diag_before.get("minFreeHeap")
    stop_before = diag_before.get("stopLatched")
    dropped_before = diag_before.get("notifyDroppedCount", 0)
    queue_before = diag_before.get("notifyQueueDepth", 0)
    worker_before = diag_before.get("notifyWorkerReady")
    pin35_before = diag_before.get("pin35Mode")
    mismatches_before = diag_before.get("activeConfirmMismatches", [])

    print(f"  freeHeap          : {heap_before}")
    print(f"  minFreeHeap       : {min_heap_before}")
    print(f"  stopLatched       : {stop_before}")
    print(f"  pin35Mode         : {pin35_before}")
    print(f"  notifyWorkerReady : {worker_before}")
    print(f"  notifyQueueDepth  : {queue_before}")
    print(f"  notifyDroppedCount: {dropped_before}")
    print(f"  confirmMismatches : {mismatches_before}")
    print(f"  Записей в журнале : {len(log_before)}")

    # Идентификация записей — по паре (t, ms, e), чтобы корректно увидеть новые
    def entry_key(e):
        return (e.get("t"), e.get("ms"), e.get("e"))

    keys_before = set(entry_key(e) for e in log_before if isinstance(e, dict))

    # -------- НАБЛЮДЕНИЕ --------
    print(f"\n{CYAN}=== НАБЛЮДЕНИЕ ({args.duration}s) ==={RESET}")
    print(f"{GRAY}Если хочешь проверить хотфикс — активируй STOP прямо сейчас.{RESET}\n")

    deadline = time.time() + args.duration
    last_heap = heap_before
    last_dropped = dropped_before
    last_stop = stop_before

    while time.time() < deadline:
        time.sleep(2.0)
        try:
            d = http_get_json(f"{base}/api/v1/diag", timeout=3.0)
            h = d.get("freeHeap")
            dr = d.get("notifyDroppedCount", 0)
            st = d.get("stopLatched")
            if st != last_stop:
                marker = "АКТИВИРОВАН" if st else "СНЯТ"
                print(f"\n  {YELLOW}>>> STOP {marker} <<<{RESET}")
                last_stop = st
            if dr != last_dropped:
                print(f"\n  {YELLOW}>>> notifyDroppedCount: {last_dropped} → {dr}{RESET}")
                last_dropped = dr
            last_heap = h
        except Exception:
            pass

        remaining = int(deadline - time.time())
        print(f"\r{GRAY}    осталось {remaining:3d}s, "
              f"heap={last_heap}, stop={last_stop}, dropped={last_dropped}{RESET}    ",
              end="")
    print("\n")

    # -------- СНИМОК ПОСЛЕ --------
    print(f"{CYAN}=== СНИМОК ПОСЛЕ ==={RESET}")
    try:
        diag_after = http_get_json(f"{base}/api/v1/diag")
        log_after = http_get_json(f"{base}/api/v1/log")
        if not isinstance(log_after, list):
            log_after = []
    except Exception as ex:
        print(f"{RED}Не удалось получить итоговые снимки: {ex}{RESET}")
        return 2

    heap_after = diag_after.get("freeHeap")
    min_heap_after = diag_after.get("minFreeHeap")
    stop_after = diag_after.get("stopLatched")
    dropped_after = diag_after.get("notifyDroppedCount", 0)
    queue_after = diag_after.get("notifyQueueDepth", 0)
    worker_after = diag_after.get("notifyWorkerReady")
    mismatches_after = diag_after.get("activeConfirmMismatches", [])

    print(f"  freeHeap          : {heap_before} → {heap_after}  (Δ = {(heap_after or 0)-(heap_before or 0):+d})")
    print(f"  minFreeHeap       : {min_heap_before} → {min_heap_after}")
    print(f"  stopLatched       : {stop_before} → {stop_after}")
    print(f"  notifyWorkerReady : {worker_before} → {worker_after}")
    print(f"  notifyQueueDepth  : {queue_before} → {queue_after}")
    print(f"  notifyDroppedCount: {dropped_before} → {dropped_after}  (Δ = +{dropped_after-dropped_before})")
    print(f"  confirmMismatches : {mismatches_after}")
    print(f"  Записей в журнале : {len(log_before)} → {len(log_after)}")

    # -------- АНАЛИЗ ЖУРНАЛА --------
    keys_after = set(entry_key(e) for e in log_after if isinstance(e, dict))
    new_entries = [e for e in log_after if isinstance(e, dict) and entry_key(e) not in keys_before]
    suspicious = [e for e in new_entries if is_suspicious(e)]

    print(f"\n{CYAN}=== АНАЛИЗ ЖУРНАЛА ==={RESET}")
    print(f"  Новых записей за период: {len(new_entries)}")
    print(f"  Подозрительных (WER/STUCK/timeout): {len(suspicious)}")

    if args.show_all_new and new_entries:
        print(f"\n  {GRAY}Все новые записи:{RESET}")
        for e in new_entries:
            print(f"    {fmt_entry(e)}")

    if suspicious:
        print(f"\n  {RED}ПОДОЗРИТЕЛЬНЫЕ ЗАПИСИ:{RESET}")
        for e in suspicious:
            print(f"    {RED}{fmt_entry(e)}{RESET}")

    # -------- ВЕРДИКТ --------
    print(f"\n{CYAN}=== ВЕРДИКТ ==={RESET}")
    problems = []

    # 1. Ложные WER
    if suspicious:
        problems.append(f"в журнале {len(suspicious)} подозрительных записей")
    # 2. Просадка heap
    if heap_before and heap_after:
        delta = heap_after - heap_before
        if delta < -4096:
            problems.append(f"freeHeap просел на {-delta} байт (норма ≤ 4 KB)")
    # 3. NotifyWorker завис
    if worker_after is False:
        problems.append("notifyWorkerReady=false — воркер уведомлений не готов")
    # 4. Очередь застряла
    if queue_after and queue_after > 4:
        problems.append(f"notifyQueueDepth={queue_after} — очередь не разгружается")
    # 5. Рассинхрон каналов
    if mismatches_after:
        problems.append(f"activeConfirmMismatches не пуст: {mismatches_after}")

    if not problems:
        print(f"  {GREEN}✓ Признаков проблем не обнаружено{RESET}")
        print(f"  {GREEN}✓ Хотфикс работает: ложных WER нет, heap стабилен, воркер живой{RESET}")
        return 0
    else:
        print(f"  {RED}✗ Обнаружены проблемы:{RESET}")
        for p in problems:
            print(f"    {RED}- {p}{RESET}")
        return 1


if __name__ == "__main__":
    sys.exit(main())