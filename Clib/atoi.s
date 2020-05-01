atoi    ; Convertit la chaine passée sur la pile en entier signé retourné dans d0.
        move.l  4(sp),a1
        
        ; Skip initial white space
.ltrim  move.b  (a1),d1
        cmp.b   #32,d1
        beq.s   .ltrim1
        cmp.b   #13,d1
        bgt.s   .ltrimmed
        cmp.b   #9,d1
        blt.s   .ltrimmed
.ltrim1 adda.l  #1,a1
        bra.s   .ltrim
.ltrimmed       
        
        ; Skip any "+" sign
        cmp.b   #'+',(a1)
        bne.s   .plusskiped
        adda.l  #1,a1
.plusskiped
        
        ; Detect "-" sign, and remember number will be negative
        cmp.b   #'-',(a1)
        seq     d2      ; d2: negative flag
        bne.s   .signdetected
        adda.l  #1,a1
.signdetected
        
        sub.l   d0,d0
.dgtloop        move.b  (a1)+,d1
        beq.s   .stop
        ; Is digit ?
        sub.b   #'0',d1
        cmp.b   #'9',d1
        bgt.s   .stop
        ; d0 = d0*10 + d1
        move.l  d0,d3
        lsl.l   #3,d0
        add.l   d3,d0
        add.l   d3,d0
        add.l   d1,d0
        bra.s   .dgtloop
.stop   
        ; If there was a - sign, make the number negative
        tst.b   d2
        beq.s   .end
        neg.l   d0

.end    rts