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

// this
#include "conf_board.h"
#include "render.h"
#include "teletype.h"

#define FIRSTRUN_KEY 0x22

#define METRO_SCRIPT 8

u8 preset_mode, preset_select, front_timer;
u8 glyph[8];

u8 clock_phase;
u16 clock_time, clock_temp;

u16 adc[4];

typedef struct {
	s16 now;
	u16 target;
	u16 slew;
	s16 step;
} aout_t;

aout_t aout[4];

char input[32];
uint8_t pos;

error_t status;

tele_script_t script[10];
uint8_t edit;

uint8_t metro_act;
unsigned int metro_time;

// typedef const struct {
// } nvram_data_t;

// NVRAM data structure located in the flash array.
// __attribute__((__section__(".flash_nvram")))
// static nvram_data_t flashy;


uint8_t d[4] = {1,3,7,15};


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


static u8 flash_is_fresh(void);
static void flash_unfresh(void);
static void flash_write(void);
static void flash_read(void);

static void refresh_outputs(void);


static void tele_metro(int, int, uint8_t);


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
	u8 i;

	for(i=0;i<4;i++)
		if(aout[i].now != aout[i].target) {
			aout[i].now += (aout[i].target - aout[i].now) / aout[i].step;
			aout[i].step--;
		}

	spi_selectChip(SPI,DAC_SPI);
	spi_write(SPI,0x31);
	spi_write(SPI,aout[2].now>>4);
	spi_write(SPI,aout[2].now<<4);
	spi_write(SPI,0x31);
	spi_write(SPI,aout[0].now>>4);
	spi_write(SPI,aout[0].now<<4);
	spi_unselectChip(SPI,DAC_SPI);

	spi_selectChip(SPI,DAC_SPI);
	spi_write(SPI,0x38);
	spi_write(SPI,aout[3].now>>4);
	spi_write(SPI,aout[3].now<<4);
	spi_write(SPI,0x38);
	spi_write(SPI,aout[1].now>>4);
	spi_write(SPI,aout[1].now<<4);
	spi_unselectChip(SPI,DAC_SPI);
}

static void clockTimer_callback(void* o) {  
	// static event_t e;
	// e.type = kEventTimer;
	// e.data = 0;
	// event_post(&e);
	// print_dbg("\r\ntimer.");

	clock_phase++;
	if(clock_phase>1) clock_phase=0;
	// (*clock_pulse)(clock_phase);

	clock_time++;

	tele_tick();

	// i2c_master_tx(d);
}

static void refreshTimer_callback(void* o) {  
	// static event_t e;
	// e.type = kEventTimer;
	// e.data = 0;
	// event_post(&e);
	// print_dbg("\r\ntimer.");

	if(sdirty) {
		render_in(input);
		sdirty = 0;
		render(output);
		render_edit(edit);
		render_update();
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
	print_dbg("*");

	if(script[METRO_SCRIPT].l) {
		process(&script[METRO_SCRIPT].c[0]);
		refresh_outputs();
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

	tele_set_val(3,adc[0]);	// IN
	tele_set_val(4,adc[1]);	// PARAM

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
	u8 i, n;

	const s8* frame;
	if(hid_get_frame_dirty()) {
		frame = (const s8*)hid_get_frame_data();

     	for(i=2;i<8;i++) {
     		if(frame[i] == 0)
     			break;

     		if(frame_compare(frame[i]) == false) {
     			// print_dbg_hex(frame[i]);
     			switch(frame[i]) {
     				case 0x30:
     					edit++;
     					if(edit==10) edit = 0;
     					// edit &= 0x7;
     					break;
     				case 0x2F:
     					if(edit) edit--;
     					else edit = 9;
     					break;
     				case BACKSPACE:
     					if(pos) {
     						input[--pos] = 0;
	     				}
     					break;
     				case RETURN:
     					// print_dbg("\n\r");
     					status = parse(input);

     					if(status == E_OK) {
							status = validate(&temp);

							if(status == E_OK) {
								process(&temp);
								refresh_outputs();
								memcpy(&script[edit].c[0], &temp, sizeof(tele_command_t));
								script[edit].l = 1;
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

     					for(n = 0;n < 32;n++)
     						input[n] = 0;
     					pos = 0;
     					break;
 					default:
     					// print_dbg_char(hid_to_ascii(frame[i], frame[0]));
     					input[pos] = hid_to_ascii(frame[i], frame[0]);
     					pos++;
     			}

     			sdirty++;
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
	// print_dbg("*");

	// for(int n=0;n<script.l;n++)
	if(script[data].l) {
		process(&script[data].c[0]);
		refresh_outputs();
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



void refresh_outputs() {
	if(odirty) {
		if(tele_get_array(0,0))
			gpio_set_pin_high(B08);
		else
			gpio_set_pin_low(B08);

		if(tele_get_array(0,1))
			gpio_set_pin_high(B09);
		else
			gpio_set_pin_low(B09);

		if(tele_get_array(0,2))
			gpio_set_pin_high(B10);
		else
			gpio_set_pin_low(B10);

		if(tele_get_array(0,3))
			gpio_set_pin_high(B11);
		else
			gpio_set_pin_low(B11);

		aout[0].target = tele_get_array(1,0);
		aout[1].target = tele_get_array(1,1);
		aout[2].target = tele_get_array(1,2);
		aout[3].target = tele_get_array(1,3);

		aout[0].step = 1;
		aout[1].step = 1;
		aout[2].step = 1;
		aout[3].step = 1;
	}
}



static void tele_metro(int m, int m_act, uint8_t m_reset) {
	print_dbg("\r\nupdate");

	metro_time = m;
	if(m_act && !metro_act) {
		print_dbg("\r\nTURN ON METRO");
		metro_act = 1;
		timer_add(&metroTimer, metro_time, &metroTimer_callback, NULL);
	}
	else if(!m_act && metro_act) {
		print_dbg("\r\nTURN OFF METRO");
		metro_act = 0;
		timer_remove(&metroTimer);
	}
	else {
		print_dbg("\r\nSET METRO");
		timer_set(&metroTimer, metro_time);
	}
}


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

	timer_add(&clockTimer,10,&clockTimer_callback, NULL);
	timer_add(&refreshTimer,63,&refreshTimer_callback, NULL);
	timer_add(&cvTimer,6,&cvTimer_callback, NULL);
	timer_add(&keyTimer,51,&keyTimer_callback, NULL);
	timer_add(&adcTimer,61,&adcTimer_callback, NULL);
	
	metro_act = 1;
	metro_time = 1000;
	timer_add(&metroTimer, metro_time ,&metroTimer_callback, NULL);


	render_init();

	clear_delays();

	sdirty++;

	update_metro = &tele_metro;

	while (true) {
		check_events();
	}
}
