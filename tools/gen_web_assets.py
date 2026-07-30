#!/usr/bin/env python3
"""Generate Arduino header assets from web source files.

Usage:
    python tools/gen_web_assets.py --input OUT/page-app.js --output WebPageAppJs.h --symbol PAGE_APP_JS_GZ --length-symbol PAGE_APP_JS_GZ_LEN
"""

from __future__ import annotations

import argparse
import gzip
import hashlib
import io
from pathlib import Path


def default_comment(input_path: Path) -> str:
    return f"Auto-generated from {input_path.as_posix()}. Do not edit manually."


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


def choose_raw_delimiter(text: str) -> str:
    preferred = "RCWEB"
    if f"){preferred}\"" not in text:
        return preferred

    digest = hashlib.sha1(text.encode("utf-8")).hexdigest().upper()
    candidates = [f"RC{digest[:8]}", f"RC{digest[:12]}"]
    candidates.extend(f"RC{i:04X}" for i in range(0x10000))

    for delim in candidates:
        if len(delim) <= 16 and f"){delim}\"" not in text:
            return delim

    raise ValueError("Could not find a collision-free raw-string delimiter for the asset content.")


def format_text_header(comment: str, symbol: str, length_symbol: str, text: str, delim: str) -> str:
    return (
        f"// {comment}\n"
        "#pragma once\n"
        "#include <Arduino.h>\n\n"
        f"static const char {symbol}[] PROGMEM = R\"{delim}({text}){delim}\";\n\n"
        f"static const size_t {length_symbol} = sizeof({symbol}) - 1;\n"
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, help="Source asset file")
    parser.add_argument("--output", required=True, help="Generated header file")
    parser.add_argument("--symbol", required=True, help="C symbol for payload byte array")
    parser.add_argument("--length-symbol", required=True, help="C symbol for payload length")
    mode_group = parser.add_mutually_exclusive_group()
    mode_group.add_argument(
        "--raw",
        action="store_true",
        help="Write the source bytes as-is without gzip compression.",
    )
    mode_group.add_argument(
        "--raw-string",
        action="store_true",
        help="Write UTF-8 text as a readable const char[] PROGMEM raw string literal.",
    )
    parser.add_argument(
        "--mtime",
        type=int,
        default=None,
        help="Override gzip MTIME. Defaults to the input file mtime.",
    )
    parser.add_argument(
        "--comment",
        default=None,
        help="Comment for the first generated line, without leading //",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    input_path = Path(args.input)
    output_path = Path(args.output)
    data = input_path.read_bytes()
    comment = args.comment if args.comment is not None else default_comment(input_path)

    if args.raw_string:
        if b"\x00" in data:
            raise ValueError(f"{input_path.as_posix()} contains NUL bytes and cannot be emitted as readable text.")
        try:
            text = data.decode("utf-8")
        except UnicodeDecodeError as exc:
            raise ValueError(
                f"{input_path.as_posix()} is not valid UTF-8 and cannot be emitted as readable text."
            ) from exc
        delim = choose_raw_delimiter(text)
        rendered = format_text_header(comment, args.symbol, args.length_symbol, text, delim)
    else:
        mtime = args.mtime if args.mtime is not None else int(input_path.stat().st_mtime)
        payload = data if args.raw else gzip_bytes(data, mtime=mtime, compresslevel=9)
        rendered = format_header(comment, args.symbol, args.length_symbol, payload)

    output_path.write_text(rendered, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
