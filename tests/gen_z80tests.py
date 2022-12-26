#!/usr/bin/env python3

import textwrap

# http://z80.info/decoding.htm
#
# Bits in opcode (MSB → LSB)
# 7 6   -> x
# 5 4 3 -> y
# 5 4   -> p
# 3     -> q
# 2 1 0 -> z

from typing import NamedTuple, Callable, Union
from ctypes import c_ubyte

TestBits = NamedTuple('TestBits', [('template', str),
                                   ('subst', dict),
                                   ('prefix', bytes),
                                   ('mcbits', list[Union[c_ubyte, str]])])


def byte_to_hex(b: c_ubyte | int | str) -> str:
    if isinstance(b, str):
        return b
    return "0x%0.2X" % b


def templ_add_extra(tb: TestBits, text: str):
    if 'extra' in tb.subst:
        tb.subst['extra'] += text
    else:
        tb.subst['extra'] = text


class T:
    def __init__(self, value: TestBits):
        self.value = value

    def __rshift__(self, other):
        return other(self.value)


# Track duplicates, like tests with generated parameters and instructions unaffected by prefixes
all_tests: set[str] = set()


def done(tb: TestBits):

    if 'name' not in tb.subst:
        tb.subst['name'] = tb.subst['instr']

    if tb.subst['name'] in all_tests:
        return

    mc = []
    for b in tb.mcbits:
        if isinstance(b, str):
            mc.append(b)
        else:
            mc.append(byte_to_hex(b))
    tb.subst['mc'] = ', '.join(mc)

    # Remove unnecessary quotes
    tb.subst['asm'] = tb.subst['asm'].replace('""', '')

    # Remove trailing comma in ASM if any
    ss = '+ ","'
    if tb.subst['asm'].endswith(ss):
        tb.subst['asm'] = tb.subst['asm'][:-len(ss)]
    ss = ',"'
    if tb.subst['asm'].endswith(ss):
        tb.subst['asm'] = tb.subst['asm'][:-len(ss)] + '"'

    # Make sure we don't generate tests with the same name
    all_tests.add(tb.subst['name'])

    print(tb.template % tb.subst)


gen_common = """
        auto Asm = Assembler{};
    """


def i(name: str, instr_s: str) -> T:
    return T(TestBits(
        template="""
    TEST_CASE("%(name)s") {
        %(common)s%(extra)s
        REQUIRE (ASM(%(asm)s) == DB {%(mc)s });
    }
    """,
        subst={'name': name, 'asm': '" ' + instr_s + '"', 'common': gen_common, 'extra': ''},
        prefix=bytes(),
        mcbits=[0]
    ))


def i1(instr: str) -> T:
    return i(instr, instr)


def set_mcbits(tb: TestBits, x: int) -> T:
    tb.mcbits[0] |= x
    return T(tb)


def sx(x: int) -> Callable[[TestBits], T]:
    return lambda tb: set_mcbits(tb, x << 6)


def sy(y: int) -> Callable[[TestBits], T]:
    return lambda tb: set_mcbits(tb, y << 3)


def sz(z: int) -> Callable[[TestBits], T]:
    return lambda tb: set_mcbits(tb, z)


def sp(p: int) -> Callable[[TestBits], T]:
    return lambda tb: set_mcbits(tb, p << 4)


def sq(q: int) -> Callable[[TestBits], T]:
    return lambda tb: set_mcbits(tb, q << 3)


def wrap_uint8_t(asm: str) -> str:
    return '(uint8_t)(' + asm + ')'


def gen_cc(tb: TestBits) -> T:
    templ_add_extra(tb, """
        auto CC = GENERATE(range(0, 8));
    """)
    tb.subst['asm'] += '" " + TableCC[CC] + ","'
    tb.mcbits[0] = wrap_uint8_t(byte_to_hex(tb.mcbits[0]) + ' | (CC << 3)')
    return T(tb)


def gen_cc2(tb: TestBits) -> T:
    templ_add_extra(tb, """
        auto CC = GENERATE(range(0, 4));
    """)
    tb.subst['asm'] += '" " + TableCC[CC] + ","'
    tb.mcbits[0] = wrap_uint8_t(byte_to_hex(tb.mcbits[0]) + ' | (CC << 3)')
    return T(tb)


def gen_disp(tb: TestBits) -> T:
    templ_add_extra(tb, """
        auto D = GENERATE(range(-128, 128));
    """)
    tb.subst['asm'] += ' + dispToText(D)'
    tb.mcbits.append('(uint8_t)D')
    return T(tb)


def rp_p(tb: TestBits) -> T:
    templ_add_extra(tb, """
        auto RP = GENERATE(range(0, 4));
    """)
    tb.subst['asm'] += '" " + TableRP[RP] + ","'
    tb.mcbits[0] = wrap_uint8_t(byte_to_hex(tb.mcbits[0]) + ' | (RP << 4)')
    return T(tb)


def rp2_p(tb: TestBits) -> T:
    templ_add_extra(tb, """
        auto RP = GENERATE(range(0, 4));
    """)
    tb.subst['asm'] += '" " + TableRP2[RP] + ","'
    tb.mcbits[0] = wrap_uint8_t(byte_to_hex(tb.mcbits[0]) + ' | (RP << 4)')
    return T(tb)


