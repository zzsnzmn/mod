/* issues

- preset save blip with internal timer vs ext trig

*/

#include <stdio.h>

// asf
#include "delay.h"
#include "compiler.h"
#include "flashc.h"
#include "preprocessor.h"
#include "print_funcs.h"
#include "intc.h"
#include "pm.h"
#include "gpio.h"
#include "spi.h"
#include "sysclk.h"

// skeleton
#include "types.h"
#include "events.h"
#include "i2c.h"
#include "init.h"
#include "interrupts.h"
#include "monome.h"
#include "timers.h"
#include "adc.h"
#include "util.h"
#include "ftdi.h"

// this
#include "conf_board.h"
#include "ii.h"


#define FIRSTRUN_KEY 0x22


const u16 SCALES[24][16] = {

0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                                                    // ZERO
0, 68, 136, 170, 238, 306, 375, 409, 477, 545, 579, 647, 715, 784, 818,    886,                    // ionian [2, 2, 1, 2, 2, 2, 1]
0, 68, 102, 170, 238, 306, 340, 409, 477, 511, 579, 647, 715, 750, 818,    886,                    // dorian [2, 1, 2, 2, 2, 1, 2]
0, 34, 102, 170, 238, 272, 340, 409, 443, 511, 579, 647, 681, 750, 818,    852,                    // phrygian [1, 2, 2, 2, 1, 2, 2]
0, 68, 136, 204, 238, 306, 375, 409, 477, 545, 613, 647, 715, 784, 818,    886,                    // lydian [2, 2, 2, 1, 2, 2, 1]
0, 68, 136, 170, 238, 306, 340, 409, 477, 545, 579, 647, 715, 750, 818,    886,                    // mixolydian [2, 2, 1, 2, 2, 1, 2]
0, 68, 136, 170, 238, 306, 340, 409, 477, 545, 579, 647, 715, 750, 818,    886,                    // aeolian [2, 1, 2, 2, 1, 2, 2]
0, 34, 102, 170, 204, 272, 340, 409, 443, 511, 579, 613, 681, 750, 818,    852,                    // locrian [1, 2, 2, 1, 2, 2, 2]

0, 34, 68, 102, 136, 170, 204, 238, 272, 306, 341, 375, 409, 443, 477, 511,                        // chromatic
0, 68, 136, 204, 272, 341, 409, 477, 545, 613, 682, 750, 818, 886, 954,    1022,                   // whole
0, 170, 340, 511, 681, 852, 1022, 1193, 1363, 1534, 1704, 1875, 2045, 2215, 2386, 2556,            // fourths
0, 238, 477, 715, 954, 1193, 1431, 1670, 1909, 2147, 2386, 2625, 2863, 3102, 3340, 3579,           // fifths
0, 17, 34, 51, 68, 85, 102, 119, 136, 153, 170, 187, 204, 221, 238,    255,                        // quarter
0, 8, 17, 25, 34, 42, 51, 59, 68, 76, 85, 93, 102, 110, 119, 127,                                  // eighth
0, 61, 122, 163, 245, 327, 429, 491, 552, 613, 654, 736, 818, 920, 982, 1105,                      // just
0, 61, 130, 163, 245, 337, 441, 491, 552, 621, 654, 736, 828, 932, 982, 1105,                      // pythagorean

0, 272, 545, 818, 1090, 1363, 1636, 1909, 2181, 2454, 2727, 3000, 3272, 3545, 3818, 4091,          // equal 10v
0, 136, 272, 409, 545, 682, 818, 955, 1091, 1228, 1364, 1501, 1637, 1774, 1910, 2047,              // equal 5v
0, 68, 136, 204, 272, 341, 409, 477, 545, 613, 682, 750, 818, 886, 954, 1023,                      // equal 2.5v
0, 34, 68, 102, 136, 170, 204, 238, 272, 306, 340, 374, 408, 442, 476, 511,                        // equal 1.25v
0, 53, 118, 196, 291, 405, 542, 708, 908, 1149, 1441, 1792, 2216, 2728, 3345, 4091,                // log-ish 10v
0, 136, 272, 409, 545, 682, 818, 955, 1091, 1228, 1364, 1501, 1637, 1774, 1910, 2047,              // log-ish 5v
0, 745, 1362, 1874, 2298, 2649, 2941, 3182, 3382, 3548, 3685, 3799, 3894, 3972, 4037, 4091,        // exp-ish 10v
0, 372, 681, 937, 1150, 1325, 1471, 1592, 1692, 1775, 1844, 1901, 1948, 1987, 2020, 2047           // exp-ish 5v

};

