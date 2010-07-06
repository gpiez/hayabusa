;    hayabusa, chess engine
;    Copyright (C) 2009-2010 Gunther Piez

;    This program is free software: you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation, either version 3 of the License, or
;    (at your option) any later version.

;    This program is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.

;    You should have received a copy of the GNU General Public License
;	along with this program.  If not, see <http://www.gnu.org/licenses/>.

			format	ELF64
			
			public  clrPiece
			public	setPiece
			public  movPiece
			
			extrn	masks
			
include		"offsets.inc"

			;testcase	 	.	o	o	.	n	.	.	.	n = new piece at pos=2*8+4, o = old at 2
			;before len 0	1	1	5	4	3	2	1	0
			;				1	1	2	1	3	2	1	0
			;
			;before	len=4   0   1   1   1   2   3   4   5
			;		xlen=4  0   1   2   2   3   4   5   6	xlen[pos] = len[pos] + len[pos + diroff*len[pos]]
			;											   xxlen[pos] = len[pos] +xlen[pos + diroff*len[pos]]
			;												xatt[pos] = att[dir][pos + diroff*len[dir^4][pos]]
			;pmovmsk xatt   7	7	6   5   5   3   3   3 = after + 76543210
			;after			0	1	1	1	2	1	2	3
			;				0	1	2	2	3	3	4	5

			;diff								2	2	2	len[pos] = pos - endpos[pos]
			;									1	1	1

			;before
			;				16	16	17	18	18	18	18	18
			;				17	18	23	23	23	23	23	23
			;after
			;endpos (-4)	16	16	17	18	18	20	20	20
			;endpos  (0)	17	18	20	20	23	23	23	23

section		".text"

macro		setpiecedir dir {
            attLenPtr = (dir shl attDir) + attLen - $80
            maskOff = (dir shl maskDir)

			movzx	eax, byte [rdi+rdx+ attLenPtr]	        ;l/len len[pos]
			shl		eax, maskLen						    ;len * sizeof(mask[])
			lea		rbp, [rbx + rax + maskOff]			    ;masks[len][][pos][]

            movdqa  xmm0, [rdi + attLenPtr]                 ;old attLen.0
            psubb   xmm0, [rbp]                             ;LenMask.len.0
            movdqa  xmm1, [rdi + attLenPtr + $10]           ;old attLen.1
            psubb   xmm1, [rbp + $10]                       ;LenMask.len.1
            movdqa  xmm2, [rdi + attLenPtr + $20]           ;old attLen.2
            psubb   xmm2, [rbp + $20]                       ;LenMask.len.2
            movdqa  xmm3, [rdi + attLenPtr + $30]           ;old attLen.3
            psubb   xmm4, [rbp + $30]                       ;LenMask.len.3
            movdqa  [rdi + attLenPtr], xmm0                 ;save new attLen.0
            movdqa  [rdi + attLenPtr + $10], xmm1           ;save new attLen.1
            movdqa  [rdi + attLenPtr + $20], xmm2           ;save new attLen.2
            movdqa  [rdi + attLenPtr + $30], xmm3           ;save new attLen.3
			}

macro		clrpiecedir dir {
            attLenPtr = (dir shl attDir) + attLen - $80
            maskOff = (dir shl maskDir)

            movzx   eax, byte [rsi+rdx+ attLenPtr]          ;l/len len[pos]
            shl     eax, maskLen                            ;len * sizeof(mask[])
            lea     rbp, [rbx + rax + maskOff]              ;masks[len][][pos][]

            movdqa  xmm0, [rsi + attLenPtr]                 ;old attLen.0
            paddb   xmm0, [rbp]                             ;LenMask.len.0
            movdqa  xmm1, [rsi + attLenPtr + $10]           ;old attLen.0
            paddb   xmm1, [rbp + $10]                       ;LenMask.len.0
            movdqa  xmm2, [rsi + attLenPtr + $20]           ;old attLen.0
            paddb   xmm2, [rbp + $20]                       ;LenMask.len.0
            movdqa  xmm3, [rsi + attLenPtr + $30]           ;old attLen.0
            paddb   xmm4, [rbp + $30]                       ;LenMask.len.0
            movdqa  [rdi + attLenPtr], xmm0                 ;save new attLen.0
            movdqa  [rdi + attLenPtr + $10], xmm1           ;save new attLen.0
            movdqa  [rdi + attLenPtr + $20], xmm2           ;save new attLen.0
            movdqa  [rdi + attLenPtr + $30], xmm3           ;save new attLen.0
			}


