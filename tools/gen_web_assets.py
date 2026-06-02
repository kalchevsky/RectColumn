#!/usr/bin/env python3
"""Generate gzip-compressed Arduino header assets from web source files.

Usage:
    python tools/gen_web_assets.py --input OUT/page-app.js --output WebPageAppJs.h --symbol PAGE_APP_JS_GZ --length-symbol PAGE_APP_JS_GZ_LEN
"""

from __future__ import annotations

import argparse
import gzip
import io
from pathlib import Path


LEGACY_COMMENT = "Auto-generated from OUT/page-app.js. Do not edit manually."


def gzip_bytes(data: bytes, mtime: int, compresslevel: int = 9) -> bytes:
    buf = io.BytesIO()
    with gzip.GzipFile(
        filename="",
        mode="wb",
        fileobj=buf,
        compresslevel=compresslevel,
        mtime=mtime,
    ) as gz:
        gz.write(data)
    return buf.getvalue()


def format_header(comment: str, symbol: str, length_symbol: str, payload: bytes) -> str:
    lines = [
        f"// {comment}",
        "#pragma once",
        "#include <Arduino.h>",
        "",
        f"static const uint8_t {symbol}[] PROGMEM = {{",
    ]
    for i in range(0, len(payload), 12):
        chunk = payload[i : i + 12]
        lines.append("  " + ", ".join(f"0x{byte:02X}" for byte in chunk) + ",")
    lines.extend(
        [
            "};",
            "",
            f"static const size_t {length_symbol} = {len(payload)};",
            "",
        ]
    )
    return "\n".join(lines)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, help="Source asset file")
    parser.add_argument("--output", required=True, help="Generated header file")
    parser.add_argument("--symbol", required=True, help="C symbol for gzip byte array")
    parser.add_argument("--length-symbol", required=True, help="C symbol for gzip payload length")
    parser.add_argument(
        "--mtime",
        type=int,
        default=None,
        help="Override gzip MTIME. Defaults to the input file mtime.",
    )
    parser.add_argument(
        "--comment",
        default=LEGACY_COMMENT,
        help="Comment for the first generated line, without leading //",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    input_path = Path(args.input)
    output_path = Path(args.output)
    data = input_path.read_bytes()
    mtime = args.mtime if args.mtime is not None else int(input_path.stat().st_mtime)
    payload = gzip_bytes(data, mtime=mtime, compresslevel=9)
    output_path.write_text(
        format_header(args.comment, args.symbol, args.length_symbol, payload),
        encoding="utf-8",
        newline="\n",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
