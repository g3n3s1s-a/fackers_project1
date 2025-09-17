/* Music3 for DOS / DOSBox (OpenWatcom 16-bit)
   - Interactive poly keyboard when no args
   - Sheet playback with ASCII oscilloscope (single averaged trace) when file given
   - Beat units: B=Q|E|H|W|S|DQ (dotted-quarter)
   - Tempo: T=### (beats per chosen unit)
   - Instrument: I=###
   - Sustain pedal: SUS=ON|OFF (CC64)
   - Grace overlap: OVL=ms (default 20ms) to slightly overlap consecutive notes/chords

   Build: wcl -bt=dos -ms -l=dos -fe:music3.exe music3.c
*/

#include <dos.h>     /* delay, int86, _dos_getvect, _dos_setvect */
#include <i86.h>     /* _disable, _enable, int86 */
#include <conio.h>   /* putch, cputs, kbhit, getch */
#include <stdio.h>   /* FILE, fopen, fgets, sprintf */
#include <string.h>  /* memset, strchr, strcmp */
#include <ctype.h>   /* isspace, isdigit, tolower, toupper */

/* ===== Low-level port helpers ===== */
#if defined(__WATCOMC__)
  #define inb(p)    inp((unsigned)(p))
  #define outb(p,v) outp((unsigned)(p), (int)(v))
#else
  #define inb(p)    inportb((unsigned)(p))
  #define outb(p,v) outportb((unsigned)(p), (unsigned char)(v))
#endif

/* BIOS cursor move (INT 10h, AH=02h) — 1-based x,y */
#ifdef gotoxy
#undef gotoxy
#endif
static void bios_gotoxy(int x, int y){
    union REGS r;
    if (x < 1) x = 1; if (y < 1) y = 1;
    r.h.ah = 0x02; r.h.bh = 0x00;
    r.h.dh = (unsigned char)(y - 1);
    r.h.dl = (unsigned char)(x - 1);
    int86(0x10, &r, &r);
}
#define gotoxy(x,y)  bios_gotoxy((x),(y))
#define cputch(ch)   putch((int)(ch))

/* Clear 80x24 area */
static void cls_area(void){
    int r, c;
    for (r = 1; r <= 24; ++r) { gotoxy(1, r); for (c = 0; c < 80; ++c) cputch(' '); }
}

/* ===== MIDI via MPU-401 UART (DOSBox: mpu401=uart, mididevice=default) ===== */
#define MPU_DATA 0x330
#define MPU_CMD  0x331
#define MPU_STAT 0x331

static int  mpu_wait_tx_ready(void){ unsigned long t=65535UL; while(t--) if((inb(MPU_STAT)&0x40)==0) return 1; return 0; }
static void mpu_write_byte(unsigned char b){ if(mpu_wait_tx_ready()) outb(MPU_DATA,b); }
static void mpu_cmd(unsigned char c){ unsigned long t=65535UL; while(t--) if((inb(MPU_STAT)&0x40)==0){ outb(MPU_CMD,c); return; } }
static void mpu_init_uart(void){ mpu_cmd(0xFF); delay(10); mpu_cmd(0x3F); delay(10); }

static void midi_note_on (unsigned ch, unsigned note, unsigned vel){
    mpu_write_byte(0x90 | (ch&0x0F)); mpu_write_byte(note&0x7F); mpu_write_byte(vel&0x7F);
}
static void midi_note_off(unsigned ch, unsigned note, unsigned vel){
    mpu_write_byte(0x80 | (ch&0x0F)); mpu_write_byte(note&0x7F); mpu_write_byte(vel&0x7F);
}
static void midi_prog_change(unsigned ch,unsigned prog){
    mpu_write_byte(0xC0 | (ch&0x0F)); mpu_write_byte(prog&0x7F);
}
static void midi_cc(unsigned ch, unsigned cc, unsigned val){
    mpu_write_byte(0xB0 | (ch&0x0F)); mpu_write_byte(cc & 0x7F); mpu_write_byte(val & 0x7F);
}
static void midi_all_notes_off(unsigned ch){
    mpu_write_byte(0xB0 | (ch&0x0F)); mpu_write_byte(123); mpu_write_byte(0);
}

