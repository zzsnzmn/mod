#ifndef _TELETYPE_H_
#define _TELETYPE_H_

#define SCRIPT_MAX_COMMANDS 4
#define COMMAND_MAX_LENGTH 8
#define STACK_SIZE 8
#define Q_SIZE 8
#define D_SIZE 8

#define WELCOME "TELETYPE 1.0"

#define TRUE 1
#define FALSE 0

enum varnames {V_I, V_TIME, V_TIME_ACT, V_IN, V_PARAM, V_PRESET, V_M, V_M_ACT, V_X, V_Y, V_Z, V_T, V_A, V_B, V_C, V_D};

typedef enum { 
	E_OK,
	E_WELCOME, 
	E_PARSE, 
	E_LENGTH,
	E_NEED_PARAMS,
	E_EXTRA_PARAMS,
	E_NO_MOD_HERE,
	E_MANY_SEP,
	E_NEED_SEP
} error_t;

typedef enum {NUMBER, MOD, SEP, OP, VAR, ARRAY} tele_word_t;

typedef struct {
	tele_word_t t;
	int v;
} tele_data_t;

typedef struct {
	char l;
	signed char separator;
	tele_data_t data[COMMAND_MAX_LENGTH];
} tele_command_t;

typedef struct {
	char l;
	tele_command_t c[SCRIPT_MAX_COMMANDS];
} tele_script_t;

typedef struct {
	const char *name;
	int v;
	tele_word_t t;
} tele_var_t;

typedef struct {
	const char *name;
	int v[4];
	tele_word_t t[4];
} tele_array_t;

typedef struct {
	const char *name;
	void (*func)(tele_command_t *c);
	char params;
	const char *doc;
} tele_mod_t;

typedef struct {
	const char *name;
	void (*func)(void);
	char params;
	const char* doc;
} tele_op_t;

error_t parse(char *cmd);
error_t validate(tele_command_t *c);
void process(tele_command_t *c);
char * print_command(const tele_command_t *c);

void tele_tick(void);

void clear_delays(void);

int tele_get_array(uint8_t a, uint8_t i);
void tele_set_array(uint8_t a, uint8_t i, uint16_t v);

void tele_set_val(uint8_t i, uint16_t v);

const char * tele_error(error_t);

extern tele_command_t temp;

typedef void(*update_metro_t)(int, int, uint8_t);
extern volatile update_metro_t update_metro;

extern char error_detail[16];
extern int output;
extern uint8_t odirty;

#endif