const u16 FOURTHS_GRID[8][16] = {
     0, 170, 340, 511, 681, 852, 1022, 1193, 1363, 1534, 1704, 1875, 2045, 2215, 2386, 2556 ,
     34, 204, 374, 545, 715, 886, 1056, 1227, 1397, 1568, 1738, 1909, 2079, 2249, 2420, 2590 ,
     68, 238, 408, 579, 749, 920, 1090, 1261, 1431, 1602, 1772, 1943, 2113, 2283, 2454, 2624 ,
     102, 272, 442, 613, 783, 954, 1124, 1295, 1465, 1636, 1806, 1977, 2147, 2317, 2488, 2658 ,
     136, 306, 476, 647, 817, 988, 1158, 1329, 1499, 1670, 1840, 2011, 2181, 2351, 2522, 2692 ,
     170, 340, 510, 681, 851, 1022, 1192, 1363, 1533, 1704, 1874, 2045, 2215, 2385, 2556, 2726 ,
     204, 374, 544, 715, 885, 1056, 1226, 1397, 1567, 1738, 1908, 2079, 2249, 2419, 2590, 2760 ,
     238, 408, 578, 749, 919, 1090, 1260, 1431, 1601, 1772, 1942, 2113, 2283, 2453, 2624, 2794
};


typedef enum {
    mTrig, mMap, mSeries
} edit_modes;

typedef enum {
    mForward, mReverse, mDrunk, mRandom
} step_modes;

typedef struct {
    u8 loop_start, loop_end, loop_len, loop_dir;
    u16 step_choice;
    u8 cv_mode[2];
    u8 tr_mode;
    step_modes step_mode;
    u8 steps[16];
    u8 step_probs[16];
    u16 cv_values[16];
    u16 cv_steps[2][16];
    u16 cv_curves[2][16];
    u8 cv_probs[2][16];
} whale_pattern;

typedef struct {
    whale_pattern wp[16];
    u16 series_list[64];
    u8 series_start, series_end;
    u8 tr_mute[4];
    u8 cv_mute[2];
} whale_set;

typedef const struct {
    u8 fresh;
    edit_modes edit_mode;
    u8 preset_select;
    u8 glyph[8][8];
    whale_set w[8];
} nvram_data_t;

whale_set w;

u8 preset_mode, preset_select, front_timer;
u8 glyph[8];

edit_modes edit_mode;
u8 edit_cv_step, edit_cv_ch;
s8 edit_cv_value;
u8 edit_prob, live_in, scale_select;
u8 pattern, next_pattern, pattern_jump;

u8 series_pos, series_next, series_jump, series_playing, scroll_pos;

u8 key_alt, key_meta, center;
u8 held_keys[32], key_count, key_times[256];
u8 keyfirst_pos, keysecond_pos;
s8 keycount_pos, keycount_series, keycount_cv;

s8 pos, cut_pos, next_pos, drunk_step, triggered;
u8 cv_chosen[2];
u16 cv0, cv1;

u8 param_accept, *param_dest8;
u16 clip;
u16 *param_dest;
u8 quantize_in;

u8 clock_phase;
u16 clock_time, clock_temp;
u8 series_step;

u16 adc[4];
u8 SIZE, LENGTH, VARI;

typedef void(*re_t)(void);
re_t re;


// NVRAM data structure located in the flash array.
__attribute__((__section__(".flash_nvram")))
static nvram_data_t flashy;


