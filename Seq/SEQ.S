* Gestion des s‚quences.
* Ce module permet de g‚rer un buffer d'‚vŠnements.
* La gestion est faite de la maniŠre suivante: les ‚vŠnements constituent
* une liste simplement chain‚e (suiv). status=0 veut dire que l'‚venement
* n'est pas utilis‚.
* Les ‚vŠnements libres sont aussi organis‚s en liste chain‚e dont le premier
* est "libre" et le dernier est ... "dernier". Quand on ajoute un ‚vŠnement,
* on utilise le premier libre. Quand on supprime un ‚vŠnement, on le rajoute
* aprŠs le dernier.
* Notes: 
* 1 Les ‚vŠnements sont identifi‚s par leur offset, qui est un multiple
* de 4 (taille d'un mot long) pour des raisons de performance. Mais avec 
* les facteurs d'‚chelle du 68030 on pourrait aller plus vite.
* 2 Le consommateur de ce module a la charge de savoir quel est le d‚but
* d'une s‚quence (i.e. le premier ‚l‚ment de la liste chain‚e). Ceci lui est
* retourn‚ aprŠs une insertion. Donc apres la premiere insertion il faut le
* m‚moriser.

	XDEF	seq_init	; void seq_init(void)
	XDEF	seq_deinit	; void seq_deinit(void)
	XDEF	seq_insere	; long seq_insere(ev*,start.l)
	XDEF	seq_supprime	; long seq_supprime(start.l,index.l)

TAILLE_BUFFER	EQU	10000

seq_init: ; void seq_init(void)
	; Efface les buffers
	lea	suiv,a1
	lea	status,a2
	lea	data1,a3
	lea	data2,a4
	lea	timestamp,a5
	move.l	#TAILLE_BUFFER-1,d1
.efface	clr.w	(a1)+
	clr.w	(a2)+
	clr.b	(a3)+
	clr.b	(a4)+
	clr.l	(a5)+
	dbra	d1,.efface
	;
	; Cr‚e la liste chaine d'‚vŠnements libres
	move.l	#1*4,libre	; 0 est reserv‚ car ambigu
	move.l	#TAILLE_BUFFER-3,d2 ; on demarre a 1, on ne fait pas le dernier, et dbra
	moveq	#2*4,d1
	lea	suiv+4,a1
.boucle	move.l	d1,(a1)+
	addq.l	#4,d1
	dbra	d2,.boucle	
	clr.l	(a1)	; le dernier n'a pas de suivant 
	move.l	#(TAILLE_BUFFER-1)*4,dernier
	;
	rts

seq_deinit:
	rts


seq_insere:
	; a0: Pointeur sur une structure d'‚vŠnement
	; d0.w: N'importe quel ‚vŠnement pr‚cedent dans la s‚quence,
	; 0 si inconnu.
	; Retourne l'index du point d'insertion (… utiliser comme d0 dans le prochain appel pour de meilleures performances)
	; Avec les facteurs d'echelle du 68030 on pourrait aller plus vite !
	; Flingue a1,a2,a3,d0,d1,d2,d3,d4
	;
	; Trouve le point d'insertion
	move.l	d0,d1	; d1: indice de boucle (index)
	move.l	(a0),d3	; Tempon dans d3, mode d'addressage plus rapide ?
	; Il y a un prochain ‚vŠnement dans la s‚quence ?
	lea	status,a1
	lea	timestamp,a2
	lea	suiv,a3
	move.l	d0,d4	: d4: index pr‚c‚dent
	; Examine la s‚quence jusqu'… trouver la fin
	; ou un ‚vŠnement dont le tempon est inf‚rieur … celui de l'‚vŠnement … inserer
.boucle	tst.w	(a3,d1.l)	; Si pas de suivant,
	beq.s	.trouve 	; on est … la fin de la s‚quence
	cmp.l	(a2,d1.l),d3
	bmi.s	.trouve ; Le prochain ‚vŠnement est avant
	move.l	d1,d4	; d4: index pr‚c‚dent (il faut s'en souvenir pour r‚gler son suiv)
	move.l	(a3,d1.l),d1 ; Passe au suivant
	bra.s	.boucle
	;
.trouve	; InsŠre les informations dans a0 vers le prochain emplacement libre
	move.l	d1,d2
	lsr.l	#1,d2		; d2: indexeur (mot)
	move.l	libre(pc),d5
	move.l	d5,d0		; C'est ce qu'on va retourner
	beq.s	.plein		; Si index libre = 0 ‡a veut dire plus de m‚moire
	cmp.l	dernier(pc),d5	; Si on utilise le dernier libre, alors dernier = libre = 0
	bne.s	.pasdernier
	clr.l	dernier
	clr.l	libre
	bra.s	.copie
.pasdernier
	move.l	(a3,d5.l),libre	; libre = libre->suiv
.copie	; Copie les donn‚es dans l'emplacement libre
	move.l	(a0),(a2,d5.l)	; Copie timestamp
	move.l	d5,d2
	lsr.l	#1,d2		; d2: offset en word pour status
	move.w	4(a0),(a1,d2.l)	; Copie status
	lea	data1,a1
	lsr.l	#1,d2		: d2: offset en octets pour data
	move.b	6(a0),(a1,d2.l)	: Copie data 1
	lea	data2,a1
	move.b	7(a0),(a1,d2.l)	: Copie data 2
	; Met a jour la liste chainee (a3: suiv)
	move.l	(a3,d4.l),d2	; d3: prec->suiv
	move.l	d5,(a3,d4.l)	: prec->suiv = nouveau
	move.l	d2,(a3,d5.l)	: nouveau->suivant = prec->(ancien)suiv
	;
	clr.l	(a3)		; Ne modifie pas l'index 0, il est reserv‚
.plein	rts
		

seq_supprime:
	; Supprime de la s‚quence d‚marrant a d0.l l'‚vŠnement dont
	; l'index est dans d1.l
	; Examine la s‚quence jusqu'a trouver la fin
	; ou un ‚vŠnement dont le suivant est celui … supprimer
	; d0.l retourne l'index de l'‚vŠnement suivant celui qui a ‚t‚ supprim‚,
	; ou 0 si aucun n'existe
	lea	suiv,a1
	lea	status,a2
	move.l	d0,d2		: d2: iterateur (long)
	cmp.l	d2,d1
	beq.s	.trouve		; On efface le premier de la s‚quence
	;
.boucle	move.l	d2,d3
	lsr.l	#1,d3		: d3: index (mot)
	tst.w	(a2,d3)
	beq.s	.pastrouve	; Status=0, fin de sequence (ev a effacer pas partie de la sequence)
	move.l	(a1,d2.l),d2	; Charge le suivant
	cmp.l	d2,d1
	bne.s	.boucle
	;
.trouve	; d2.l est l'index du pr‚c‚dent (dont il faut maj le suivant)
	move.l	(a1,d1.l),d0	; Retourne le suivant de l'ev effac‚
	move.l	d0,(a1,d2.l)	; Met a jour l'‚vŠnement pr‚c‚dent pour sauter l'ev effac‚
	move.l	d1,d2
	lsr.l	#1,d2
	clr.w	(a2,d2.l)		; Efface status (marque l'ev comme libre)
.majdernierlibre
	clr.l	(a1,d1.l)		; Efface "suivant", puisqu'on devient le dernier
	move.l	dernier(pc),d2	; Ajoute l'‚vŠnement lib‚r‚ a la fin de la liste
	beq.s	.pasdedernier	; des ‚vŠnements libres, et designe le comme
	move.l	d1,(a1,d2.l)	; suivant[dernier] := lib‚r‚
	move.l	d2,dernier 	; dernier := lib‚r‚	
.end	rts
.pasdedernier	; Le buffer ‚tait plein, l'‚vŠnement lib‚r‚ devient le premier et le dernier libre
	move.l	d1,libre
	move.l	d1,dernier
	rts
.pastrouve	; L'‚vŠnement … effacer ne fait pas partie de la s‚quence
	clr.l	d0
	rts

	EVEN
libre	ds.l	1 ; Index*4 du premier libre (on puise dedans)
dernier	ds.l	1 ; Index*4 du dernier libre (on rajoute les ‚vŠnements lib‚r‚s apres)
suiv	ds.l	TAILLE_BUFFER ; Index*4 du suivant dans la sequence, ou 0 si on est le dernier

	; Ci dessous la structure d'un ‚vŠnement
timestamp	ds.l	TAILLE_BUFFER ; Tempons
status	ds.w	TAILLE_BUFFER ; Octet de status ou 00 si inutilise, LSB est l'octet de status MIDI
data1	ds.b	TAILLE_BUFFER ; Premier octet du message MIDI
data2	ds.b	TAILLE_BUFFER ; Deuxieme octet du message MIDI
