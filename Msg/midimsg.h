#ifndef VINZEQ_TYPES_H
#define VINZEQ_TYPES_H

#ifndef UBYTE
#define UBYTE  unsigned char
#endif

#ifndef WORD
#define WORD short
#endif

typedef struct {
    UBYTE channel;
    UBYTE note;
    UBYTE velocity;
} MIDIMSG_NOTE_ON;

typedef struct {
    UBYTE channel;
    UBYTE note;
    UBYTE velocity;
} MIDIMSG_NOTE_OFF;

typedef struct {
    UBYTE channel;
    UBYTE note;
    UBYTE value;
} MIDIMSG_POLY_PRESSURE;

typedef struct {
    UBYTE channel;
    UBYTE control;
    UBYTE value;
} MIDIMSG_CONTROL_CHANGE;

typedef struct {
    UBYTE channel;
    UBYTE program;
} MIDIMSG_PROGRAM_CHANGE;

typedef struct {
    UBYTE channel;
    UBYTE value;
} MIDIMSG_CHANNEL_PRESSURE;

typedef struct {
    UBYTE channel;
    WORD value;
} MIDIMSG_PITCH_BEND;

#define MIDIMSG_MESSAGE_ABORTED 1


typedef struct {
    void (*error)(int number);
    /* Channel messages */
    void (*note_on)(MIDIMSG_NOTE_ON*);
    void (*note_off)(MIDIMSG_NOTE_OFF*);
    void (*poly_pressure)(MIDIMSG_POLY_PRESSURE*);
    void (*control_change)(MIDIMSG_CONTROL_CHANGE*);
    void (*program_change)(MIDIMSG_PROGRAM_CHANGE*);
    void (*channel_pressure)(MIDIMSG_CHANNEL_PRESSURE*);
    void (*pitch_bend)(MIDIMSG_PITCH_BEND*);

    /* System real-time message */
    void (*clock)(void);
    void (*song_start)(void);
    void (*song_continue)(void);
    void (*song_stop)(void);
    void (*active_sensing)(void);
    void (*reset)(void);
} MIDIMSG_CALLBACKS;

extern MIDIMSG_CALLBACKS midimsg_callbacks;
    
#endif
