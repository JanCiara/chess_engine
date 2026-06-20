#!/usr/bin/env python3
"""Generate precomputed magic bitboard attack tables for compile-time inclusion."""

from pathlib import Path


def get_bit(bb: int, sq: int) -> int:
    return (bb >> sq) & 1


def set_bit(bb: int, sq: int) -> int:
    return bb | (1 << sq)


def pop_bit(bb: int, sq: int) -> int:
    return bb & ~(1 << sq)


def count_bits(value: int) -> int:
    return value.bit_count()


def get_lsb(value: int) -> int:
    return (value & -value).bit_length() - 1


def add_ray_squares(attacks, tr, tf, dr, df, r_min, r_max, f_min, f_max, block, stop):
    r, f = tr + dr, tf + df
    while r_min <= r <= r_max and f_min <= f <= f_max:
        sq = r * 8 + f
        attacks = set_bit(attacks, sq)
        if stop and get_bit(block, sq):
            break
        r += dr
        f += df
    return attacks


def compute_slider(square, block, directions, stop_at_block):
    attacks = 0
    tr, tf = square // 8, square % 8
    for dr, df in directions:
        if stop_at_block:
            attacks = add_ray_squares(attacks, tr, tf, dr, df, 0, 7, 0, 7, block, True)
        else:
            r_min = 1 if dr < 0 else 0
            r_max = 6 if dr > 0 else 7
            f_min = 1 if df < 0 else 0
            f_max = 6 if df > 0 else 7
            attacks = add_ray_squares(attacks, tr, tf, dr, df, r_min, r_max, f_min, f_max, 0, False)
    return attacks


BISHOP_DIRS = [(1, 1), (-1, 1), (1, -1), (-1, -1)]
ROOK_DIRS = [(1, 0), (-1, 0), (0, 1), (0, -1)]


def mask_bishop(square):
    return compute_slider(square, 0, BISHOP_DIRS, False)


def mask_rook(square):
    return compute_slider(square, 0, ROOK_DIRS, False)


def bishop_on_the_fly(square, block):
    return compute_slider(square, block, BISHOP_DIRS, True)


def rook_on_the_fly(square, block):
    return compute_slider(square, block, ROOK_DIRS, True)


def magic_index(occupancy: int, magic: int, relevant_bits: int) -> int:
    product = (occupancy * magic) & ((1 << 64) - 1)
    return product >> (64 - relevant_bits)


def set_occupancy(index, bits_in_mask, attack_mask):
    occupancy = 0
    mask = attack_mask
    for count in range(bits_in_mask):
        square = get_lsb(mask)
        mask = pop_bit(mask, square)
        if index & (1 << count):
            occupancy = set_bit(occupancy, square)
    return occupancy


ROOK_MAGIC = [
    0x8A80104000800020, 0x140002000100040, 0x2801880A0017001, 0x100081001000420,
    0x200020010080420, 0x3001C0002010008, 0x8480008002000100, 0x2080088004402900,
    0x800098204000, 0x2024401000200040, 0x100802000801000, 0x120800800801000,
    0x208808088000400, 0x2802200800400, 0x2200800100020080, 0x801000060821100,
    0x80044006422000, 0x100808020004000, 0x12108A0010204200, 0x140848010000802,
    0x481828014002800, 0x8094004002004100, 0x4010040010010802, 0x20008806104,
    0x100400080208000, 0x2040002120081000, 0x21200680100081, 0x20100080080080,
    0x2000A00200410, 0x20080800400, 0x80088400100102, 0x80004600042881,
    0x4040008040800020, 0x440003000200801, 0x4200011004500, 0x188020010100100,
    0x14800401802800, 0x2080040080800200, 0x124080204001001, 0x200046502000484,
    0x480400080088020, 0x1000422010034000, 0x30200100110040, 0x100021010009,
    0x2002080100110004, 0x202008004008002, 0x20020004010100, 0x2048440040820001,
    0x101002200408200, 0x40802000401080, 0x4008142004410100, 0x2060820C0120200,
    0x1001004080100, 0x20C020080040080, 0x2935610830022400, 0x44440041009200,
    0x280001040802101, 0x2100190040002085, 0x80C0084100102001, 0x4024081001000421,
    0x20030A0244872, 0x12001008414402, 0x2006104900A0804, 0x1004081002402,
]