def r_y(tb: TestBits) -> T:
    templ_add_extra(tb, """
        auto Y = GENERATE(range(0, 8));
    """)
    tb.subst['asm'] += '" " + TableR[Y] + ","'
    tb.mcbits[0] = wrap_uint8_t(byte_to_hex(tb.mcbits[0]) + ' | (Y << 3)')
    return T(tb)


def r_z(tb: TestBits) -> T:
    templ_add_extra(tb, """
        auto Z = GENERATE(range(0, 8));
    """)
    tb.subst['asm'] += '" " + TableR[Z] + ","'
    tb.mcbits[0] = wrap_uint8_t(byte_to_hex(tb.mcbits[0]) + ' | Z')
    return T(tb)


def r_z_yz_not6(tb: TestBits) -> T:
    """Required to account for the `ld (hl), (hl) -> halt` exception"""
    templ_add_extra(tb, """
        auto Z = Catch::Generators::generate( "custom capture generator", CATCH_INTERNAL_LINEINFO,
            [&Y]{
                using namespace Catch::Generators;
                return makeGenerators(filter([&](int i) { return !(i == 6 && Y == 6); }, range(0, 8)));
            }
        );
    """)
    tb.subst['asm'] += '" " + TableR[Z] + ","'
    tb.mcbits[0] = wrap_uint8_t(byte_to_hex(tb.mcbits[0]) + ' | Z')
    return T(tb)


def imm8(tb: TestBits) -> T:
    templ_add_extra(tb, """
        auto Imm = GENERATE(range(0, 0x100));
    """)
    tb.subst['asm'] += '" " + std::to_string(Imm) + ","'
    tb.mcbits.append(wrap_uint8_t('Imm'))
    return T(tb)


def imm8port(tb: TestBits) -> T:
    templ_add_extra(tb, """
        auto Imm = GENERATE(range(0, 0x100));
    """)
    tb.subst['asm'] += '" (" + std::to_string(Imm) + "),"'
    tb.mcbits.append(wrap_uint8_t('Imm'))
    return T(tb)


def imm16(tb: TestBits) -> T:
    templ_add_extra(tb, """
        auto Imm = GENERATE(take(1024, random(0, 0xffff)));
    """)
    tb.subst['asm'] += '" " + std::to_string(Imm) + ","'
    tb.mcbits.append(wrap_uint8_t('Imm & 0xff'))
    tb.mcbits.append(wrap_uint8_t('Imm >> 8'))
    return T(tb)


def immaddr(tb: TestBits) -> T:
    templ_add_extra(tb, """
        auto Addr = GENERATE(take(1024, random(0, 0xffff)));
    """)
    tb.subst['asm'] += '" (" + std::to_string(Addr) + "),"'
    tb.mcbits.append(wrap_uint8_t('Addr & 0xff'))
    tb.mcbits.append(wrap_uint8_t('Addr >> 8'))
    return T(tb)


def arg(text: str) -> Callable[[TestBits], T]:
    def set_arg(tb: TestBits, t: str) -> T:
        tb.subst['asm'] += '" ' + t + ',"'
        return T(tb)
    return lambda tb: set_arg(tb, text)


def alu(tb: TestBits) -> T:
    templ_add_extra(tb, """
        auto Y = GENERATE(range(0, 8));
    """)
    tb.subst['asm'] = '" " + TableALU[Y] + '
    tb.mcbits[0] = wrap_uint8_t(byte_to_hex(tb.mcbits[0]) + ' | (Y << 3)')
    return T(tb)


print(textwrap.dedent("""// Generated by gen_z80_tests.py

#include "z80testutil.h"

#define ASM Asm.assembleString
#define DB std::vector<uint8_t>

"""))

