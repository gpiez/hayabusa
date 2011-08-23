/*
    hayabusa, chess engine
    Copyright (C) 2009-2010 Gunther Piez

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include <pch.h>

#include "boardbase.h"
#include "boardbase.tcc"

Castling BoardBase::castlingMask[nSquares];
const BoardBase::Bits BoardBase::bits[nSquares] = { // 1 KByte    1<<(sq^070)  : 1<<(sq^070)
{{ 0x00000000000000fe, 0x0101010101010100 }, { 0x8040201008040200, 0x0000000000000000 }, { 0x0000000000000001, 0x0000000000000001 }, { 0x0100000000000000, 0x0100000000000000 }, 0x0000000000020400, 0x0000000000030707, { 0x8000000000000000, 0x8000000000000000 }},  {{ 0x00000000000000fd, 0x0202020202020200 }, { 0x0080402010080400, 0x0000000000000100 }, { 0x0000000000000002, 0x0000000000000002 }, { 0x0200000000000000, 0x0200000000000000 }, 0x0000000000050800, 0x0000000000070f0f, { 0x4000000000000000, 0x4000000000000000 }},  {{ 0x00000000000000fb, 0x0404040404040400 }, { 0x0000804020100800, 0x0000000000010200 }, { 0x0000000000000004, 0x0000000000000004 }, { 0x0400000000000000, 0x0400000000000000 }, 0x00000000000a1100, 0x00000000000e1f1f, { 0x2000000000000000, 0x2000000000000000 }},  {{ 0x00000000000000f7, 0x0808080808080800 }, { 0x0000008040201000, 0x0000000001020400 }, { 0x0000000000000008, 0x0000000000000008 }, { 0x0800000000000000, 0x0800000000000000 }, 0x0000000000142200, 0x00000000001c3e3e, { 0x1000000000000000, 0x1000000000000000 }},  {{ 0x00000000000000ef, 0x1010101010101000 }, { 0x0000000080402000, 0x0000000102040800 }, { 0x0000000000000010, 0x0000000000000010 }, { 0x1000000000000000, 0x1000000000000000 }, 0x0000000000284400, 0x0000000000387c7c, { 0x0800000000000000, 0x0800000000000000 }},  {{ 0x00000000000000df, 0x2020202020202000 }, { 0x0000000000804000, 0x0000010204081000 }, { 0x0000000000000020, 0x0000000000000020 }, { 0x2000000000000000, 0x2000000000000000 }, 0x0000000000508800, 0x000000000070f8f8, { 0x0400000000000000, 0x0400000000000000 }},  {{ 0x00000000000000bf, 0x4040404040404000 }, { 0x0000000000008000, 0x0001020408102000 }, { 0x0000000000000040, 0x0000000000000040 }, { 0x4000000000000000, 0x4000000000000000 }, 0x0000000000a01000, 0x0000000000e0f0f0, { 0x0200000000000000, 0x0200000000000000 }},  {{ 0x000000000000007f, 0x8080808080808000 }, { 0x0000000000000000, 0x0102040810204000 }, { 0x0000000000000080, 0x0000000000000080 }, { 0x8000000000000000, 0x8000000000000000 }, 0x0000000000402000, 0x0000000000c0e0e0, { 0x0100000000000000, 0x0100000000000000 }},
{{ 0x000000000000fe00, 0x0101010101010001 }, { 0x4020100804020000, 0x0000000000000002 }, { 0x0000000000000100, 0x0000000000000100 }, { 0x0001000000000000, 0x0001000000000000 }, 0x0000000002040004, 0x0000000003070707, { 0x0080000000000000, 0x0080000000000000 }},  {{ 0x000000000000fd00, 0x0202020202020002 }, { 0x8040201008040001, 0x0000000000010004 }, { 0x0000000000000200, 0x0000000000000200 }, { 0x0002000000000000, 0x0002000000000000 }, 0x0000000005080008, 0x00000000070f0f0f, { 0x0040000000000000, 0x0040000000000000 }},  {{ 0x000000000000fb00, 0x0404040404040004 }, { 0x0080402010080002, 0x0000000001020008 }, { 0x0000000000000400, 0x0000000000000400 }, { 0x0004000000000000, 0x0004000000000000 }, 0x000000000a110011, 0x000000000e1f1f1f, { 0x0020000000000000, 0x0020000000000000 }},  {{ 0x000000000000f700, 0x0808080808080008 }, { 0x0000804020100004, 0x0000000102040010 }, { 0x0000000000000800, 0x0000000000000800 }, { 0x0008000000000000, 0x0008000000000000 }, 0x0000000014220022, 0x000000001c3e3e3e, { 0x0010000000000000, 0x0010000000000000 }},  {{ 0x000000000000ef00, 0x1010101010100010 }, { 0x0000008040200008, 0x0000010204080020 }, { 0x0000000000001000, 0x0000000000001000 }, { 0x0010000000000000, 0x0010000000000000 }, 0x0000000028440044, 0x00000000387c7c7c, { 0x0008000000000000, 0x0008000000000000 }},  {{ 0x000000000000df00, 0x2020202020200020 }, { 0x0000000080400010, 0x0001020408100040 }, { 0x0000000000002000, 0x0000000000002000 }, { 0x0020000000000000, 0x0020000000000000 }, 0x0000000050880088, 0x0000000070f8f8f8, { 0x0004000000000000, 0x0004000000000000 }},  {{ 0x000000000000bf00, 0x4040404040400040 }, { 0x0000000000800020, 0x0102040810200080 }, { 0x0000000000004000, 0x0000000000004000 }, { 0x0040000000000000, 0x0040000000000000 }, 0x00000000a0100010, 0x00000000e0f0f0f0, { 0x0002000000000000, 0x0002000000000000 }},  {{ 0x0000000000007f00, 0x8080808080800080 }, { 0x0000000000000040, 0x0204081020400000 }, { 0x0000000000008000, 0x0000000000008000 }, { 0x0080000000000000, 0x0080000000000000 }, 0x0000000040200020, 0x00000000c0e0e0e0, { 0x0001000000000000, 0x0001000000000000 }},
{{ 0x0000000000fe0000, 0x0101010101000101 }, { 0x2010080402000000, 0x0000000000000204 }, { 0x0000000000010000, 0x0000000000010000 }, { 0x0000010000000000, 0x0000010000000000 }, 0x0000000204000402, 0x0000000307070703, { 0x0000800000000000, 0x0000800000000000 }},  {{ 0x0000000000fd0000, 0x0202020202000202 }, { 0x4020100804000100, 0x0000000001000408 }, { 0x0000000000020000, 0x0000000000020000 }, { 0x0000020000000000, 0x0000020000000000 }, 0x0000000508000805, 0x000000070f0f0f07, { 0x0000400000000000, 0x0000400000000000 }},  {{ 0x0000000000fb0000, 0x0404040404000404 }, { 0x8040201008000201, 0x0000000102000810 }, { 0x0000000000040000, 0x0000000000040000 }, { 0x0000040000000000, 0x0000040000000000 }, 0x0000000a1100110a, 0x0000000e1f1f1f0e, { 0x0000200000000000, 0x0000200000000000 }},  {{ 0x0000000000f70000, 0x0808080808000808 }, { 0x0080402010000402, 0x0000010204001020 }, { 0x0000000000080000, 0x0000000000080000 }, { 0x0000080000000000, 0x0000080000000000 }, 0x0000001422002214, 0x0000001c3e3e3e1c, { 0x0000100000000000, 0x0000100000000000 }},  {{ 0x0000000000ef0000, 0x1010101010001010 }, { 0x0000804020000804, 0x0001020408002040 }, { 0x0000000000100000, 0x0000000000100000 }, { 0x0000100000000000, 0x0000100000000000 }, 0x0000002844004428, 0x000000387c7c7c38, { 0x0000080000000000, 0x0000080000000000 }},  {{ 0x0000000000df0000, 0x2020202020002020 }, { 0x0000008040001008, 0x0102040810004080 }, { 0x0000000000200000, 0x0000000000200000 }, { 0x0000200000000000, 0x0000200000000000 }, 0x0000005088008850, 0x00000070f8f8f870, { 0x0000040000000000, 0x0000040000000000 }},  {{ 0x0000000000bf0000, 0x4040404040004040 }, { 0x0000000080002010, 0x0204081020008000 }, { 0x0000000000400000, 0x0000000000400000 }, { 0x0000400000000000, 0x0000400000000000 }, 0x000000a0100010a0, 0x000000e0f0f0f0e0, { 0x0000020000000000, 0x0000020000000000 }},  {{ 0x00000000007f0000, 0x8080808080008080 }, { 0x0000000000004020, 0x0408102040000000 }, { 0x0000000000800000, 0x0000000000800000 }, { 0x0000800000000000, 0x0000800000000000 }, 0x0000004020002040, 0x000000c0e0e0e0c0, { 0x0000010000000000, 0x0000010000000000 }},
{{ 0x00000000fe000000, 0x0101010100010101 }, { 0x1008040200000000, 0x0000000000020408 }, { 0x0000000001000000, 0x0000000001000000 }, { 0x0000000100000000, 0x0000000100000000 }, 0x0000020400040200, 0x0000030707070300, { 0x0000008000000000, 0x0000008000000000 }},  {{ 0x00000000fd000000, 0x0202020200020202 }, { 0x2010080400010000, 0x0000000100040810 }, { 0x0000000002000000, 0x0000000002000000 }, { 0x0000000200000000, 0x0000000200000000 }, 0x0000050800080500, 0x0000070f0f0f0700, { 0x0000004000000000, 0x0000004000000000 }},  {{ 0x00000000fb000000, 0x0404040400040404 }, { 0x4020100800020100, 0x0000010200081020 }, { 0x0000000004000000, 0x0000000004000000 }, { 0x0000000400000000, 0x0000000400000000 }, 0x00000a1100110a00, 0x00000e1f1f1f0e00, { 0x0000002000000000, 0x0000002000000000 }},  {{ 0x00000000f7000000, 0x0808080800080808 }, { 0x8040201000040201, 0x0001020400102040 }, { 0x0000000008000000, 0x0000000008000000 }, { 0x0000000800000000, 0x0000000800000000 }, 0x0000142200221400, 0x00001c3e3e3e1c00, { 0x0000001000000000, 0x0000001000000000 }},  {{ 0x00000000ef000000, 0x1010101000101010 }, { 0x0080402000080402, 0x0102040800204080 }, { 0x0000000010000000, 0x0000000010000000 }, { 0x0000001000000000, 0x0000001000000000 }, 0x0000284400442800, 0x0000387c7c7c3800, { 0x0000000800000000, 0x0000000800000000 }},  {{ 0x00000000df000000, 0x2020202000202020 }, { 0x0000804000100804, 0x0204081000408000 }, { 0x0000000020000000, 0x0000000020000000 }, { 0x0000002000000000, 0x0000002000000000 }, 0x0000508800885000, 0x000070f8f8f87000, { 0x0000000400000000, 0x0000000400000000 }},  {{ 0x00000000bf000000, 0x4040404000404040 }, { 0x0000008000201008, 0x0408102000800000 }, { 0x0000000040000000, 0x0000000040000000 }, { 0x0000004000000000, 0x0000004000000000 }, 0x0000a0100010a000, 0x0000e0f0f0f0e000, { 0x0000000200000000, 0x0000000200000000 }},  {{ 0x000000007f000000, 0x8080808000808080 }, { 0x0000000000402010, 0x0810204000000000 }, { 0x0000000080000000, 0x0000000080000000 }, { 0x0000008000000000, 0x0000008000000000 }, 0x0000402000204000, 0x0000c0e0e0e0c000, { 0x0000000100000000, 0x0000000100000000 }},
{{ 0x000000fe00000000, 0x0101010001010101 }, { 0x0804020000000000, 0x0000000002040810 }, { 0x0000000100000000, 0x0000000100000000 }, { 0x0000000001000000, 0x0000000001000000 }, 0x0002040004020000, 0x0003070707030000, { 0x0000000080000000, 0x0000000080000000 }},  {{ 0x000000fd00000000, 0x0202020002020202 }, { 0x1008040001000000, 0x0000010004081020 }, { 0x0000000200000000, 0x0000000200000000 }, { 0x0000000002000000, 0x0000000002000000 }, 0x0005080008050000, 0x00070f0f0f070000, { 0x0000000040000000, 0x0000000040000000 }},  {{ 0x000000fb00000000, 0x0404040004040404 }, { 0x2010080002010000, 0x0001020008102040 }, { 0x0000000400000000, 0x0000000400000000 }, { 0x0000000004000000, 0x0000000004000000 }, 0x000a1100110a0000, 0x000e1f1f1f0e0000, { 0x0000000020000000, 0x0000000020000000 }},  {{ 0x000000f700000000, 0x0808080008080808 }, { 0x4020100004020100, 0x0102040010204080 }, { 0x0000000800000000, 0x0000000800000000 }, { 0x0000000008000000, 0x0000000008000000 }, 0x0014220022140000, 0x001c3e3e3e1c0000, { 0x0000000010000000, 0x0000000010000000 }},  {{ 0x000000ef00000000, 0x1010100010101010 }, { 0x8040200008040201, 0x0204080020408000 }, { 0x0000001000000000, 0x0000001000000000 }, { 0x0000000010000000, 0x0000000010000000 }, 0x0028440044280000, 0x00387c7c7c380000, { 0x0000000008000000, 0x0000000008000000 }},  {{ 0x000000df00000000, 0x2020200020202020 }, { 0x0080400010080402, 0x0408100040800000 }, { 0x0000002000000000, 0x0000002000000000 }, { 0x0000000020000000, 0x0000000020000000 }, 0x0050880088500000, 0x0070f8f8f8700000, { 0x0000000004000000, 0x0000000004000000 }},  {{ 0x000000bf00000000, 0x4040400040404040 }, { 0x0000800020100804, 0x0810200080000000 }, { 0x0000004000000000, 0x0000004000000000 }, { 0x0000000040000000, 0x0000000040000000 }, 0x00a0100010a00000, 0x00e0f0f0f0e00000, { 0x0000000002000000, 0x0000000002000000 }},  {{ 0x0000007f00000000, 0x8080800080808080 }, { 0x0000000040201008, 0x1020400000000000 }, { 0x0000008000000000, 0x0000008000000000 }, { 0x0000000080000000, 0x0000000080000000 }, 0x0040200020400000, 0x00c0e0e0e0c00000, { 0x0000000001000000, 0x0000000001000000 }},
{{ 0x0000fe0000000000, 0x0101000101010101 }, { 0x0402000000000000, 0x0000000204081020 }, { 0x0000010000000000, 0x0000010000000000 }, { 0x0000000000010000, 0x0000000000010000 }, 0x0204000402000000, 0x0307070703000000, { 0x0000000000800000, 0x0000000000800000 }},  {{ 0x0000fd0000000000, 0x0202000202020202 }, { 0x0804000100000000, 0x0001000408102040 }, { 0x0000020000000000, 0x0000020000000000 }, { 0x0000000000020000, 0x0000000000020000 }, 0x0508000805000000, 0x070f0f0f07000000, { 0x0000000000400000, 0x0000000000400000 }},  {{ 0x0000fb0000000000, 0x0404000404040404 }, { 0x1008000201000000, 0x0102000810204080 }, { 0x0000040000000000, 0x0000040000000000 }, { 0x0000000000040000, 0x0000000000040000 }, 0x0a1100110a000000, 0x0e1f1f1f0e000000, { 0x0000000000200000, 0x0000000000200000 }},  {{ 0x0000f70000000000, 0x0808000808080808 }, { 0x2010000402010000, 0x0204001020408000 }, { 0x0000080000000000, 0x0000080000000000 }, { 0x0000000000080000, 0x0000000000080000 }, 0x1422002214000000, 0x1c3e3e3e1c000000, { 0x0000000000100000, 0x0000000000100000 }},  {{ 0x0000ef0000000000, 0x1010001010101010 }, { 0x4020000804020100, 0x0408002040800000 }, { 0x0000100000000000, 0x0000100000000000 }, { 0x0000000000100000, 0x0000000000100000 }, 0x2844004428000000, 0x387c7c7c38000000, { 0x0000000000080000, 0x0000000000080000 }},  {{ 0x0000df0000000000, 0x2020002020202020 }, { 0x8040001008040201, 0x0810004080000000 }, { 0x0000200000000000, 0x0000200000000000 }, { 0x0000000000200000, 0x0000000000200000 }, 0x5088008850000000, 0x70f8f8f870000000, { 0x0000000000040000, 0x0000000000040000 }},  {{ 0x0000bf0000000000, 0x4040004040404040 }, { 0x0080002010080402, 0x1020008000000000 }, { 0x0000400000000000, 0x0000400000000000 }, { 0x0000000000400000, 0x0000000000400000 }, 0xa0100010a0000000, 0xe0f0f0f0e0000000, { 0x0000000000020000, 0x0000000000020000 }},  {{ 0x00007f0000000000, 0x8080008080808080 }, { 0x0000004020100804, 0x2040000000000000 }, { 0x0000800000000000, 0x0000800000000000 }, { 0x0000000000800000, 0x0000000000800000 }, 0x4020002040000000, 0xc0e0e0e0c0000000, { 0x0000000000010000, 0x0000000000010000 }},
{{ 0x00fe000000000000, 0x0100010101010101 }, { 0x0200000000000000, 0x0000020408102040 }, { 0x0001000000000000, 0x0001000000000000 }, { 0x0000000000000100, 0x0000000000000100 }, 0x0400040200000000, 0x0707070300000000, { 0x0000000000008000, 0x0000000000008000 }},  {{ 0x00fd000000000000, 0x0200020202020202 }, { 0x0400010000000000, 0x0100040810204080 }, { 0x0002000000000000, 0x0002000000000000 }, { 0x0000000000000200, 0x0000000000000200 }, 0x0800080500000000, 0x0f0f0f0700000000, { 0x0000000000004000, 0x0000000000004000 }},  {{ 0x00fb000000000000, 0x0400040404040404 }, { 0x0800020100000000, 0x0200081020408000 }, { 0x0004000000000000, 0x0004000000000000 }, { 0x0000000000000400, 0x0000000000000400 }, 0x1100110a00000000, 0x1f1f1f0e00000000, { 0x0000000000002000, 0x0000000000002000 }},  {{ 0x00f7000000000000, 0x0800080808080808 }, { 0x1000040201000000, 0x0400102040800000 }, { 0x0008000000000000, 0x0008000000000000 }, { 0x0000000000000800, 0x0000000000000800 }, 0x2200221400000000, 0x3e3e3e1c00000000, { 0x0000000000001000, 0x0000000000001000 }},  {{ 0x00ef000000000000, 0x1000101010101010 }, { 0x2000080402010000, 0x0800204080000000 }, { 0x0010000000000000, 0x0010000000000000 }, { 0x0000000000001000, 0x0000000000001000 }, 0x4400442800000000, 0x7c7c7c3800000000, { 0x0000000000000800, 0x0000000000000800 }},  {{ 0x00df000000000000, 0x2000202020202020 }, { 0x4000100804020100, 0x1000408000000000 }, { 0x0020000000000000, 0x0020000000000000 }, { 0x0000000000002000, 0x0000000000002000 }, 0x8800885000000000, 0xf8f8f87000000000, { 0x0000000000000400, 0x0000000000000400 }},  {{ 0x00bf000000000000, 0x4000404040404040 }, { 0x8000201008040201, 0x2000800000000000 }, { 0x0040000000000000, 0x0040000000000000 }, { 0x0000000000004000, 0x0000000000004000 }, 0x100010a000000000, 0xf0f0f0e000000000, { 0x0000000000000200, 0x0000000000000200 }},  {{ 0x007f000000000000, 0x8000808080808080 }, { 0x0000402010080402, 0x4000000000000000 }, { 0x0080000000000000, 0x0080000000000000 }, { 0x0000000000008000, 0x0000000000008000 }, 0x2000204000000000, 0xe0e0e0c000000000, { 0x0000000000000100, 0x0000000000000100 }},
{{ 0xfe00000000000000, 0x0001010101010101 }, { 0x0000000000000000, 0x0002040810204080 }, { 0x0100000000000000, 0x0100000000000000 }, { 0x0000000000000001, 0x0000000000000001 }, 0x0004020000000000, 0x0707030000000000, { 0x0000000000000080, 0x0000000000000080 }},  {{ 0xfd00000000000000, 0x0002020202020202 }, { 0x0001000000000000, 0x0004081020408000 }, { 0x0200000000000000, 0x0200000000000000 }, { 0x0000000000000002, 0x0000000000000002 }, 0x0008050000000000, 0x0f0f070000000000, { 0x0000000000000040, 0x0000000000000040 }},  {{ 0xfb00000000000000, 0x0004040404040404 }, { 0x0002010000000000, 0x0008102040800000 }, { 0x0400000000000000, 0x0400000000000000 }, { 0x0000000000000004, 0x0000000000000004 }, 0x00110a0000000000, 0x1f1f0e0000000000, { 0x0000000000000020, 0x0000000000000020 }},  {{ 0xf700000000000000, 0x0008080808080808 }, { 0x0004020100000000, 0x0010204080000000 }, { 0x0800000000000000, 0x0800000000000000 }, { 0x0000000000000008, 0x0000000000000008 }, 0x0022140000000000, 0x3e3e1c0000000000, { 0x0000000000000010, 0x0000000000000010 }},  {{ 0xef00000000000000, 0x0010101010101010 }, { 0x0008040201000000, 0x0020408000000000 }, { 0x1000000000000000, 0x1000000000000000 }, { 0x0000000000000010, 0x0000000000000010 }, 0x0044280000000000, 0x7c7c380000000000, { 0x0000000000000008, 0x0000000000000008 }},  {{ 0xdf00000000000000, 0x0020202020202020 }, { 0x0010080402010000, 0x0040800000000000 }, { 0x2000000000000000, 0x2000000000000000 }, { 0x0000000000000020, 0x0000000000000020 }, 0x0088500000000000, 0xf8f8700000000000, { 0x0000000000000004, 0x0000000000000004 }},  {{ 0xbf00000000000000, 0x0040404040404040 }, { 0x0020100804020100, 0x0080000000000000 }, { 0x4000000000000000, 0x4000000000000000 }, { 0x0000000000000040, 0x0000000000000040 }, 0x0010a00000000000, 0xf0f0e00000000000, { 0x0000000000000002, 0x0000000000000002 }},  {{ 0x7f00000000000000, 0x0080808080808080 }, { 0x0040201008040201, 0x0000000000000000 }, { 0x8000000000000000, 0x8000000000000000 }, { 0x0000000000000080, 0x0000000000000080 }, 0x0020400000000000, 0xe0e0c00000000000, { 0x0000000000000001, 0x0000000000000001 }}
};
const uint64_t BoardBase::knightAttacks[nSquares+2] = { // 0,5 kByte
    0x0000000000020400, 0x0000000000050800, 0x00000000000a1100, 0x0000000000142200, 0x0000000000284400, 0x0000000000508800, 0x0000000000a01000, 0x0000000000402000,
    0x0000000002040004, 0x0000000005080008, 0x000000000a110011, 0x0000000014220022, 0x0000000028440044, 0x0000000050880088, 0x00000000a0100010, 0x0000000040200020,
    0x0000000204000402, 0x0000000508000805, 0x0000000a1100110a, 0x0000001422002214, 0x0000002844004428, 0x0000005088008850, 0x000000a0100010a0, 0x0000004020002040,
    0x0000020400040200, 0x0000050800080500, 0x00000a1100110a00, 0x0000142200221400, 0x0000284400442800, 0x0000508800885000, 0x0000a0100010a000, 0x0000402000204000,
    0x0002040004020000, 0x0005080008050000, 0x000a1100110a0000, 0x0014220022140000, 0x0028440044280000, 0x0050880088500000, 0x00a0100010a00000, 0x0040200020400000,
    0x0204000402000000, 0x0508000805000000, 0x0a1100110a000000, 0x1422002214000000, 0x2844004428000000, 0x5088008850000000, 0xa0100010a0000000, 0x4020002040000000,
    0x0400040200000000, 0x0800080500000000, 0x1100110a00000000, 0x2200221400000000, 0x4400442800000000, 0x8800885000000000, 0x100010a000000000, 0x2000204000000000,
    0x0004020000000000, 0x0008050000000000, 0x00110a0000000000, 0x0022140000000000, 0x0044280000000000, 0x0088500000000000, 0x0010a00000000000, 0x0020400000000000
};
const uint64_t BoardBase::kAttacked1[nSquares+2] = {
0x0000000000000303, 0x0000000000000707, 0x0000000000000e0e, 0x0000000000001c1c, 0x0000000000003838, 0x0000000000007070, 0x000000000000e0e0, 0x000000000000c0c0,
0x0000000000030303, 0x0000000000070707, 0x00000000000e0e0e, 0x00000000001c1c1c, 0x0000000000383838, 0x0000000000707070, 0x0000000000e0e0e0, 0x0000000000c0c0c0,
0x0000000003030300, 0x0000000007070700, 0x000000000e0e0e00, 0x000000001c1c1c00, 0x0000000038383800, 0x0000000070707000, 0x00000000e0e0e000, 0x00000000c0c0c000,
0x0000000303030000, 0x0000000707070000, 0x0000000e0e0e0000, 0x0000001c1c1c0000, 0x0000003838380000, 0x0000007070700000, 0x000000e0e0e00000, 0x000000c0c0c00000,
0x0000030303000000, 0x0000070707000000, 0x00000e0e0e000000, 0x00001c1c1c000000, 0x0000383838000000, 0x0000707070000000, 0x0000e0e0e0000000, 0x0000c0c0c0000000,
0x0003030300000000, 0x0007070700000000, 0x000e0e0e00000000, 0x001c1c1c00000000, 0x0038383800000000, 0x0070707000000000, 0x00e0e0e000000000, 0x00c0c0c000000000,
0x0303030000000000, 0x0707070000000000, 0x0e0e0e0000000000, 0x1c1c1c0000000000, 0x3838380000000000, 0x7070700000000000, 0xe0e0e00000000000, 0xc0c0c00000000000,
0x0303000000000000, 0x0707000000000000, 0x0e0e000000000000, 0x1c1c000000000000, 0x3838000000000000, 0x7070000000000000, 0xe0e0000000000000, 0xc0c0000000000000
};
const uint64_t BoardBase::kAttacked2[nSquares+2] = {
0x0000000000070404, 0x00000000000f0808, 0x00000000001f1111, 0x00000000003e2222, 0x00000000007c4444, 0x0000000000f88888, 0x0000000000f01010, 0x0000000000e02020,
0x0000000007040404, 0x000000000f080808, 0x000000001f111111, 0x000000003e222222, 0x000000007c444444, 0x00000000f8888888, 0x00000000f0101010, 0x00000000e0202020,
0x0000000704040407, 0x0000000f0808080f, 0x0000001f1111111f, 0x0000003e2222223e, 0x0000007c4444447c, 0x000000f8888888f8, 0x000000f0101010f0, 0x000000e0202020e0,
0x0000070404040700, 0x00000f0808080f00, 0x00001f1111111f00, 0x00003e2222223e00, 0x00007c4444447c00, 0x0000f8888888f800, 0x0000f0101010f000, 0x0000e0202020e000,
0x0007040404070000, 0x000f0808080f0000, 0x001f1111111f0000, 0x003e2222223e0000, 0x007c4444447c0000, 0x00f8888888f80000, 0x00f0101010f00000, 0x00e0202020e00000,
0x0704040407000000, 0x0f0808080f000000, 0x1f1111111f000000, 0x3e2222223e000000, 0x7c4444447c000000, 0xf8888888f8000000, 0xf0101010f0000000, 0xe0202020e0000000,
0x0404040700000000, 0x0808080f00000000, 0x1111111f00000000, 0x2222223e00000000, 0x4444447c00000000, 0x888888f800000000, 0x101010f000000000, 0x202020e000000000,
0x0404070000000000, 0x08080f0000000000, 0x11111f0000000000, 0x22223e0000000000, 0x44447c0000000000, 0x8888f80000000000, 0x1010f00000000000, 0x2020e00000000000
};
const uint64_t BoardBase::kAttacked[nSquares+2] = {
0x0000000000030707, 0x0000000000070f0f, 0x00000000000e1f1f, 0x00000000001c3e3e, 0x0000000000387c7c, 0x000000000070f8f8, 0x0000000000e0f0f0, 0x0000000000c0e0e0,
0x0000000003070707, 0x00000000070f0f0f, 0x000000000e1f1f1f, 0x000000001c3e3e3e, 0x00000000387c7c7c, 0x0000000070f8f8f8, 0x00000000e0f0f0f0, 0x00000000c0e0e0e0,
0x0000000307070703, 0x000000070f0f0f07, 0x0000000e1f1f1f0e, 0x0000001c3e3e3e1c, 0x000000387c7c7c38, 0x00000070f8f8f870, 0x000000e0f0f0f0e0, 0x000000c0e0e0e0c0,
0x0000030707070300, 0x0000070f0f0f0700, 0x00000e1f1f1f0e00, 0x00001c3e3e3e1c00, 0x0000387c7c7c3800, 0x000070f8f8f87000, 0x0000e0f0f0f0e000, 0x0000c0e0e0e0c000,
0x0003070707030000, 0x00070f0f0f070000, 0x000e1f1f1f0e0000, 0x001c3e3e3e1c0000, 0x00387c7c7c380000, 0x0070f8f8f8700000, 0x00e0f0f0f0e00000, 0x00c0e0e0e0c00000,
0x0307070703000000, 0x070f0f0f07000000, 0x0e1f1f1f0e000000, 0x1c3e3e3e1c000000, 0x387c7c7c38000000, 0x70f8f8f870000000, 0xe0f0f0f0e0000000, 0xc0e0e0e0c0000000,
0x0707070300000000, 0x0f0f0f0700000000, 0x1f1f1f0e00000000, 0x3e3e3e1c00000000, 0x7c7c7c3800000000, 0xf8f8f87000000000, 0xf0f0f0e000000000, 0xe0e0e0c000000000,
0x0707030000000000, 0x0f0f070000000000, 0x1f1f0e0000000000, 0x3e3e1c0000000000, 0x7c7c380000000000, 0xf8f8700000000000, 0xf0f0e00000000000, 0xe0e0c00000000000
};
uint64_t BoardBase::kingAttacks[16][nSquares];
//Length masks[nLengths][nSquares][nDirs/2][nSquares];

void BoardBase::buildAttacks() {
    buildAttacks<White>();
    buildAttacks<Black>();
    buildPins<White>();
    buildPins<Black>();
}

void BoardBase::initTables() {
    // if a piece is moved from or to a position in this table, the castling status
    // is disabled if the appropriate rook or king squares match.
    for (unsigned int sq=0; sq<nSquares; ++sq) {
        castlingMask[sq].data4 = ~0;
        if (sq == a1 || sq == e1)
            castlingMask[sq].color[0].q = 0;
        if (sq == h1 || sq == e1)
            castlingMask[sq].color[0].k = 0;
        if (sq == a8 || sq == e8)
            castlingMask[sq].color[1].q = 0;
        if (sq == h8 || sq == e8)
            castlingMask[sq].color[1].k = 0;
    }

    for (int y = 0; y < (signed)nRows; ++y)
    for (int x = 0; x < (signed)nFiles; ++x) {
        uint64_t p=0;
        for (unsigned mask=0; mask<16; ++mask) {
            if (popcount(mask) > 2) continue;
            p=0;
            for (unsigned int dir=0; dir<8; ++dir)
            if (~mask & (1ULL<<(int[]){0,2,1,3}[(dir&3)])) {
                int x0 = x+xOffsets[dir]; int y0=y+yOffsets[dir];
                if ( x0>=0 && x0<=7 && y0>=0 && y0<=7 )
                    p |= 1ULL << (x0+y0*nRows);
            }
            kingAttacks[mask][x+y*nRows] = p;
        }
    }

    for (unsigned int right = 0; right < 8; ++right)
    for (unsigned int left = 0; left < 8-right; ++left) {
//        unsigned int lr = left*8+right;
        for (unsigned int dir = 0; dir < nDirs/2; ++dir)
        for (unsigned int y = 0; y < nRows; ++y)
        for (unsigned int x = 0; x < nFiles; ++x) {
            int xt = x + xOffsets[dir];
            int yt = y + yOffsets[dir];
            for (unsigned int len=1; xt >= 0 && xt <= 7 && yt >= 0 && yt <= 7 && len<=right; len++) {
//                masks[lr][8 * y + x][dir][yt*8 + xt].left = left;
                xt += xOffsets[dir];
                yt += yOffsets[dir];
            }
            xt = x - xOffsets[dir];
            yt = y - yOffsets[dir];
            for (unsigned int len=1; xt >= 0 && xt <= 7 && yt >= 0 && yt <= 7 && len<=left; len++) {
//                masks[lr][8 * y + x][dir][yt*8 + xt].right = right;
                xt -= xOffsets[dir];
                yt -= yOffsets[dir];
            }
        }
    }
}

// Clear board. Do not use a default constructor for clearing,
// because the contents of the cleared board are immeditly overwritten
// in most cases.
void BoardBase::init() {
    *this = (BoardBase){{0}};
    keyScore.pawnKey = 0x12345678;
}

void BoardBase::print() const {
    static const char chessPieces[nTotalPieces] =
//        { L'♚', L'♟', L'♞', L'♛', L'♝', L'♜', ' ', L'♖', L'♗', L'♕', L'♘', L'♙', L'♔' };
        { 'k', 'p', 'n', 'q', 'b', 'r', ' ', 'R', 'B', 'Q', 'N', 'P', 'K' };
//    std::cout.setCodec("UTF-8");
    std::cout << "--------------------------------" << std::endl;
    for (unsigned int y = 0; y < nRows; ++y) {
        for (unsigned int x = 0; x < nFiles; ++x) {
            std::cout << "| " << chessPieces[6 + getPieceFromBit(1ULL << ((7-y)*8+x))] << ' ';
        }
        std::cout << std::endl << "--------------------------------" << std::endl;
    }
}

unsigned BoardBase::getPieceFromBit(uint64_t bit) const {
    ASSERT( !((getPieces<White,King>() | getPieces<Black,King>()) & bit) );
#ifdef __SSE4_1__
    __v2di piece = _mm_set_epi64x(bit, bit);
    const __v2di zero = _mm_set_epi64x(0, 0);
    __v2di r = ~_mm_cmpeq_epi64( _mm_and_si128( get2Pieces<Rook  >(), piece), zero) & _mm_set_epi64x(Rook, Rook);
    __v2di b = ~_mm_cmpeq_epi64( _mm_and_si128( get2Pieces<Bishop>(), piece), zero) & _mm_set_epi64x(Bishop, Bishop);
    __v2di q = ~_mm_cmpeq_epi64( _mm_and_si128( get2Pieces<Queen >(), piece), zero) & _mm_set_epi64x(Queen, Queen);
    __v2di n = ~_mm_cmpeq_epi64( _mm_and_si128( get2Pieces<Knight>(), piece), zero) & _mm_set_epi64x(Knight, Knight);
    __v2di p = ~_mm_cmpeq_epi64( _mm_and_si128( get2Pieces<Pawn  >(), piece), zero) & _mm_set_epi64x(Pawn, Pawn);
//        __v2di k = ~_mm_cmpeq_epi64( _mm_and_si128( get2Pieces<King  >(), piece), zero) & _mm_set_epi64x(King, King);
    return fold(r|b|q|n|p);
#else
    return
//            (getPieces<White,King>() | getPieces<Black,King>()) & bit ? King:
        (getPieces<White,Pawn>() | getPieces<Black,Pawn>()) & bit ? Pawn:
        (getPieces<White,Knight>() | getPieces<Black,Knight>()) & bit ? Knight:
        (getPieces<White,Queen>() | getPieces<Black,Queen>()) & bit ? Queen:
        (getPieces<White,Bishop>() | getPieces<Black,Bishop>()) & bit ? Bishop:
        (getPieces<White,Rook>() | getPieces<Black,Rook>()) & bit ? Rook:
        NoPiece;
#endif
}

void BoardBase::copyPieces(BoardBase& next) const
{
#if defined(__AVX__)
    __m256i ymm1 = _mm256_loadu_si256((__m256i*)pieces[1]);
    __m256i ymm2 = _mm256_loadu_si256((__m256i*)pieces[3]);
    __m256i ymm3 = _mm256_loadu_si256((__m256i*)pieces[5]);
    _mm256_storeu_si256((__m256i*)next.pieces[1], ymm1);
    _mm256_storeu_si256((__m256i*)next.pieces[3], ymm2);
    _mm256_storeu_si256((__m256i*)next.pieces[5], ymm3);
#else
#if defined(__SSE2__)
    __m128i xmm1 = _mm_load_si128((__m128i*)pieces+1);
    __m128i xmm2 = _mm_load_si128((__m128i*)pieces+2);
    __m128i xmm3 = _mm_load_si128((__m128i*)pieces+3);
    _mm_store_si128((__m128i*)next.pieces+1, xmm1);
    _mm_store_si128((__m128i*)next.pieces+2, xmm2);
    _mm_store_si128((__m128i*)next.pieces+3, xmm3);
    xmm1 = _mm_load_si128((__m128i*)pieces+4);
    xmm2 = _mm_load_si128((__m128i*)pieces+5);
    xmm3 = _mm_load_si128((__m128i*)pieces+6);
    _mm_store_si128((__m128i*)next.pieces+4, xmm1);
    _mm_store_si128((__m128i*)next.pieces+5, xmm2);
    _mm_store_si128((__m128i*)next.pieces+6, xmm3);
#else
    memcpy(next.pieces[1], pieces[1], nPieces*nColors*sizeof(uint64_t));
#endif
#endif    
}



