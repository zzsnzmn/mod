#ifndef _RENDER_H_
#define _RENDER_H_

extern void render_init(void);
extern void render_activity(int);
extern void render_edit(int);
extern void render_input(char *);
extern void render_list(void);
extern void render(void);

enum {R_ACTIVITY, R_INPUT, R_EDIT, R_LIST};
extern uint8_t dirties[4];

#endif