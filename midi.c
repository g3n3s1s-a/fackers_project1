/* Simple MPU-401 UART MIDI demo for DOS (OpenWatcom-friendly)
   Plays a C major scale on channel 1 (MIDI ch 0).
*/
#include <dos.h>
#include <conio.h>

/* Use portable wrappers so Watcom and Borland both work */
#if defined(__WATCOMC__)
  /* Watcom provides inp()/outp() */
  #define inb(p)    inp((unsigned)(p))
  #define outb(p,v) outp((unsigned)(p), (int)(v))
#else
  /* Borland/Turbo C provide inportb()/outportb() */
  #define inb(p)    inportb((unsigned)(p))
  #define outb(p,v) outportb((unsigned)(p), (unsigned char)(v))
#endif

#define MPU_DATA   0x330  /* write data here */
#define MPU_CMD    0x331  /* write commands here; also read status */
#define MPU_STAT   0x331  /* read status here (same port as CMD) */

/* Bit 6 (0x40) of status = 1 means output NOT ready. Wait until it's 0. */
static int mpu_wait_tx_ready(void) {
    unsigned long timeout = 65535UL;
    while (timeout--) {
        if ( (inb(MPU_STAT) & 0x40) == 0 ) return 1;
    }
    return 0; /* timed out */
}

static void mpu_write_byte(unsigned char b) {
    if (mpu_wait_tx_ready()) outb(MPU_DATA, b);
}

static void mpu_cmd(unsigned char cmd) {
    unsigned long timeout = 65535UL;
    while (timeout--) {
        if ( (inb(MPU_STAT) & 0x40) == 0 ) { outb(MPU_CMD, cmd); return; }
    }
}

static void mpu_init_uart(void) {
    mpu_cmd(0xFF);  /* Reset */
    delay(10);
    mpu_cmd(0x3F);  /* UART mode */
    delay(10);
}

/* Basic MIDI helpers */
static void midi_prog_change(unsigned char ch, unsigned char program) {
    mpu_write_byte(0xC0 | (ch & 0x0F));
    mpu_write_byte(program & 0x7F);
}

static void midi_note_on(unsigned char ch, unsigned char note, unsigned char vel) {
    mpu_write_byte(0x90 | (ch & 0x0F));
    mpu_write_byte(note & 0x7F);
    mpu_write_byte(vel & 0x7F);
}

static void midi_note_off(unsigned char ch, unsigned char note, unsigned char vel) {
    mpu_write_byte(0x80 | (ch & 0x0F));
    mpu_write_byte(note & 0x7F);
    mpu_write_byte(vel & 0x7F);
}

int main(void) {
    unsigned char scale[] = {60,62,64,65,67,69,71,72}; /* C4..C5 */
    int i;

    mpu_init_uart();

    /* Channel 1 (MIDI ch 0) — Acoustic Grand (program 0) */
    midi_prog_change(0, 0);

    for (i = 0; i < (int)(sizeof(scale)); ++i) {
        midi_note_on(0, scale[i], 100);
        delay(300);
        midi_note_off(0, scale[i], 64);
        delay(60);
    }
    return 0;
}