BISHOP_MAGIC = [
    0x40040844404084, 0x2004208A004208, 0x10190041080202, 0x108060845042010,
    0x581104180800210, 0x2112080446200010, 0x1080820820060210, 0x3C0808410220200,
    0x4050404440404, 0x21001420088, 0x24D0080801082102, 0x1020A0A020400,
    0x40308200402, 0x4011002100800, 0x401484104104005, 0x801010402020200,
    0x400210C3880100, 0x404022024108200, 0x810018200204102, 0x4002801A02003,
    0x85040820080400, 0x810102C808880400, 0xE900410884800, 0x8002020480840102,
    0x220200865090201, 0x2010100A02021202, 0x152048408022401, 0x20080002081110,
    0x4001001021004000, 0x800040400A011002, 0xE4004081011002, 0x1C004001012080,
    0x8004200962A00220, 0x8422100208500202, 0x2000402200300C08, 0x8646020080080080,
    0x80020A0200100808, 0x2010004880111000, 0x623000A080011400, 0x42008C0340209202,
    0x209188240001000, 0x400408A884001800, 0x110400A6080400, 0x1840060A44020800,
    0x90080104000041, 0x201011000808101, 0x1A2208080504F080, 0x8012020600211212,
    0x500861011240000, 0x180806108200800, 0x4000020E01040044, 0x300000261044000A,
    0x802241102020002, 0x20906061210001, 0x5A84841004010310, 0x4010801011C04,
    0xA010109502200, 0x4A02012000, 0x500201010098B028, 0x8040002811040900,
    0x28000010020204, 0x6000020202D0240, 0x8918844842082200, 0x4010011029020020,
]

ROOK_RELEVANT_BITS = [
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12,
]

BISHOP_RELEVANT_BITS = [
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6,
]


def init_slider(mask_fn, attacks_fn, magic_numbers, relevant_bits, max_entries):
    masks = [0] * 64
    attacks = [[0] * max_entries for _ in range(64)]

    for square in range(64):
        masks[square] = mask_fn(square)
        attack_mask = masks[square]
        relevant_bits_count = count_bits(attack_mask)
        occupancy_indices = 1 << relevant_bits_count

        for index in range(occupancy_indices):
            occupancy = set_occupancy(index, relevant_bits_count, attack_mask)
            index_value = magic_index(occupancy, magic_numbers[square], relevant_bits[square])
            attacks[square][index_value] = attacks_fn(square, occupancy)

    return masks, attacks


def fmt_u64(value: int) -> str:
    return f"0x{value:016X}ULL"


def write_array1d(name: str, data) -> str:
    lines = [f"inline constexpr std::array<U64, 64> {name} = {{"]
    for i in range(0, 64, 4):
        chunk = ", ".join(fmt_u64(value) for value in data[i : i + 4])
        lines.append(f"    {chunk},")
    lines.append("};")
    return "\n".join(lines)


def write_int_array1d(name: str, data) -> str:
    lines = [f"inline constexpr std::array<int, 64> {name} = {{"]
    for i in range(0, 64, 8):
        chunk = ", ".join(str(value) for value in data[i : i + 8])
        lines.append(f"    {chunk},")
    lines.append("};")
    return "\n".join(lines)


def write_cpp_array1d(name: str, data) -> str:
    lines = [f"const std::array<U64, 64> {name} = {{"]
    for i in range(0, 64, 4):
        chunk = ", ".join(fmt_u64(value) for value in data[i : i + 4])
        lines.append(f"    {chunk},")
    lines.append("};")
    return "\n".join(lines)


