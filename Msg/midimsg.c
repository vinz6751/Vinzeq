/* This module gets a stream of midi bytes and analyses it.
 * It calls callback functions whenever a message is received.
 * This is nothing Atari specific.
 */

#include <stdio.h>

#include "midimsg.h"

/* These are structures which get filled up with bytes received, and which
 * are then passed to the user's callback functions */
static MIDIMSG_NOTE_ON note_on;
static MIDIMSG_NOTE_OFF note_off;
static MIDIMSG_POLY_PRESSURE poly_pressure;
static MIDIMSG_CONTROL_CHANGE control_change;
static MIDIMSG_PROGRAM_CHANGE program_change;
static MIDIMSG_CHANNEL_PRESSURE channel_pressure;
static MIDIMSG_PITCH_BEND pitch_bend;
static int sysex_max_size;
static MIDIMSG_SYSEX system_exclusive;
static int sysex_errored;
static MIDIMSG_MTC_QUARTER_FRAME mtc_quarter_frame;
static UWORD song_position;

MIDIMSG_CALLBACKS midimsg_callbacks = {
    0L, /* Error */
    0L,	/* Note off */
    0L, /* Note on */
    0L, /* Polyphonic pressure */
    0L, /* Control change */
    0L, /* Program change */
    0L, /* Channel pressure */
    0L, /* Pitch bend */
};

/* Channel message storage functions */
static void noteoff_channel(UBYTE);
static void noteoff_note(UBYTE);
static void noteoff_velocity(UBYTE);
static void noteon_channel(UBYTE);
static void noteon_note(UBYTE);
static void noteon_velocity(UBYTE);
static void polyp_channel(UBYTE);
static void polyp_note(UBYTE);
static void polyp_value(UBYTE);
static void controlc_channel(UBYTE);
static void controlc_control(UBYTE);
static void controlc_value(UBYTE);
static void programc_channel(UBYTE);
static void programc_program(UBYTE);
static void channelp_channel(UBYTE);
static void channelp_value(UBYTE);
static void pitchb_channel(UBYTE);
static void pitchb_lsb(UBYTE);
static void pitchb_msb(UBYTE);

/* System real time message storage functions */
static void clock(void);
static void song_start(void);
static void song_continue(void);
static void song_stop(void);
static void active_sensing(void);
static void reset(void);

/* System common storage functions */
static void sysex(UBYTE);
static void mtc_quarter_frame_status(UBYTE);
static void mtc_quarter_frame_data(UBYTE);
static void song_position_status(UBYTE);
static void song_position_lsb(UBYTE);
static void song_position_msb(UBYTE);
static void song_select_status(UBYTE);
static void song_select_number(UBYTE);
static void tune_request(UBYTE);

/* Error callbacks (to be called next time we receive a byte after we've detected something wrong */
static void err_message_aborted(UBYTE);
static void err_message_unexpected_data(UBYTE);
/* Empty callbacks */
static void reset_callbacks(void);
static void empty_void(void) { }
static void empty_byte(UBYTE whatever) { }

static void (*store_next)(UBYTE) = err_message_aborted;
static void (*channel_msg_store[])(UBYTE) =
{
    noteoff_channel, 
    noteon_channel,
    polyp_channel,
    controlc_channel,
    programc_channel,
    channelp_channel,
    pitchb_channel
};
static void (*realtime_msg_store[])(void) =
{
    clock,
    empty_void,
    song_start,
    song_continue,
    song_stop,
    empty_void,
    active_sensing,
    reset
};
static void (*common_msg_store[])(UBYTE) =
{
    sysex,
    mtc_quarter_frame_status,
    song_position_status,
    song_select_status,
    empty_byte,
    empty_byte,
    tune_request,
    sysex
};

