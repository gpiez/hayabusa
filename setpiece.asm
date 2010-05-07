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
			
			public	setPiece
			public  clrPiece
			public	chgPiece
			
			extrn	broadcastTab
			extrn	masks
			extrn	vecLookup
			extrn	shortAttacks
			extrn	squareControl
			
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

mask		equ		xmm1
wlkup		equ		xmm4
blkup		equ		xmm6
low4bits	equ		xmm0

macro		setpieceline dir, s, longAttW, longAttB {
			attLenPtr = (dir shl attDir) + (s shl attSlice) + attLen
			attVecPtr = (dir shl attDir) + (s shl attSlice) + attVec
			maskOff	= (dir shl maskDir) + (s shl maskSlice)
			movdqa	xmm3, [rdi + attLenPtr]					;old attLen.0
			movdqa	mask, [rbp + maskOff + $40]				;LenMask.mask.0
			movdqa	xmm2, [rax]								;broadcast attVec[pos]
			movdqa	xmm7, [rdi + attVecPtr]					;old attVec.0
			psubb	xmm3, [rbp + maskOff]					;LenMask.len.0

			andps	xmm2, mask								;diff to be removed = masked attVec[pos]
			psubb	xmm7, xmm2								;remove old attVecIndex

			movdqa	[rdi + attLenPtr], xmm3					;save new attLen.0

			movdqa	xmm3, xmm2
			andps	xmm2, low4bits							;lower 4 bits (right direction)
			psrlw	xmm3, 4									;shift diff 4 bits to the right
			andps	xmm3, low4bits							;upper 4 bits (left dir) of diff now in lower 4 bits of each byte

		if dir=0 
			movdqa	longAttW, [rdi+longAttack + s*$10]
			movdqa  longAttB, [rdi+$40+longAttack + s*$10]
		end if

			movdqa	xmm5, wlkup								;restore lookuptable, pshufb destroys it
			pshufb	xmm5, xmm2								;lookup white right 
			psubb	longAttW, xmm5							;subtract right white attacks

			movdqa	xmm5, blkup
			pshufb	xmm5, xmm2								;lookup black right
			psubb	longAttB, xmm5							;subtract right black attacks

			movdqa	xmm5, wlkup								;restore lookuptable, pshufb destroys it
			pshufb	xmm5, xmm3								;lookup left dir
			psubb	longAttW, xmm5							;update longAttack.w.0 with white left dir attacks

			movdqa	xmm5, blkup								;black
			pshufb	xmm5, xmm3								;lookup left
			psubb	longAttB, xmm5							;update longAttack.b.0 with black left attack

			andps	mask, [rsi]								;rdi broadcasted into all 32 halfbytes
			paddb	xmm7, mask								;new attVecIndex.0

			movdqa	[rdi + attVecPtr], xmm7; <xmm7 free      save back attVec.0

			movdqa	xmm3, mask								;move upper 4 bits in each byte to lower 4 bits.
			psrlw	xmm3, 4
			movdqa	xmm5, wlkup								;restore lookuptable, pshufb destroys it
			paddb	xmm3, mask
			pand	xmm3, low4bits
			movdqa	xmm2, blkup

			pshufb	xmm5, xmm3
			paddb	longAttW, xmm5							;sum up longAttack.w.0

			pshufb	xmm2, xmm3
			paddb	longAttB, xmm2							;sum up longAttack.b.0

		if dir=3
			movdqa	[rdi+longAttack + s*$10], longAttW			;longAttack[white].0
			movdqa  [rdi+$40+longAttack + s*$10], longAttB		;longAttack[black].0
		end if

			}

macro		setpiecedir dir {
			movzx	eax, byte [rdi+rdx+ (dir shl attDir) + attLen]	;l/len len[pos]
			shl		eax, maskLen						;len * sizeof(mask[])
			lea		rbp, [rbx + rax]			;masks[len][][pos][]
			movzx	eax, byte[rdi+rdx+ (dir shl attDir) + attVec]	;old attVecIndex[pos]
			shl		eax, 4
			lea		rax, [rax + broadcastTab]
			
			setpieceline dir, 0, xmm8, xmm12		
			setpieceline dir, 1, xmm9, xmm13
			setpieceline dir, 2, xmm10, xmm14
			setpieceline dir, 3, xmm11, xmm15
			}

