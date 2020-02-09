; Routines to receive a MIDI byte and fire callbacks.
; Public domain
; Vincent Barrilliot 09-Feb-2020

; See MIDIMSG.H.
; How to use:
; After initializing, you can set callbacks functios for all MIDI events.
; By default it's doing nothing (rts).
; There is a public reference to midimsg_callbacks which is a
; MIDIMSG_CALLBACKS structure.
; Then you can call repeatedly midimsg_process with the incoming byte, and
; each time a MIDI message is recognized, the correspoding callback will be
; fired.
;
; How it works: we detect if a new message starts or not. If the byte is >=$80
; it means a new message starts. We process the realtime and system common
; messages first because they contain synchronisation information (clock, song
; position, MTC etc.).
; When a message starts we find a chain of functions that will take actions for
; each received byte of that message. For instance for note on, the first function
; will store the channel, the second function will store the note number, and
; the third function fires the callback. For this purpose we have a next_store
; pointer which points to then next function to call.
; Running status is managed simply by setting the next_store to the 2nd
; function of the chain. If running status is not allowed (e.g. for sysex)
; then the next_store points to a method which fires the error callback.
; If there is a problem, the error callback is called with an error code 
; described below.
;
; This code is written for Brainstorm Assemble, which is so much better than
; PASM (the Pure C's assembler). PASM has problems with the macro.

; Error codes
MESSAGE_ABORTED	EQU	1
UNEXPECTED_DATA	EQU	2
SYSEX_TOO_LARGE	EQU	3

DEBUG	EQU	0	; Set this 1 to to have extended debug info

	OUTPUT	midimsg.o
	OPT 	L1; Pure C object format
	OPT	O+ OW+; All optimisations, all warnings
	MC68000	; This code is 68000 friendly
	USER	; We're not using privileged instructions

	IF	DEBUG
	OPT	E+,X+,Y+; All debug symbols
	ELSE
	OPT	E-,X-,Y-; All debug symbols
	ENDIF
	
	TEXT

SEND_ERROR	MACRO
	move.w	#\1,d0
	move.l	midimsg_callbacks+0(pc),a1
	jmp	(a1)
	ENDM

empty_void:
	rts

err_message_unexpected_data:
	SEND_ERROR	UNEXPECTED_DATA

noteoff_channel:
	move.b	d0,note_off
	move.l	#noteoff_note,store_next
	rts
noteoff_note:
	move.b	d0,note_off+1
	move.l	#noteoff_velocity,store_next
	rts
noteoff_velocity:
	move.b	d0,note_off+2
	move.l	#noteoff_note,store_next
	lea.l	note_off,a0
	move.l	midimsg_callbacks+4(pc),a1
	jmp 	(a1)

noteon_channel:
	move.b	d0,note_on
	move.l	#noteon_note,store_next
	rts
noteon_note:
	move.b	d0,note_on+1
	move.l	#noteon_velocity,store_next
	rts
noteon_velocity:
	move.b	d0,note_on+2
	move.l	#noteon_note,store_next
	lea	note_on,a0
	move.l	midimsg_callbacks+8(pc),a1
	jmp 	(a1)

polyp_channel:
	move.b	d0,poly_pressure
	move.l	#polyp_note,store_next
	rts
polyp_note:
	move.b	d0,poly_pressure+1
	move.l	#polyp_value,store_next
	rts
polyp_value:
	move.b	d0,poly_pressure+2
	move.l	#polyp_note,store_next
	lea	poly_pressure,a0
	move.l	midimsg_callbacks+12(pc),a1
	jmp	(a1)

controlc_channel:
	move.b	d0,control_change
	move.l	#controlc_control,store_next
	rts
controlc_control:
	move.b	d0,control_change+1
	move.l	#controlc_value,store_next
	rts
controlc_value:
	move.b	d0,control_change+2
	move.l	#controlc_control,store_next
	lea	control_change,a0
	move.l 	midimsg_callbacks+16(pc),a1
	jmp	(a1)

programc_channel:
	move.b	d0,program_change
	move.l	#programc_program,store_next
	rts
programc_program:
	move.b	d0,program_change+1
	move.l	#programc_program,store_next
	lea	program_change,a0
	move.l	midimsg_callbacks+20(pc),a1
	jmp	(a1)

channelp_channel:
	move.b	d0,channel_pressure
	move.l	#channelp_value,store_next
	rts
channelp_value:
	move.b	d0,channel_pressure+1
	move.l	#channelp_value,store_next
	lea	channel_pressure,a0
	move.l	midimsg_callbacks+24(pc),a1
	jmp	(a1)

pitchb_channel:
	move.b	d0,pitch_bend
	move.l	#pitchb_lsb,store_next
	rts
pitchb_lsb:
	ext.w	d0
	move.w	d0,pitch_bend+2
	move.l	#pitchb_msb,store_next
	rts
pitchb_msb:
	ext.w	d0
	lsl.w	#7,d0
	or.w	d0,pitch_bend+2
	move.l	#pitchb_lsb,store_next
	lea	pitch_bend,a0
	move.l	midimsg_callbacks+28(pc),a1
	jmp	(a1)

clock:
	move.l	midimsg_callbacks+32(pc),a0
	jmp	(a0)

song_start:
	move.l	midimsg_callbacks+36(pc),a0
	jmp (a0)

song_continue:
	move.l	midimsg_callbacks+40(pc),a0
	jmp	(a0)

song_stop:
	move.l	midimsg_callbacks+44(pc),a0
	jmp	(a0)

active_sensing:
	move.l	midimsg_callbacks+48(pc),a0
	jmp 	(a0)

reset:
	move.l	midimsg_callbacks+52(pc),a0
	jmp 	(a0)

mtc_quarter_frame_status:
	move.l	#mtc_quarter_frame_data,store_next
	rts

mtc_quarter_frame_data: ; TODO
	move.b	d0,d1
	and.b	#15,d1
	move.b	d1,mtc_quarter_frame+1
	lea	mtc_quarter_frame,a0
	and.b 	#112,d0
	move.b 	d0,(a0)
	move.l 	#err_message_unexpected_data,store_next
	move.l 	midimsg_callbacks+60(pc),a1
	jmp 	(a1)

song_position_status:
	move.l 	#song_position_lsb,store_next
	rts
song_position_lsb:
	move.b 	d0,song_position
	move.l 	#song_position_msb,store_next
	rts
song_position_msb:
	ext.w 	d0
	lsl.w 	#7,d0
	or.b 	song_position,d0
	move.l 	#err_message_unexpected_data,store_next
	move.l 	midimsg_callbacks+64(pc),a1
	jmp 	(a1)

song_select_status:
	move.l 	#song_select_number,store_next
	rts
song_select_number:
	ext.w 	d0
	move.l 	midimsg_callbacks+68(pc),a1
	jmp 	(a1)

tune_request:
	move.l 	midimsg_callbacks+72(pc),a1
	jmp 	(a1)

sysex:
	move.w 	system_exclusive(pc),d2	; length
	cmp.b 	#$f0,d0
	beq.s 	.start_sysex

.check_max_size:
	cmp.w 	sysex_max_size(pc),d2
	blt.s 	.ok_to_store
	tst.b 	sysex_errored ;too big, raise error if not already done
	beq.s 	.sysex_error

.test_eox:
	cmp.b 	#$f7,d0
	beq.s 	.eox
.end:	rts
.start_sysex:
	tst.w 	d2 ;message already in progress ?
	bne.s 	.fake_eox
	clr.b 	sysex_errored
	clr.w 	system_exclusive ;length
	move.l 	#sysex,store_next
	bra.s 	.check_max_size

.fake_eox:
	; Start of sys-ex auto-termiantes any sys-ex in progress,
	; so we send ourselves a fake EOX (F7)
	move.w 	d0,-(sp)
	move.w 	#$f7,d0
	bsr.s 	sysex
	move.w 	(sp)+,d0
	clr.b 	sysex_errored
	clr.w 	system_exclusive; length
	move.l 	#sysex,store_next
	bra.s 	.check_max_size
.ok_to_store: ; L42
	move.l	system_exclusive+2(pc),a0 ; buffer
	move.b	d0,(a0,d2.w)
	addq.w	#1,d2
	move.w	d2,system_exclusive; length
	cmp.b	#$f7,d0
	bne.s	.end2
.eox:	tst.b	sysex_errored
	beq.s	.fire_sysex
	clr.w	system_exclusive; length
	move.l	#err_message_unexpected_data,store_next
.end2:	rts

.sysex_error: ; L47
	move.b	#1,sysex_errored
	move.w	#SYSEX_TOO_LARGE,d0
	move.l	midimsg_callbacks+0(pc),a1
	jmp	(a1)
	bra	.test_eox

.fire_sysex:
	lea	system_exclusive(pc),a0
	move.l	midimsg_callbacks+56(pc),a1
	jsr	(a1)
	clr.w	system_exclusive ;length
	move.l	#err_message_unexpected_data,store_next
	bra.s	.end2


	EVEN
	XDEF	midimsg_init
midimsg_init:
	move.w	d0,sysex_max_size
	move.l	a0,system_exclusive+2 ; buffer
	clr.b	sysex_errored
	clr.w	system_exclusive ; length
	move.l	#empty_void,d0
	move.l	d0,midimsg_callbacks+8
	move.l	d0,midimsg_callbacks+4
	move.l	d0,midimsg_callbacks+12
	move.l	d0,midimsg_callbacks+16
	move.l	d0,midimsg_callbacks+20
	move.l	d0,midimsg_callbacks+24
	move.l	d0,midimsg_callbacks+28
	move.l	d0,midimsg_callbacks+32
	move.l	d0,midimsg_callbacks+36
	move.l	d0,midimsg_callbacks+40
	move.l	d0,midimsg_callbacks+44
	move.l	d0,midimsg_callbacks+48
	move.l	d0,midimsg_callbacks+52
	move.l	d0,midimsg_callbacks+56
	move.l	d0,midimsg_callbacks+60
	move.l	d0,midimsg_callbacks+64
	move.l	d0,midimsg_callbacks+68
	move.l	err_message_unexpected_data(pc),store_next
	rts

	XDEF	midimsg_exit
midimsg_exit:
	rts

	XDEF	midimsg_process
midimsg_process:
	; Test if new message
	cmp.b	#$f8,d0
	bhs.s	.realtime
	cmp.b	#$f0,d0
	bhs.s	.system
	tst.b	d0	;  $80 ?
	blt.s	.channel
	; Not a new message, continue building current one
	movea.l	store_next,a1
	cmpa.w	#0,a1
	beq.s	.unexpected_data
	and.w	#$ff,d0
	jmp	(a1)
.unexpected_data:
	SEND_ERROR	UNEXPECTED_DATA

.realtime:
	and.w	#$ff,d0
	subi.b	#$f8,d0
	add.w	d0,d0
	add.w	d0,d0
	lea.l	realtime_msg_store,a1
	movea.l	(a1,d0.w),a1
	jmp	(a1)

.system: ; d0: start of system common message
	and.w	#$ff,d0
	move.w	d0,d1
	subi.b	#$F0,d1
	add.w	d1,d1
	add.w	d1,d1
	movea.l #common_msg_store,a1
	movea.l	(a1,d1.w),a1
	jmp	(a1)

.channel: ; d0: start of channel message
	and.w	#$ff,d0
	move.w	d0,d1
	subi.b	#$80,d1
	asr.w	#4,d1
	add.w	d1,d1
	add.w	d1,d1
	movea.l	#channel_msg_store,a1
	movea.l	(a1,d1.w),a1
	jmp	(a1)

	XDEF midimsg_callbacks
midimsg_callbacks:	ds.l 19*4;we have 19 callbacks
sysex_max_size:		ds.w 1
sysex_errored:		ds.w 1 ;.w for alignment
system_exclusive:	ds.w 1 ;length
			ds.l 1 ;buffer

	SECTION DATA
	EVEN
	; Functions in charge of handling reception of byte
realtime_msg_store:
	dc.l	clock
	dc.l	empty_void
	dc.l	song_start
	dc.l	song_continue
	dc.l	song_stop
	dc.l	empty_void
	dc.l	active_sensing
	dc.l	reset
common_msg_store:
	dc.l	sysex
	dc.l	mtc_quarter_frame_status
	dc.l	song_position_status
	dc.l	song_select_status
	dc.l	empty_void
	dc.l	empty_void
	dc.l	tune_request
	dc.l	sysex
channel_msg_store:
	dc.l	noteoff_channel
	dc.l	noteon_channel
	dc.l	polyp_channel
	dc.l	controlc_channel
	dc.l	programc_channel
	dc.l	channelp_channel
	dc.l	pitchb_channel

	SECTION BSS
store_next:	ds.l 	1 ;function to call to store the next received byte
pitch_bend:		ds.b	3 ;messages being received. Careful with
song_position:		ds.b	1 ;alignment of word values (pitch bend)
mtc_quarter_frame:	ds.b	2 
channel_pressure: 	ds.b	3
program_change:		ds.b	3
control_change:		ds.b	3
poly_pressure:		ds.b	3
note_on:		ds.b	3
note_off:		ds.b	3