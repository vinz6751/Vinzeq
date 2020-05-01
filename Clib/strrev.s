strrev	movea.l	a0,a1	; a0: chaine; a1:depuis le debut
	movea.l	a0,a2	; a2: depuis la fin
	move.l	a0,d2	; teste a0
	beq.s	.end
.chrchfin	tst.b	(a2)+
	bne.s	.chrchfin
	suba.l	#1,a2
.boucle	move.l	a1,d1
	cmp.l	a2,d1	; on ne peut pas comparer 2 registres d'adresse
	bcc.s	.end
	move.b	-(a2),d0
	move.b	(a1),(a2)
	move.b	d0,(a1)+
	bra.s	.boucle
.end	move.l	a0,d0