/*
 * Based on arch/arm/lib/bitops.h
 *
 * Copyright (C) 2013 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/linkage.h>
#include <asm/assembler.h>

/*
 * x0: bits 5:0  bit offset
 *     bits 31:6 word offset
 * x1: address
 */
	.macro	bitop, name, instr
ENTRY(	\name	)
	and	w3, w0, #63	
	eor	w0, w0, w3	
	mov	x2, #1
	add	x1, x1, x0, lsr #3
	lsl	x3, x2, x3	
1:	ldxr	x2, [x1]
	\instr	x2, x2, x3
	stxr	w0, x2, [x1]
	cbnz	w0, 1b
	ret
ENDPROC(\name	)
	.endm

	.macro	testop, name, instr
ENTRY(	\name	)
	and	w3, w0, #63	
	eor	w0, w0, w3	
	mov	x2, #1
	add	x1, x1, x0, lsr #3
	lsl	x4, x2, x3	
1:	ldxr	x2, [x1]
	lsr	x0, x2, x3	
	\instr	x2, x2, x4	
	stlxr	w5, x2, [x1]
	cbnz	w5, 1b
	dmb	ish
	and	x0, x0, #1
3:	ret
ENDPROC(\name	)
	.endm

/*
 * Atomic bit operations.
 */
	bitop	change_bit, eor
	bitop	clear_bit, bic
	bitop	set_bit, orr

	testop	test_and_change_bit, eor
	testop	test_and_clear_bit, bic
	testop	test_and_set_bit, orr