////////////////////////////////////////////////////////////////////////////////
// prototypes

static void refresh(void);
static void refresh_mono(void);
static void refresh_preset(void);
static void clock(u8 phase);

// start/stop monome polling/refresh timers
extern void timers_set_monome(void);
extern void timers_unset_monome(void);

// check the event queue
static void check_events(void);

// handler protos
static void handler_None(s32 data) { ;; }
static void handler_KeyTimer(s32 data);
static void handler_Front(s32 data);
static void handler_ClockNormal(s32 data);

static void ww_process_ii(uint8_t i, int d);

u8 flash_is_fresh(void);
void flash_unfresh(void);
void flash_write(void);
void flash_read(void);




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// application clock code

// XXX: phase...?
void clock(u8 phase) {
    static u8 i1, count;
    static u16 found[16];

    if(phase) {
        // lights n stuff
        gpio_set_gpio_pin(B10);
        pos = next_pos;
        monomeFrameDirty++;
    } else {
        gpio_clr_gpio_pin(B10);
    }
    // print_dbg("\r\n pos: ");
    // print_dbg_ulong(pos);
}



////////////////////////////////////////////////////////////////////////////////
// timers

static softTimer_t clockTimer = { .next = NULL, .prev = NULL };
static softTimer_t keyTimer = { .next = NULL, .prev = NULL };
static softTimer_t adcTimer = { .next = NULL, .prev = NULL };
static softTimer_t monomePollTimer = { .next = NULL, .prev = NULL };
static softTimer_t monomeRefreshTimer  = { .next = NULL, .prev = NULL };



static void clockTimer_callback(void* o) {
    // static event_t e;
    // e.type = kEventTimer;
    // e.data = 0;
    // event_post(&e);
    if(clock_external == 0) {
        // print_dbg("\r\ntimer.");
        clock_phase++;
        if(clock_phase>1) clock_phase=0;
        (*clock_pulse)(clock_phase);
    }
}

static void keyTimer_callback(void* o) {
    static event_t e;
    e.type = kEventKeyTimer;
    e.data = 0;
    event_post(&e);
}

static void adcTimer_callback(void* o) {
    static event_t e;
    e.type = kEventPollADC;
    e.data = 0;
    event_post(&e);
}

// monome polling callback
static void monome_poll_timer_callback(void* obj) {
  // asynchronous, non-blocking read
  // UHC callback spawns appropriate events
    ftdi_read();
}

// monome refresh callback
static void monome_refresh_timer_callback(void* obj) {
    if(monomeFrameDirty > 0) {
        static event_t e;
        e.type = kEventMonomeRefresh;
        event_post(&e);
    }
}

// monome: start polling
void timers_set_monome(void) {
    // print_dbg("\r\n setting monome timers");
    timer_add(&monomePollTimer, 20, &monome_poll_timer_callback, NULL );
    timer_add(&monomeRefreshTimer, 30, &monome_refresh_timer_callback, NULL );
}

// monome: stop polling
void timers_unset_monome(void) {
    // print_dbg("\r\n unsetting monome timers");
    timer_remove( &monomePollTimer );
    timer_remove( &monomeRefreshTimer );
}



////////////////////////////////////////////////////////////////////////////////
// event handlers

static void handler_FtdiConnect(s32 data) { ftdi_setup(); }
static void handler_FtdiDisconnect(s32 data) {
    timers_unset_monome();
    // event_t e = { .type = kEventMonomeDisconnect };
    // event_post(&e);
}

static void handler_MonomeConnect(s32 data) {
    u8 i1;
    // print_dbg("\r\n// monome connect /////////////////");
    keycount_pos = 0;
    key_count = 0;
    SIZE = monome_size_x();
    LENGTH = SIZE - 1;
    // print_dbg("\r monome size: ");
    // print_dbg_ulong(SIZE);
    VARI = monome_is_vari();
    // print_dbg("\r monome vari: ");
    // print_dbg_ulong(VARI);

    // here is where re is defined--- function pointer....
    if(VARI) re = &refresh;
    else re = &refresh_mono;

    for(i1=0;i1<16;i1++)
        if(w.wp[i1].loop_end > LENGTH)
            w.wp[i1].loop_end = LENGTH;


    // monome_set_quadrant_flag(0);
    // monome_set_quadrant_flag(1);
    timers_set_monome();
}

