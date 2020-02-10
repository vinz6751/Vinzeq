/* Tests for midimsg */

#include <stdio.h>
#include <tos.h>

#include "midimsg.h"

static void midi_error(short code) {
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

/* Channel messages */
static void note_off(MIDIMSG_NOTE_OFF *msg) {
	printf("%04lu Note Off     : %2x %2x %2x\n", msg->timestamp, msg->channel, msg->note, msg->velocity);
}

static void note_on(MIDIMSG_NOTE_ON *msg) {
	printf("%04lu Note On      : %2x %2x %2x\n", msg->timestamp, msg->channel, msg->note, msg->velocity);
}

static void polyp(MIDIMSG_POLY_PRESSURE *msg) {
	printf("%04lu Poly pressure: %2x %2x %2x\n", msg->timestamp, msg->channel, msg->note, msg->value);
}

static void controlc(MIDIMSG_CONTROL_CHANGE *msg) {
	printf("%04lu Ctrl change  : %2x %2x %2x\n", msg->timestamp, msg->channel, msg->control, msg->value);
}

static void programc(MIDIMSG_PROGRAM_CHANGE *msg) {
	printf("%04lu Prgrm change : %2x %2x\n", msg->timestamp, msg->channel, msg->program);
}

static void aftertouch(MIDIMSG_CHANNEL_PRESSURE *msg) {
	printf("%04lu Aftertouch   : %2x %2x\n", msg->timestamp, msg->channel, msg->value);
}

static void pitchbend(MIDIMSG_PITCH_BEND *msg) {
	printf("%04lu Pitch bend   : %2x %5d\n", msg->timestamp, msg->channel, msg->value);
}

/* System realtime message */
static void clock(TIMESTAMP ts) {
	printf("%04lu Clock\n",ts);
}

static void song_start(TIMESTAMP ts) {
	printf("%04lu Song start\n", ts);
}

static void song_continue(TIMESTAMP ts) {
	printf("%04lu Song continue\n", ts);
}

static void song_stop(TIMESTAMP ts) {
	printf("%04lu Song stop\n", ts);
}

static void active_sensing(TIMESTAMP ts) {
	printf("%04lu Active sensing\n", ts);
}

static void reset(TIMESTAMP ts) {
	printf("%04lu Reset\n", ts);
}

/* System common messages */
static void system_exclusive(MIDIMSG_SYSEX *sysex) {
	short i;
	
	printf("%04lu Sysex : ", sysex->timestamp);
	for (i=0; i<sysex->length; i++)
		printf("%2x ", sysex->data[i]);
	printf("\n");
}

static void mtc_quarter_frame(MIDIMSG_MTC_QUARTER_FRAME *mtc)
{
	printf("%04lu MTC 1/4 frame: T:%d V:%d\n", mtc->timestamp, mtc->type, mtc->value);
}

static void song_position(MIDIMSG_SONG_POSITION *pos)
{
	printf("%04lu Song position: %4x\n", pos->timestamp, pos->position);
}

static void song_select(MIDIMSG_SONG_SELECT *song)
{
	printf("%04lu Song selection: %d\n", song->timestamp, song->song);
}

static void tune_request(TIMESTAMP ts)
{
	printf("%04lu Tune request\n", ts);
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
		0xF0, 0x41, 0xF7, 
		0xF0, 0x41, 0x10, 0x42, 0x12, 0x50, 0x50, 0x30, 0x23, 0xF7, /* Sysex */
		0xF0, 0x41, 0x10, 0x42, 0x12, 0x50, 0x50, 0x30, 0x23, 0x10, 0x10, 0xF7, /* Sysex too long */
		0xF0, 0x12, 0x30, 0xF0, 0x32, 0x50, 0xF7, /* New sysex auto terminates other one */
		0xF1, 0x42, /* Midi Time Code Quarter Frame */
		0xF2, 0x00, 0x00, /* Song pointer (mini) */
		0xF2, 0x7f, 0x7f, /* Song pointer (maxi) */
		0x45, /* Unexpected data */
		0xF3, 0x02, /* Song select */
		0xF6 /* Tune request */
	};
	
	UBYTE sysex_buffer[10];
	short i;
	TIMESTAMP ts;
	(void)argc;
    (void)argv;

	Cconws("MIDIMSG tests.\r\n");
	Cconws("TIME Message\r\n");

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
    midimsg_callbacks.mtc_quarter_frame = mtc_quarter_frame;
    midimsg_callbacks.song_position = song_position;
    midimsg_callbacks.song_select = song_select;
    midimsg_callbacks.tune_request = tune_request;
    
    /* Send the bytes to midimsg_process and let it fire callbacks */
    for (i=0; i<sizeof(data)/sizeof(UBYTE); i++)
    {
    	ts = i+1;
    	midimsg_process(data[i], ts);
    }

	/* Flush MIDI IN then keyboard */
	while (Bconstat(3))
		Bconin(3);
	while (Cconis())
		Cconin();
	Cconws("Press a key to start monitoring MIDI input.\r\n");
	Cnecin();
	Cconws("\33EMonitoring MIDI input. Press a key to exit !\r\n");
	while (!Cconis()) {
		if (Bconstat(3)) {
			short b = Bconin(3) & 0xff;
			midimsg_process(b, ++ts);
		}
	}
		

    midimsg_exit();
    
	Cconws("Press a key.");
    Cnecin();
    return 0;
}
