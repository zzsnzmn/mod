#ifndef _RENDER_H_
#define _RENDER_H_

extern void render_init(void);
extern void render(int);
extern void render_in(char *);
extern void render_edit(int);
extern void render_update(void);

extern uint8_t sdirty;

#endif