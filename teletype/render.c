#include <stdio.h>
#include <string.h>

#include "screen.h"
#include "region.h"
#include "font.h"

#include "render.h"


static region r = {.w = 128, .h = 16, .x = 0, .y = 0};
static region in = {.w = 118, .h = 16, .x = 10, .y = 16};
static region r_edit = {.w = 10, .h = 14, .x = 0, .y = 50};

uint8_t sdirty;

void render_init(void) {
	region_alloc(&r);
	region_alloc(&in);
	region_alloc(&r_edit);
}

void render(int i) {
	char s[8];

	sprintf(s, "%d", i);
	// itoa(i, &s, 10);

	region_fill(&r, 1);
	region_string(&r, s, 4, 4, 0xf, 1, 0);
}

void render_edit(int i) {
	char s[8];

	sprintf(s, "%d", i);
	// itoa(i, &s, 10);

	region_fill(&r_edit, 0);
	region_string(&r_edit, s, 2, 0, 0xf, 0, 1);
}

void render_in(char *s) {
	char str[32];
	strcpy(str, s);
	strcat(str, "_");
	region_fill(&in, 0);
	region_string(&in, str, 4, 4, 0xf, 0, 0);
}

void render_update(void) {
	// app pause?
	region_draw(&r);
	region_draw(&r_edit);
	region_draw(&in);
	// app resume?
}