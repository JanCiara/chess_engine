#!/usr/bin/env python3
"""
Automated Elo / SPRT testing for the chess_engine UCI binary via cutechess-cli.

Usage:
    python run_elo_tests.py                  # full setup + 1000-game match
    python run_elo_tests.py --dry-run        # print the cutechess-cli command only
    python run_elo_tests.py --games 200     # shorter run
    python run_elo_tests.py --engine build/Release/chess_engine.exe

Prerequisites:
    Build the engine first (Release recommended):
        cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release

    After the match finishes, filter out invalid games and estimate Elo:

        python analyze_elo.py                  # reads testing/results.pgn
        python run_elo_tests.py --analyze-only   # same analysis

    Writes testing/results_valid.pgn (mates, adjudication, no stalls/disconnects).
    For larger samples, also run Ordo on the filtered PGN:
        git clone https://github.com/michaeladams/ordo.git
        cd ordo && make
        ./ordo -a 60 -p testing/results_valid.pgn -c

    Ordo on the full PGN (includes invalid games — not recommended):
        ./ordo -a 60 -p testing/results.pgn -c

    BayesElo (Windows GUI or command line):
        Download from https://www.remi-coulom.fr/BayesElo/
        Open testing/results_valid.pgn, set the reference engine rating (Sunfish ~2000,
        TSCP ~1600 on fast time controls), then run "Rating" -> "Rate results".

    cutechess-cli also prints live Elo / LOS / SPRT status during the match.
    SPRT stops early when H0 or H1 is accepted; otherwise all --games are played.
"""

from __future__ import annotations

import argparse
import json
import os
import platform
import re
import shutil
import ssl
import subprocess
import sys
import tarfile
import tempfile
import zipfile
from pathlib import Path
from typing import Iterable
from urllib.error import URLError
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parent
TESTING_DIR = ROOT / "testing"
TOOLS_DIR = TESTING_DIR / "tools"
ENGINES_DIR = TESTING_DIR / "engines"
BOOKS_DIR = TESTING_DIR / "books"
RESULTS_PGN = TESTING_DIR / "results.pgn"

CUTECHESS_RELEASE = "v1.5.1"
CUTECHESS_FALLBACK_RELEASE = "v1.3.1"

SUNFISH_BASE = "https://raw.githubusercontent.com/thomasahle/sunfish/master"
SUNFISH_FILES = (
    "sunfish.py",
    "tools/__init__.py",
    "tools/uci.py",
)

OPENING_BOOK_URL = (
    "https://raw.githubusercontent.com/official-stockfish/books/master/2moves_v1.epd.zip"
)
OPENING_BOOK_NAME = "2moves_v1.epd"

TSCP_EXE_URL = "https://www.tckerrigan.com/Chess/TSCP/tscp181.exe.zip"
TSCP_EXE_NAME = "tscp181.exe"


def log(msg: str) -> None:
    print(msg, flush=True)


def warn(msg: str) -> None:
    print(f"WARNING: {msg}", file=sys.stderr, flush=True)


def die(msg: str, code: int = 1) -> None:
    print(f"ERROR: {msg}", file=sys.stderr, flush=True)
    raise SystemExit(code)


def ensure_dirs() -> None:
    for path in (TESTING_DIR, TOOLS_DIR, ENGINES_DIR, BOOKS_DIR):
        path.mkdir(parents=True, exist_ok=True)


def download_url(url: str, dest: Path, insecure: bool = False) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    log(f"  downloading {url}")
    ctx = ssl.create_default_context()
    if insecure:
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
    request = Request(url, headers={"User-Agent": "chess-engine-elo-test/1.0"})
    try:
        with urlopen(request, context=ctx, timeout=120) as response:
            data = response.read()
    except URLError as exc:
        die(f"download failed for {url}: {exc}")
    dest.write_bytes(data)


def github_release_assets(tag: str, insecure: bool) -> dict[str, str]:
    api = f"https://api.github.com/repos/cutechess/cutechess/releases/tags/{tag}"
    ctx = ssl.create_default_context()
    if insecure:
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
    request = Request(api, headers={"User-Agent": "chess-engine-elo-test/1.0"})
    try:
        with urlopen(request, context=ctx, timeout=60) as response:
            payload = json.load(response)
    except (URLError, json.JSONDecodeError, TimeoutError):
        return {}
    assets: dict[str, str] = {}
    for item in payload.get("assets", []):
        name = item.get("name", "")
        url = item.get("browser_download_url", "")
        if name and url:
            assets[name] = url
    return assets


