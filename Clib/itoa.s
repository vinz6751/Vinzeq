ltoa10	; d0.w entrée, a0: buffer
	tst.l	d0
	smi	d2	; d2: negative flag
	bpl.s	.pos
	neg.l	d0
.pos	move.l	a0,a1
	lea	.data(pc),a2

	move.l	d0,d3
	
.loop	divu.w	#10,d3
	move.w	d3,d1
	swap	d3
	move.b	(a2,d3.w),(a1)+
	move.w	d1,d3
	ext.l	d3
	beq.s	.1
	bra.s	.loop	
	
.1	tst.w	d2
	beq.s	.2
	move.b	#'-',(a1)+
.2	clr.b	(a1)
	jmp	strrev
.data	dc.b	'0123456789'