static void handler_MonomePoll(s32 data) {
    monome_read_serial();
}

static void handler_MonomeRefresh(s32 data) {
    if(monomeFrameDirty) {
        if(preset_mode == 0) (*re)(); //refresh_mono();
        else refresh_preset();

        (*monome_refresh)();
    }
}

static void handler_Front(s32 data) {
    print_dbg("\r\n FRONT HOLD");

    if(data == 0) {
        front_timer = 15;
        if(preset_mode) preset_mode = 0;
        else preset_mode = 1;
    }
    else {
        front_timer = 0;
    }

    monomeFrameDirty++;
}

static void handler_PollADC(s32 data) {
    u16 i;
    adc_convert(&adc);

    // CLOCK POT INPUT
    i = adc[0];
    i = i>>2;
    if(i != clock_temp) {
        // 1000ms - 24ms
        clock_time = 25000 / (i + 25);
        // print_dbg("\r\nnew clock (ms): ");
        // print_dbg_ulong(clock_time);

        timer_set(&clockTimer, clock_time);
    }
    clock_temp = i;

    // PARAM POT INPUT
    if(param_accept && edit_prob) {
        *param_dest8 = adc[1] >> 4; // scale to 0-255;
        // print_dbg("\r\nnew prob: ");
        // print_dbg_ulong(*param_dest8);
        // print_dbg("\t" );
        // print_dbg_ulong(adc[1]);
    }
    else if(param_accept) {
        if(quantize_in)
            *param_dest = (adc[1] / 34) * 34;
        else
            *param_dest = adc[1];
        monomeFrameDirty++;
    }
    else if(key_meta) {
        i = adc[1]>>6;
        if(i > 58)
            i = 58;
        if(i != scroll_pos) {
            scroll_pos = i;
            monomeFrameDirty++;
            // print_dbg("\r scroll pos: ");
            // print_dbg_ulong(scroll_pos);
        }
    }
}

static void handler_SaveFlash(s32 data) {
    flash_write();
}

// spooky territory
static void handler_KeyTimer(s32 data) {
    static u16 i1,x,n1;

    if(front_timer) {
        if(front_timer == 1) {
            static event_t e;
            e.type = kEventSaveFlash;
            event_post(&e);

            preset_mode = 0;
            front_timer--;
        }
        else front_timer--;
    }

    // loop through all keys held down
    for(i1=0;i1<key_count;i1++) {
        // FIXME
        if(key_times[held_keys[i1]]) {
            if(--key_times[held_keys[i1]]==0) {
                if(edit_mode != mSeries && preset_mode == 0) {
                    // preset copy
                    if(held_keys[i1] / 16 == 2) {
                        x = held_keys[i1] % 16;
                        // for ( shared variable in variables struct A)
                        //  copy to struct B

                        monomeFrameDirty++;

                        // print_dbg("\r\n saved pattern: ");
                    }
                }
                else if(preset_mode == 1) {
                    if(held_keys[i1] % 16 == 0) {
                        preset_select = held_keys[i1] / 16;
                        // flash_write();
                        static event_t e;
                        e.type = kEventSaveFlash;
                        event_post(&e);
                        preset_mode = 0;
                    }
                }
                // print_dbg("\rlong press: ");
                // print_dbg_ulong(held_keys[i1]);
            }
        }
    }
}

static void handler_ClockNormal(s32 data) {
    clock_external = !gpio_get_pin_value(B09);
}


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// application grid code