;			Place a piece piece at position pos in board this
;			Can only be used after the board has been copied (by clrPiece)
;			this in rdi	
;			piece in rsi
;			pos in edx

setPiece:	mov		[rsp-8], rbp
			mov		[rsp-$10], rbx

			mov		ebp, esi
			and		ebp, shortColor						;offset for black shortAtt if piece<0
			
			mov		ebx, esi							;piece
			sar		ebx, 31								;(piece<0) ? -1:0
			mov		eax, esi							;piece
			xor		eax, ebx							;(piece<0) ? ~piece:piece
			sub		eax, ebx							;abs(piece)
			shl		eax, shortPiece						;offset for abs(piece)

			mov		ebx, edx							;pos
			shl		ebx, shortPos
			lea		rbx, [rbx + rax + shortAttacks]		;offest for [piece][pos][]
; for piece=1,2,3 [rbx+rbp] is always zero
; for piece=4,6 [rbx+rpb] == [rbx]						
			movdqa	xmm0, [rdi + rbp + shortAttack]		;now rdi+rbp points at shortAttack[color]
			paddb	xmm0, [rbx + rbp]
			movdqa	xmm1, [rdi + rbp + shortAttack + $10]
			paddb	xmm1, [rbx + rbp + $10]
			movdqa	xmm2, [rdi + rbp + shortAttack + $20]
			paddb	xmm2, [rbx + rbp + $20]
			movdqa	xmm3, [rdi + rbp + shortAttack + $30]
			paddb	xmm3, [rbx + rbp + $30]
			movdqa  [rdi + rbp + shortAttack], xmm0
			movdqa  [rdi + rbp + shortAttack + $10], xmm1
			movdqa  [rdi + rbp + shortAttack + $20], xmm2
			movdqa  [rdi + rbp + shortAttack + $30], xmm3
						
			movdqa	low4bits, [low4bitsMask]
			 
			mov		ebx, edx				;pos
			shl		ebx, maskPos			;pos * sizeof(mask[][][])
			lea		rbx, [rbx + masks]		;masks[][][pos]
			
			and		esi, 0xf				;clear upper 28 bits, now we have a signed halfbyte zero extended
			imul	esi, 100010000b			;mul by 16 for the broadcastTab index, mul by 10001b to get the halfbyte to the lower and upper half
			lea		rsi, [rsi + broadcastTab];broadcastTab[piece*17]

			movdqa	wlkup, [vecLookup]		;load horizontal lookuptable for white
			movdqa	blkup, [vecLookup+16]	;load lookuptable for black
			setpiecedir 0
			setpiecedir 2

			movdqa	wlkup, [vecLookup+32]	;load diagonal lookuptable for white
			movdqa	blkup, [vecLookup+48]	;load lookuptable for black
			setpiecedir 1
			setpiecedir 3
	
			mov		rbp, [rsp-8]
			mov		rbx, [rsp-$10]
			ret

