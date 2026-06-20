#!/usr/bin/env python3
"""Analyze match PGN: Elo from legitimately finished games only."""

from __future__ import annotations

import math
import re
import sys
from collections import Counter
from pathlib import Path

REFERENCE_ELO = 2000
INVALID_TERMINATIONS = frozenset(
    {"stalled connection", "abandoned", "disconnected", "illegal move", "time forfeit"}
)
INVALID_LAST_MOVE = re.compile(r"disconnect|connection stall", re.IGNORECASE)


def split_games(pgn_text: str) -> list[str]:
    return [g for g in re.split(r"\n\n(?=\[Event )", pgn_text.strip()) if g.strip()]


def game_headers(game: str) -> dict[str, str]:
    headers: dict[str, str] = {}
    for line in game.splitlines():
        if not line.startswith("[") or not line.endswith("]"):
            break
        match = re.match(r'\[(\w+)\s+"(.*)"\]', line)
        if match:
            headers[match.group(1)] = match.group(2)
    return headers


def move_text(game: str) -> str:
    lines = [line for line in game.splitlines() if line and not line.startswith("[")]
    return "\n".join(lines)


def last_move_comment(game: str) -> str:
    text = move_text(game)
    if not text:
        return ""
    comments = re.findall(r"\{([^}]*)\}", text)
    return comments[-1] if comments else ""


def is_valid_game(game: str) -> bool:
    headers = game_headers(game)
    termination = headers.get("Termination", "").lower()
    if termination in INVALID_TERMINATIONS:
        return False
    if INVALID_LAST_MOVE.search(last_move_comment(game)):
        return False
    plies = int(headers.get("PlyCount", "0"))
    if plies < 6:
        return False
    return True


def engine_score(valid_games: list[str]) -> tuple[int, int, int]:
    wins = draws = 0
    for game in valid_games:
        headers = game_headers(game)
        result = headers.get("Result", "*")
        white = headers.get("White", "")
        engine_white = white == "chess_engine"
        if result == "1/2-1/2":
            draws += 1
        elif (result == "1-0" and engine_white) or (result == "0-1" and not engine_white):
            wins += 1
    losses = len(valid_games) - wins - draws
    return wins, losses, draws


def elo_stats(wins: int, losses: int, draws: int, reference: int = REFERENCE_ELO) -> dict[str, float | int]:
    n = wins + losses + draws
    if n == 0:
        return {"games": 0, "score_pct": 0.0, "elo_diff": 0.0, "elo_err": 0.0, "rating": float(reference)}

    score = (wins + 0.5 * draws) / n
    if score <= 0 or score >= 1:
        elo_diff = math.copysign(float("inf"), score - 0.5)
        elo_err = 0.0
    else:
        elo_diff = -400 * math.log10(1 / score - 1)
        se = math.sqrt(score * (1 - score) / n)
        elo_err = 400 * se / (score * (1 - score))

    return {
        "games": n,
        "wins": wins,
        "losses": losses,
        "draws": draws,
        "score_pct": 100 * score,
        "elo_diff": elo_diff,
        "elo_err": elo_err * 1.96,
        "rating": reference + elo_diff,
    }


def analyze_pgn(path: Path, reference_elo: int = REFERENCE_ELO) -> dict:
    pgn_text = path.read_text(encoding="utf-8", errors="replace")
    games = split_games(pgn_text)
    terminations = Counter(
        game_headers(g).get("Termination", "(none)").lower() for g in games
    )
    valid = [g for g in games if is_valid_game(g)]
    wins, losses, draws = engine_score(valid)
    stats = elo_stats(wins, losses, draws, reference_elo)
    stats["total_games"] = len(games)
    stats["excluded"] = len(games) - len(valid)
    stats["terminations"] = dict(terminations)
    return stats


def write_filtered_pgn(source: Path, dest: Path) -> int:
    pgn_text = source.read_text(encoding="utf-8", errors="replace")
    valid = [g for g in split_games(pgn_text) if is_valid_game(g)]
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text("\n\n\n".join(valid) + "\n", encoding="utf-8")
    return len(valid)


def format_report(stats: dict, reference_elo: int = REFERENCE_ELO) -> str:
    lines = [
        "Elo analysis (legitimate games only)",
        f"  Source total:  {stats['total_games']} games",
        f"  Excluded:      {stats['excluded']} (stalls / disconnects / too short)",
        f"  Valid games:   {stats['games']}",
        f"  Score:         {stats['wins']}-{stats['losses']}-{stats['draws']}"
        f" ({stats['score_pct']:.1f}%)",
    ]
    n = stats["games"]
    if n == 0:
        lines.append("  No valid games to estimate Elo.")
        return "\n".join(lines)

    if math.isfinite(stats["elo_diff"]):
        lines.append(
            f"  vs Sunfish {reference_elo}: {stats['elo_diff']:+.0f} +/- {stats['elo_err']:.0f} Elo"
        )
        lines.append(
            f"  Estimated rating: ~{stats['rating']:.0f} +/- {stats['elo_err']:.0f}"
        )
        return "\n".join(lines)

    # Perfect 0% or 100% — report Wilson 95% bound (sample usually too small).
    wins = stats["wins"] + 0.5 * stats["draws"]
    p = wins / n
    z = 1.96
    denom = 1 + z * z / n
    margin = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / denom
    center = (p + z * z / (2 * n)) / denom
    if p >= 0.5:
        bound = max(0.01, center - margin)
        elo_bound = -400 * math.log10(1 / bound - 1)
        lines.append(
            f"  vs Sunfish {reference_elo}: >{elo_bound:+.0f} Elo (95% lower bound, n={n})"
        )
        lines.append(
            f"  Estimated rating: >{reference_elo + elo_bound:.0f} (n={n}, not statistically reliable)"
        )
    else:
        bound = min(0.99, center + margin)
        elo_bound = -400 * math.log10(1 / bound - 1)
        lines.append(
            f"  vs Sunfish {reference_elo}: <{elo_bound:+.0f} Elo (95% upper bound, n={n})"
        )
        lines.append(
            f"  Estimated rating: <{reference_elo + elo_bound:.0f} (n={n}, not statistically reliable)"
        )
    return "\n".join(lines)


def main() -> None:
    root = Path(__file__).resolve().parent
    pgn = root / "testing" / "results.pgn"
    if len(sys.argv) > 1:
        pgn = Path(sys.argv[1])
    if not pgn.is_file():
        print(f"ERROR: PGN not found: {pgn}", file=sys.stderr)
        raise SystemExit(1)

    stats = analyze_pgn(pgn)
    print(format_report(stats))
    filtered = pgn.with_name("results_valid.pgn")
    count = write_filtered_pgn(pgn, filtered)
    print(f"  Filtered PGN:  {filtered} ({count} games)")


if __name__ == "__main__":
    main()