def pick_asset(assets: dict[str, str], patterns: Iterable[str]) -> str | None:
    for pattern in patterns:
        rx = re.compile(pattern, re.IGNORECASE)
        for name, url in assets.items():
            if rx.search(name):
                return url
    return None


def find_on_path(name: str) -> Path | None:
    found = shutil.which(name)
    return Path(found) if found else None


def print_cutechess_install_help() -> None:
    system = platform.system()
    log("")
    log("cutechess-cli was not found and automatic installation failed.")
    log("Install it manually, then re-run this script:")
    log("")
    if system == "Windows":
        log("  Windows:")
        log("    1. Download cutechess-*-win64.zip from")
        log("       https://github.com/cutechess/cutechess/releases")
        log("    2. Extract the full zip into testing/tools/cutechess/")
        log("    3. Or add the folder containing cutechess-cli.exe to your PATH")
    elif system == "Darwin":
        log("  macOS:")
        log("    brew install cutechess")
        log("    # or build from https://github.com/cutechess/cutechess")
    else:
        log("  Linux:")
        log("    sudo apt install cutechess-cli        # Debian/Ubuntu")
        log("    sudo dnf install cutechess-cli        # Fedora")
        log("    # or download the linux64 tarball from GitHub releases")
    log("")


def extract_cutechess_from_zip(zip_path: Path, dest_dir: Path) -> Path | None:
    target_name = "cutechess-cli.exe" if sys.platform == "win32" else "cutechess-cli"
    install_root = dest_dir / "cutechess"
    if install_root.exists():
        shutil.rmtree(install_root)
    install_root.mkdir(parents=True, exist_ok=True)

    with zipfile.ZipFile(zip_path) as archive:
        archive.extractall(install_root)
        for path in install_root.rglob(target_name):
            if path.is_file():
                return path
    return None


def extract_cutechess_from_targz(tar_path: Path, dest_dir: Path) -> Path | None:
    install_root = dest_dir / "cutechess"
    if install_root.exists():
        shutil.rmtree(install_root)
    install_root.mkdir(parents=True, exist_ok=True)

    with tarfile.open(tar_path, "r:gz") as archive:
        archive.extractall(install_root)
        for path in install_root.rglob("cutechess-cli"):
            if path.is_file():
                path.chmod(path.stat().st_mode | 0o111)
                return path
    return None


def install_cutechess_cli(insecure: bool) -> Path | None:
    system = platform.system()
    assets = github_release_assets(CUTECHESS_RELEASE, insecure)
    if not assets:
        assets = github_release_assets(CUTECHESS_FALLBACK_RELEASE, insecure)

    with tempfile.TemporaryDirectory(prefix="cutechess-dl-") as tmp:
        tmp_dir = Path(tmp)

        if system == "Windows":
            url = pick_asset(
                assets,
                [r"win64\.zip$", r"win.*\.zip$"],
            )
            if not url:
                url = (
                    f"https://github.com/cutechess/cutechess/releases/download/"
                    f"{CUTECHESS_RELEASE}/cutechess-1.5.1-win64.zip"
                )
            archive_path = tmp_dir / "cutechess.zip"
            download_url(url, archive_path, insecure=insecure)
            return extract_cutechess_from_zip(archive_path, TOOLS_DIR)

        if system == "Linux":
            url = pick_asset(
                assets,
                [r"cutechess-cli.*linux64\.tar\.gz$", r"linux64\.tar\.gz$"],
            )
            if not url:
                url = (
                    "https://github.com/cutechess/cutechess/releases/download/"
                    f"{CUTECHESS_FALLBACK_RELEASE}/cutechess-cli-1.3.1-linux64.tar.gz"
                )
            archive_path = tmp_dir / "cutechess.tar.gz"
            download_url(url, archive_path, insecure=insecure)
            return extract_cutechess_from_targz(archive_path, TOOLS_DIR)

        if system == "Darwin":
            url = pick_asset(assets, [r"\.dmg$", r"macos", r"darwin"])
            if url:
                warn("macOS auto-download is not supported; use Homebrew instead.")
            return None

    return None