for x in range(4):
    match x:
        case 0:
            for z in range(8):
                match z:
                    case 0:
                        for y in range(8):
                            match y:
                                case 0:
                                    i1('NOP') >> sx(x) >> sy(y) >> sz(z) >> done
                                case 1:
                                    i1('EX AF, AF\'') >> sx(x) >> sy(y) >> sz(z) >> done
                                case 2:
                                    i('DJNZ', 'DJNZ') >> sx(x) >> sy(y) >> sz(z) >> gen_disp >> done
                                case 3:
                                    i('JR', 'JR') >> sx(x) >> sy(y) >> sz(z) >> gen_disp >> done
                                case _:  # 4..7
                                    i('JR cc', 'JR') >> sx(x) >> sy(y) >> sz(z) >> gen_cc2 >> gen_disp >> done
                    case 1:
                        for q in range(2):
                            match q:
                                case 0:
                                    i('LD rp, nn', 'LD') >> sx(x) >> sz(z) >> sq(q) >> rp_p >> imm16 >> done
                                case 1:
                                    i('ADD HL, rp', 'ADD HL,') >> sx(x) >> sz(z) >> sq(q) >> rp_p >> done
                    case 2:
                        for q in range(2):
                            match q:
                                case 0:
                                    for p in range(4):
                                        match p:
                                            case 0:
                                                i1('LD (BC), A') >> sx(x) >> sz(z) >> sp(p) >> sq(q) >> done
                                            case 1:
                                                i1('LD (DE), A') >> sx(x) >> sz(z) >> sp(p) >> sq(q) >> done
                                            case 2:
                                                i('LD (nn), HL', 'LD') >> sx(x) >> sz(z) >> sp(p) >> sq(q) >> \
                                                immaddr >> arg('HL') >> done
                                            case 3:
                                                i('LD (nn), A', 'LD') >> sx(x) >> sz(z) >> sp(p) >> sq(q) >> \
                                                immaddr >> arg('A') >> done
                                case 1:
                                    for p in range(4):
                                        match p:
                                            case 0:
                                                i1('LD A, (BC)') >> sx(x) >> sz(z) >> sp(p) >> sq(q) >> done
                                            case 1:
                                                i1('LD A, (DE)') >> sx(x) >> sz(z) >> sp(p) >> sq(q) >> done
                                            case 2:
                                                i('LD HL, (nn)', 'LD') >> sx(x) >> sz(z) >> sp(p) >> sq(q) >> \
                                                arg('HL') >> immaddr >> done
                                            case 3:
                                                i('LD A, (nn)', 'LD') >> sx(x) >> sz(z) >> sp(p) >> sq(q) >> \
                                                arg('A') >> immaddr >> done
                    case 3:
                        for q in range(2):
                            match q:
                                case 0:
                                    i('INC rp', 'INC') >> sx(x) >> sz(z) >> sq(q) >> rp_p >> done
                                case 1:
                                    i('DEC rp', 'DEC') >> sx(x) >> sz(z) >> sq(q) >> rp_p >> done
                    case 4:
                        i('INC r', 'INC') >> sx(x) >> sz(z) >> r_y >> done
                    case 5:
                        i('DEC r', 'DEC') >> sx(x) >> sz(z) >> r_y >> done
                    case 6:
                        i('LD r, n', 'LD') >> sx(x) >> sz(z) >> r_y >> imm8 >> done
                    case 7:
                        for y, j in enumerate(['RLCA', 'RRCA', 'RLA', 'RRA',
                                               'DAA', 'CPL', 'SCF', 'CCF']):
                            i1(j) >> sx(x) >> sy(y) >> sz(z) >> done
        case 1:
            i('LD r1, r2', 'LD') >> sx(x) >> r_y >> r_z_yz_not6 >> done
            i1('HALT') >> sx(x) >> sy(6) >> sz(6) >> done
        case 2:
            i('ALU r', '') >> sx(x) >> alu >> r_z >> done
        case 3:
            for z in range(8):
                match z:
                    case 0:
                        i('RET cc', 'RET') >> sx(x) >> sz(z) >> gen_cc >> done
                    case 1:
                        for q in range(2):
                            match q:
                                case 0:
                                    i('POP rp2', 'POP') >> sx(x) >> sz(z) >> sq(q) >> rp2_p >> done
                                case 1:
                                    for p, j in enumerate(['RET', 'EXX', 'JP HL', 'LD SP, HL']):
                                        i1(j) >> sx(x) >> sz(z) >> sq(q) >> sp(p) >> done
                    case 2:
                        i('JP cc, nn', 'JP') >> sx(x) >> sz(z) >> gen_cc >> imm16 >> done
                    case 3:
                        for y in range(8):
                            match y:
                                case 0:
                                    i('JP nn', 'JP') >> sx(x) >> sy(y) >> sz(z) >> imm16 >> done
                                case 1:
                                    # CB prefix
                                    pass
                                case 2:
                                    i('OUT (n), A', 'OUT') >> sx(x) >> sy(y) >> sz(z) >> imm8port >> arg('A') >> done
                                case 3:
                                    i('IN A, (n)', 'IN A, ') >> sx(x) >> sy(y) >> sz(z) >> imm8port >> done
                                case _:
                                    i1(['EX (SP), HL', 'EX DE, HL', 'DI', 'EI'][y - 4]) >> sx(x) >> sy(y) >> sz(z) >> done
                    case 4:
                        i('CALL cc, nn', 'CALL') >> sx(x) >> sz(z) >> gen_cc >> imm16 >> done
                    case 5:
                        for q in range(2):
                            match q:
                                case 0:
                                    i('PUSH rp2', 'PUSH') >> sx(x) >> sz(z) >> sq(q) >> rp2_p >> done
                                case 1:
                                    for p in range(4):
                                        match p:
                                            case 0:
                                                i('CALL nn', 'CALL') >> sx(x) >> sz(z) >> sq(q) >> sp(p) >> imm16 >> done
                                            case _:
                                                # DD / ED / FD prefixes
                                                pass
                    case 6:
                        i('ALU n', '') >> sx(x) >> sz(z) >> alu >> imm8 >> done
                    case 7:
                        for y in range(8):
                            i1('RST ' + byte_to_hex(y * 8)) >> sx(x) >> sy(y) >> sz(z) >> done
