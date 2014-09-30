#include <stdio.h>

// asf
#include "delay.h"
#include "compiler.h"
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
#include "init.h"
#include "interrupts.h"
#include "monome.h"
#include "timers.h"
#include "adc.h"

// this
#include "conf_board.h"
	

#define L2 12
#define L1 8
#define L0 4


static s8 positions[8] = {3,1,2,2,3,3,5,7};
static s8 points[8] = {3,1,2,2,3,3,5,7};
static s8 points_save[8] = {3,1,2,2,3,3,5,7};
static u8 triggers[8] = {0,0,0,0,0,0,0,0};
static u8 trig_dests[8] = {0,0,0,0,0,0,0,0};
static u8 rules[8] = {0,0,0,0,0,0,0,0};
static u8 rule_dests[8] = {0,1,2,3,4,5,6,7};

static u8 edit_row, key_count = 0, mode = 0, prev_mode = 0;

static u8 XSIZE = 16;

const u8 glyph[8][8] = {{0,0,0,0,0,0,0,0},         // o
       {0,24,24,126,126,24,24,0},     // +
       {0,0,0,126,126,0,0,0},       // -
       {0,96,96,126,126,96,96,0},     // >
       {0,6,6,126,126,6,6,0},       // <
       {0,102,102,24,24,102,102,0},   // * rnd
       {0,120,120,102,102,30,30,0},   // <> up/down
       {0,126,126,102,102,126,126,0}};  // [] return


u32 a1 = 0x19660d;
u32 c1 = 0x3c6ef35f;
u32 x1 = 1234567;  // seed
u32 a2 = 0x19660d;
u32 c2 = 0x3c6ef35f;
u32 x2 = 7654321;  // seed

const u8 outs[8] = {B00, B01, B02, B03, B04, B05, B06, B07};


volatile u8 phase;

static void op_cascades_trigger(u8 n);
static u32 rnd(void);

static void refresh(void);

static void clock(u8 phase);

// start monome polling/refresh timers
extern void timers_set_monome(void);

// stop monome polling/refresh timers
extern void timers_unset_monome(void);

// check the event queue
static void check_events(void);

////////////////////////////////////////////////////////////////////////////////
// handlers
static void handler_None(s32 data) { ;; }
static void handler_This(s32 data) { print_dbg("+"); }
static void handler_Timer(s32 data) { ;; }



void clock(u8 phase) {
  static u8 i;

	if(phase) {
		gpio_set_gpio_pin(B10);

		// clear last round
		for(i=0;i<8;i++)
			triggers[i] = 0;

		// main
		op_cascades_trigger(0);

		// ensure bounds, output triggers
		for(i=0;i<8;i++) {
			if(positions[i] < 0)
				positions[i] = 0;
			else if(positions[i] > points[i])
				positions[i] = points[i];

			// send out
			if(triggers[i])
				gpio_set_gpio_pin(outs[i]);
		}

		monomeFrameDirty++;
	}
	else {
		for(i=0;i<8;i++) gpio_clr_gpio_pin(outs[i]);

		gpio_clr_gpio_pin(B10);
 	}
}

static softTimer_t keepTimer = { .next = NULL, .prev = NULL };
static softTimer_t monomePollTimer = { .next = NULL, .prev = NULL };
static softTimer_t monomeRefreshTimer  = { .next = NULL, .prev = NULL };



static void keepTimer_callback(void* o) {  
	// static event_t e;
	// e.type = kEventTimer;
	// e.data = 0;
	// event_post(&e);
	if(clock_external == 0) {
		// print_dbg("\r\ntimer.");

		phase++;
		if(phase>1) phase=0;
		(*clock_pulse)(phase);
	}
}


// monome polling callback
static void monome_poll_timer_callback(void* obj) {
  // asynchronous, non-blocking read
  // UHC callback spawns appropriate events
	ftdi_read();
}

// monome refresh callback
static void monome_refresh_timer_callback(void* obj) {
  //  if (monomeConnect) {
  //    print_dbg("\r\n posting monome refresh event");
	if(monomeFrameDirty > 0) {
		refresh();

		static event_t e;
		e.type = kEventMonomeRefresh;
		event_post(&e);
	}
  //  }
}

// monome: start polling
void timers_set_monome(void) {
	// print_dbg("\r\n setting monome timers");
	timer_add(&monomePollTimer, 20, &monome_poll_timer_callback, NULL );
	timer_add(&monomeRefreshTimer, 50, &monome_refresh_timer_callback, NULL );
}

// monome stop polling
void timers_unset_monome(void) {
	// print_dbg("\r\n unsetting monome timers");
	timer_remove( &monomePollTimer );
	timer_remove( &monomeRefreshTimer ); 
}