static void reset_callbacks(void)
{
    midimsg_callbacks.note_off = (void(*)(MIDIMSG_NOTE_OFF*))empty_void;
    midimsg_callbacks.note_on = (void(*)(MIDIMSG_NOTE_ON*))empty_void;
    midimsg_callbacks.poly_pressure= (void(*)(MIDIMSG_POLY_PRESSURE*))empty_void;
    midimsg_callbacks.control_change = (void(*)(MIDIMSG_CONTROL_CHANGE*))empty_void;
    midimsg_callbacks.program_change = (void(*)(MIDIMSG_PROGRAM_CHANGE*))empty_void;
    midimsg_callbacks.channel_pressure = (void(*)(MIDIMSG_CHANNEL_PRESSURE*))empty_void;
    midimsg_callbacks.pitch_bend = (void(*)(MIDIMSG_PITCH_BEND*))empty_void;

    midimsg_callbacks.clock = empty_void;
    midimsg_callbacks.song_start = empty_void;
    midimsg_callbacks.song_continue = empty_void;
    midimsg_callbacks.song_stop = empty_void;
    midimsg_callbacks.active_sensing = empty_void;
    midimsg_callbacks.reset = empty_void;

    midimsg_callbacks.system_exclusive = (void(*)(MIDIMSG_SYSEX*))empty_void;
    midimsg_callbacks.mtc_quarter_frame = (void(*)(MIDIMSG_MTC_QUARTER_FRAME*))empty_void;
    midimsg_callbacks.song_position = (void(*)(UWORD))empty_void;
    midimsg_callbacks.song_select = (void(*)(UBYTE))empty_void;
}

void midimsg_init(UBYTE *sysex_buffer, int sysex_buffer_size)
{
    sysex_max_size = sysex_buffer_size;
    sysex_errored = 0;
    system_exclusive.length = 0;
    system_exclusive.data = sysex_buffer;

    reset_callbacks();
}

void midimsg_exit(void)
{
}


/* This is the one method to be used by users of this module. */
void midimsg_process(UBYTE byte)
{
  if (byte >= 0xF8) /* Realtime message */
      (*realtime_msg_store[byte - 0xF8])();
  else if (byte >=0xF0) /* System common message */
      (*common_msg_store[byte - 0xF0])(byte);
  else if (byte >= 0x80) /* Channel message */
      (*channel_msg_store[(byte - 0x80) >> 4])(byte);
  else
  {
      if (store_next)
	  (*store_next)(byte);
      else
	  (*midimsg_callbacks.error)(MIDIMSG_UNEXPECTED_DATA);
  }
}

static void err_message_aborted(UBYTE code)
{
    (*midimsg_callbacks.error)(MIDIMSG_MESSAGE_ABORTED);
}

static void err_message_unexpected_data(UBYTE whatever)
{
    (*midimsg_callbacks.error)(MIDIMSG_UNEXPECTED_DATA);
}

/* Channel message storage functions */

static void noteoff_channel(UBYTE channel)
{
    note_off.channel = channel;
    store_next = noteoff_note;
}

static void noteoff_note(UBYTE note)
{
    note_off.note = note;
    store_next = noteoff_velocity;
}

static void noteoff_velocity(UBYTE velocity)
{
    note_off.velocity = velocity;
    store_next = noteoff_note;
    (*midimsg_callbacks.note_off)(&note_off);
}

static void noteon_channel(UBYTE channel)
{
    note_on.channel = channel;
    store_next = noteon_note;
}

static void noteon_note(UBYTE note)
{
    note_on.note = note;
    store_next = noteon_velocity;
}

static void noteon_velocity(UBYTE velocity)
{
    note_on.velocity = velocity;
    store_next = noteon_note;
    (*midimsg_callbacks.note_on)(&note_on);
}

static void polyp_channel(UBYTE channel)
{
    poly_pressure.channel = channel;
    store_next = polyp_note;
}

static void polyp_note(UBYTE note)
{
    poly_pressure.note = note;
    store_next = polyp_value;
}

static void polyp_value(UBYTE value)
{
    poly_pressure.value = value;
    store_next = polyp_note;
    (*midimsg_callbacks.poly_pressure)(&poly_pressure);    
}

