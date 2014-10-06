// ASF
#include "compiler.h"
#include "delay.h"
#include "gpio.h"
#include "intc.h"
#include "print_funcs.h"
#include "tc.h"
#include "usart.h"

#include "conf_board.h"
#include "conf_tc_irq.h"
#include "events.h"
#include "interrupts.h"
#include "timers.h"
#include "types.h"

//#define UI_IRQ_PRIORITY AVR32_INTC_INT2

void clock_null(u8 phase) {;;}


//------------------------
//----- variables
// timer tick counter
volatile u64 tcTicks = 0;
volatile u8 tcOverflow = 0;
static const u64 tcMax = (U64)0x7fffffff;
static const u64 tcMaxInv = (u64)0x10000000;

volatile clock_pulse_t clock_pulse = &clock_null;
volatile u8 clock_external;

//----------------------
//---- static functions 
// interrupt handlers

// irq for app timer
__attribute__((__interrupt__))
static void irq_tc(void);

// irq for PA08-PA15
__attribute__((__interrupt__))
static void irq_port0_line1(void);

// irq for PB08-PB15
__attribute__((__interrupt__))
static void irq_port1_line1(void);

// irq for uart
// __attribute__((__interrupt__))
// static void irq_usart(void);


//---------------------------------
//----- static function definitions


// timer irq
__attribute__((__interrupt__))
static void irq_tc(void) {
  tcTicks++;
  // overflow control
  if(tcTicks > tcMax) { 
    tcTicks = 0;
    tcOverflow = 1;
  } else {
    tcOverflow = 0;
  }
  process_timers();
  // clear interrupt flag by reading timer SR
  tc_read_sr(APP_TC, APP_TC_CHANNEL);
}



// interrupt handler for PA08-PA15
__attribute__((__interrupt__))
static void irq_port0_line1(void) {
    if(gpio_get_pin_interrupt_flag(NMI)) {
      gpio_clear_pin_interrupt_flag(NMI);
      // print_dbg("\r\n ### NMI ### ");
      static event_t e;
      e.type = kEventFront;
      e.data = gpio_get_pin_value(NMI);
      event_post(&e);
    }
}

// interrupt handler for PB08-PB15
__attribute__((__interrupt__))
static void irq_port1_line1(void) {
    // print_dbg("\r\ninterrupt on PB08-PB15.");

    // static event_t e;   
    // e.type = kSwitchEvents[swIdx];
    // e.data = gpio_get_pin_value(kSwitchPins[swIdx]); 
    // event_post(&e);

    // clock norm
    if(gpio_get_pin_interrupt_flag(B09)) {

      static event_t e;
      e.type = kEventClockNormal;
      e.data = !gpio_get_pin_value(B09);
      event_post(&e);

      gpio_clear_pin_interrupt_flag(B09);
    }

    // clock in
    if(gpio_get_pin_interrupt_flag(B08)) {
      // CLOCK BOUNCY WITHOUT THESE PRINTS
      print_dbg("\rclk: ");
      print_dbg_ulong(gpio_get_pin_value(B08));
      (*clock_pulse)(gpio_get_pin_value(B08));
      gpio_clear_pin_interrupt_flag(B08);
    }
}


// interrupt handler for uart
// __attribute__((__interrupt__))
// static void irq_usart(void) {
// }

//-----------------------------
//---- external function definitions

// register interrupts
void register_interrupts(void) {
  // enable interrupts on GPIO inputs
  gpio_enable_pin_interrupt( NMI, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt( B08, GPIO_PIN_CHANGE);
  gpio_enable_pin_interrupt( B09,	GPIO_PIN_CHANGE);


  // PA08 - PA15
  INTC_register_interrupt( &irq_port0_line1, AVR32_GPIO_IRQ_0 + (AVR32_PIN_PA08 / 8), UI_IRQ_PRIORITY);

  // PB08 - PB15
  INTC_register_interrupt( &irq_port1_line1, AVR32_GPIO_IRQ_0 + (AVR32_PIN_PB08 / 8), UI_IRQ_PRIORITY);

  // register TC interrupt
  INTC_register_interrupt(&irq_tc, APP_TC_IRQ, UI_IRQ_PRIORITY);

  // register uart interrupt
  // INTC_register_interrupt(&irq_usart, AVR32_USART0_IRQ, UI_IRQ_PRIORITY);
}