macro		clrpieceline dir, s, longAttW, longAttB {
			attLenPtr = (dir shl attDir) + (s shl attSlice) + attLen
			attVecPtr = (dir shl attDir) + (s shl attSlice) + attVec
			maskOff	= (dir shl maskDir) + (s shl maskSlice)
			movdqa	xmm3, [rdi + attLenPtr]					;old attLen.0
			movdqa	mask, [rbp + maskOff + $40]				;LenMask.mask.0
			movdqa	xmm2, [rax]								;broadcast attVec[pos]
			movdqa	xmm7, [rdi + attVecPtr]					;old attVec.0
			paddb	xmm3, [rbp + maskOff]					;LenMask.len.0

			andps	xmm2, mask								;diff to be removed = masked attVec[pos]
			paddb	xmm7, xmm2								;remove old attVecIndex

			movdqa	[rsi + attLenPtr], xmm3					;save new attLen.0

			movdqa	xmm3, xmm2
			andps	xmm2, low4bits							;lower 4 bits (right direction)
			psrlw	xmm3, 4									;shift diff 4 bits to the right
			andps	xmm3, low4bits							;upper 4 bits (left dir) of diff now in lower 4 bits of each byte

		if dir=0 
			movdqa	longAttW, [rdi+longAttack + s*$10]
			movdqa  longAttB, [rdi+$40+longAttack + s*$10]
		end if

			movdqa	xmm5, wlkup								;restore lookuptable, pshufb destroys it
			pshufb	xmm5, xmm2								;lookup white right 
			paddb	longAttW, xmm5							;subtract right white attacks

			movdqa	xmm5, blkup
			pshufb	xmm5, xmm2								;lookup black right
			paddb	longAttB, xmm5							;subtract right black attacks

			movdqa	xmm5, wlkup							;restore lookuptable, pshufb destroys it
			pshufb	xmm5, xmm3								;lookup left dir
			paddb	longAttW, xmm5							;update longAttack.w.0 with white left dir attacks

			movdqa	xmm5, blkup								;black
			pshufb	xmm5, xmm3								;lookup left
			paddb	longAttB, xmm5							;update longAttack.b.0 with black left attack

			andps	mask, [rdx]								;rdi broadcasted into all 32 halfbytes
			psubb	xmm7, mask								;new attVecIndex.0

			movdqa	[rsi + attVecPtr], xmm7; <xmm7 free      save back attVec.0

			movdqa	xmm3, mask								;move upper 4 bits in each byte to lower 4 bits.
			psrlw	xmm3, 4
			movdqa	xmm5, wlkup								;restore lookuptable, pshufb destroys it
			paddb	xmm3, mask
			pand	xmm3, low4bits
			movdqa	xmm2, blkup

			pshufb	xmm5, xmm3
			psubb	longAttW, xmm5							;sum up longAttack.w.0

			pshufb	xmm2, xmm3
			psubb	longAttB, xmm2							;sum up longAttack.b.0

		if dir=3
			movdqa	[rsi+longAttack + s*$10], longAttW			;longAttack[white].0
			movdqa  [rsi+$40+longAttack + s*$10], longAttB		;longAttack[black].0
		end if

			}

macro		clrpiecedir dir {
			movzx	eax, byte [rdi+rcx+ (dir shl attDir) + attLen]	;l/len len[pos]
			shl		eax, maskLen						;len * sizeof(mask[])
			lea		rbp, [rbx + rax]			;masks[len][][pos][]
			movzx	eax, byte[rdi+rcx+ (dir shl attDir) + attVec]	;old attVecIndex[pos]
			shl		eax, 4
			lea		rax, [rax + broadcastTab]
			
			clrpieceline dir, 0, xmm8, xmm12		
			clrpieceline dir, 1, xmm9, xmm13
			clrpieceline dir, 2, xmm10, xmm14
			clrpieceline dir, 3, xmm11, xmm15
			}


;			Copy a board from prev to this and

;			prev in rdi
;			this in rsi	
;			piece in edx <- rsi
;			pos in ecx <- edx

