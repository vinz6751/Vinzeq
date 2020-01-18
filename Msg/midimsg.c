/* This module gets a stream of midi bytes and analyses it.
 * It calls callback functions whenever a message is received.
 * This is written for speed.
 */

#include <stdio.h>

#include "midimsg.h"

static int msg_in_progress = 0;

/* These are structures which get filled up with bytes received, and which
 * are then passed to the user's callback functions */
MIDIMSG_NOTE_ON note_on;
MIDIMSG_NOTE_OFF note_off;
MIDIMSG_POLY_PRESSURE poly_pressure;
MIDIMSG_CONTROL_CHANGE control_change;
MIDIMSG_PROGRAM_CHANGE program_change;
MIDIMSG_CHANNEL_PRESSURE channel_pressure;
MIDIMSG_PITCH_BEND pitch_bend;

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

/* System real time messagse */
static void clock(void);
static void song_start(void);
static void song_continue(void);
static void song_stop(void);
static void active_sensing(void);
static void reset(void);

static UBYTE running_status;

static void err_message_aborted(UBYTE);
/* Empty callbacks */
static void reset_callbacks(void);
static void empty_realtime(void) { }
static void empty_channel(void) { }

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
    empty_realtime,
    song_start,
    song_continue,
    song_stop,
    empty_realtime,
    active_sensing,
    reset
};

static void reset_callbacks(void)
{
    midimsg_callbacks.note_off = (void(*)(MIDIMSG_NOTE_OFF*))empty_channel;
    midimsg_callbacks.note_on = (void(*)(MIDIMSG_NOTE_ON*))empty_channel;
    midimsg_callbacks.poly_pressure= (void(*)(MIDIMSG_POLY_PRESSURE*))empty_channel;
    midimsg_callbacks.control_change = (void(*)(MIDIMSG_CONTROL_CHANGE*))empty_channel;
    midimsg_callbacks.program_change = (void(*)(MIDIMSG_PROGRAM_CHANGE*))empty_channel;
    midimsg_callbacks.channel_pressure = (void(*)(MIDIMSG_CHANNEL_PRESSURE*))empty_channel;
    midimsg_callbacks.pitch_bend = (void(*)(MIDIMSG_PITCH_BEND*))empty_channel;

    midimsg_callbacks.clock = empty_realtime;
    midimsg_callbacks.song_start = empty_realtime;
    midimsg_callbacks.song_continue = empty_realtime;
    midimsg_callbacks.song_stop = empty_realtime;
    midimsg_callbacks.active_sensing = empty_realtime;
    midimsg_callbacks.reset = empty_realtime;
}

void midimsg_init(void)
{
    reset_callbacks();
}

void midimsg_exit(void)
{
    reset_callbacks();
}


/* This is the one method to be used by users of this module. */
void midimsg_process(UBYTE byte)
{
  if (byte >= 0xF8)
  {
      /* Realtime message */
      register UBYTE index = byte - 0xF8;
      (*realtime_msg_store[index])();
  }
  else if (byte >=0xF0)
  {
      /* System common message */
      register UBYTE index = byte - 0xF8;
      running_status = 0;
  }
  else if (byte >= 0x80)
  {
      /* Channel message */
      running_status = byte;
      register UBYTE index = (byte - 0x80) >> 4;
      (*channel_msg_store[index])(byte);
  }
  else
  {
      if (store_next)
      {
	  (*store_next)(byte);
      }
      else
      {
	  (*midimsg_callbacks.error)(MIDIMSG_UNEXPECTED_DATA);
      }
  }
}

static void err_message_aborted(UBYTE b)
{
    (*midimsg_callbacks.error)(MIDIMSG_MESSAGE_ABORTED);
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