def resolve_cutechess_cli(insecure: bool, auto_install: bool) -> Path:
    local_name = "cutechess-cli.exe" if sys.platform == "win32" else "cutechess-cli"

    for path in TOOLS_DIR.rglob(local_name):
        if path.is_file():
            log(f"Using local cutechess-cli: {path}")
            return path

    legacy = TOOLS_DIR / local_name
    if legacy.is_file():
        log(f"Using local cutechess-cli: {legacy}")
        return legacy

    on_path = find_on_path("cutechess-cli")
    if on_path:
        log(f"Using cutechess-cli from PATH: {on_path}")
        return on_path

    if auto_install:
        log("cutechess-cli not found; attempting automatic download...")
        installed = install_cutechess_cli(insecure)
        if installed and installed.is_file():
            log(f"Installed cutechess-cli to {installed}")
            return installed

    print_cutechess_install_help()
    die("cutechess-cli is required.")


def verify_cutechess(cli: Path) -> None:
    try:
        result = subprocess.run(
            [str(cli), "-version"],
            capture_output=True,
            text=True,
            timeout=30,
            check=False,
            cwd=cli.parent,
        )
    except OSError as exc:
        die(f"cannot execute {cli}: {exc}")
    if result.returncode != 0:
        detail = (result.stderr or result.stdout or "").strip()
        die(f"cutechess-cli -version failed{f': {detail}' if detail else ''}")


def find_python() -> Path:
    for name in ("python3", "python", "py"):
        found = find_on_path(name)
        if found:
            return found
    die("Python 3 is required to run the Sunfish reference engine.")


def download_sunfish(insecure: bool) -> Path:
    sunfish_dir = ENGINES_DIR / "sunfish"
    sunfish_dir.mkdir(parents=True, exist_ok=True)
    for rel in SUNFISH_FILES:
        dest = sunfish_dir / rel
        if dest.is_file() and dest.stat().st_size > 0:
            continue
        if rel.endswith("__init__.py"):
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_text("", encoding="utf-8")
            continue
        url = f"{SUNFISH_BASE}/{rel}"
        download_url(url, dest, insecure=insecure)
    main = sunfish_dir / "sunfish.py"
    if not main.is_file():
        die("Sunfish download incomplete.")
    log(f"Sunfish ready at {sunfish_dir}")
    return sunfish_dir


def download_tscp(insecure: bool) -> Path:
    tscp_dir = ENGINES_DIR / "tscp"
    tscp_dir.mkdir(parents=True, exist_ok=True)
    exe_path = tscp_dir / TSCP_EXE_NAME
    if exe_path.is_file():
        log(f"TSCP ready at {exe_path}")
        return exe_path

    with tempfile.TemporaryDirectory(prefix="tscp-dl-") as tmp:
        zip_path = Path(tmp) / "tscp.zip"
        download_url(TSCP_EXE_URL, zip_path, insecure=insecure)
        with zipfile.ZipFile(zip_path) as archive:
            for member in archive.namelist():
                if member.lower().endswith(".exe"):
                    archive.extract(member, tscp_dir)
                    extracted = tscp_dir / Path(member).name
                    if extracted != exe_path:
                        if exe_path.exists():
                            exe_path.unlink()
                        extracted.rename(exe_path)
                    log(f"TSCP ready at {exe_path}")
                    return exe_path
    die("TSCP download failed: no .exe found in archive.")


def download_opening_book(insecure: bool) -> Path:
    book_path = BOOKS_DIR / OPENING_BOOK_NAME
    if book_path.is_file() and book_path.stat().st_size > 0:
        log(f"Opening book already present: {book_path}")
        return book_path

    with tempfile.TemporaryDirectory(prefix="book-dl-") as tmp:
        zip_path = Path(tmp) / "book.zip"
        download_url(OPENING_BOOK_URL, zip_path, insecure=insecure)
        with zipfile.ZipFile(zip_path) as archive:
            names = [n for n in archive.namelist() if n.lower().endswith(".epd")]
            if not names:
                die(f"no .epd file found inside {OPENING_BOOK_URL}")
            book_path.write_bytes(archive.read(names[0]))

    log(f"Opening book saved to {book_path}")
    return book_path


def resolve_reference_engine(reference: str, insecure: bool) -> tuple[list[str], str]:
    if reference == "sunfish":
        sunfish_dir = download_sunfish(insecure)
        python = find_python()
        return (
            [
                f"cmd={python}",
                "arg=sunfish.py",
                f"dir={sunfish_dir}",
                "name=Sunfish",
                "proto=uci",
            ],
            "Sunfish (~2000 Elo at fast TC with CPython; use PyPy for ~250 Elo more)",
        )

    if reference == "tscp":
        if sys.platform != "win32":
            die("TSCP auto-download is Windows-only; use --reference sunfish on Unix.")
        tscp_exe = download_tscp(insecure)
        return (
            [
                f"cmd={tscp_exe}",
                "name=TSCP",
                "proto=xboard",
            ],
            "TSCP (~1600 Elo, XBoard protocol)",
        )

    if sys.platform == "win32":
        try:
            return resolve_reference_engine("sunfish", insecure)
        except SystemExit:
            warn("Sunfish setup failed; falling back to TSCP.")
            return resolve_reference_engine("tscp", insecure)
    return resolve_reference_engine("sunfish", insecure)


