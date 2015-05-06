#ifndef _INTERRUPTS_H_
#define _INTERRUPTS_H_

// global count of uptime, and overflow flag.
volatile u64 tcTicks;
volatile u8 tcOverflow;

extern volatile u8 clock_external;

typedef void(*clock_pulse_t)(u8 phase);
extern volatile clock_pulse_t clock_pulse;

extern void register_interrupts(void);

void clock_null(u8);

#endif