static void handler_MonomeGridKey(s32 data) {


    u8 x, y, z, index, i1, found, count;
    s16 delta;
    monome_grid_key_parse_event_data(data, &x, &y, &z);

    // track the amount of keys pressed so that we can send a low gate
    // if the key_count == 0
    if(key_count <= 0) {
        key_count = 0;
    }

    if(z != 0) {
        key_count++;

        // xor so that we go lowest to highest
        // 0 -> 7, 1 -> 6 ... 7 -> 0
        cv0 = FOURTHS_GRID[y ^ 7][x];
        monomeLedBuffer[y * 16 + x] = 15;

        spi_selectChip(SPI,DAC_SPI);
        // spi_write(SPI,0x39);    // update both
        spi_write(SPI,0x31);    // update A
        // spi_write(SPI,0x38);    // update B
        // spi_write(SPI,pos*15);    // send position
        // spi_write(SPI,0);
        spi_write(SPI,cv0>>4);
        spi_write(SPI,cv0<<4);
        spi_unselectChip(SPI,DAC_SPI);

        spi_selectChip(SPI,DAC_SPI);
        spi_write(SPI,0x38);    // update B
        spi_write(SPI,cv1>>4);
        spi_write(SPI,cv1<<4);
        spi_unselectChip(SPI,DAC_SPI);

        gpio_set_gpio_pin(B00);
        gpio_set_gpio_pin(B01);
        gpio_set_gpio_pin(B02);
        gpio_set_gpio_pin(B03);


    } else {
        monomeLedBuffer[y * 16 + x] = 0;
        key_count--;
    }

    if(key_count <= 0) {

        // send low voltages
        gpio_clr_gpio_pin(B00);
        gpio_clr_gpio_pin(B01);
        gpio_clr_gpio_pin(B02);
        gpio_clr_gpio_pin(B03);
    }
}

////////////////////////////////////////////////////////////////////////////////
// application grid redraw
//
static void refresh() {
    u8 i1,i2;

    // for(i1=0;i1<128;i1+=2) {
    //     monomeLedBuffer[i1] = 0;
    // }

    // Normally this would update the display all statefully,
    // sending led's on/off with this bizz

    monome_set_quadrant_flag(0);
    monome_set_quadrant_flag(1);
}


// application grid redraw without varibright
// FIXME this should work for 64
static void refresh_mono() {
}


// XXX: not used---
static void refresh_preset() {
    u8 i1,i2;

    for(i1=0;i1<128;i1++)
        monomeLedBuffer[i1] = 0;

    monomeLedBuffer[preset_select * 16] = 11;

    for(i1=0;i1<8;i1++)
        for(i2=0;i2<8;i2++)
            if(glyph[i1] & (1<<i2))
                monomeLedBuffer[i1*16+i2+8] = 11;

    monome_set_quadrant_flag(0);
    monome_set_quadrant_flag(1);
}

static void ww_process_ii(uint8_t i, int d) {
}

// assign event handlers
static inline void assign_main_event_handlers(void) {
    app_event_handlers[ kEventFront ]    = &handler_Front;
    // app_event_handlers[ kEventTimer ]    = &handler_Timer;
    app_event_handlers[ kEventPollADC ]    = &handler_PollADC;
    app_event_handlers[ kEventKeyTimer ] = &handler_KeyTimer;
    app_event_handlers[ kEventSaveFlash ] = &handler_SaveFlash;
    app_event_handlers[ kEventClockNormal ] = &handler_ClockNormal;
    app_event_handlers[ kEventFtdiConnect ]    = &handler_FtdiConnect ;
    app_event_handlers[ kEventFtdiDisconnect ]    = &handler_FtdiDisconnect ;
    app_event_handlers[ kEventMonomeConnect ]    = &handler_MonomeConnect ;
    app_event_handlers[ kEventMonomeDisconnect ]    = &handler_None ;
    app_event_handlers[ kEventMonomePoll ]    = &handler_MonomePoll ;
    app_event_handlers[ kEventMonomeRefresh ]    = &handler_MonomeRefresh ;
    app_event_handlers[ kEventMonomeGridKey ]    = &handler_MonomeGridKey ;
}

// app event loop
void check_events(void) {
    static event_t e;
    if( event_next(&e) ) {
        (app_event_handlers)[e.type](e.data);
    }
}

