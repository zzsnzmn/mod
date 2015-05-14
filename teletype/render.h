#ifndef _RENDER_H_
#define _RENDER_H_

extern void render_init(void);
extern void render_preset(int);
extern void render_activity(int);
extern void render_message(int);
extern void render_input(int, char *);
extern void render_list(void);
extern void render(void);

enum {R_PRESET, R_ACTIVITY, R_INPUT, R_MESSAGE, R_LIST};
extern uint8_t dirties[5];

#endif