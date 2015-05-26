#include <stdio.h>	//sprintf
#include <string.h> //memcpy

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

// system
#include "types.h"
#include "events.h"
#include "init.h"
#include "interrupts.h"
#include "i2c.h"
#include "kbd.h"
// #include "monome.h"
#include "timers.h"
#include "adc.h"
#include "util.h"
// #include "ftdi.h"
#include "hid.h"
#include "screen.h"
#include "region.h"
#include "font.h"

// this
#include "conf_board.h"
#include "teletype.h"

#define FIRSTRUN_KEY 0x22

#define METRO_SCRIPT 8
#define INIT_SCRIPT 9

#define RATE_CLOCK 10
#define RATE_CV 6

#define A_METRO 0x1
#define A_TR 0x2
#define A_SLEW 0x4
#define A_DELAY 0x8
#define A_Q 0x10
#define A_X 0x20


u8 preset, preset_dirty, preset_mode, preset_select, front_timer;
char preset_name[17] = "NAMED";

u8 glyph[8];


// u8 clock_phase;
// u16 clock_time, clock_temp;

u16 adc[4];

typedef struct {
	u16 now;
	u16 off;
	u16 target;
	u16 slew;
	u16 step;
	s32 delta;
	u32 a;
} aout_t;

aout_t aout[4];

error_t status;

char input[32];
char input_buffer[32];
uint8_t pos;

tele_script_t script[10];
tele_script_t history;
uint8_t edit, edit_line;
uint8_t live;

uint8_t activity;
uint8_t activity_prev;


uint8_t metro_act;
unsigned int metro_time;

// typedef const struct {
// } nvram_data_t;

// NVRAM data structure located in the flash array.
// __attribute__((__section__(".flash_nvram")))
// static nvram_data_t flashy;


#define R_PRESET (1<<0)
#define R_ACTIVITY (1<<1)
#define R_INPUT (1<<2)
#define R_MESSAGE (1<<3)
#define R_LIST1 (1<<4)
#define R_LIST2 (1<<5)
#define R_LIST3 (1<<6)
#define R_LIST4 (1<<7)
#define R_LIST (0xf<<4)
uint8_t r_edit_dirty;

static region r_preset = {.w = 96, .h = 8, .x = 0, .y = 0};
static region r_activity = {.w = 32, .h = 3, .x = 96, .y = 0};
static region r_list1 = {.w = 128, .h = 8, .x = 0, .y = 8};
static region r_list2 = {.w = 128, .h = 8, .x = 0, .y = 16};
static region r_list3 = {.w = 128, .h = 8, .x = 0, .y = 24};
static region r_list4 = {.w = 128, .h = 8, .x = 0, .y = 32};
static region r_message = {.w = 124, .h = 8, .x = 2, .y = 44};
static region r_input = {.w = 124, .h = 8, .x = 2, .y = 56};

uint8_t sdirty;


////////////////////////////////////////////////////////////////////////////////
// prototypes

// check the event queue
static void check_events(void);

// handler protos
// static void handler_None(s32 data) { ;; }
static void handler_KeyTimer(s32 data);
static void handler_Front(s32 data);
static void handler_HidConnect(s32 data);
static void handler_HidDisconnect(s32 data);
static void handler_HidPacket(s32 data);
static void handler_Trigger(s32 data);
static void handler_ScreenRefresh(s32 data);


static u8 flash_is_fresh(void);
static void flash_unfresh(void);
static void flash_write(void);
static void flash_read(void);

static void render_init(void);

static void tele_metro(int, int, uint8_t);
static void tele_tr(uint8_t i, int v);
static void tele_cv(uint8_t i, int v);
static void tele_cv_slew(uint8_t i, int v);
static void tele_delay(uint8_t i);
static void tele_q(uint8_t i);
static void tele_cv_off(uint8_t i, int v);


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// application


////////////////////////////////////////////////////////////////////////////////
// timers

static softTimer_t clockTimer = { .next = NULL, .prev = NULL };
static softTimer_t refreshTimer = { .next = NULL, .prev = NULL };
static softTimer_t keyTimer = { .next = NULL, .prev = NULL };
static softTimer_t cvTimer = { .next = NULL, .prev = NULL };
static softTimer_t adcTimer = { .next = NULL, .prev = NULL };
static softTimer_t hidTimer = { .next = NULL, .prev = NULL };
static softTimer_t metroTimer = { .next = NULL, .prev = NULL };