/* delay for arbitrary milliseconds (delay() takes int) */
static void delay_ms_ul(unsigned long ms){
    while(ms){
        unsigned chunk = (ms > 32000UL) ? 32000U : (unsigned)ms;
        delay((int)chunk);
        ms -= chunk;
    }
}

/* ===== Visual (ASCII oscilloscope, single averaged trace) ===== */
static const signed char SINE[128] = {
   0,  5, 10, 15, 20, 24, 29, 33, 37, 41, 45, 48, 52, 55, 58, 61,
  64, 66, 69, 71, 73, 75, 76, 78, 79, 80, 81, 82, 82, 83, 83, 83,
  83, 83, 82, 82, 81, 80, 79, 78, 76, 75, 73, 71, 69, 66, 64, 61,
  58, 55, 52, 48, 45, 41, 37, 33, 29, 24, 20, 15, 10,  5,  0, -5,
 -10,-15,-20,-24,-29,-33,-37,-41,-45,-48,-52,-55,-58,-61,-64,-66,
 -69,-71,-73,-75,-76,-78,-79,-80,-81,-82,-82,-83,-83,-83,-83,-83,
 -82,-82,-81,-80,-79,-78,-76,-75,-73,-71,-69,-66,-64,-61,-58,-55,
 -52,-48,-45,-41,-37,-33,-29,-24,-20,-15,-10,-5
};
#define SW 80
#define SH 24
static unsigned freq10_to_step(unsigned freq10){ unsigned long num=(unsigned long)freq10*256UL; return (unsigned)(num/2500U); }
static void draw_wave(unsigned step, unsigned amp, unsigned char mid, unsigned char ch){
    unsigned phase=0; int x;
    for(x=0;x<SW;x++){
        signed char s = SINE[phase & 0x7F];
        int y = (int)mid - ((int)amp * (int)s) / 100;
        if(y<1) y=1; if(y>SH) y=SH;
        gotoxy(x+1,y); cputch(ch);
        phase += step;
    }
}
/* averaged trace */
static void draw_avg_trace(const unsigned short *f10, int n){
    int i; unsigned long acc=0;
    if(n<=0){ int x; gotoxy(1,12); for(x=0;x<SW;x++) cputch('-'); return; }
    for(i=0;i<n;i++) acc += f10[i];
    { unsigned avg = (unsigned)(acc/(unsigned)n); unsigned step=freq10_to_step(avg); draw_wave(step,10,12,'*'); }
}

/* ===== Interactive key map (scancode set 1) ===== */
typedef struct { unsigned char sc; unsigned char note; unsigned short f10; } Key;
static const Key KEYS[] = {
  {0x1E,60,2616},{0x11,61,2772},{0x1F,62,2937},{0x12,63,3111},
  {0x20,64,3296},{0x21,65,3492},{0x14,66,3700},{0x22,67,3920},
  {0x15,68,4153},{0x23,69,4400},{0x16,70,4662},{0x24,71,4939},
  {0x25,72,5233},{0x18,73,5544},{0x19,74,5873},{0,0,0}
};
static int idx_from_sc(unsigned char sc){ int i; for(i=0; KEYS[i].sc; ++i) if(KEYS[i].sc==sc) return i; return -1; }

/* ===== Interactive mode: INT 9 ISR ===== */
volatile unsigned char key_down[16];
volatile unsigned char esc_down = 0;
volatile unsigned char space_down = 0;
static void (__interrupt __far *old_int9)(void) = 0;

