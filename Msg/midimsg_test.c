/* Tests for midimsg */

#include <stdio.h>

#include "midimsg.h"

static void midi_error(int code) {
    switch (code) {
    case MIDIMSG_MESSAGE_ABORTED:
	printf("Message aborted !\n");
	break;
    case MIDIMSG_UNEXPECTED_DATA:
	printf("Unexpected data !\n");
	break;
    case MIDIMSG_SYSEX_TOO_LARGE:
	printf("Sysex too large !\n");
	break;
    default:
	printf("Unknown error message %d\n !");
    }
}

static void note_off(MIDIMSG_NOTE_OFF *msg) {
    printf("Note Off     : %2x %2x %2x\n", msg->channel, msg->note, msg->velocity);
}

static void note_on(MIDIMSG_NOTE_ON *msg) {
    printf("Note On      : %2x %2x %2x\n", msg->channel, msg->note, msg->velocity);
}

static void polyp(MIDIMSG_POLY_PRESSURE *msg) {
    printf("Poly pressure: %2x %2x %2x\n", msg->channel, msg->note, msg->value);
}

static void controlc(MIDIMSG_CONTROL_CHANGE *msg) {
    printf("Ctrl change  : %2x %2x %2x\n", msg->channel, msg->control, msg->value);
}

static void programc(MIDIMSG_PROGRAM_CHANGE *msg) {
    printf("Prgrm change : %2x %2x\n", msg->channel, msg->program);
}

static void aftertouch(MIDIMSG_CHANNEL_PRESSURE *msg) {
    printf("Aftertouch   : %2x %2x\n", msg->channel, msg->value);
}

static void pitchbend(MIDIMSG_PITCH_BEND *msg) {
    printf("Pitch bend   : %2x %5d\n", msg->channel, msg->value);
}

/* System realtime message */
static void clock(void) {
    printf("Clock\n");
}

static void song_start(void) {
    printf("Song start\n");
}

static void song_continue(void) {
    printf("Song continue\n");
}

static void song_stop(void) {
    printf("Song stop\n");
}

static void active_sensing(void) {
    printf("Active sensing\n");
}

static void reset(void) {
    printf("Reset\n");
}

static void system_exclusive(MIDIMSG_SYSEX *sysex) {
    printf("Sysex        : ");
    for (int i; i<sysex->length; i++)
	printf("%2x ", sysex->data[i]);
    printf("\n");
}


int main(int argc, char *argv[]) {
    UBYTE data[] = {
	0x90, 0x40, 0xF8, 0x7F, /* Clock in note on */
	0x80, 0x40, 0x00, /* Note off */
	0x90, 0x36, 0x20, 0x36, 0x00, /* Note on with running status */
	0xA0, 0x40, 0x20, /* Poly pressure */
	0x40, 0x22, 0x42, 0x23, /* Poly pressure running status */
	0xB0, 0x07, 0x7F, /* Control change */
	0x40, 0x00, /* Control change running status */
	0xC0, 0x12, /* Program change */
	0x13, /* Program change running status */
	0xD0, 0x45, /* Aftertouch (channel pressure) */
	0x46, 0x47, /* Aftertouch running status */
	0xE5, 0x00, 0x40, /* Pitch bend (center) */
	0x00, 0x00, /* Pitch bend (mini) with running status */
	0x7f, 0x7f, /* Pitch bend (maxi) with running status */
	0xFA, /* Song start */
	0xFB, /* Song continue */
	0xFC, /* Song stop */
	0xFE, /* Active sensing */
	0xFF, /* Reset */
	0xF0, 0x41, 0x10, 0x42, 0x12, 0x50, 0x50, 0x30, 0x23, 0xF7, /* Sysex */
	0xF0, 0x41, 0x10, 0x42, 0x12, 0x50, 0x50, 0x30, 0x23, 0x10, 0x10, 0xF7, /* Sysex too long */
	0xF0, 0x12, 0x30, 0xF0, 0x32, 0x50, 0xF7 /* New sysex auto terminates other one */
    };

    UBYTE sysex_buffer[10];
    (void)argc;
    (void)argv;

    midimsg_init(sysex_buffer, sizeof(sysex_buffer)/sizeof(UBYTE));

    /* Set callbacks */
    midimsg_callbacks.error = midi_error;
    midimsg_callbacks.note_off = note_off;
    midimsg_callbacks.note_on = note_on;
    midimsg_callbacks.poly_pressure = polyp;
    midimsg_callbacks.clock = clock;
    midimsg_callbacks.control_change = controlc;
    midimsg_callbacks.program_change = programc;
    midimsg_callbacks.channel_pressure = aftertouch;
    midimsg_callbacks.pitch_bend = pitchbend;
    midimsg_callbacks.song_start = song_start;
    midimsg_callbacks.song_continue = song_continue;
    midimsg_callbacks.song_stop = song_stop;
    midimsg_callbacks.active_sensing = active_sensing;
    midimsg_callbacks.reset = reset;
    midimsg_callbacks.system_exclusive = system_exclusive;

    

    /* Send the bytes to midimsg_process and let it fire callbacks */
    for (int i=0; i<sizeof(data)/sizeof(UBYTE); i++)
    {
	midimsg_process(data[i]);
    }

    midimsg_exit();
    
    return 0;
}
