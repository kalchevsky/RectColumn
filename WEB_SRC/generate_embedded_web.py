import argparse
import gzip
import io
import sys
from dataclasses import dataclass
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_LABEL = "WEB_SRC/generate_embedded_web.py"
LEGACY_UPLOT_GZIP_MTIME = 0x6A574F3E


@dataclass(frozen=True)
class AssetSpec:
    mode: str
    source: str
    output: str
    symbol: str
    source_label: str
    gzip_mtime: int | None = None


ASSET_SPECS = (
    AssetSpec(
        mode="text",
        source="WEB_SRC/page-app.js",
        output="WebPageAppJs.h",
        symbol="PAGE_APP_JS",
        source_label="WEB_SRC/page-app.js",
    ),
    AssetSpec(
        mode="text",
        source="WEB_SRC/app.css",
        output="WebPageAppCss.h",
        symbol="PAGE_APP_CSS",
        source_label="WEB_SRC/app.css",
    ),
    AssetSpec(
        mode="gzip",
        source="WEB_SRC/uplot.min.js",
        output="WebUplotJs.h",
        symbol="UPLOT_JS_GZ",
        source_label="WEB_SRC/uplot.min.js",
        gzip_mtime=LEGACY_UPLOT_GZIP_MTIME,
    ),
    AssetSpec(
        mode="gzip",
        source="WEB_SRC/uplot.min.css",
        output="WebUplotCss.h",
        symbol="UPLOT_CSS_GZ",
        source_label="WEB_SRC/uplot.min.css",
        gzip_mtime=LEGACY_UPLOT_GZIP_MTIME,
    ),
)


def choose_raw_delimiter(text: str) -> str:
    for delimiter in ("RCWEB", "RCWEB_A", "RCWEB_B", "RCWEB_C"):
        if f"){delimiter}\"" not in text:
            return delimiter
    raise ValueError("Could not find a safe raw-string delimiter")


def read_text_exact(path: Path) -> str:
    return path.read_bytes().decode("utf-8")


def build_text_header(source_label: str, symbol: str, text: str) -> str:
    delimiter = choose_raw_delimiter(text)
    return (
        f"// Auto-generated from {source_label} by {SCRIPT_LABEL}. Do not edit manually.\n"
        "#pragma once\n"
        "#include <Arduino.h>\n"
        "\n"
        f"static const char {symbol}[] PROGMEM = R\"{delimiter}({text}){delimiter}\";\n"
        "\n"
        f"static const size_t {symbol}_LEN = sizeof({symbol}) - 1;\n"
    )


def gzip_bytes(data: bytes, mtime: int) -> bytes:
    buf = io.BytesIO()
    with gzip.GzipFile(filename="", mode="wb", fileobj=buf, mtime=mtime) as gz:
        gz.write(data)
    return buf.getvalue()


def format_byte_lines(blob: bytes, per_line: int = 12) -> str:
    lines: list[str] = []
    for start in range(0, len(blob), per_line):
        chunk = blob[start : start + per_line]
        items = ", ".join(f"0x{value:02X}" for value in chunk)
        lines.append(f"  {items},")
    return "\n".join(lines)


def build_gzip_header(source_label: str, symbol: str, data: bytes, gzip_mtime: int) -> str:
    blob = gzip_bytes(data, gzip_mtime)
    body = format_byte_lines(blob)
    return (
        f"// Auto-generated from {source_label} by {SCRIPT_LABEL}. Do not edit manually.\n"
        "#pragma once\n"
        "#include <Arduino.h>\n"
        "\n"
        f"static const uint8_t {symbol}[] PROGMEM = {{\n"
        f"{body}\n"
        "};\n"
        "\n"
        f"static const size_t {symbol}_LEN = {len(blob)};\n"
    )


def render_asset(
    mode: str,
    source: Path,
    output: Path,
    symbol: str,
    source_label: str,
    gzip_mtime: int | None,
) -> str:
    if mode == "text":
        return build_text_header(source_label, symbol, read_text_exact(source))
    if mode == "gzip":
        if gzip_mtime is None:
            raise ValueError("gzip assets require --gzip-mtime")
        return build_gzip_header(source_label, symbol, source.read_bytes(), gzip_mtime)
    raise ValueError(f"Unsupported mode: {mode}")


def write_or_check(output: Path, content: str, check: bool) -> bool:
    encoded = content.encode("utf-8")
    current = output.read_bytes() if output.exists() else None
    changed = current != encoded
    if check:
        return changed
    output.write_bytes(encoded)
    return changed


def run_one(
    *,
    mode: str,
    source: Path,
    output: Path,
    symbol: str,
    source_label: str,
    gzip_mtime: int | None,
    check: bool,
) -> bool:
    content = render_asset(mode, source, output, symbol, source_label, gzip_mtime)
    changed = write_or_check(output, content, check)
    verb = "DIFF" if check and changed else "OK" if check else "UPDATED" if changed else "UNCHANGED"
    print(f"{verb}: {output.relative_to(REPO_ROOT)} <- {source_label}")
    return changed


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate embedded Web UI headers from WEB_SRC assets."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    parser_all = subparsers.add_parser("all", help="Generate every embedded Web UI header.")
    parser_all.add_argument(
        "--check",
        action="store_true",
        help="Verify that outputs are up to date without rewriting files.",
    )

    for command in ("text", "gzip"):
        sub = subparsers.add_parser(command, help=f"Generate a single {command} header.")
        sub.add_argument("--source", required=True, help="Source file path, relative to repo root.")
        sub.add_argument("--output", required=True, help="Output header path, relative to repo root.")
        sub.add_argument("--symbol", required=True, help="C/C++ symbol name.")
        sub.add_argument(
            "--source-label",
            required=True,
            help="Path string embedded into the generated comment.",
        )
        sub.add_argument(
            "--check",
            action="store_true",
            help="Verify that the output is up to date without rewriting it.",
        )
        if command == "gzip":
            sub.add_argument(
                "--gzip-mtime",
                required=True,
                type=lambda value: int(value, 0),
                help="Deterministic gzip mtime, for example 0x6A574F3E.",
            )

    return parser.parse_args()


def main() -> int:
    args = parse_args()
    changed = False

    if args.command == "all":
        for spec in ASSET_SPECS:
            changed = run_one(
                mode=spec.mode,
                source=REPO_ROOT / spec.source,
                output=REPO_ROOT / spec.output,
                symbol=spec.symbol,
                source_label=spec.source_label,
                gzip_mtime=spec.gzip_mtime,
                check=args.check,
            ) or changed
        return 1 if args.check and changed else 0

    gzip_mtime = getattr(args, "gzip_mtime", None)
    changed = run_one(
        mode=args.command,
        source=REPO_ROOT / args.source,
        output=REPO_ROOT / args.output,
        symbol=args.symbol,
        source_label=args.source_label,
        gzip_mtime=gzip_mtime,
        check=args.check,
    )
    return 1 if args.check and changed else 0


if __name__ == "__main__":
    sys.exit(main())