static void controlc_channel(UBYTE channel)
{
    control_change.channel = channel;
    store_next = controlc_control;
}

static void controlc_control(UBYTE control)
{
    control_change.control = control;
    store_next = controlc_value;
}

static void controlc_value(UBYTE value)
{
    control_change.value = value;
    store_next = controlc_control;
    (*midimsg_callbacks.control_change)(&control_change);    
}

static void programc_channel(UBYTE channel)
{
    program_change.channel = channel;
    store_next = programc_program;
}

static void programc_program(UBYTE program)
{
    program_change.program = program;
    store_next = programc_program;
    (*midimsg_callbacks.program_change)(&program_change);
}

static void channelp_channel(UBYTE channel)
{
    channel_pressure.channel = channel;
    store_next = channelp_value;
}

static void channelp_value(UBYTE value)
{
    channel_pressure.value = value;
    store_next = channelp_channel;
    (*midimsg_callbacks.channel_pressure)(&channel_pressure);
}

static void pitchb_channel(UBYTE channel)
{
    pitch_bend.channel = channel;
    store_next = pitchb_lsb;
}

static void pitchb_lsb(UBYTE lsb)
{
    pitch_bend.value = lsb;
    store_next = pitchb_msb;
}

static void pitchb_msb(UBYTE msb)
{
    pitch_bend.value |= (msb << 7);
    store_next = pitchb_lsb;
    (*midimsg_callbacks.pitch_bend)(&pitch_bend);
}

/* System real-time messages */
static void clock(void)
{
    (*midimsg_callbacks.clock)();
}

static void song_start(void)
{
    (*midimsg_callbacks.song_start)();
}

static void song_continue(void)
{
    (*midimsg_callbacks.song_continue)();
}

static void song_stop(void)
{
    (*midimsg_callbacks.song_stop)();
}

static void active_sensing(void)
{
    (*midimsg_callbacks.active_sensing)();
}

static void reset(void)
{
    (*midimsg_callbacks.reset)();
}

/* System common messages */
static void sysex(UBYTE byte)
{
    /* A sysex starting terminates the previous one. */
    if (byte == 0xF0)
    {	    
	if (system_exclusive.length)
	    sysex(0xF7);

	sysex_errored = 0;
	system_exclusive.length = 0;
	store_next = sysex;
    }

    if (system_exclusive.length >= sysex_max_size)
    {
	if (!sysex_errored) /* Only fire the error once */
	{
	    (*midimsg_callbacks.error)(MIDIMSG_SYSEX_TOO_LARGE);
	    sysex_errored = 1;
	}
    }
    else
	system_exclusive.data[system_exclusive.length++] = byte;

    if (byte == 0xF7)
    {
	if (!sysex_errored)
	    (*midimsg_callbacks.system_exclusive)(&system_exclusive);
	system_exclusive.length = 0;
	store_next = err_message_unexpected_data;
    }
}

static void mtc_quarter_frame_status(UBYTE msg)
{
    store_next = mtc_quarter_frame_data;
}

static void mtc_quarter_frame_data(UBYTE data)
{
    mtc_quarter_frame.value = data & 0x0f;
    mtc_quarter_frame.type = data & 0x70;
    (*midimsg_callbacks.mtc_quarter_frame)(&mtc_quarter_frame);
    store_next = err_message_unexpected_data;
}

static void song_position_status(UBYTE spos)
{
    store_next = song_position_lsb;
}

static void song_position_lsb(UBYTE lsb)
{
    song_position = lsb;
    store_next = song_position_msb;
}

static void song_position_msb(UBYTE msb)
{
    song_position |= (msb << 7);
    store_next = err_message_unexpected_data;
    (*midimsg_callbacks.song_position)(song_position);
}

static void song_select_status(UBYTE msg)
{
    store_next = song_select_number;
}

static void song_select_number(UBYTE song)
{
    (*midimsg_callbacks.song_select)(song);
}

static void tune_request(UBYTE whatever)
{
    (*midimsg_callbacks.tune_request)();
}

