	.include "asm/macros/function.inc"
	.include "constants/gba_constants.inc"
	.text
	.syntax unified

	arm_func_start _start
_start: @ 0x02000000
	b _init
	.global RomHeaderNintendoLogo
RomHeaderNintendoLogo:
	.space 156

RomHeaderGameTitle:
	.space 12

	.global RomHeaderGameCode
RomHeaderGameCode:
	.space 4

RomHeaderMakerCode:
	.space 2

RomHeaderMagic:
	.byte 0

RomHeaderMainUnitCode:
	.byte 0

RomHeaderDeviceType:
	.byte 0

RomHeaderReserved1:
	.space 7

	.global RomHeaderSoftwareVersion
RomHeaderSoftwareVersion:
	.byte 0

RomHeaderChecksum:
	.byte 0

RomHeaderReserved2:
	.space 2
_init:
	mov r0, #0x12
	msr cpsr_fc, r0
	ldr sp, _020000F8 @=0x03007fa0
	mov r0, #0x1f
	msr cpsr_fc, r0
	ldr sp, _020000F4 @=0x03007e60
	adr r1, PrepareBoot
	orr r1, r1, #0x01
	mov lr, pc
	bx r1
    ldr r0, Bottom
	bl DecompressBLZ
	ldr r1, _020000FC @=INTR_VECTOR
	adr r0, _intr
	str r0, [r1]
	ldr r1, _02000100 @=AgbMain
	mov lr, pc
	bx r1
	b _init
	.align 2, 0

_020000F4: .4byte 0x03007e60
_020000F8: .4byte 0x03007fa0
_020000FC: .4byte INTR_VECTOR
_02000100: .4byte AgbMain
    .global Bottom
Bottom: 
    .word 0

	arm_func_start _intr
_intr: @ 0x02000104
	mov r3, #0x4000000
	add r3, r3, #0x200
	ldr r2, [r3]
	ldrh r1, [r3, #8]
	mrs r0, spsr
	push {r0, r1, r2, r3, lr}
	mov r0, #1
	strh r0, [r3, #8]
	and r1, r2, r2, lsr #16
	mov ip, #0
	ands r0, r1, #INTR_FLAG_VCOUNT
	bne _020001D4
	add ip, ip, #4
	ands r0, r1, #INTR_FLAG_SERIAL
	bne _020001D4
	add ip, ip, #4
	ands r0, r1, #INTR_FLAG_TIMER3
	bne _020001D4
	add ip, ip, #4
	ands r0, r1, #INTR_FLAG_HBLANK
	bne _020001D4
	add ip, ip, #4
	ands r0, r1, #INTR_FLAG_VBLANK
	bne _020001D4
	add ip, ip, #4
	ands r0, r1, #INTR_FLAG_TIMER0
	bne _020001D4
	add ip, ip, #4
	ands r0, r1, #INTR_FLAG_TIMER1
	bne _020001D4
	add ip, ip, #4
	ands r0, r1, #INTR_FLAG_TIMER2
	bne _020001D4
	add ip, ip, #4
	ands r0, r1, #INTR_FLAG_DMA0
	bne _020001D4
	add ip, ip, #4
	ands r0, r1, #INTR_FLAG_DMA1
	bne _020001D4
	add ip, ip, #4
	ands r0, r1, #INTR_FLAG_DMA2
	bne _020001D4
	add ip, ip, #4
	ands r0, r1, #INTR_FLAG_DMA3
	bne _020001D4
	add ip, ip, #4
	ands r0, r1, #INTR_FLAG_KEYPAD
	bne _020001D4
	add ip, ip, #4
	ands r0, r1, #INTR_FLAG_GAMEPAK
	strbne r0, [r3, #-0x17c]
_020001D0:
	bne _020001D0
_020001D4:
	strh r0, [r3, #2]
	mov r1, #0x20c0
	bic r2, r2, r0
	and r1, r1, r2
	strh r1, [r3]
	mrs r3, apsr
	bic r3, r3, #0xdf
	orr r3, r3, #0x1f
	msr cpsr_fc, r3
	ldr r1, =gIntrTable
	add r1, r1, ip
	ldr r0, [r1]
	stmdb sp!, {lr}
	adr lr, _intr_ret
	bx r0
_intr_ret: @ 0x02000210
	ldm sp!, {lr}
	mrs r3, apsr
	bic r3, r3, #0xdf
	orr r3, r3, #0x92
	msr cpsr_fc, r3
	pop {r0, r1, r2, r3, lr}
	strh r2, [r3]
	strh r1, [r3, #8]
	msr spsr_fc, r0
	bx lr
	.align 2, 0
	.pool

    thumb_func_start PrepareBoot
PrepareBoot:
	push	{lr}
	movs	r0, #128
	lsls	r0, r0, #20
	mov	r1, pc
	cmp	r1, r0
	bcc	1f
	ldr	r1, begin
	ldr	r3, end
	subs	r3, r3, r1
	lsrs	r2, r3, #31
	adds	r2, r2, r3
	lsls	r2, r2, #10
	lsrs	r2, r2, #11
	bl	CpuSet
	ldr	r0, begin
	bx	r0
1:
	pop	{r0}
	bx	r0
	.align 2, 0
begin:
	.word	__begin__
end:
	.word	__end__

arm_func_start DecompressBLZ
DecompressBLZ:
	cmp	r0, #0
	beq	exit
	stmfd	sp!, {r4-r7}
	ldmdb	r0, {r1-r2}
	add	r2, r0, r2
	sub	r3, r0, r1, LSR #24
	bic	r1, r1, #0xff000000
	sub	r1, r0, r1
	mov	r4, r2
loop:
	cmp	r3, r1            // Exit if r3==r1
	ble	end_loop
	ldrb	r5, [r3, #-1]!       // r4 = compress_r5 = *--r3
	mov	r6, #8
loop8:
	subs	r6, r6, #1
	blt	loop
	tst	r5, #0x80
	bne	blockcopy
bytecopy:
	ldrb	r0, [r3, #-1]!
	strb	r0, [r2, #-1]!      // Copy 1 byte
	b	joinhere
blockcopy:
	ldrb	r12, [r3, #-1]!
	ldrb	r7, [r3, #-1]!
	orr	r7, r7, r12, LSL #8
	bic	r7, r7, #0xf000
	add	r7, r7, #0x0002
	add	r12, r12, #0x0020
patterncopy:
	ldrb	r0, [r2, r7]
	strb	r0, [r2, #-1]!
	subs	r12, r12, #0x0010
	bge	patterncopy

joinhere:
	cmp	r3, r1
	mov	r5, r5, LSL #1
	bgt	loop8
end_loop:
	
	ldmfd	sp!, {r4-r7}
exit:
	bx	lr

