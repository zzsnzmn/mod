#include "screen.h"
#include "region.h"
#include "font.h"

#include "render.h"
#include "util.h"

uint8_t dirties[4];

static region r_activity = {.w = 10, .h = 16, .x = 0, .y = 0};
static region r_input = {.w = 118, .h = 16, .x = 10, .y = 50};
static region r_edit = {.w = 10, .h = 14, .x = 0, .y = 50};
static region r_list = {.w = 118, .h = 16, .x = 10, .y = 0};

uint8_t sdirty;

void render_init(void) {
	region_alloc(&r_activity);
	region_alloc(&r_input);
	region_alloc(&r_edit);
	region_alloc(&r_list);
}

void render_activity(int i) {
	char s[8];

	itoa(i, s, 10);

	region_fill(&r_activity, 1);
	region_string(&r_activity, s, 4, 4, 0xf, 1, 0);
}

void render_edit(int i) {
	char s[4];

	if(i == 8) s[0] = 'M';
	else if(i == 9) s[0] = 'I';
	else s[0] = i+48;
	itoa(i, s, 10);

	region_fill(&r_edit, 0);
	region_string(&r_edit, s, 2, 0, 0xf, 0, 1);
}

void render_input(char *s) {
	region_fill(&r_input, 0);
	region_string(&r_input, s, 4, 4, 0xf, 0, 0);
}

void render_list() {
	// region_fill(&r_list, 1);
	// region_string(&in, s, 4, 4, 0xf, 0, 0);
}

void render(void) {
	// app pause?
	region_draw(&r_activity);
	region_draw(&r_edit);
	region_draw(&r_input);
	region_draw(&r_list);
	// app resume?
}