macro		movpiecedir dir {    //rdx to,  rcx=from
            attLenPtr = (dir shl attDir) + attLen - $80
            maskOff = (dir shl maskDir)

            movzx   ebp, byte [rsi+rdx+ attLenPtr]          ;l/len len[pos]
            shl     ebp, maskLen                            ;len * sizeof(mask[])
            lea     rbp, [rbx + rax + maskOff]              ;masks[len][][pos][]
            movzx   eax, byte [rsi+rcx+ attLenPtr]
            shl     eax, maskLen
            lea     rax, [r12 + rax + maskOff]

            movdqa  xmm0, [rsi + attLenPtr]                 ;old attLen.0
            psubb   xmm0, [rbp]                             ;LenMask.len.0
            paddb   xmm0, [rax]
            movdqa  xmm1, [rsi + attLenPtr + $10]           ;old attLen.0
            psubb   xmm1, [rbp + $10]                       ;LenMask.len.0
            paddb   xmm1, [rax + $10]
            movdqa  xmm2, [rsi + attLenPtr + $20]           ;old attLen.0
            psubb   xmm2, [rbp + $20]                       ;LenMask.len.0
            paddb   xmm2, [rax + $20]
            movdqa  xmm3, [rsi + attLenPtr + $30]           ;old attLen.0
            psubb   xmm3, [rbp + $30]                       ;LenMask.len.0
            paddb   xmm3, [rax + $30]
            movdqa  [rdi + attLenPtr], xmm0                 ;save new attLen.0
            movdqa  [rdi + attLenPtr + $10], xmm1           ;save new attLen.0
            movdqa  [rdi + attLenPtr + $20], xmm2           ;save new attLen.0
            movdqa  [rdi + attLenPtr + $30], xmm3           ;save new attLen.0
			}

;			Place a piece piece at position pos in board this
;			Can only be used after the board has been copied (by clrPiece)
;			this in rdi
;			pos in rsi

setPiece:	mov		r10, rbp
			mov		r11, rbx
			lea 	rdi, [rdi + $80]						;add offset to use shorter displacements later

			mov		ebx, esi				;pos
			shl		ebx, maskPos			;pos * sizeof(mask[][][])
			lea		rbx, [rbx + masks]		;masks[][][pos]

			setpiecedir 0
			setpiecedir 2
			setpiecedir 1
			setpiecedir 3

			mov		rbp, r10
			mov		rbx, r11
			ret

;			remove a piece from this

;			this in rdi
;			prev in rsi
;           pos in rdx

clrPiece:	mov		r10, rbp
            mov		r11, rbx
			lea 	rdi, [rdi+$80]
			lea		rsi, [rsi+$80]

			mov		ebx, edx				;pos
			shl		ebx, maskPos			;pos * dxzeof(mask[][][])
			lea		rbx, [rbx + masks]		;masks[][][pos]

			clrpiecedir 0
			clrpiecedir 2
			clrpiecedir 1
			clrpiecedir 3
	
			mov		rbp, r10
			mov		rbx, r11
			ret

;			this in rdi
;			prev in rsi
;           to in edx
;			from in ecx

movPiece:	mov		r10, rbp
			mov		r11, rbx
			lea		rdi, [rdi + $80]		;preadd offset to allow more short offsets later
            lea     rsi, [rsi+$80]

			mov		r12d, ecx				;from
			shl		r12d, maskPos			;pos * dxzeof(mask[][][])
			lea		r12, [r12 + masks]		;masks[][][pos]

            mov     ebx, edx                ;pos
            shl     ebx, maskPos            ;pos * dxzeof(mask[][][])
            lea     rbx, [rbx + masks]      ;masks[][][pos]

			movpiecedir 0
			movpiecedir 2
			movpiecedir 1
			movpiecedir 3

			mov		rbp, r10
			mov		rbx, r11
			ret