def write_cpp_int_array1d(name: str, data) -> str:
    lines = [f"const std::array<int, 64> {name} = {{"]
    for i in range(0, 64, 8):
        chunk = ", ".join(str(value) for value in data[i : i + 8])
        lines.append(f"    {chunk},")
    lines.append("};")
    return "\n".join(lines)


def write_cpp_flat_array(name: str, data, inner: int) -> str:
    flat = [value for row in data for value in row]
    lines = [f"const U64 {name}[{len(flat)}] = {{"]
    for i in range(0, len(flat), 4):
        chunk = ", ".join(fmt_u64(value) for value in flat[i : i + 4])
        lines.append(f"    {chunk},")
    lines.append("};")
    return "\n".join(lines)


def main() -> None:
    bishop_masks, bishop_attacks = init_slider(
        mask_bishop, bishop_on_the_fly, BISHOP_MAGIC, BISHOP_RELEVANT_BITS, 512
    )
    rook_masks, rook_attacks = init_slider(
        mask_rook, rook_on_the_fly, ROOK_MAGIC, ROOK_RELEVANT_BITS, 4096
    )

    header_path = Path(__file__).resolve().parent.parent / "attacks_data.h"
    source_path = Path(__file__).resolve().parent.parent / "attacks_data.cpp"

    header_parts = [
        "#pragma once",
        '#include "defs.h"',
        "",
        "#include <array>",
        "",
        "// Auto-generated by tools/generate_attacks.py — do not edit manually.",
        "",
        "extern const std::array<U64, 64> BISHOP_MASKS;",
        "extern const std::array<U64, 64> ROOK_MASKS;",
        "extern const std::array<U64, 64> BISHOP_MAGIC_NUMBERS;",
        "extern const std::array<U64, 64> ROOK_MAGIC_NUMBERS;",
        "extern const std::array<int, 64> BISHOP_RELEVANT_BITS;",
        "extern const std::array<int, 64> ROOK_RELEVANT_BITS;",
        "extern const U64 BISHOP_ATTACKS[32768];",
        "extern const U64 ROOK_ATTACKS[262144];",
        "",
        "inline U64 bishop_attack_entry(int square, int index) {",
        "    return BISHOP_ATTACKS[(square * 512) + index];",
        "}",
        "",
        "inline U64 rook_attack_entry(int square, int index) {",
        "    return ROOK_ATTACKS[(square * 4096) + index];",
        "}",
        "",
    ]

    source_parts = [
        '#include "attacks_data.h"',
        "",
        "// Auto-generated by tools/generate_attacks.py — do not edit manually.",
        "",
        write_cpp_array1d("BISHOP_MASKS", bishop_masks),
        "",
        write_cpp_array1d("ROOK_MASKS", rook_masks),
        "",
        write_cpp_array1d("BISHOP_MAGIC_NUMBERS", BISHOP_MAGIC),
        "",
        write_cpp_array1d("ROOK_MAGIC_NUMBERS", ROOK_MAGIC),
        "",
        write_cpp_int_array1d("BISHOP_RELEVANT_BITS", BISHOP_RELEVANT_BITS),
        "",
        write_cpp_int_array1d("ROOK_RELEVANT_BITS", ROOK_RELEVANT_BITS),
        "",
        write_cpp_flat_array("BISHOP_ATTACKS", bishop_attacks, 512),
        "",
        write_cpp_flat_array("ROOK_ATTACKS", rook_attacks, 4096),
        "",
    ]

    header_path.write_text("\n".join(header_parts), encoding="utf-8")
    source_path.write_text("\n".join(source_parts), encoding="utf-8")
    print(f"Wrote {header_path}")
    print(f"Wrote {source_path}")


if __name__ == "__main__":
    main()