// core event handlers
static void handler_FtdiConnect(s32 data) { ftdi_setup(); }
static void handler_FtdiDisconnect(s32 data) { 
	timers_unset_monome();
	/// FIXME: assuming that FTDI == monome
	event_t e = { .type = kEventMonomeDisconnect };
	event_post(&e);
}

static void handler_MonomeConnect(s32 data) {
	// print_dbg("\r\n// monome connect /////////////////"); 
	monome_set_quadrant_flag(0);
	monome_set_quadrant_flag(1);
	timers_set_monome();
}
static void handler_MonomePoll(s32 data) { monome_read_serial(); }
static void handler_MonomeRefresh(s32 data) { (*monome_refresh)(); }
static void handler_MonomeGridKey(s32 data) { 
	static u8 x, y, z;
	monome_grid_key_parse_event_data(data, &x, &y, &z);

	prev_mode = mode;

	// mode check
	if(x == 0) {
		key_count += (z<<1)-1;

		if(key_count == 1 && z == 1)
			mode = 1; 
		else if(key_count == 0)
			mode = 0;

		if(z == 1 && mode == 1) {
			edit_row = y;
			monomeFrameDirty++;
	  // post("edit row:",edit_row);
		}
	}
	else if(x == 1 && mode != 0) {
		if(mode == 1 && z == 1)
			mode = 2;
		else if(mode == 2 && z == 0)
			mode = 1;
	}
	else if(mode == 0 && z == 1) {
		points[y] = x;
		points_save[y] = x;
		positions[y] = x;
		monomeFrameDirty++;
	}
	else if(mode == 1 && z == 1) {
		if(y != edit_row) {    // filter out self-triggering
			trig_dests[edit_row] ^= (1<<y);
			monomeFrameDirty++;
		  // post("\ntrig_dests", edit_row, ":", trig_dests[edit_row]);
		}
	}
	else if(mode == 2 && z == 1) {
		if(x > 1 && x < 6) {
			rule_dests[edit_row] = y;
			monomeFrameDirty++;
		  // post("\nrule_dests", edit_row, ":", rule_dests[edit_row]);
		}
		else if(x > 5) {
			rules[edit_row] = y;
			monomeFrameDirty++;
		  // post("\nrules", edit_row, ":", rules[edit_row]);
		}
	}

	if(mode != prev_mode) {
		monomeFrameDirty++;
		// post("\nnew mode", mode);
	}
}


static void refresh() {
	u8 i1, i2, i3;

	// clear grid
	for(i1=0;i1<128;i1++)
		monomeLedBuffer[i1] = 0;

	// SET POSITIONS
	if(mode == 0) {
		for(i1=0;i1<8;i1++) {
			for(i2=positions[i1];i2<=points[i1];i2++)
				monomeLedBuffer[i1*16 + i2] = L1;

			monomeLedBuffer[i1*16 + positions[i1]] = L2;
		}
	}
	// SET ROUTING
	else if(mode == 1) {
		monomeLedBuffer[edit_row * 16] = L1;
		monomeLedBuffer[edit_row * 16 + 1] = L1;

		for(i1=0;i1<8;i1++) {
			if((trig_dests[edit_row] & (1<<i1)) != 0) {
				for(i2=2;i2<16;i2++)
					monomeLedBuffer[i1*16 + i2] = L2;
			}
			monomeLedBuffer[i1*16 + positions[i1]] = L0;
		}
	}
	// SET RULES
	else if(mode == 2) {
		monomeLedBuffer[edit_row * 16] = L1;
		monomeLedBuffer[edit_row * 16 + 1] = L1;

		for(i1=2;i1<6;i1++)
			monomeLedBuffer[rule_dests[edit_row] * 16 + i1] = L2;

		for(i1=6;i1<16;i1++)
			monomeLedBuffer[rules[edit_row] * 16 + i1] = L0;

		for(i1=0;i1<8;i1++) 
			monomeLedBuffer[i1*16 + positions[i1]] = L0;

		for(i1=0;i1<8;i1++) {
			i3 = glyph[rules[edit_row]][i1];
			for(i2=0;i2<8;i2++) {
				if((i3 & (1<<i2)) != 0)
					monomeLedBuffer[i1*16 + 8 + i2] = L2;
			}
		}

		monomeLedBuffer[rules[edit_row] * 16 + 7] = L2;
	}

	monome_set_quadrant_flag(0);
	monome_set_quadrant_flag(1);
}