clrPiece:	mov		[rsp-8], rbp
			mov		[rsp-$10], rbx

			mov		ebp, edx
			and		ebp, $40							;offset for black shortAtt if piece<0
			
			mov		ebx, edx							;piece
			sar		ebx, 31								;(piece<0) ? -1:0
			mov		eax, edx							;piece
			xor		eax, ebx							;(piece<0) ? ~piece:piece
			sub		eax, ebx							;abs(piece)
			shl		eax, shortPiece						;offset for abs(piece)

			mov		ebx, ecx							;pos
			shl		ebx, shortPos
			lea		rbx, [rbx + rax + shortAttacks]		;offest for [piece][pos][]
						
			movdqa	xmm0, [rdi + rbp + shortAttack]					;now rdi+rbp points at shortAttack[color]
			psubb	xmm0, [rbx + rbp]
			movdqa	xmm1, [rdi + rbp + shortAttack + $10]
			psubb	xmm1, [rbx + rbp + $10]
			movdqa	xmm2, [rdi + rbp + shortAttack + $20]
			psubb	xmm2, [rbx + rbp + $20]
			movdqa	xmm3, [rdi + rbp + shortAttack + $30]
			psubb	xmm3, [rbx + rbp + $30]
			movdqa  [rsi + rbp + shortAttack], xmm0
			movdqa  [rsi + rbp + shortAttack + $10], xmm1
			movdqa  [rsi + rbp + shortAttack + $20], xmm2
			movdqa  [rsi + rbp + shortAttack + $30], xmm3
						
			xor		ebp, $40
			movdqa	xmm0, [rdi + rbp + shortAttack]					;now rdi+rbp points at shortAttack[opposite color]
			movdqa	xmm1, [rdi + rbp + shortAttack + $10]
			movdqa	xmm2, [rdi + rbp + shortAttack + $20]
			movdqa	xmm3, [rdi + rbp + shortAttack + $30]
			movdqa  [rsi + rbp + shortAttack], xmm0
			movdqa  [rsi + rbp + shortAttack + $10], xmm1
			movdqa  [rsi + rbp + shortAttack + $20], xmm2
			movdqa  [rsi + rbp + shortAttack + $30], xmm3
			
			
			movdqa	low4bits, [low4bitsMask]
			 
			mov		ebx, ecx				;pos
			shl		ebx, maskPos			;pos * dxzeof(mask[][][])
			lea		rbx, [rbx + masks]		;masks[][][pos]
			
			and		edx, 0xf				;clear upper 28 bits, now we have a dxgned halfbyte zero extended
			imul	edx, 100010000b			;mul by 16 for the broadcastTab index, mul by 10001b to get the halfbyte to the lower and upper half
			lea		rdx, [rdx + broadcastTab];broadcastTab[piece*17]

			movdqa	wlkup, [vecLookup]		;load horizontal lookuptable for white
			movdqa	blkup, [vecLookup+16]	;load lookuptable for black
			clrpiecedir 0
			clrpiecedir 2

			movdqa	wlkup, [vecLookup+32]	;load diagonal lookuptable for white
			movdqa	blkup, [vecLookup+48]	;load lookuptable for black
			clrpiecedir 1
			clrpiecedir 3
	
			mov		rbp, [rsp-8]
			mov		rbx, [rsp-$10]
			ret

chgPiece:	;//TODO add chgPiece

genMoves:	xorps	xmm0, xmm0
			movdqa	xmm1, [rdi + pieces]
			pcmpgtb xmm0, xmm1					;find black pieces
			movdqa	xmm1, xmm0
			pand	xmm0, [rdi + longAttack]	;find attacked pieces


movePiece:	
;			Copy a board from prev to this and
;			move a from position from to position to

;			this in rdi
;			prev in rsi	
;			from in edx
;			to in ecx

			movsx	eax, byte [rdx + rdi]				;piece
			
			mov		rbx, [rax*8 + pieceList + 6*8]	;get 8 positions at once

nextPiece:	bsf		rcx, rbx						;find least significant bit
			jz		donePiece
			and		cl, -8							;round down to multiple of 8
			shr		rbx, cl							;mov first valid pos into lsb
			movzx	ebp, bl							;first pos

			movzx	ecx, byte[rax + pieceList + 13*8]   ;count
			
			sub		ecx, 1
			jb		donePiece
			movzx	ebp, byte[rcx + rax*8 + pieceList + 6*8]
			
donePiece:
			ret

;			this in rdi
;			convert longAttack[0], longAttack[1], shortAttack[0], shortAttack[1] into index in SSE table
;			needs to be dense in the lower 12 bits
;			long w		long b		short w		short b
;			AAQQBBRR	aaqqbbrr	AANNNPPK
;			Q:			2
;			B:			2
;			R:			3
;			P:			3
;			N:			3
;			K:			2
;			n1n0p1p0r1r0k0b0q0
;			8 7 6 5 4 3 2 1 0
macro 		build4sq	file 
{
			movd		ebx, xmm0					;attack of square a1, used as index in seeTab table
			movd		ecx, xmm1					;e1
			movd		r8d, xmm2					;a2
			movd		r9d, xmm3					;e2
		if	file <> 3
			psrldq		xmm0, 4
			psrldq		xmm1, 4
			psrldq		xmm2, 4
			psrldq		xmm3, 4
		end if
			movsx		eax, word [rsi + rbx*2]			;the index rbx has a "hole" from bit 13 to 15 (always 0). Those pages are never used.
			movsx		edx, word [rsi + rcx*2]			;the index rbx has a "hole" from bit 13 to 15 (always 0). Those pages are never used.
			movsx		r10d, word [rsi + r8*2]			;the index rbx has a "hole" from bit 13 to 15 (always 0). Those pages are never used.
			movsx		r11d, word [rsi + r9*2]			;the index rbx has a "hole" from bit 13 to 15 (always 0). Those pages are never used.
			pinsrw		xmm4, eax, file
			pinsrw		xmm4, edx, file + 4
			pinsrw		xmm5, r10d, file
			pinsrw		xmm5, r11d, file + 4
}