void __interrupt __far kb_isr(void){
    unsigned char sc   = inb(0x60);
    unsigned char make = (sc & 0x80) ? 0 : 1;
    unsigned char code = sc & 0x7F;

    if(code==0x01){ esc_down   = make ? 1 : 0; }      /* ESC */
    else if(code==0x39){ space_down = make ? 1 : 0; } /* Space */
    else {
        int idx = idx_from_sc(code);
        if(idx >= 0) key_down[idx] = make ? 1 : 0;
    }
    /* ACK keyboard + EOI */
    { unsigned char v = inb(0x61); outb(0x61,(unsigned char)(v|0x80)); outb(0x61,(unsigned char)(v&0x7F)); }
    outb(0x20,0x20);
}

/* ===== Tempo / beat-unit state ===== */
static unsigned g_tempo_bpm = 120;           /* T=... */
typedef enum { BEAT_Q, BEAT_E, BEAT_H, BEAT_W, BEAT_S, BEAT_DQ } beat_t;
static beat_t g_beat = BEAT_Q;               /* B=... (default quarter) */
static unsigned long g_quarter_ms = 500UL;   /* derived ms for one quarter */
static unsigned g_program = 0;               /* instrument program */
static unsigned g_sustain = 0;               /* 0/1 -> CC64 off/on */
static unsigned g_overlap_ms = 20;           /* grace overlap between events */

/* compute g_quarter_ms from g_tempo_bpm and g_beat */
static void recompute_quarter_ms(void){
    unsigned long beat_ms = (g_tempo_bpm ? (60000UL / (unsigned long)g_tempo_bpm) : 500UL);
    switch(g_beat){
        case BEAT_Q:  g_quarter_ms = beat_ms;                break; /* 1 beat = quarter */
        case BEAT_H:  g_quarter_ms = beat_ms / 2UL;          break; /* beat is half note */
        case BEAT_W:  g_quarter_ms = beat_ms / 4UL;          break; /* beat is whole    */
        case BEAT_E:  g_quarter_ms = beat_ms * 2UL;          break; /* beat is eighth   */
        case BEAT_S:  g_quarter_ms = beat_ms * 4UL;          break; /* beat is 16th     */
        case BEAT_DQ: g_quarter_ms = (beat_ms * 2UL) / 3UL;  break; /* beat is dotted quarter */
    }
    if(g_quarter_ms == 0) g_quarter_ms = 1;
}

/* ===== Shared parser helpers ===== */
static const char* skip_ws(const char *p){ while(*p && isspace(*p)) ++p; return p; }

/* Parse NoteName[#|b]Octave -> MIDI (C4=60). *adv gets consumed chars. -1 on fail */
static int note_from_name(const char *p, int *adv){
    int i=0, sem=0, oct=4, midi=-1; char c = toupper(p[i]);
    if(c<'A'||c>'G') return -1;
    switch(c){ case 'C': sem=0; break; case 'D': sem=2; break; case 'E': sem=4; break;
               case 'F': sem=5; break; case 'G': sem=7; break; case 'A': sem=9; break; case 'B': sem=11; break; }
    i++;
    if(p[i]=='#'){ sem++; i++; } else if(p[i]=='b'||p[i]=='B'){ sem--; i++; }
    if(!isdigit(p[i])) return -1;
    oct=0; while(isdigit(p[i])){ oct = oct*10 + (p[i]-'0'); i++; }
    midi = 12*(oct+1) + sem; /* C-1=0 → C4=60 */
    if(adv) *adv = i; return midi;
}

/* MIDI note -> approximate frequency*10 (C4..B4 table, octave shift) */
static unsigned short midi_to_freq10(int midi){
    static const unsigned short base[12] = {
        2616,2772,2937,3111,3296,3492,3700,3920,4153,4400,4662,4939 /* C..B for octave 4 */
    };
    int rel = midi - 60;                 /* relative to C4 */
    int octave = rel / 12;
    int sem    = rel % 12;
    unsigned long f;

    if(sem < 0){ sem += 12; octave--; }
    f = base[sem];
    while(octave > 0){ f <<= 1; octave--; }  /* up octaves */
    while(octave < 0){ f >>= 1; octave++; }  /* down octaves */
    if(f > 60000UL) f = 60000UL;
    return (unsigned short)f;
}