static void cvTimer_callback(void* o) { 
	u8 i, r=0;
	u16 a;

	activity &= ~A_SLEW;

	for(i=0;i<4;i++)
		if(aout[i].step) {
			aout[i].step--;

			if(aout[i].step == 0) {
				aout[i].now = aout[i].target;
				r_edit_dirty |= R_ACTIVITY;
			}
			else {
				aout[i].a += aout[i].delta;
				aout[i].now = aout[i].a >> 16;
				activity |= A_SLEW;
			}

			r++;
		}

	if(r) {
		spi_selectChip(SPI,DAC_SPI);
		spi_write(SPI,0x31);
		a = aout[2].now >> 2;
		spi_write(SPI,a>>4);
		spi_write(SPI,a<<4);
		spi_write(SPI,0x31);
		a = aout[0].now >> 2;
		spi_write(SPI,a>>4);
		spi_write(SPI,a<<4);
		spi_unselectChip(SPI,DAC_SPI);

		spi_selectChip(SPI,DAC_SPI);
		spi_write(SPI,0x38);
		a = aout[3].now >> 2;
		spi_write(SPI,a>>4);
		spi_write(SPI,a<<4);
		spi_write(SPI,0x38);
		a = aout[1].now >> 2;
		spi_write(SPI,a>>4);
		spi_write(SPI,a<<4);
		spi_unselectChip(SPI,DAC_SPI);
	}
}

static void clockTimer_callback(void* o) {  
	// static event_t e;
	// e.type = kEventTimer;
	// e.data = 0;
	// event_post(&e);
	// print_dbg("\r\ntimer.");

	// clock_phase++;
	// if(clock_phase>1) clock_phase=0;
	// (*clock_pulse)(clock_phase);

	// clock_time++;

	tele_tick(RATE_CLOCK);

	// i2c_master_tx(d);
}

