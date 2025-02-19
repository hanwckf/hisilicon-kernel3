	.arch armv7-a
	.fpu softvfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 2
	.eabi_attribute 30, 2
	.eabi_attribute 34, 0
	.eabi_attribute 18, 4
	.file	"vdm_hal_real8.c"

	.text
	.align	2
	.global	RV8HAL_V400R003_InitHal
	.type	RV8HAL_V400R003_InitHal, %function
RV8HAL_V400R003_InitHal:
	.fnstart


	.movsp ip
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	.pad #4
	.save {fp, ip, lr}
	.setfp fp, ip, #-4
	sub	fp, ip, #4
	push	{lr}
	bl	__gnu_mcount_nc
	mov	r0, #0
	ldmfd	sp, {fp, sp, pc}
	.fnend
	.size	RV8HAL_V400R003_InitHal, .-RV8HAL_V400R003_InitHal
	.align	2
	.global	RV8HAL_V400R003_WriteReg
	.type	RV8HAL_V400R003_WriteReg, %function
RV8HAL_V400R003_WriteReg:
	.fnstart


	.movsp ip
	mov	ip, sp
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, fp, ip, lr, pc}
	.pad #4
	.save {r4, r5, r6, r7, r8, r9, fp, ip, lr}
	.setfp fp, ip, #-4
	sub	fp, ip, #4
	.pad #16
	sub	sp, sp, #16
	push	{lr}
	bl	__gnu_mcount_nc
	mov	r8, r0
	mov	r6, r1
	mov	r0, #2
	ldr	r1, .L8
	mov	r4, r2
	mov	r5, r3
	bl	dprint_vfmw
	cmp	r4, #0
	ble	.L3
	mov	r0, #0
	mov	r3, r4
	str	r0, [sp]
	ldr	r2, .L8+4
	ldr	r1, .L8+8
	bl	dprint_vfmw
	sub	sp, fp, #36
	ldmfd	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}