// flash commands
u8 flash_is_fresh(void) {
  return (flashy.fresh != FIRSTRUN_KEY);
  // flashc_memcpy((void *)&flashy.fresh, &i, sizeof(flashy.fresh),   true);
  // flashc_memset32((void*)&(flashy.fresh), fresh_MAGIC, 4, true);
  // flashc_memset((void *)nvram_data, 0x00, 8, sizeof(*nvram_data), true);
}

// write fresh status
void flash_unfresh(void) {
  flashc_memset8((void*)&(flashy.fresh), FIRSTRUN_KEY, 4, true);
}

void flash_write(void) {
    // print_dbg("\r write preset ");
    // print_dbg_ulong(preset_select);
    flashc_memcpy((void *)&flashy.w[preset_select], &w, sizeof(w), true);
    flashc_memcpy((void *)&flashy.glyph[preset_select], &glyph, sizeof(glyph), true);
    flashc_memset8((void*)&(flashy.preset_select), preset_select, 1, true);
    flashc_memset32((void*)&(flashy.edit_mode), edit_mode, 4, true);
}

void flash_read(void) {
    // figure out how to make this work
    u8 i1, i2;

    print_dbg("\r\n read preset ");
    print_dbg_ulong(preset_select);

    for(i1=0;i1<16;i1++) {
        for(i2=0;i2<16;i2++) {
            w.wp[i1].steps[i2] = flashy.w[preset_select].wp[i1].steps[i2];
            w.wp[i1].step_probs[i2] = flashy.w[preset_select].wp[i1].step_probs[i2];
            w.wp[i1].cv_probs[0][i2] = flashy.w[preset_select].wp[i1].cv_probs[0][i2];
            w.wp[i1].cv_probs[1][i2] = flashy.w[preset_select].wp[i1].cv_probs[1][i2];
            w.wp[i1].cv_curves[0][i2] = flashy.w[preset_select].wp[i1].cv_curves[0][i2];
            w.wp[i1].cv_curves[1][i2] = flashy.w[preset_select].wp[i1].cv_curves[1][i2];
            w.wp[i1].cv_steps[0][i2] = flashy.w[preset_select].wp[i1].cv_steps[0][i2];
            w.wp[i1].cv_steps[1][i2] = flashy.w[preset_select].wp[i1].cv_steps[1][i2];
            w.wp[i1].cv_values[i2] = flashy.w[preset_select].wp[i1].cv_values[i2];
        }

        w.wp[i1].step_choice = flashy.w[preset_select].wp[i1].step_choice;
        w.wp[i1].loop_end = flashy.w[preset_select].wp[i1].loop_end;
        w.wp[i1].loop_len = flashy.w[preset_select].wp[i1].loop_len;
        w.wp[i1].loop_start = flashy.w[preset_select].wp[i1].loop_start;
        w.wp[i1].loop_dir = flashy.w[preset_select].wp[i1].loop_dir;
        w.wp[i1].step_mode = flashy.w[preset_select].wp[i1].step_mode;
        w.wp[i1].cv_mode[0] = flashy.w[preset_select].wp[i1].cv_mode[0];
        w.wp[i1].cv_mode[1] = flashy.w[preset_select].wp[i1].cv_mode[1];
        w.wp[i1].tr_mode = flashy.w[preset_select].wp[i1].tr_mode;
    }

    w.series_start = flashy.w[preset_select].series_start;
    w.series_end = flashy.w[preset_select].series_end;

    w.tr_mute[0] = flashy.w[preset_select].tr_mute[0];
    w.tr_mute[1] = flashy.w[preset_select].tr_mute[1];
    w.tr_mute[2] = flashy.w[preset_select].tr_mute[2];
    w.tr_mute[3] = flashy.w[preset_select].tr_mute[3];
    w.cv_mute[0] = flashy.w[preset_select].cv_mute[0];
    w.cv_mute[1] = flashy.w[preset_select].cv_mute[1];

    for(i1=0;i1<64;i1++)
        w.series_list[i1] = flashy.w[preset_select].series_list[i1];
}




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// main