static void refreshTimer_callback(void* o) {  
	static event_t e;
	e.type = kEventScreenRefresh;
	e.data = 0;
	event_post(&e);

	if(sdirty) {
		// region_draw(&r_preset);
		region_draw(&r_activity);
		region_draw(&r_message);
		region_draw(&r_input);
		region_draw(&r_list1);
		region_draw(&r_list2);
		region_draw(&r_list3);
		region_draw(&r_list4);

		sdirty = 0;
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

static void hidTimer_callback(void* o) {
	static event_t e;
	e.type = kEventHidTimer;
	e.data = 0;
	event_post(&e);
}

static void metroTimer_callback(void* o) {
	// print_dbg("*");
	uint8_t i;

	for(i=0;i<script[METRO_SCRIPT].l;i++) {
		process(&script[METRO_SCRIPT].c[i]);
		activity |= A_METRO;
	}
}






////////////////////////////////////////////////////////////////////////////////
// event handlers

static void handler_Front(s32 data) {
	// print_dbg("\r\n //// FRONT HOLD");

	if(data == 0) {
		front_timer = 15;
		if(preset_mode) preset_mode = 0;
		else preset_mode = 1;
	}
	else {
		front_timer = 0;
	}

	// monomeFrameDirty++;
}

static void handler_PollADC(s32 data) {
	adc_convert(&adc);

	tele_set_val(V_IN,adc[0]<<2);	// IN
	tele_set_val(V_PARAM,adc[1]<<2);	// PARAM

	// print_dbg("\r\nadc:\t"); print_dbg_ulong(adc[0]);
	// print_dbg("\t"); print_dbg_ulong(adc[1]);
	// print_dbg("\t"); print_dbg_ulong(adc[2]);
	// print_dbg("\t"); print_dbg_ulong(adc[3]);
}

static void handler_SaveFlash(s32 data) {
	flash_write();
}

static void handler_KeyTimer(s32 data) {
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
}

static void handler_HidConnect(s32 data) {
	print_dbg("\r\nhid connect\r\n");
	timer_add(&hidTimer,47,&hidTimer_callback, NULL);
}

static void handler_HidDisconnect(s32 data) {
	timer_remove(&hidTimer);
	print_dbg("\r\nno more hid");

}

static void handler_HidTimer(s32 data) {
	u8 i,n;

	const s8* frame;
	if(hid_get_frame_dirty()) {
		frame = (const s8*)hid_get_frame_data();

     	for(i=2;i<8;i++) {
     		if(frame[i] == 0)
     			break;

     		if(frame_compare(frame[i]) == false) {
     			// CTRL = 1
     			// SHIFT = 2
     			// ALT = 4
     			// META = 8

     			print_dbg("\r\nk: ");
     			print_dbg_hex(frame[i]);
     			print_dbg("\r\nmod: ");
     			print_dbg_hex(frame[0]);
     			switch(frame[i]) {
     				case 0x2B: // tab
     					if(live) {
     						live = 0;
     						if(edit_line > script[edit].l)
	     						edit_line = script[edit].l;
	     					strcpy(input,print_command(&script[edit].c[edit_line]));
	 						pos = strlen(input);
	 						for(n = pos;n < 32;n++) input[n] = 0;
     						r_edit_dirty |= R_LIST | R_MESSAGE;
     					}
     					else {
     						for(n = 0;n < 32;n++) input[n] = 0;
		 					pos = 0;
     						live = 1;
     						edit_line = 4;
     						r_edit_dirty |= R_LIST | R_MESSAGE;
     					}
     					break;

     				case 0x51: // down
     					if(edit_line < 3) {
     						if(script[edit].l > edit_line) {
     							edit_line++;
     							strcpy(input,print_command(&script[edit].c[edit_line]));
     							pos = strlen(input);
     							for(n = pos;n < 32;n++) input[n] = 0;
	 							r_edit_dirty |= R_LIST;
	 						}
     						else if(live) {
     							edit_line++;
     							strcpy(input,print_command(&history.c[edit_line]));
     							pos = strlen(input);
     							for(n = pos;n < 32;n++) input[n] = 0;
	 							r_edit_dirty |= R_LIST;
	 						}
	 					}
     					break;

     				case 0x52: // up
     					if(edit_line) {
     						edit_line--;
     						if(live)
     							strcpy(input,print_command(&history.c[edit_line]));
     						else
     							strcpy(input,print_command(&script[edit].c[edit_line]));

     						pos = strlen(input);
	     					for(n = pos;n < 32;n++) input[n] = 0;
 							r_edit_dirty |= R_LIST;
 						}
     					break;

     				case 0x30: // ]
						live = 0;
     					edit++;
     					if(edit==10)
     						edit = 0;
     					if(edit_line > script[edit].l)
     						edit_line = script[edit].l;
     					strcpy(input,print_command(&script[edit].c[edit_line]));
 						pos = strlen(input);
     					for(n = pos;n < 32;n++) input[n] = 0;


     					r_edit_dirty |= R_LIST;
     					break;

     				case 0x2F: // [
     					live = 0;
     					if(edit) 
     						edit--;
     					else 
     						edit = 9;
     					if(edit_line > script[edit].l)
     						edit_line = script[edit].l;
     					strcpy(input,print_command(&script[edit].c[edit_line]));
 						pos = strlen(input);
     					for(n = pos;n < 32;n++) input[n] = 0;
     					r_edit_dirty |= R_LIST;
     					break;
     				case 0x4C:
     					for(n = 0;n < 32;n++)
     						input[n] = 0;
     					pos = 0;
     					break;

     				case BACKSPACE:
     					if(pos) {
     						pos--;
     						// input[pos] = ' ';
     						for(int x = pos; x < 31; x++)
     							input[x] = input[x+1];
	     				}
     					break;

     				case 0x50: // back
     					if(pos) {
     						pos--;
     					}
     					break;

     				case 0x4f: // forward
     					if(pos < strlen(input)) {
     						pos++;
     					}

     					break;

     				case RETURN:
     					status = parse(input);

     					if(status == E_OK) {
							status = validate(&temp);

							if(status == E_OK) {
								if(live) {
									edit_line = 4;

									if(temp.l) {
										memcpy(&history.c[0], &history.c[1], sizeof(tele_command_t));
										memcpy(&history.c[1], &history.c[2], sizeof(tele_command_t));
										memcpy(&history.c[2], &history.c[3], sizeof(tele_command_t));
										memcpy(&history.c[3], &temp, sizeof(tele_command_t));

										process(&temp);
									}

									for(n = 0;n < 32;n++)
			     						input[n] = 0;
			     					pos = 0;
								}
								else {
									if(temp.l == 0) {	// BLANK LINE
										if(script[edit].l && script[edit].c[edit_line].l) {
											print_dbg("\r\nl ");
											print_dbg_ulong(script[edit].l);

											script[edit].l--;

		     								for(n=edit_line;n<script[edit].l;n++)
		     									memcpy(&script[edit].c[n], &script[edit].c[n+1], sizeof(tele_command_t));

		     								script[edit].c[script[edit].l].l = 0;
		     								
		     								if(edit_line > script[edit].l)
		     									edit_line = script[edit].l;
		     								strcpy(input,print_command(&script[edit].c[edit_line]));
					 						pos = strlen(input);
					 						// print_dbg(" -> ");
											// print_dbg_ulong(script[edit].l);
		     							}
									}
									else if(frame[0] == 2 || frame[0] == 0x20) { // SHIFT = INSERT
										for(n=script[edit].l;n>edit_line;n--) 
	     									memcpy(&script[edit].c[n], &script[edit].c[n-1], sizeof(tele_command_t));

	     								if(script[edit].l < 4)
	     									script[edit].l++;

										memcpy(&script[edit].c[edit_line], &temp, sizeof(tele_command_t));
										if((edit_line == script[edit].l) && (script[edit].l < 4))
											script[edit].l++;
										if(edit_line < 3) {
											edit_line++;
											strcpy(input,print_command(&script[edit].c[edit_line]));
			     							pos = strlen(input);
			     							for(n = pos;n < 32;n++) input[n] = 0;
			     						}
			     					}
		     						else {
										memcpy(&script[edit].c[edit_line], &temp, sizeof(tele_command_t));
										if((edit_line == script[edit].l) && (script[edit].l < 4))
											script[edit].l++;
										if(edit_line < 3) {
											edit_line++;
											strcpy(input,print_command(&script[edit].c[edit_line]));
			     							pos = strlen(input);
			     							for(n = pos;n < 32;n++) input[n] = 0;
			     						}
		     						}

			     					r_edit_dirty |= R_MESSAGE;
		     					}
		     					r_edit_dirty |= R_LIST;
							}
							else {
								print_dbg("\r\nvalidate: ");
								print_dbg(tele_error(status));
 							}
						}
						else {
							print_dbg("\r\nERROR: ");
							print_dbg(tele_error(status));
						}

						// print_dbg("\r\n\n> ");

     					r_edit_dirty |= R_MESSAGE;
     					break;

 					default:
 						if(frame[0] == 0) {
	 						if(pos<31) {
		     					// print_dbg_char(hid_to_ascii(frame[i], frame[0]));
		     					n = hid_to_ascii(frame[i], frame[0]);
		     					if(n) {
		     						for(int x = 31; x > pos; x--)
		     							input[x] = input[x-1];

		     						input[pos] = n;
		     						pos++;
		     					}
		     					// pos++;
		     					// input[pos] = 0;
	     					}
	     				}
	     				else if(frame[0] == 4 || frame[0] == 0x40) {	// ALT
	     					if(frame[i] == 0x1b) {	// x CUT
	     						memcpy(&input_buffer, &input, sizeof(input));
	     						if(live) {
	     							for(n = 0;n < 32;n++)
			     						input[n] = 0;
			     					pos = 0;
	     						}
	     						else {
	     							if(script[edit].l) {
	     								script[edit].l--;
	     								for(n=edit_line;n<script[edit].l;n++)
	     									memcpy(&script[edit].c[n], &script[edit].c[n+1], sizeof(tele_command_t));

	     								script[edit].c[script[edit].l].l = 0;
	     								if(edit_line > script[edit].l)
	     									edit_line = script[edit].l;
	     								strcpy(input,print_command(&script[edit].c[edit_line]));
				 						pos = strlen(input);
	     							}

	     							r_edit_dirty |= R_LIST;
	     						}
	     					}
	     					else if(frame[i] == 0x06) { // c COPY
	     						memcpy(&input_buffer, &input, sizeof(input));
	     					}
	     					else if(frame[i] == 0x19) { // v PASTE
	     						memcpy(&input, &input_buffer, sizeof(input));
	     						pos = strlen(input);
	     					}
	     				}

     					break;
     			}

     			r_edit_dirty |= R_INPUT;
     		}
     	}

     	set_old_frame(frame);

		// print_dbg("\r\nhid:\t");
		// for(i=0;i<8;i++) {
		// 	print_dbg_ulong( (int) frame[i] ); 
		// 	print_dbg("\t");  
		// }
	}

    hid_clear_frame_dirty();
}


static void handler_HidPacket(s32 data) {
	print_dbg("\r\nhid packet");
}


static void handler_Trigger(s32 data) {
	uint8_t i;
	// print_dbg("*");

	// for(int n=0;n<script.l;n++)
	for(i=0;i<script[data].l;i++) {
		process(&script[data].c[i]);
	}
}

static void handler_ScreenRefresh(s32 data) {
	static uint8_t a;
	static char s[32];

	if(r_edit_dirty & R_PRESET) {
		strcpy(s," /  ");

		s[0] = preset + 48;
		s[2] = preset + 48; // PATTERN

		strcat(s,preset_name);

		region_fill(&r_preset, 0);
		if(preset_dirty)
			region_string(&r_preset, s, 0, 0, 0xa, 0, 0);
		else
			region_string(&r_preset, s, 0, 0, 0x4, 0, 0);
		sdirty++;
		r_edit_dirty &= ~R_PRESET;
	}
	// if(r_edit_dirty & R_ACTIVITY) {
	if(activity != activity_prev) {

		region_fill(&r_activity, 0);
		
/*		a = 1;

		r_activity.data[ 0 + 0 * r_activity.w ] = a;
		r_activity.data[ 1 + 2 * r_activity.w ] = a;
		r_activity.data[ 2 + 0 * r_activity.w ] = a;
		r_activity.data[ 3 + 2 * r_activity.w ] = a;
		r_activity.data[ 4 + 0 * r_activity.w ] = a;
		r_activity.data[ 5 + 2 * r_activity.w ] = a;
		r_activity.data[ 6 + 0 * r_activity.w ] = a;
		r_activity.data[ 7 + 2 * r_activity.w ] = a;

		if(activity & A_METRO) a = 15;
		else a = 1;

		r_activity.data[ 9 + 0 + 0 * r_activity.w ] = a;
		r_activity.data[ 9 + 0 + 1 * r_activity.w ] = a;
		r_activity.data[ 9 + 0 + 2 * r_activity.w ] = a;

*/
		if(activity & A_SLEW) a = 15;
		else a = 1;

		// r_activity.data[ 16 + 0 + 4 * r_activity.w ] = a;
		// r_activity.data[ 16 + 1 + 3 * r_activity.w ] = a;
		r_activity.data[ 16 + 0 + 2 * r_activity.w ] = a;
		r_activity.data[ 16 + 1 + 1 * r_activity.w ] = a;
		r_activity.data[ 16 + 2 + 0 * r_activity.w ] = a;

		if(activity & A_DELAY) a = 15;
		else a = 1;

		// r_activity.data[ 16 + 0 + 4 * r_activity.w ] = a;
		// r_activity.data[ 16 + 1 + 3 * r_activity.w ] = a;
		r_activity.data[ 20 + 0 + 0 * r_activity.w ] = a;
		r_activity.data[ 20 + 1 + 0 * r_activity.w ] = a;
		r_activity.data[ 20 + 2 + 0 * r_activity.w ] = a;
		r_activity.data[ 20 + 0 + 1 * r_activity.w ] = a;
		r_activity.data[ 20 + 2 + 1 * r_activity.w ] = a;
		r_activity.data[ 20 + 0 + 2 * r_activity.w ] = a;
		r_activity.data[ 20 + 2 + 2 * r_activity.w ] = a;

		if(activity & A_Q) a = 15;
		else a = 1;

		// r_activity.data[ 16 + 0 + 4 * r_activity.w ] = a;
		// r_activity.data[ 16 + 1 + 3 * r_activity.w ] = a;
		r_activity.data[ 24 + 0 + 0 * r_activity.w ] = a;
		r_activity.data[ 24 + 1 + 0 * r_activity.w ] = a;
		r_activity.data[ 24 + 2 + 0 * r_activity.w ] = a;
		r_activity.data[ 24 + 0 + 2 * r_activity.w ] = a;
		r_activity.data[ 24 + 1 + 2 * r_activity.w ] = a;
		r_activity.data[ 24 + 2 + 2 * r_activity.w ] = a;

		if(activity & A_X) a = 15;
		else a = 1;

		// r_activity.data[ 16 + 0 + 4 * r_activity.w ] = a;
		// r_activity.data[ 16 + 1 + 3 * r_activity.w ] = a;
		r_activity.data[ 28 + 0 + 0 * r_activity.w ] = a;
		r_activity.data[ 28 + 0 + 2 * r_activity.w ] = a;
		r_activity.data[ 28 + 1 + 1 * r_activity.w ] = a;
		r_activity.data[ 28 + 2 + 0 * r_activity.w ] = a;
		r_activity.data[ 28 + 2 + 2 * r_activity.w ] = a;

		activity_prev = activity;

		activity &= ~A_METRO;
		// activity &= ~A_X;

			
		sdirty++;
		r_edit_dirty &= ~R_ACTIVITY;
	}
	if(r_edit_dirty & R_INPUT) {
		s[0] = '>';
 		s[1] = ' ';
		s[2] = 0;

		if(!live) {
			if(edit == 8) s[0] = 'M';
			else if(edit == 9) s[0] = 'I';
			else s[0] = edit+49;
		}

		strcat(s,input);
		strcat(s," ");

		region_fill(&r_input, 0);
		// region_string(&r_input, s, 0, 0, 0xf, 0, 0);
		// font_string_region_clip(&r_input, s, 0, 0, 0xf, 0);
		font_string_region_clip_hi(&r_input, s, 0, 0, 0xf, 0, pos+2);
		sdirty++;
		r_edit_dirty &= ~R_INPUT;
	}
	if(r_edit_dirty & R_MESSAGE) {
		if(status) {
			strcpy(s,tele_error(status));
			if(error_detail[0]) {
				strcat(s, ": ");
				strcat(s, error_detail);
				error_detail[0] = 0;
			}
			status = E_OK;
		}
		else if(output_new) {
			output_new = 0;
			if(live)
				itoa(output, s, 10);
			// strcat(s, " ");
			// strcat(s, to_v(output));
			else
				s[0] = 0;
		}
		else {
			s[0] = 0;
		}
		region_fill(&r_message, 0);
		font_string_region_clip(&r_message, s, 0, 0, 0x4, 0);
		sdirty++;
		r_edit_dirty &= ~R_MESSAGE;
	}
	if(r_edit_dirty & R_LIST1) {
		a = edit_line == 0;
		region_fill(&r_list1, a);
		if(live) {
			strcpy(s,print_command(&history.c[0]));
			region_string(&r_list1, s, 2, 0, 0xf, a, 0);
		}
		else if(script[edit].l > 0) {
			strcpy(s,print_command(&script[edit].c[0]));
			region_string(&r_list1, s, 2, 0, 0xf, a, 0);
		}
		sdirty++;
		r_edit_dirty &= ~R_LIST1;
	}
	if(r_edit_dirty & R_LIST2) {
		a = edit_line == 1;
		region_fill(&r_list2, a);
		if(live) {
			strcpy(s,print_command(&history.c[1]));
			region_string(&r_list2, s, 2, 0, 0xf, a, 0);
		}
		else if(script[edit].l > 1) {
			strcpy(s,print_command(&script[edit].c[1]));
			region_string(&r_list2, s, 2, 0, 0xf, a, 0);
		}
		// region_string(&in, s, 4, 4, 0xf, a, 0);
		sdirty++;
		r_edit_dirty &= ~R_LIST2;
	}
	if(r_edit_dirty & R_LIST3) {
		a = edit_line == 2;
		region_fill(&r_list3, a);
		if(live) {
			strcpy(s,print_command(&history.c[2]));
			region_string(&r_list3, s, 2, 0, 0xf, a, 0);
		}
		else if(script[edit].l > 2) {
			strcpy(s,print_command(&script[edit].c[2]));
			region_string(&r_list3, s, 2, 0, 0xf, a, 0);
		}
		// region_string(&in, s, 4, 4, 0xf, a, 0);
		sdirty++;
		r_edit_dirty &= ~R_LIST3;
	}
	if(r_edit_dirty & R_LIST4) {
		a = edit_line == 3;
		region_fill(&r_list4, a);
		if(live) {
			strcpy(s,print_command(&history.c[3]));
			region_string(&r_list4, s, 2, 0, 0xf, a, 0);
		}
		else if(script[edit].l > 3) {
			strcpy(s,print_command(&script[edit].c[3]));
			region_string(&r_list4, s, 2, 0, 0xf, a, 0);
		}
		
		// region_string(&in, s, 4, 4, 0xf, a, 0);
		sdirty++;
		r_edit_dirty &= ~R_LIST4;
	}
}




// assign event handlers
static inline void assign_main_event_handlers(void) {
	app_event_handlers[ kEventFront ]	= &handler_Front;
	app_event_handlers[ kEventPollADC ]	= &handler_PollADC;
	app_event_handlers[ kEventKeyTimer ] = &handler_KeyTimer;
	app_event_handlers[ kEventSaveFlash ] = &handler_SaveFlash;
	app_event_handlers[ kEventHidConnect ]	= &handler_HidConnect;
	app_event_handlers[ kEventHidDisconnect ]	= &handler_HidDisconnect;
	app_event_handlers[ kEventHidPacket ]	= &handler_HidPacket;
	app_event_handlers[ kEventHidTimer ]	= &handler_HidTimer;
	app_event_handlers[ kEventTrigger ]	= &handler_Trigger;
	app_event_handlers[ kEventScreenRefresh ]	= &handler_ScreenRefresh;
}

// app event loop
void check_events(void) {
	static event_t e;
	if( event_next(&e) ) {
		(app_event_handlers)[e.type](e.data);
	}
}



////////////////////////////////////////////////////////////////////////////////
// funcs


u8 flash_is_fresh(void) {
  // return (flashy.fresh != FIRSTRUN_KEY);
	return 0;
  // flashc_memcpy((void *)&flashy.fresh, &i, sizeof(flashy.fresh),   true);
  // flashc_memset32((void*)&(flashy.fresh), fresh_MAGIC, 4, true);
  // flashc_memset((void *)nvram_data, 0x00, 8, sizeof(*nvram_data), true);
}

// write fresh status
void flash_unfresh(void) {
  // flashc_memset8((void*)&(flashy.fresh), FIRSTRUN_KEY, 4, true);
}

void flash_write(void) {
	// print_dbg("\r write preset ");
	// print_dbg_ulong(preset_select);

	// flashc_memcpy((void *)&flashy.es[preset_select], &es, sizeof(es), true);
	// flashc_memcpy((void *)&flashy.glyph[preset_select], &glyph, sizeof(glyph), true);
	// flashc_memset8((void*)&(flashy.preset_select), preset_select, 1, true);
}

void flash_read(void) {
	// print_dbg("\r\n read preset ");
	// print_dbg_ulong(preset_select);
}



void render_init(void) {
	region_alloc(&r_preset);
	region_alloc(&r_activity);
	region_alloc(&r_input);
	region_alloc(&r_message);
	region_alloc(&r_list1);
	region_alloc(&r_list2);
	region_alloc(&r_list3);
	region_alloc(&r_list4);
}



static void tele_metro(int m, int m_act, uint8_t m_reset) {
	metro_time = m;

	if(m_act && !metro_act) {
		// print_dbg("\r\nTURN ON METRO");
		metro_act = 1;
		timer_add(&metroTimer, metro_time, &metroTimer_callback, NULL);
	}
	else if(!m_act && metro_act) {
		// print_dbg("\r\nTURN OFF METRO");
		metro_act = 0;
		timer_remove(&metroTimer);
	}
	else if(!m_reset) {
		// print_dbg("\r\nSET METRO");
		timer_set(&metroTimer, metro_time);
	}
	else {
		// print_dbg("\r\nRESET METRO");
		timer_reset(&metroTimer);
	}
}

static void tele_tr(uint8_t i, int v) {
	if(v)
		gpio_set_pin_high(B08+i);
	else
		gpio_set_pin_low(B08+i);
}

static void tele_cv(uint8_t i, int v) {
	aout[i].target = v + aout[i].off;
	if(aout[i].target < 0)
		aout[i].target = 0;
	else if(aout[i].target > 16383)
		aout[i].target = 16383;
	aout[i].step = aout[i].slew;
	aout[i].delta = ((aout[i].target - aout[i].now)<<16) / aout[i].step;
	aout[i].a = aout[i].now<<16;
	r_edit_dirty |= R_ACTIVITY;
}

static void tele_cv_slew(uint8_t i, int v) {
	aout[i].slew = v / RATE_CV;
	if(aout[i].slew == 0)
		aout[i].slew = 1;
}

static void tele_delay(uint8_t i) {
	if(i) {
		activity |= A_DELAY;
	}
	else
		activity &= ~A_DELAY;
}

static void tele_q(uint8_t i) {
	if(i) {
		activity |= A_Q;
	}
	else
		activity &= ~A_Q;
}

static void tele_cv_off(uint8_t i, int v) {
	aout[i].off = v;
}









////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// main

int main(void)
{
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
	// init_monome();

	init_oled();

	init_i2c();

	print_dbg("\r\n\n// teletype! //////////////////////////////// ");
	// print_dbg_ulong(sizeof(flashy));

	if(flash_is_fresh()) {
		print_dbg("\r\nfirst run.");
		flash_unfresh();

		// clear out some reasonable defaults

		// save all presets, clear glyphs
		// for(i1=0;i1<8;i1++) {
		// 	flashc_memcpy((void *)&flashy.es[i1], &es, sizeof(es), true);
		// 	glyph[i1] = (1<<i1);
		// 	flashc_memcpy((void *)&flashy.glyph[i1], &glyph, sizeof(glyph), true);
		// }

	}
	else {
		// load from flash at startup
	}

	// tele_command_t stored;

	// stored.data[0].t = OP;
	// stored.data[0].v = 2;
	// stored.data[1].t = NUMBER;
	// stored.data[1].v = 8;
	// stored.data[2].t = NUMBER;
	// stored.data[2].v = 10;
	// stored.separator = -1;
	// stored.l = 3;

	// process(&stored);



	// setup daisy chain for two dacs
	spi_selectChip(SPI,DAC_SPI);
	spi_write(SPI,0x80);
	spi_write(SPI,0xff);
	spi_write(SPI,0xff);
	spi_unselectChip(SPI,DAC_SPI);

	timer_add(&clockTimer, RATE_CLOCK, &clockTimer_callback, NULL);
	timer_add(&refreshTimer, 63, &refreshTimer_callback, NULL);
	timer_add(&cvTimer, RATE_CV, &cvTimer_callback, NULL);
	timer_add(&keyTimer, 51, &keyTimer_callback, NULL);
	timer_add(&adcTimer, 61, &adcTimer_callback, NULL);
	
	metro_act = 1;
	metro_time = 1000;
	timer_add(&metroTimer, metro_time ,&metroTimer_callback, NULL);


	render_init();

	clear_delays();

	tele_init();

	aout[0].slew = 1;
	aout[1].slew = 1;
	aout[2].slew = 1;
	aout[3].slew = 1;

	status = 1;
	live = 1; edit_line = 4;
	r_edit_dirty = 0xff;
	activity = 0;
	activity_prev = 0xff;

	update_metro = &tele_metro;
	update_tr = &tele_tr;
	update_cv = &tele_cv;
	update_cv_slew = &tele_cv_slew;
	update_delay = &tele_delay;
	update_q = &tele_q;
	update_cv_off = &tele_cv_off;

	while (true) {
		check_events();
	}
}