.L3:
	ldr	r0, [r6, #4]
	mov	r7, #0
	ldr	ip, [r6, #8]
	mov	r1, r7
	mov	lr, #1
	bfi	lr, r7, #1, #1
	mov	r3, r5
	mov	r2, r4
	mul	ip, r0, ip
	mov	r0, #8
	sub	ip, ip, #1
	bfi	r1, ip, #0, #20
	str	r1, [fp, #-40]
	strb	lr, [fp, #-37]
	mov	r1, r1, lsr #16
	bfi	r1, r7, #7, #1
	strb	r1, [fp, #-38]
	ldr	r9, [fp, #-40]
	mov	r1, r9
	bl	MFDE_ConfigReg
	mov	r2, r9
	ldr	r1, .L8+12
	mov	r0, #3
	bl	dprint_vfmw
	str	r7, [fp, #-40]
	ldrh	r1, [fp, #-38]
	mov	r0, #8
	ldr	ip, [r6, #84]
	mov	r3, #0
	ldr	r2, [r6, #64]
	bfi	r1, r7, #0, #12
	strb	r0, [fp, #-40]
	bfi	r3, ip, #4, #1
	strb	r3, [fp, #-39]
	mov	r3, r1, lsr r0
	ldrh	r0, [fp, #-40]
	mov	r2, r2, lsr #6
	and	r3, r3, #239
	strh	r1, [fp, #-38]
	bfi	r0, r2, #4, #7
	bfi	r3, r7, #5, #1
	strh	r0, [fp, #-40]
	mov	r2, r4
	strb	r3, [fp, #-37]
	mov	r0, #12
	ldr	r9, [r6, #40]
	mov	r3, r5
	ldrb	r1, [fp, #-39]
	ldr	lr, [r6, #80]
	ldrb	ip, [fp, #-37]
	bfi	r1, r9, #6, #1
	bfi	ip, lr, #6, #1
	mvn	r1, r1, asl #25
	mvn	r1, r1, lsr #25
	strb	r1, [fp, #-39]
	mov	r1, ip
	bfi	r1, r7, #7, #1
	strb	r1, [fp, #-37]
	ldr	r9, [fp, #-40]
	mov	r1, r9
	bl	MFDE_ConfigReg
	mov	r2, r9
	ldr	r1, .L8+16
	mov	r0, #3
	bl	dprint_vfmw
	ldr	r9, [r8, #56]
	mov	r3, r5
	mov	r2, r4
	bic	r9, r9, #15
	mov	r0, #16
	mov	r1, r9
	bl	MFDE_ConfigReg
	mov	r2, r9
	ldr	r1, .L8+20
	mov	r0, #3
	bl	dprint_vfmw
	ldr	r9, [r8, #40]
	mov	r3, r5
	mov	r2, r4
	bic	r9, r9, #15
	mov	r0, #20
	mov	r1, r9
	bl	MFDE_ConfigReg
	mov	r2, r9
	ldr	r1, .L8+24
	mov	r0, #3
	bl	dprint_vfmw
	ldr	r9, [r6, #44]
	mov	r3, r5
	mov	r2, r4
	mov	r0, #24
	mov	r1, r9
	str	r9, [fp, #-40]
	bl	MFDE_ConfigReg
	mov	r2, r9
	ldr	r1, .L8+28
	mov	r0, #3
	bl	dprint_vfmw
	ldr	r1, [r6, #4]
	mov	r3, r5
	mov	r2, r4
	cmp	r1, #256
	mov	r0, #4
	movhi	r1, #0
	movls	r1, #1
	bl	SCD_ConfigReg
	movw	r1, #3075
	mov	r3, r5
	mov	r2, r4
	mov	r0, #60
	movt	r1, 48
	bl	MFDE_ConfigReg
	movw	r1, #3075
	mov	r3, r5
	mov	r2, r4
	mov	r0, #64
	movt	r1, 48
	bl	MFDE_ConfigReg
	movw	r1, #3075
	mov	r3, r5
	mov	r2, r4
	mov	r0, #68
	movt	r1, 48
	bl	MFDE_ConfigReg
	movw	r1, #3075
	mov	r3, r5
	mov	r2, r4
	mov	r0, #72
	movt	r1, 48
	bl	MFDE_ConfigReg
	movw	r1, #3075
	mov	r3, r5
	mov	r2, r4
	mov	r0, #76
	movt	r1, 48
	bl	MFDE_ConfigReg
	movw	r1, #3075
	mov	r3, r5
	mov	r2, r4
	mov	r0, #80
	movt	r1, 48
	bl	MFDE_ConfigReg
	movw	r1, #3075
	mov	r3, r5
	mov	r2, r4
	mov	r0, #84
	movt	r1, 48
	bl	MFDE_ConfigReg
	movw	r2, #3075
	movt	r2, 48
	ldr	r1, .L8+32
	mov	r0, #3
	bl	dprint_vfmw
	ldr	r9, [r6, #56]
	mov	r3, r5
	mov	r2, r4
	bic	r9, r9, #15
	mov	r0, #96
	mov	r1, r9
	bl	MFDE_ConfigReg
	mov	r2, r9
	ldr	r1, .L8+36
	mov	r0, #3
	bl	dprint_vfmw
	ldr	r9, [r6, #64]
	mov	r3, r5
	mov	r2, r4
	mov	r0, #100
	mov	r1, r9
	bl	MFDE_ConfigReg
	mov	r2, r9
	ldr	r1, .L8+40
	mov	r0, #3
	bl	dprint_vfmw
	ldr	r9, [r6, #88]
	mov	r3, r5
	mov	r2, r4
	mov	r0, #104
	mov	r1, r9
	bl	MFDE_ConfigReg
	mov	r2, r9
	ldr	r1, .L8+44
	mov	r0, #3
	bl	dprint_vfmw
	ldr	r6, [r6, #96]
	mov	r3, r5
	mov	r2, r4
	mov	r0, #108
	mov	r1, r6
	bl	MFDE_ConfigReg
	mov	r2, r6
	ldr	r1, .L8+48
	mov	r0, #3
	bl	dprint_vfmw
	ldr	r6, [r8, #1204]
	mov	r3, r5
	mov	r2, r4
	mov	r0, #144
	mov	r1, r6
	bl	MFDE_ConfigReg
	mov	r2, r6
	ldr	r1, .L8+52
	mov	r0, #3
	bl	dprint_vfmw
	mov	r3, r5
	mov	r1, r7
	mov	r2, r4
	mov	r0, #148
	str	r7, [fp, #-40]
	bl	MFDE_ConfigReg
	mov	r2, r7
	ldr	r1, .L8+56
	mov	r0, #3
	bl	dprint_vfmw
	ldrb	r1, [fp, #-40]
	mov	r3, r5
	mov	r2, r4
	bfi	r1, r7, #0, #1
	strb	r1, [fp, #-40]
	ldr	r6, [fp, #-40]
	mov	r0, #152
	mov	r1, r6
	bl	MFDE_ConfigReg
	mov	r2, r6
	ldr	r1, .L8+60
	mov	r0, #3
	bl	dprint_vfmw
	mov	r3, r5
	mov	r2, r4
	mvn	r1, #0
	mov	r0, #32
	bl	MFDE_ConfigReg
	sub	sp, fp, #36
	ldmfd	sp, {r4, r5, r6, r7, r8, r9, fp, sp, pc}
.L9:
	.align	2
.L8:
	.word	.LC0
	.word	.LANCHOR0
	.word	.LC1
	.word	.LC2
	.word	.LC3
	.word	.LC4
	.word	.LC5
	.word	.LC6
	.word	.LC7
	.word	.LC8
	.word	.LC9
	.word	.LC10
	.word	.LC11
	.word	.LC12
	.word	.LC13
	.word	.LC14
	.fnend
	.size	RV8HAL_V400R003_WriteReg, .-RV8HAL_V400R003_WriteReg
	.align	2
	.global	RV8HAL_V400R003_WritePicMsg
	.type	RV8HAL_V400R003_WritePicMsg, %function
RV8HAL_V400R003_WritePicMsg:
	.fnstart


	.movsp ip
	mov	ip, sp
	stmfd	sp!, {r4, r5, r6, r7, fp, ip, lr, pc}
	.pad #4
	.save {r4, r5, r6, r7, fp, ip, lr}
	.setfp fp, ip, #-4
	sub	fp, ip, #4
	.pad #8
	sub	sp, sp, #8
	push	{lr}
	bl	__gnu_mcount_nc
	mov	r6, r0
	mov	r5, r1
	mov	r0, #2
	ldr	r1, .L14
	bl	dprint_vfmw
	ldr	r0, [r6, #56]
	bic	r0, r0, #15
	bl	MEM_Phy2Vir
	subs	r4, r0, #0
	beq	.L13
	ldrb	r2, [r5, #12]
	mov	r7, #0
	mov	r3, #0
	str	r7, [fp, #-32]
	bfi	r3, r2, #0, #2
	strb	r3, [fp, #-32]
	ldr	r2, [fp, #-32]
	mov	r0, #4
	ldr	r1, .L14+4
	str	r2, [r4]
	bl	dprint_vfmw
	ldmib	r5, {r2, r3}
	mov	r0, #0
	sub	r3, r3, #1
	sub	r2, r2, #1
	mov	r1, r0
	bfi	r0, r2, #0, #9
	bfi	r1, r3, #0, #9
	strh	r0, [fp, #-32]
	strh	r1, [fp, #-30]
	mov	r0, #4
	ldr	r2, [fp, #-32]
	ldr	r1, .L14+8
	str	r2, [r4, #4]
	bl	dprint_vfmw
	ldr	r2, [r5, #16]
	ldr	r3, [r5, #20]
	mov	r0, #4
	ldr	r1, .L14+12
	strh	r2, [fp, #-32]
	strh	r3, [fp, #-30]
	ldr	r2, [fp, #-32]
	str	r2, [r4, #8]
	bl	dprint_vfmw
	str	r7, [fp, #-32]
	mov	r3, #0
	bfi	r3, r7, #1, #3
	strb	r3, [fp, #-32]
	mov	r0, #4
	ldr	r2, [fp, #-32]
	ldr	r1, .L14+16
	str	r2, [r4, #12]
	bl	dprint_vfmw
	ldr	r2, [r5, #24]
	str	r7, [fp, #-32]
	mov	r3, #0
	bfi	r3, r2, #0, #5
	strb	r3, [fp, #-32]
	ldr	r2, [fp, #-32]
	mov	r0, #4
	ldr	r1, .L14+20
	str	r2, [r4, #16]
	bl	dprint_vfmw
	ldr	r2, [r5, #28]
	str	r7, [fp, #-32]
	mov	r3, #0
	bfi	r3, r2, #0, #5
	strb	r3, [fp, #-32]
	ldr	r2, [fp, #-32]
	mov	r0, #4
	ldr	r1, .L14+24
	str	r2, [r4, #20]
	bl	dprint_vfmw
	ldr	r2, [r5, #32]
	str	r7, [fp, #-32]
	mov	r3, #0
	bfi	r3, r2, #0, #5
	strb	r3, [fp, #-32]
	ldr	r2, [fp, #-32]
	mov	r0, #4
	ldr	r1, .L14+28
	str	r2, [r4, #24]
	bl	dprint_vfmw
	ldr	r2, [r5, #60]
	ldr	r1, .L14+32
	mov	r0, #4
	bic	r2, r2, #15
	str	r2, [r4, #64]
	bl	dprint_vfmw
	ldr	r2, [r5, #52]
	ldr	r1, .L14+36
	mov	r0, #4
	bic	r2, r2, #15
	str	r2, [r4, #68]
	bl	dprint_vfmw
	ldr	r2, [r5, #48]
	ldr	r1, .L14+40
	mov	r0, #4
	bic	r2, r2, #15
	str	r2, [r4, #72]
	bl	dprint_vfmw
	ldr	r2, [r5, #72]
	ldr	r1, .L14+44
	mov	r0, #4
	str	r2, [r4, #76]
	bl	dprint_vfmw
	ldr	r2, [r5, #76]
	ldr	r1, .L14+48
	mov	r0, #4
	str	r2, [r4, #80]
	bl	dprint_vfmw
	ldr	r2, [r6, #1144]
	ldr	r1, .L14+52
	mov	r0, #4
	bic	r2, r2, #15
	str	r2, [r4, #84]
	bl	dprint_vfmw
	ldr	r2, [r6, #1148]
	ldr	r1, .L14+56
	mov	r0, #4
	bic	r2, r2, #15
	str	r2, [r4, #88]
	bl	dprint_vfmw
	ldr	r2, [r6, #1152]
	ldr	r1, .L14+60
	mov	r0, #4
	bic	r2, r2, #15
	str	r2, [r4, #92]
	bl	dprint_vfmw
	ldr	r2, [r6, #1160]
	ldr	r1, .L14+64
	mov	r0, #4
	str	r2, [r4, #96]
	bl	dprint_vfmw
	ldr	r2, [r6, #56]
	ldr	r1, .L14+68
	mov	r0, #4
	bic	r2, r2, #15
	add	r2, r2, #256
	str	r2, [r4, #252]
	bl	dprint_vfmw
	sub	sp, fp, #28
	ldmfd	sp, {r4, r5, r6, r7, fp, sp, pc}
.L13:
	ldr	r3, .L14+72
	ldr	r2, .L14+76
	ldr	r1, .L14+80
	bl	dprint_vfmw
	sub	sp, fp, #28
	ldmfd	sp, {r4, r5, r6, r7, fp, sp, pc}
.L15:
	.align	2
.L14:
	.word	.LC15
	.word	.LC18
	.word	.LC19
	.word	.LC20
	.word	.LC21
	.word	.LC22
	.word	.LC23
	.word	.LC24
	.word	.LC25
	.word	.LC26
	.word	.LC27
	.word	.LC28
	.word	.LC29
	.word	.LC30
	.word	.LC31
	.word	.LC32
	.word	.LC33
	.word	.LC34
	.word	.LC16
	.word	.LANCHOR0+28
	.word	.LC17
	.fnend
	.size	RV8HAL_V400R003_WritePicMsg, .-RV8HAL_V400R003_WritePicMsg
	.align	2
	.global	RV8HAL_V400R003_WriteSliceMsg
	.type	RV8HAL_V400R003_WriteSliceMsg, %function
RV8HAL_V400R003_WriteSliceMsg:
	.fnstart


	.movsp ip
	mov	ip, sp
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, fp, ip, lr, pc}
	.pad #4
	.save {r4, r5, r6, r7, r8, r9, r10, fp, ip, lr}
	.setfp fp, ip, #-4
	sub	fp, ip, #4
	.pad #28
	sub	sp, sp, #28
	push	{lr}
	bl	__gnu_mcount_nc
	mov	r4, r0
	mov	r10, r1
	mov	r0, #2
	ldr	r1, .L37
	bl	dprint_vfmw
	ldr	r4, [r4, #56]
	add	r9, r10, #100
	bic	r4, r4, #15
	add	r5, r4, #256
	mov	r0, r5
	bl	MEM_Phy2Vir
	subs	r7, r0, #0
	beq	.L34
	ldr	r3, [r10, #136]
	cmp	r3, #0
	bne	.L35
.L19:
	ldr	r2, [r10, #36]
	cmp	r2, #0
	beq	.L16
	mov	r4, #0
	add	r2, r5, r3, lsl #6
	mov	r3, r3, asl #8
	str	r2, [fp, #-64]
	str	r3, [fp, #-60]
.L28:
	cmp	r4, #0
	beq	.L31
	mov	r5, #44
	mla	r5, r5, r4, r9
	ldr	r2, [r5, #36]
	ldr	r3, [r5, #-8]
	cmp	r2, r3
	ble	.L22
.L21:
	ldr	r1, [r5, #16]
	mov	r8, #0
	ldr	r2, [r5, #8]
	mov	r3, r8
	bfi	r3, r1, #0, #24
	str	r3, [fp, #-48]
	ldrb	r3, [fp, #-45]
	mov	r0, #4
	ldr	r1, [fp, #-60]
	bfi	r3, r2, #0, #7
	strb	r3, [fp, #-45]
	ldr	r2, [fp, #-48]
	add	r6, r1, r4, lsl #8
	ldr	r1, .L37+4
	add	r4, r4, #1
	str	r2, [r7, r6]
	bl	dprint_vfmw
	ldr	r1, [r5]
	add	r3, r6, #4
	mov	r2, r8
	mov	r0, #4
	bfi	r2, r1, #0, #24
	ldr	r1, .L37+8
	str	r2, [fp, #-48]
	str	r2, [r7, r3]
	bl	dprint_vfmw
	ldr	r1, [r5, #20]
	ldr	r2, [r5, #12]
	mov	r3, r8
	bfi	r3, r1, #0, #24
	str	r3, [fp, #-48]
	ldrb	r3, [fp, #-45]
	add	ip, r6, #8
	ldr	r1, .L37+12
	mov	r0, #4
	bfi	r3, r2, #0, #7
	strb	r3, [fp, #-45]
	ldr	r2, [fp, #-48]
	str	r2, [r7, ip]
	bl	dprint_vfmw
	ldr	r1, [r5, #4]
	add	r3, r6, #12
	mov	r2, r8
	mov	r0, #4
	bfi	r2, r1, #0, #24
	ldr	r1, .L37+16
	str	r2, [fp, #-48]
	str	r2, [r7, r3]
	bl	dprint_vfmw
	ldr	r3, [r5, #24]
	ldr	r1, [r5, #28]
	add	ip, r6, #16
	ldr	r2, [r5, #32]
	and	r3, r3, #1
	bfi	r3, r1, #1, #2
	str	r8, [fp, #-48]
	bfi	r3, r2, #3, #5
	strb	r3, [fp, #-46]
	ldr	r2, [fp, #-48]
	mov	r0, #4
	ldr	r1, .L37+20
	str	r2, [r7, ip]
	bl	dprint_vfmw
	ldr	r2, [r10, #36]
	ldr	r1, [r5, #36]
	cmp	r4, r2
	str	r8, [fp, #-48]
	strh	r1, [fp, #-48]
	bcs	.L23
	mov	r3, #44
	mul	r3, r3, r4
	add	r0, r9, r3
	ldr	r0, [r0, #36]
	cmp	r1, r0
	blt	.L23
	sub	r3, r3, #44
	add	r3, r9, r3
	b	.L24
.L25:
	ldr	r0, [r3, #80]
	cmp	r1, r0
	blt	.L23
.L24:
	add	r4, r4, #1
	add	r3, r3, #44
	cmp	r4, r2
	bcc	.L25
.L23:
	cmp	r2, r4
	beq	.L36
	mov	r2, #44
	ldr	r3, [fp, #-64]
	mla	r2, r2, r4, r9
	add	r8, r3, r4, lsl #8
	mov	r3, r8
	ldr	r2, [r2, #36]
	sub	r2, r2, #1
	str	r2, [r10, #140]
.L27:
	ldr	r2, [r5, #40]
	add	ip, r6, #20
	ldr	r1, .L37+24
	mov	r0, #4
	str	r3, [fp, #-56]
	add	r6, r6, #252
	strh	r2, [fp, #-46]
	sub	r4, r4, #1
	ldr	r2, [fp, #-48]
	str	r2, [r7, ip]
	bl	dprint_vfmw
	ldr	r3, [fp, #-56]
	mov	r2, r8
	ldr	r1, .L37+28
	mov	r0, #4
	str	r8, [fp, #-48]
	str	r3, [r7, r6]
	bl	dprint_vfmw
.L22:
	ldr	r3, [r10, #36]
	add	r4, r4, #1
	cmp	r3, r4
	bhi	.L28
.L16:
	sub	sp, fp, #40
	ldmfd	sp, {r4, r5, r6, r7, r8, r9, r10, fp, sp, pc}
.L31:
	mov	r5, r9
	b	.L21
.L36:
	ldr	r1, [r10, #8]
	mov	r3, #0
	ldr	r2, [r10, #4]
	mov	r8, r3
	mul	r2, r2, r1
	sub	r2, r2, #1
	str	r2, [r10, #140]
	b	.L27
.L35:
	mov	r6, #0
	ldr	r2, [r10, #108]
	mov	r3, r6
	bfi	r3, r6, #0, #24
	str	r3, [fp, #-48]
	mov	r0, #4
	ldrb	r3, [fp, #-45]
	add	r4, r4, #512
	ldr	r1, .L37+4
	bfi	r3, r2, #0, #7
	strb	r3, [fp, #-45]
	ldr	r2, [fp, #-48]
	str	r2, [r7]
	bl	dprint_vfmw
	ldr	r2, [r10, #100]
	mov	r3, r6
	ldr	r1, .L37+8
	bfi	r3, r2, #0, #24
	mov	r0, #4
	str	r3, [fp, #-48]
	mov	r2, r3
	str	r3, [r7, #4]
	bl	dprint_vfmw
	mov	r3, r6
	bfi	r3, r6, #0, #24
	str	r3, [fp, #-48]
	ldr	r2, [r10, #112]
	mov	r0, #4
	ldrb	r3, [fp, #-45]
	ldr	r1, .L37+12
	bfi	r3, r2, #0, #7
	strb	r3, [fp, #-45]
	ldr	r2, [fp, #-48]
	str	r2, [r7, #8]
	bl	dprint_vfmw
	ldr	r2, [r10, #104]
	mov	r3, r6
	ldr	r1, .L37+16
	bfi	r3, r2, #0, #24
	mov	r0, #4
	str	r3, [fp, #-48]
	mov	r2, r3
	str	r3, [r7, #12]
	bl	dprint_vfmw
	ldr	r3, [r10, #124]
	ldr	r1, [r10, #128]
	mov	r0, #4
	ldr	r2, [r10, #132]
	and	r3, r3, #1
	bfi	r3, r1, #1, #2
	str	r6, [fp, #-48]
	bfi	r3, r2, #3, #5
	strb	r3, [fp, #-46]
	ldr	r2, [fp, #-48]
	ldr	r1, .L37+20
	str	r2, [r7, #16]
	bl	dprint_vfmw
	ldr	r3, [r10, #136]
	strh	r6, [fp, #-48]
	mov	r0, #4
	sub	r3, r3, #1
	strh	r3, [fp, #-46]
	ldr	r2, [fp, #-48]
	ldr	r1, .L37+24
	str	r2, [r7, #20]
	bl	dprint_vfmw
	ldr	r1, .L37+28
	mov	r0, #4
	str	r4, [r7, #252]
	mov	r2, r4
	str	r4, [fp, #-48]
	bl	dprint_vfmw
	mov	r3, #1
	b	.L19
.L34:
	ldr	r3, .L37+32
	ldr	r2, .L37+36
	ldr	r1, .L37+40
	bl	dprint_vfmw
	b	.L16
.L38:
	.align	2
.L37:
	.word	.LC35
	.word	.LC18
	.word	.LC19
	.word	.LC20
	.word	.LC21
	.word	.LC22
	.word	.LC23
	.word	.LC34
	.word	.LC16
	.word	.LANCHOR0+56
	.word	.LC17
	.fnend
	.size	RV8HAL_V400R003_WriteSliceMsg, .-RV8HAL_V400R003_WriteSliceMsg
	.align	2
	.global	RV8HAL_V400R003_StartDec
	.type	RV8HAL_V400R003_StartDec, %function
RV8HAL_V400R003_StartDec:
	.fnstart


	.movsp ip
	mov	ip, sp
	stmfd	sp!, {r4, r5, fp, ip, lr, pc}
	.pad #4
	.save {r4, r5, fp, ip, lr}
	.setfp fp, ip, #-4
	sub	fp, ip, #4
	.pad #16
	sub	sp, sp, #16
	push	{lr}
	bl	__gnu_mcount_nc
	cmp	r1, #0
	mov	r4, r0
	beq	.L41
	cmp	r1, #1
	bne	.L46
	mov	r0, #0
	mov	r3, r1
	str	r0, [sp]
	ldr	r2, .L48
	ldr	r1, .L48+4
	bl	dprint_vfmw
	mvn	r0, #0
	b	.L43
.L41:
	ldr	r5, .L48+8
	ldr	r3, [r5]
	cmp	r3, #0
	beq	.L47
.L44:
	ldr	lr, .L48+12
	mov	r3, r2
	mov	r1, r4
	mov	r2, #0
	ldr	r0, .L48+8
	ldr	ip, [lr]
	add	ip, ip, #1
	str	ip, [lr]
	bl	RV8HAL_V400R003_WriteReg
	mov	r1, r4
	ldr	r0, .L48+8
	bl	RV8HAL_V400R003_WritePicMsg
	ldr	r0, .L48+8
	mov	r1, r4
	bl	RV8HAL_V400R003_WriteSliceMsg
	mov	r0, #0
.L43:
	sub	sp, fp, #20
	ldmfd	sp, {r4, r5, fp, sp, pc}
.L46:
	ldr	r1, .L48+16
	mov	r0, #0
	bl	dprint_vfmw
	mvn	r0, #0
	sub	sp, fp, #20
	ldmfd	sp, {r4, r5, fp, sp, pc}
.L47:
	mov	r0, #0
	str	r2, [fp, #-24]
	movt	r0, 63683
	bl	MEM_Phy2Vir
	subs	r3, r0, #0
	strne	r3, [r5]
	ldrne	r2, [fp, #-24]
	bne	.L44
.L45:
	ldr	r1, .L48+20
	bl	dprint_vfmw
	mvn	r0, #0
	b	.L43
.L49:
	.align	2
.L48:
	.word	.LANCHOR0+88
	.word	.LC1
	.word	g_HwMem
	.word	.LANCHOR1
	.word	.LC36
	.word	.LC37
	.fnend
	.size	RV8HAL_V400R003_StartDec, .-RV8HAL_V400R003_StartDec
	.section	.rodata
	.align	2
.LANCHOR0 = . + 0
	.type	__func__.15382, %object
	.size	__func__.15382, 25
__func__.15382:
	.ascii	"RV8HAL_V400R003_WriteReg\000"
	.space	3
	.type	__func__.15390, %object
	.size	__func__.15390, 28
__func__.15390:
	.ascii	"RV8HAL_V400R003_WritePicMsg\000"
	.type	__func__.15403, %object
	.size	__func__.15403, 30
__func__.15403:
	.ascii	"RV8HAL_V400R003_WriteSliceMsg\000"
	.space	2
	.type	__func__.15374, %object
	.size	__func__.15374, 25
__func__.15374:
	.ascii	"RV8HAL_V400R003_StartDec\000"
	.data
	.align	2
.LANCHOR1 = . + 0
	.type	FrameNum, %object
	.size	FrameNum, 4
FrameNum:
	.word	-1
	.section	.rodata.str1.4,"aMS",%progbits,1
	.align	2
.LC0:
	.ascii	"configuring VDM registers...\012\000"
	.space	2
.LC1:
	.ascii	"%s: VdhId(%d) > %d\012\000"
.LC2:
	.ascii	"BASIC_V400R003_CFG0=0x%x\012\000"
	.space	2
.LC3:
	.ascii	"BASIC_V400R003_CFG1=0x%x\012\000"
	.space	2
.LC4:
	.ascii	"AVM_V400R003_ADDR=0x%x\012\000"
.LC5:
	.ascii	"VAM_V400R003_ADDR=0x%x\012\000"
.LC6:
	.ascii	"STREAM_V400R003_BASE_ADDR=0x%x\012\000"
.LC7:
	.ascii	"TIME_OUT = 0x%x\012\000"
	.space	3
.LC8:
	.ascii	"YSTADDR_V400R003_1D = 0x%x\012\000"
.LC9:
	.ascii	"YSTRIDE_V400R003_1D = 0x%x\012\000"
.LC10:
	.ascii	"UVOFFSET_V400R003_1D = 0x%x\012\000"
	.space	3
.LC11:
	.ascii	"HEAD_INF_OFFSET = 0x%x\012\000"
.LC12:
	.ascii	"DNR_MBINFO_STADDR=0x%x\012\000"
.LC13:
	.ascii	"REF_V400R003_PIC_TYPE=0x%x\012\000"
.LC14:
	.ascii	"FF_V400R003_APT_EN=0x%x\012\000"
	.space	3
.LC15:
	.ascii	"configuring Pic Msg...\012\000"
.LC16:
	.ascii	"can not map dn msg virtual address!\012\000"
	.space	3
.LC17:
	.ascii	"%s: %s\012\000"
.LC18:
	.ascii	"D0 = 0x%x\012\000"
	.space	1
.LC19:
	.ascii	"D1 = 0x%x\012\000"
	.space	1
.LC20:
	.ascii	"D2 = 0x%x\012\000"
	.space	1
.LC21:
	.ascii	"D3 = 0x%x\012\000"
	.space	1
.LC22:
	.ascii	"D4 = 0x%x\012\000"
	.space	1
.LC23:
	.ascii	"D5 = 0x%x\012\000"
	.space	1
.LC24:
	.ascii	"D6 = 0x%x\012\000"
	.space	1
.LC25:
	.ascii	"D16 = 0x%x\012\000"
.LC26:
	.ascii	"D17 = 0x%x\012\000"
.LC27:
	.ascii	"D18 = 0x%x\012\000"
.LC28:
	.ascii	"D19 = 0x%x\012\000"
.LC29:
	.ascii	"D20 = 0x%x\012\000"
.LC30:
	.ascii	"D21 = 0x%x\012\000"
.LC31:
	.ascii	"D22 = 0x%x\012\000"
.LC32:
	.ascii	"D23 = 0x%x\012\000"
.LC33:
	.ascii	"D024 = 0x%x\012\000"
	.space	3
.LC34:
	.ascii	"D63 = 0x%x\012\000"
.LC35:
	.ascii	"configuring Slice Msg...\012\000"
	.space	2
.LC36:
	.ascii	"VdhId is wrong! RV8HAL_V200R003_StartDec\012\000"
	.space	2
.LC37:
	.ascii	"vdm register virtual address not mapped, reset fail"
	.ascii	"ed!\012\000"
	.ident	"GCC: (gcc-4.9.2 + glibc-2.22 (Build by czyong) Tue Oct 27 16:32:03 CST 2015) 4.9.2 20140904 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