int main(void)
{
    u8 i1,i2;

    sysclk_init();

    init_dbg_rs232(FMCK_HZ);

    init_gpio();
    assign_main_event_handlers();
    init_events();
    init_tc();
    init_spi();
    init_adc();

    irq_initialize_vectors();
    register_interrupts();
    cpu_irq_enable();

    init_usb_host();
    init_monome();

    init_i2c_slave(0x10);


    print_dbg("\r\n\n// white whale //////////////////////////////// ");
    print_dbg_ulong(sizeof(flashy));

    print_dbg(" ");
    print_dbg_ulong(sizeof(w));

    print_dbg(" ");
    print_dbg_ulong(sizeof(glyph));

    if(flash_is_fresh()) {
        print_dbg("\r\nfirst run.");
        flash_unfresh();
        flashc_memset8((void*)&(flashy.edit_mode), mTrig, 4, true);
        flashc_memset32((void*)&(flashy.preset_select), 0, 4, true);


        // clear out some reasonable defaults
        for(i1=0;i1<16;i1++) {
            for(i2=0;i2<16;i2++) {
                w.wp[i1].steps[i2] = 0;
                w.wp[i1].step_probs[i2] = 255;
                w.wp[i1].cv_probs[0][i2] = 255;
                w.wp[i1].cv_probs[1][i2] = 255;
                w.wp[i1].cv_curves[0][i2] = 0;
                w.wp[i1].cv_curves[1][i2] = 0;
                w.wp[i1].cv_values[i2] = SCALES[2][i2];
                w.wp[i1].cv_steps[0][i2] = 1<<i2;
                w.wp[i1].cv_steps[1][i2] = 1<<i2;
            }
            w.wp[i1].step_choice = 0;
            w.wp[i1].loop_end = 15;
            w.wp[i1].loop_len = 15;
            w.wp[i1].loop_start = 0;
            w.wp[i1].loop_dir = 0;
            w.wp[i1].step_mode = mForward;
            w.wp[i1].cv_mode[0] = 0;
            w.wp[i1].cv_mode[1] = 0;
            w.wp[i1].tr_mode = 0;
        }

        w.series_start = 0;
        w.series_end = 3;

        w.tr_mute[0] = 1;
        w.tr_mute[1] = 1;
        w.tr_mute[2] = 1;
        w.tr_mute[3] = 1;
        w.cv_mute[0] = 1;
        w.cv_mute[1] = 1;

        for(i1=0;i1<64;i1++)
            w.series_list[i1] = 1;

        // save all presets, clear glyphs
        for(i1=0;i1<8;i1++) {
            flashc_memcpy((void *)&flashy.w[i1], &w, sizeof(w), true);
            glyph[i1] = (1<<i1);
            flashc_memcpy((void *)&flashy.glyph[i1], &glyph, sizeof(glyph), true);
        }
    }
    else {
        // load from flash at startup
        preset_select = flashy.preset_select;
        edit_mode = flashy.edit_mode;
        flash_read();
        for(i1=0;i1<8;i1++)
            glyph[i1] = flashy.glyph[preset_select][i1];
    }

    LENGTH = 15;
    SIZE = 16;

    // why is this renamed/assigned
    re = &refresh;

    process_ii = &ww_process_ii;

    clock_pulse = &clock;
    clock_external = !gpio_get_pin_value(B09);

    timer_add(&clockTimer,120,&clockTimer_callback, NULL);
    timer_add(&keyTimer,50,&keyTimer_callback, NULL);
    timer_add(&adcTimer,100,&adcTimer_callback, NULL);
    clock_temp = 10000; // out of ADC range to force tempo

    // setup daisy chain for two dacs
    // spi_selectChip(SPI,DAC_SPI);
    // spi_write(SPI,0x80);
    // spi_write(SPI,0xff);
    // spi_write(SPI,0xff);
    // spi_unselectChip(SPI,DAC_SPI);

    while (true) {
        check_events();
    }
}