def find_chess_engine(explicit: str | None) -> Path:
    if explicit:
        path = Path(explicit)
        if not path.is_file():
            die(f"engine not found: {path}")
        return path.resolve()

    candidates = [
        ROOT / "build" / "Release" / "chess_engine.exe",
        ROOT / "build" / "chess_engine.exe",
        ROOT / "build" / "chess_engine",
        ROOT / "chess_engine.exe",
        ROOT / "chess_engine",
    ]
    for path in candidates:
        if path.is_file():
            log(f"Using engine binary: {path}")
            return path.resolve()
    die(
        "chess_engine binary not found. Build first:\n"
        "  cmake -B build -DCMAKE_BUILD_TYPE=Release\n"
        "  cmake --build build --config Release\n"
        "Or pass --engine PATH"
    )


def build_cutechess_command(
    cli: Path,
    engine: Path,
    ref_args: list[str],
    book: Path,
    games: int,
    tc: str,
    concurrency: int,
    sprt: bool,
    sprt_elo0: float,
    sprt_elo1: float,
    sprt_alpha: float,
    sprt_beta: float,
) -> list[str]:
    cmd: list[str] = [
        str(cli),
        "-engine",
        f"cmd={engine}",
        "name=chess_engine",
        "proto=uci",
        "-engine",
        *ref_args,
        "-each",
        f"tc={tc}",
        "timemargin=200",
        "restart=on",
        "-concurrency",
        str(concurrency),
        "-rounds",
        str(games),
        "-openings",
        f"file={book}",
        "format=epd",
        "order=random",
        "plies=2",
        "-repeat",
        "-recover",
        "-draw",
        "movenumber=40",
        "movecount=8",
        "score=10",
        "-resign",
        "movecount=3",
        "score=500",
        "-pgnout",
        str(RESULTS_PGN),
        "-event",
        "chess_engine Elo test",
    ]
    if sprt:
        cmd.extend(
            [
                "-sprt",
                f"elo0={sprt_elo0}",
                f"elo1={sprt_elo1}",
                f"alpha={sprt_alpha}",
                f"beta={sprt_beta}",
            ]
        )
    return cmd


def parse_args() -> argparse.Namespace:
    cpus = os.cpu_count() or 1
    parser = argparse.ArgumentParser(
        description="Run cutechess-cli Elo/SPRT tests for chess_engine.",
    )
    parser.add_argument(
        "--engine",
        help="Path to chess_engine binary (auto-detected under build/ if omitted).",
    )
    parser.add_argument(
        "--reference",
        choices=("auto", "sunfish", "tscp"),
        default="auto",
        help="Reference opponent (default: sunfish, TSCP fallback on Windows).",
    )
    parser.add_argument(
        "--games",
        type=int,
        default=1000,
        help="Maximum number of games (-rounds). Default: 1000.",
    )
    parser.add_argument(
        "--tc",
        default="10+0.1",
        help='Time control for both engines, e.g. "10+0.1" (default).',
    )
    parser.add_argument(
        "--concurrency",
        type=int,
        default=cpus,
        help=f"Parallel games (default: {cpus}).",
    )
    parser.add_argument(
        "--no-sprt",
        action="store_true",
        help="Disable SPRT early stopping (play all games).",
    )
    parser.add_argument(
        "--sprt-elo0",
        type=float,
        default=0.0,
        help="SPRT H1: engine is stronger by at least this many Elo (default: 0).",
    )
    parser.add_argument(
        "--sprt-elo1",
        type=float,
        default=5.0,
        help="SPRT H0 band edge in Elo (default: 5).",
    )
    parser.add_argument(
        "--sprt-alpha",
        type=float,
        default=0.05,
        help="SPRT type-I error probability (default: 0.05).",
    )
    parser.add_argument(
        "--sprt-beta",
        type=float,
        default=0.05,
        help="SPRT type-II error probability (default: 0.05).",
    )
    parser.add_argument(
        "--skip-download",
        action="store_true",
        help="Skip downloading reference engine and opening book.",
    )
    parser.add_argument(
        "--no-install-cutechess",
        action="store_true",
        help="Do not attempt to download cutechess-cli automatically.",
    )
    parser.add_argument(
        "--insecure-downloads",
        action="store_true",
        help="Disable TLS certificate verification for downloads (use only if needed).",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the cutechess-cli command without running it.",
    )
    parser.add_argument(
        "--analyze-only",
        action="store_true",
        help="Analyze testing/results.pgn (valid games only) and exit.",
    )
    parser.add_argument(
        "--reference-elo",
        type=int,
        default=2000,
        help="Sunfish reference rating for analyze-only mode (default: 2000).",
    )
    return parser.parse_args()