evalKey:	movdqa		xmm4, [low4bitsMask]
			movdqa		xmm8, [low5bitsMask]	
			pxor		xmm6, xmm6
			movdqa		xmm0, [rdi + longAttack]	;lw0 lw1 lw2 lw3 lw4 lw5 lw6 lw7...
			pand		xmm0, xmm4					;mask high 4 bits of LongAttack, this leaves only one queenbit, two rookbits, one bishop bit
			pxor		xmm7, xmm7					;and discard the incoming attack bits and two of three queenbits
			movdqa		xmm2, [rdi+longAttack+16]	;lb0 lb1 lb2 lb3 lb4 lb5 lb6 lb7...
			pand		xmm2, xmm4
			pxor		xmm4, xmm4
			psllw		xmm2, 4						;move black long attacks to upper four bit
			por			xmm0, xmm2					;and merge them with white long attacks in lower 4 bits
			movdqa		xmm2, xmm0					;lwb0 lwb1...lwbf
			
			movdqa		xmm5, [rdi + shortAttack]
			pand		xmm5, xmm8					;mask high 3 bits of shortArrack, this discards incoming attacks and one of two knightbit
			
			punpcklbw 	xmm0, xmm5					;interleave long attacks with short white attacks, 
			movdqa		xmm1, xmm0					;result is lwb0 sw0 lwb1 sw1 lwb2 sw2...
			 
			punpckhbw 	xmm2, xmm5					;lwb8 sw8 lwb9 sw9 ...
			movdqa		xmm3, xmm2

			movdqa		xmm6, [rdi + shortAttack+16]
			pand		xmm6, xmm8
			
			movdqa		xmm7, xmm6
			punpcklbw	xmm6, xmm4					;sb0 0 sb1 0 sb2 ... sw7
			punpckhbw	xmm7, xmm4					;sb8 0 sb9 0 sba ... swf
								
			punpcklwd	xmm0, xmm6					;lwb0 sw0 sb0 0 lwb1 sw0 sb0 0
			punpckhwd	xmm1, xmm6					;lwb4 sw4 sb4 0 lwb5 sw5 sb5 0
			punpcklwd	xmm2, xmm7					;lwb8 sw8 sb8 0 lwb9 sw9 sb9 0
			punpckhwd	xmm3, xmm7				
						
			mov			rsi, [seeTab]
			xor			ecx, ecx						;pos
			xor			edx, edx						;sum

			build4sq	0
			build4sq	1
			build4sq	2
			build4sq	3
			


			movdqa		xmm0, [squareControl]		;a1-h1
			pmullw		xmm0, xmm4
			movdqa		xmm1, [squareControl+16]	;a2-h2
			pmullw		xmm1, xmm5			
			paddw		xmm0, xmm1
			movdqa		xmm1, xmm0
			psrldq		xmm1, 8
			paddw		xmm0, xmm1
			movdqa		xmm1, xmm0
			psrldq		xmm0, 4
			paddw		xmm0, xmm1
			movdqa		xmm1, xmm0
			psrldq		xmm0, 2
			

section		".rodata" align 16

low4bitsMask:
			db 		16 dup $f				;$0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f
low5bitsMask:
			db		16 dup $1f
			
magic0:		dw		4 dup $85A9, $9A58		;different values for long and short attack
magic1:		dw		4 dup $8A59, $95A8

section		".bss" align 16

longAttackMask:
			rb		16
seeTab:
			rq		1