/* Duration ms from token letter (w,h,q,e,s) and dotted flag, using global quarter */
static unsigned long dur_ms_from_token(char d, int dotted){
    unsigned long num=1, den=1;
    switch(d){
        case 'w': num=4; den=1; break;   /* 4 quarters  */
        case 'h': num=2; den=1; break;   /* 2 quarters  */
        case 'q': num=1; den=1; break;   /* 1 quarter   */
        case 'e': num=1; den=2; break;   /* 1/2 quarter */
        case 's': num=1; den=4; break;   /* 1/4 quarter */
        default : num=1; den=1; break;
    }
    { unsigned long ms = (g_quarter_ms * num) / den; if(dotted) ms = (ms*3UL)/2UL; if(ms==0) ms=1; return ms; }
}

/* ===== Visualization frame for an array of frequencies (averaged trace) ===== */
static void animate_notes_for(unsigned long ms, const unsigned short *freq10, int count, unsigned tempo, unsigned program){
    unsigned long elapsed = 0; const unsigned frame = 30;
    (void)tempo; (void)program;
    while(elapsed < ms){
        cls_area();
        gotoxy(1,1); cputs("Playing sheet...  Esc=stop");
        { char buf[80]; gotoxy(1,2); sprintf(buf,"Tempo:%u  Beat:%d  Notes:%d  OVL:%ums  SUS:%s",
            g_tempo_bpm, (int)g_beat, count, (unsigned)g_overlap_ms, g_sustain ? "ON":"OFF"); cputs(buf); }
        draw_avg_trace(freq10, count);
        delay((int)frame);
        elapsed += frame;
        if(kbhit()){ int ch = getch(); if(ch==27) break; } /* ESC abort */
    }
}

/* ===== Note helpers honoring sustain & overlap ===== */
static void play_single_note(unsigned midi, unsigned long ms){
    unsigned short f10 = midi_to_freq10((int)midi);
    midi_note_on(0, midi, 100);
    animate_notes_for(ms, &f10, 1, g_tempo_bpm, g_program);
    if(g_overlap_ms && !g_sustain){
        delay_ms_ul(g_overlap_ms);
    }
    midi_note_off(0, midi, 64);
    if(g_sustain){
        /* With sustain ON, pedal holds sound even after note-off; no extra handling needed here. */
    }
}

static void play_chord(const int *notes, int count, unsigned long ms){
    int i; unsigned short list[8]; int n=0;
    for(i=0;i<count;i++){ if(notes[i]>=0){ midi_note_on(0,(unsigned)notes[i],100); list[n++] = midi_to_freq10(notes[i]); } }
    animate_notes_for(ms, list, n, g_tempo_bpm, g_program);
    if(g_overlap_ms && !g_sustain) delay_ms_ul(g_overlap_ms);
    for(i=0;i<count;i++) if(notes[i]>=0) midi_note_off(0,(unsigned)notes[i],64);
}

