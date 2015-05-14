#include <string.h>

#include "screen.h"
#include "region.h"
#include "font.h"

#include "render.h"
#include "util.h"

uint8_t dirties[5];

static region r_preset = {.w = 64, .h = 8, .x = 0, .y = 0};
static region r_activity = {.w = 32, .h = 8, .x = 96, .y = 0};
static region r_list = {.w = 128, .h = 32, .x = 0, .y = 10};
static region r_message = {.w = 128, .h = 8, .x = 0, .y = 46};
static region r_input = {.w = 128, .h = 8, .x = 0, .y = 56};

uint8_t sdirty;

void render_init(void) {
	region_alloc(&r_preset);
	region_alloc(&r_activity);
	region_alloc(&r_input);
	region_alloc(&r_message);
	region_alloc(&r_list);
}

void render_preset(int i) {
	region_fill(&r_preset, 1);
	// region_string(&r_activity, s, 0, 0, 0xf, 1, 0);
}

void render_activity(int i) {
	region_fill(&r_activity, 1);
	// region_string(&r_activity, s, 0, 0, 0xf, 1, 0);
}

void render_message(int i) {
	char s[8];

	itoa(i, s, 10);

	region_fill(&r_message, 0);
	region_string(&r_message, s, 0, 0, 0xf, 0, 0);
}

void render_input(int i, char *s) {
	char ss[32];

	ss[1] = ' ';
	ss[3] = ' ';
	ss[2] = '>';
	ss[4] = 0;

	if(i == 8) ss[0] = 'M';
	else if(i == 9) ss[0] = 'I';
	else ss[0] = i+48;

	strcat(ss,s);
	strcat(ss,"_");

	region_fill(&r_input, 0);
	region_string(&r_input, ss, 0, 0, 0xf, 0, 0);
}

void render_list() {
	region_fill(&r_list, 1);
	// region_string(&in, s, 4, 4, 0xf, 0, 0);
}

void render(void) {
	// app pause?
	region_draw(&r_preset);
	region_draw(&r_activity);
	region_draw(&r_message);
	region_draw(&r_input);
	region_draw(&r_list);
	// app resume?
}