static u32 rnd() {
  x1 = x1 * c1 + a1;
  x2 = x2 * c2 + a2;
  return (x1>>16) | (x2>>16);
}


// explicitly assign default event handlers.
// this way the order of the event types enum doesn't matter.
static inline void assign_main_event_handlers(void) {
	app_event_handlers[ kEventThis ]	= &handler_This;
	app_event_handlers[ kEventTimer ]	= &handler_Timer;
	app_event_handlers[ kEventFtdiConnect ]	= &handler_FtdiConnect ;
	app_event_handlers[ kEventFtdiDisconnect ]	= &handler_FtdiDisconnect ;
	app_event_handlers[ kEventMonomeConnect ]	= &handler_MonomeConnect ;
	app_event_handlers[ kEventMonomeDisconnect ]	= &handler_None ;
	app_event_handlers[ kEventMonomeConnect ]	= &handler_MonomeConnect ;
	app_event_handlers[ kEventMonomeDisconnect ]	= &handler_None ;
	app_event_handlers[ kEventMonomePoll ]	= &handler_MonomePoll ;
	app_event_handlers[ kEventMonomeRefresh ]	= &handler_MonomeRefresh ;
	app_event_handlers[ kEventMonomeGridKey ]	= &handler_MonomeGridKey ;
}


// app event loop
void check_events(void) {
	static event_t e;
	if( event_next(&e) ) {
		(app_event_handlers)[e.type](e.data);
	}
}





static void op_cascades_trigger(u8 n) {
  u8 m;

  positions[n]--;

  // ****** the trigger # check is so we don't cause a trigger/rules multiple times per NEXT
  // a rules-based jump to position-point does not current cause a trigger. should it?
  if(positions[n] < 0 && triggers[n] == 0) {
    triggers[n]++;
  
    if(rules[n] == 1) {     // inc
      if(points[rule_dests[n]] < (XSIZE-1)) {
        points[rule_dests[n]]++;
        positions[rule_dests[n]] = points[rule_dests[n]];
      }
    }
    else if(rules[n] == 2) {  // dec
      if(points[rule_dests[n]] > 0) {
        points[rule_dests[n]]--;
        positions[rule_dests[n]] = points[rule_dests[n]];
      }
    }
    else if(rules[n] == 3) {  // max
      points[rule_dests[n]] = (XSIZE-1);
      positions[rule_dests[n]] = points[rule_dests[n]];
    }
    else if(rules[n] == 4) {  // min
      points[rule_dests[n]] = 0;
      positions[rule_dests[n]] = points[rule_dests[n]];
    }
    else if(rules[n] == 5) {  // rnd
      points[rule_dests[n]] = rnd() % XSIZE;
      
      print_dbg("\r\n op_cascades >>>>>>>>>>>>>>>>>>>> RANDOM: ");
      print_dbg_hex(points[rule_dests[n]]);
      // print_dbg_hex(rnd() % 11);

      positions[rule_dests[n]] = points[rule_dests[n]];
    }
    else if(rules[n] == 6) {  // up/down
      points[rule_dests[n]] += rnd() % 3;
      points[rule_dests[n]]--;


      if(points[rule_dests[n]] < 0) points[rule_dests[n]] = 0;
      else if(points[rule_dests[n]] > (XSIZE-1)) points[rule_dests[n]] = XSIZE-1;
      positions[rule_dests[n]] = points[rule_dests[n]];  

      print_dbg("\r\n op_cascades >>>>>>>>>>>>>>>>>>>> WANDER: ");
      print_dbg_hex(points[rule_dests[n]]);   
    }
    else if(rules[n] == 7) {  // return
      points[rule_dests[n]] = points_save[rule_dests[n]];
    }


    //reset
    positions[n] += points[n] + 1;

    //triggers
    for(m=0;m<8;m++)
      if((trig_dests[n] & (1<<m)) != 0)
        op_cascades_trigger(m);
        // post("\ntrigger",n," -> ", m);
  }
}





////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// main

int main(void)
{
	sysclk_init();

	// Switch main clock from internal RC to external Oscillator 0
	// pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

	init_dbg_rs232(FOSC0);

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


	print_dbg("\r\n\n// start ////////////////////////////////");

	clock_pulse = &clock;
	clock_external = !gpio_get_pin_value(B09);

	timer_add(&keepTimer,120,&keepTimer_callback, NULL);

	// setup daisy chain for two dacs
	spi_selectChip(SPI,DAC_SPI);
	spi_write(SPI,0x80);
	spi_write(SPI,0xff);
	spi_write(SPI,0xff);
	spi_unselectChip(SPI,DAC_SPI);

	while (true) {
		check_events();
	}
}