/* ===== Sheet player (with B=, SUS=, OVL=) ===== */
static int play_sheet_file(const char *path){
    FILE *f = fopen(path, "rt");
    char line[256];
    if(!f){ cls_area(); gotoxy(1,2); cputs("Could not open file."); delay(1000); return 1; }

    /* reset runtime state for file */
    g_tempo_bpm = 120; g_beat = BEAT_Q; recompute_quarter_ms();
    g_program = 0; g_sustain = 0; g_overlap_ms = 20;

    midi_all_notes_off(0);
    midi_cc(0,64,0);                /* ensure pedal up */
    midi_prog_change(0,g_program);

    cls_area(); gotoxy(1,1); cputs("Playing: "); cputs(path);

    while(fgets(line,sizeof(line),f)){
        const char *p = line; char *cmt;
        /* strip comments */
        cmt = strchr(line,'#'); if(cmt) *cmt=0;
        cmt = strchr(line,';'); if(cmt) *cmt=0;

        p = skip_ws(p);
        while(*p){
            if(*p=='|'){ p++; continue; }
            if(isspace(*p)){ p=skip_ws(p); continue; }

            /* Tempo: T=### (beats per chosen beat unit) */
            if( (p[0]=='T'||p[0]=='t') && p[1]=='=' ){
                unsigned v=0; p+=2; while(isdigit(*p)){ v=v*10+(*p-'0'); p++; }
                if(v>0 && v<800){ g_tempo_bpm = v; recompute_quarter_ms(); }
                continue;
            }
            /* Beat unit: B=Q|E|H|W|S|DQ */
            if( (p[0]=='B'||p[0]=='b') && p[1]=='=' ){
                p+=2;
                if( (p[0]=='D'||p[0]=='d') && (p[1]=='Q'||p[1]=='q') ){
                    g_beat = BEAT_DQ; p+=2;
                } else {
                    char u = (char)toupper(*p);
                    if(u=='Q') g_beat=BEAT_Q;
                    else if(u=='E') g_beat=BEAT_E;
                    else if(u=='H') g_beat=BEAT_H;
                    else if(u=='W') g_beat=BEAT_W;
                    else if(u=='S') g_beat=BEAT_S;
                    if(*p) p++;
                }
                recompute_quarter_ms();
                continue;
            }
            /* Instrument: I=### */
            if( (p[0]=='I'||p[0]=='i') && p[1]=='=' ){
                unsigned v=0; p+=2; while(isdigit(*p)){ v=v*10+(*p-'0'); p++; }
                if(v<128){ g_program=v; midi_prog_change(0,g_program); }
                continue;
            }
            /* Sustain pedal: SUS=ON|OFF */
            if( (p[0]=='S'||p[0]=='s') && (p[1]=='U'||p[1]=='u') && (p[2]=='S'||p[2]=='s') && p[3]=='=' ){
                p+=4;
                if( (p[0]=='O'||p[0]=='o') && (p[1]=='N'||p[1]=='n') ){ g_sustain=1; midi_cc(0,64,127); p+=2; }
                else if( (p[0]=='O'||p[0]=='o') && (p[1]=='F'||p[1]=='f') && (p[2]=='F'||p[2]=='f') ){ g_sustain=0; midi_cc(0,64,0); p+=3; }
                continue;
            }
            /* Overlap: OVL=ms */
            if( (p[0]=='O'||p[0]=='o') && (p[1]=='V'||p[1]=='v') && (p[2]=='L'||p[2]=='l') && p[3]=='=' ){
                unsigned v=0; p+=4; while(isdigit(*p)){ v=v*10+(*p-'0'); p++; }
                if(v<=200) g_overlap_ms = (unsigned)v; /* clamp */
                continue;
            }

            /* Rest: R<dur><.> */
            if(*p=='R' || *p=='r'){
                char d='q'; int dotted=0; unsigned long ms;
                p++;
                if(*p){ char c=(char)tolower(*p); if(strchr("whqes",c)){ d=c; p++; } }
                if(*p=='.'){ dotted=1; p++; }
                ms = dur_ms_from_token(d,dotted);
                /* Visualize rest as flat line */
                { unsigned short none=0; animate_notes_for(ms,&none,0,g_tempo_bpm,g_program); }
                continue;
            }

            /* Chord: [notes] <dur> <.> */
            if(*p=='['){
                int mids[8]; int ncount=0, adv=0, midi;
                char d='q'; int dotted=0;
                p++;
                while(*p && *p!=']' && ncount<8){
                    p=skip_ws(p);
                    midi = note_from_name(p,&adv);
                    if(midi>=0){ mids[ncount++]=midi; p+=adv; }
                    else { while(*p && !isspace(*p) && *p!=']') p++; }
                    p=skip_ws(p);
                }
                if(*p==']') p++;
                p=skip_ws(p);
                if(*p){ char c=(char)tolower(*p); if(strchr("whqes",c)){ d=c; p++; } }
                if(*p=='.'){ dotted=1; p++; }
                if(ncount>0){
                    unsigned long ms = dur_ms_from_token(d,dotted);
                    play_chord(mids,ncount,ms);
                }
                continue;
            }

            /* Single note: Name[#|b]Oct <dur><.> */
            {
                int adv=0; int midi = note_from_name(p,&adv);
                if(midi>=0){
                    char d='q'; int dotted=0; unsigned long ms;
                    p+=adv;
                    if(*p){ char c=(char)tolower(*p); if(strchr("whqes",c)){ d=c; p++; } }
                    if(*p=='.'){ dotted=1; p++; }
                    ms = dur_ms_from_token(d,dotted);
                    play_single_note((unsigned)midi, ms);
                    continue;
                }
            }

            /* Unknown token: skip to next space/bar */
            while(*p && !isspace(*p) && *p!='|') p++;
        }
    }

    fclose(f);
    midi_all_notes_off(0);
    midi_cc(0,64,0); /* pedal up */
    cls_area(); gotoxy(1,12); cputs("Done. Press any key...");
    getch();
    return 0;
}