def run_post_match_analysis(reference_elo: int = 2000) -> None:
    from analyze_elo import analyze_pgn, format_report, write_filtered_pgn

    if not RESULTS_PGN.is_file():
        warn(f"No PGN at {RESULTS_PGN}; skipping analysis.")
        return

    stats = analyze_pgn(RESULTS_PGN, reference_elo)
    valid_pgn = RESULTS_PGN.with_name("results_valid.pgn")
    count = write_filtered_pgn(RESULTS_PGN, valid_pgn)
    log("")
    log(format_report(stats, reference_elo))
    log(f"  Filtered PGN:  {valid_pgn} ({count} games)")


def main() -> None:
    args = parse_args()
    if args.analyze_only:
        run_post_match_analysis(args.reference_elo)
        return

    ensure_dirs()

    log("=== chess_engine Elo / SPRT test runner ===")
    log(f"Platform: {platform.system()} {platform.machine()}")

    cli = resolve_cutechess_cli(args.insecure_downloads, auto_install=not args.no_install_cutechess)
    if not args.dry_run:
        verify_cutechess(cli)

    engine = find_chess_engine(args.engine)

    if args.skip_download:
        book = BOOKS_DIR / OPENING_BOOK_NAME
        if not book.is_file():
            die(f"opening book missing: {book} (remove --skip-download)")
        if args.reference == "sunfish":
            ref_args = [
                f"cmd={find_python()}",
                "arg=sunfish.py",
                f"dir={ENGINES_DIR / 'sunfish'}",
                "name=Sunfish",
                "proto=uci",
            ]
            ref_desc = "Sunfish (cached)"
        elif args.reference == "tscp":
            ref_args = [
                f"cmd={ENGINES_DIR / 'tscp' / TSCP_EXE_NAME}",
                "name=TSCP",
                "proto=xboard",
            ]
            ref_desc = "TSCP (cached)"
        else:
            ref_args, ref_desc = resolve_reference_engine(args.reference, args.insecure_downloads)
    else:
        book = download_opening_book(args.insecure_downloads)
        ref_args, ref_desc = resolve_reference_engine(args.reference, args.insecure_downloads)

    cmd = build_cutechess_command(
        cli=cli,
        engine=engine,
        ref_args=ref_args,
        book=book,
        games=args.games,
        tc=args.tc,
        concurrency=args.concurrency,
        sprt=not args.no_sprt,
        sprt_elo0=args.sprt_elo0,
        sprt_elo1=args.sprt_elo1,
        sprt_alpha=args.sprt_alpha,
        sprt_beta=args.sprt_beta,
    )

    log("")
    log(f"Engine:    {engine}")
    log(f"Reference: {ref_desc}")
    log(f"Book:      {book}")
    log(f"Output:    {RESULTS_PGN}")
    log(f"Games:     up to {args.games}  TC: {args.tc}  Concurrency: {args.concurrency}")
    if not args.no_sprt:
        log(
            f"SPRT:      elo0={args.sprt_elo0} elo1={args.sprt_elo1} "
            f"alpha={args.sprt_alpha} beta={args.sprt_beta}"
        )
    log("")
    log("Command:")
    log(subprocess.list2cmdline(cmd))
    log("")

    if args.dry_run:
        return

    RESULTS_PGN.parent.mkdir(parents=True, exist_ok=True)
    log("Starting match (live Elo / SPRT output from cutechess-cli)...")
    log("=" * 60)
    try:
        result = subprocess.run(cmd, cwd=cli.parent, check=False)
    except KeyboardInterrupt:
        warn("Interrupted by user.")
        raise SystemExit(130) from None

    log("=" * 60)
    if result.returncode != 0:
        die(f"cutechess-cli exited with code {result.returncode}")

    if RESULTS_PGN.is_file():
        log(f"Results saved to {RESULTS_PGN}")
        run_post_match_analysis(args.reference_elo)
    else:
        warn(f"Expected PGN not found at {RESULTS_PGN}")


if __name__ == "__main__":
    main()