/* ===== main: if file given => play; else interactive ISR mode ===== */
int main(int argc, char **argv){
    int i;
    unsigned char was_down[16];
    memset((void*)was_down,0,sizeof(was_down));
    cls_area();
    mpu_init_uart();

    /* defaults */
    g_tempo_bpm = 120; g_beat = BEAT_Q; recompute_quarter_ms();
    g_program = 0; g_sustain = 0; g_overlap_ms = 20;
    midi_cc(0,64,0); midi_prog_change(0,g_program);

    if(argc >= 2){
        return play_sheet_file(argv[1]);
    }

    /* Interactive polyphonic keyboard mode */
    _disable(); old_int9 = _dos_getvect(9); _dos_setvect(9, kb_isr); _enable();
    gotoxy(1,1); cputs("Poly mode: hold multiple keys  A..K with W/E/T/Y/U/O/P");
    gotoxy(1,2); cputs("Space = All Notes Off   |   Esc = Quit");

    while(1){
        if(esc_down) break;

        if(space_down){
            for(i=0; KEYS[i].sc; ++i){
                if(was_down[i]) { midi_note_off(0, KEYS[i].note, 64); was_down[i]=0; }
            }
            midi_all_notes_off(0); space_down=0;
        }

        for(i=0; KEYS[i].sc; ++i){
            unsigned char cur = key_down[i];
            if(cur && !was_down[i]){ midi_note_on(0, KEYS[i].note, 100); was_down[i]=1; }
            else if(!cur && was_down[i]){ midi_note_off(0, KEYS[i].note, 64); was_down[i]=0; }
        }

        /* averaged trace for all active notes */
        {
            unsigned short list[16]; int n=0, x;
            cls_area();
            for(i=0; KEYS[i].sc; ++i) if(was_down[i]) list[n++] = KEYS[i].f10;
            if(n>0) draw_avg_trace(list, n);
            else { gotoxy(1,12); for(x=0;x<SW;x++) cputch('-'); }
            gotoxy(1,1); cputs("Poly mode: hold multiple keys  A..K with W/E/T/Y/U/O/P");
            gotoxy(1,2); cputs("Space = All Notes Off   |   Esc = Quit");
        }

        delay(30);
    }

    for(i=0; KEYS[i].sc; ++i) if(was_down[i]) midi_note_off(0, KEYS[i].note, 64);
    midi_all_notes_off(0);
    midi_cc(0,64,0); /* pedal up */
    _disable(); _dos_setvect(9, old_int9); _enable();

    cls_area(); gotoxy(1,12); cputs("Goodbye.");
    return 0;
}
