#include <string.h>
#include <stdio.h>		// printf
#include <stdlib.h>		// rand, strtol
#include <ctype.h>		// isdigit
#include <stdint.h>		// types

#include "teletype.h"
#include "table.h"
#include "util.h"
#include "ii.h"

#ifdef SIM
#define DBG printf("%s",dbg);
#else
#include "print_funcs.h"
#define DBG print_dbg(dbg);
#endif


static const char * errordesc[] = {
	"OK",
	WELCOME,
	"UNKOWN WORD",
	"COMMAND TOO LONG",
	"NOT ENOUGH PARAMS",
	"TOO MANY PARAMS",
	"MOD NOT ALLOWED HERE",
	"EXTRA SEPARATOR",
	"NEED SEPARATOR"
};

const char * tele_error(error_t e) {
	return errordesc[e];
}

static char dbg[32];
static char pcmd[32];


int output, output_new;

char error_detail[16];

tele_command_t temp;
tele_pattern_t tele_patterns[4];

static uint8_t pn;

static char condition;

static tele_command_t q[Q_SIZE];
static uint8_t q_top;


volatile update_metro_t update_metro;
volatile update_tr_t update_tr;
volatile update_cv_t update_cv;
volatile update_cv_slew_t update_cv_slew;
volatile update_delay_t update_delay;
volatile update_q_t update_q;
volatile update_cv_off_t update_cv_off;
volatile update_ii_t update_ii;


const char * to_v(int);



/////////////////////////////////////////////////////////////////
// STACK ////////////////////////////////////////////////////////

static int pop(void);
static void push(int);

static int top;
static int stack[STACK_SIZE];

int pop() {
	top--;
	// sprintf(dbg,"\r\npop %d", stack[top]);
	return stack[top];
}

void push(int data) {
	stack[top] = data;
	// sprintf(dbg,"\r\npush %d", stack[top]);
	top++;
}


/////////////////////////////////////////////////////////////////
// VARS ARRAYS KEYS /////////////////////////////////////////////


#define KEYS 25
static tele_key_t tele_keys[KEYS] = {
	{"WW.PRESET",WW_PRESET},
	{"WW.POS",WW_POS},
	{"WW.SYNC",WW_SYNC},
	{"WW.START",WW_START},
	{"WW.END",WW_END},
	{"WW.PMODE",WW_PMODE},
	{"WW.PATTERN",WW_PATTERN},
	{"WW.QPATTERN",WW_QPATTERN},
	{"WW.MUTE1",WW_MUTE1},
	{"WW.MUTE2",WW_MUTE2},
	{"WW.MUTE3",WW_MUTE3},
	{"WW.MUTE4",WW_MUTE4},
	{"WW.MUTEA",WW_MUTEA},
	{"WW.MUTEB",WW_MUTEB},
	{"MP.PRESET",MP_PRESET},
	{"MP.RESET",MP_RESET},
	{"ES.PRESET",ES_PRESET},
	{"ES.RESET",ES_RESET},
	{"ES.PATTERN",ES_PATTERN},
	{"ES.TRANS",ES_TRANS},
	{"ES.STOP",ES_STOP},
	{"ES.SH1",ES_SH1},
	{"ES.SH2",ES_SH2},
	{"ES.SH3",ES_SH3},
	{"ES.SH4",ES_SH4}
};


// {NAME,VAL}

// ENUM IN HEADER

static void v_P_N(uint8_t);
static void v_M(uint8_t);
static void v_M_ACT(uint8_t);
static void v_P_L(uint8_t);
static void v_P_I(uint8_t);
static void v_P_HERE(uint8_t);
static void v_P_NEXT(uint8_t);
static void v_P_PREV(uint8_t);
static void v_P_WRAP(uint8_t);
static void v_P_START(uint8_t);
static void v_P_END(uint8_t);


#define VARS 25
static tele_var_t tele_vars[VARS] = {
	{"I",NULL,0},	// gets overwritten by ITER
	{"TIME",NULL,0},
	{"TIME.ACT",NULL,1},
	{"IN",NULL,0},
	{"PARAM",NULL,0},
	{"PRESET",NULL,0},
	{"M",v_M,1000},
	{"M.ACT",v_M_ACT,1},
	{"X",NULL,0},
	{"Y",NULL,0},
	{"Z",NULL,0},
	{"T",NULL,0},
	{"A",NULL,1},
	{"B",NULL,2},
	{"C",NULL,3},
	{"D",NULL,4},
	{"P.N",v_P_N,0},
	{"P.L",v_P_L,0},
	{"P.I",v_P_I,0},
	{"P.HERE",v_P_HERE,0},
	{"P.NEXT",v_P_NEXT,0},
	{"P.PREV",v_P_PREV,0},
	{"P.WRAP",v_P_WRAP,0},
	{"P.START",v_P_START,0},
	{"P.END",v_P_END,0}
};

static void v_M(uint8_t n) {
	if(top == 0)
		push(tele_vars[V_M].v);
	else {
		tele_vars[V_M].v = pop();
		if(tele_vars[V_M].v < 10) tele_vars[V_M].v = 10;
		(*update_metro)(tele_vars[V_M].v, tele_vars[V_M_ACT].v, 0);
	}
}

static void v_M_ACT(uint8_t n) {
	if(top == 0)
		push(tele_vars[V_M_ACT].v);
	else {
		tele_vars[V_M_ACT].v = pop();
		if(tele_vars[V_M_ACT].v != 0)
			tele_vars[V_M_ACT].v = 1;
		(*update_metro)(tele_vars[V_M].v, tele_vars[V_M_ACT].v, 0);
	}
}

static void v_P_N(uint8_t n) {
	int a;
	if(top == 0) {
		push(pn);
	}
	else {
		a = pop();
		if(a < 0) pn = 0;
		else if(a > 3) pn = 3;
		else pn = a;
	}
}

static void v_P_L(uint8_t n) {
	int a;
	if(top == 0) {
		push(tele_patterns[pn].l);
	}
	else {
		a = pop();
		if(a < 0) tele_patterns[pn].l = 0;
		else if(a > 63) tele_patterns[pn].l = 63;
		else tele_patterns[pn].l = a;
	}
}

static void v_P_I(uint8_t n) {
	int a;
	if(top == 0) {
		push(tele_patterns[pn].i);
	}
	else {
		a = pop();
		if(a < 0) tele_patterns[pn].i = 0;
		else if(a > tele_patterns[pn].l) tele_patterns[pn].i = tele_patterns[pn].l;
		else tele_patterns[pn].i = a;
	}
}

static void v_P_HERE(uint8_t n) {
	int a;
	if(top == 0) {
		push(tele_patterns[pn].v[tele_patterns[pn].i]);
	}
	else {
		a = pop();
		tele_patterns[pn].v[tele_patterns[pn].i] = a;
	}
}

static void v_P_NEXT(uint8_t n) {
	int a;
	if((tele_patterns[pn].i == (tele_patterns[pn].l - 1)) || (tele_patterns[pn].i == tele_patterns[pn].end)) {
		if(tele_patterns[pn].wrap)
			tele_patterns[pn].i = tele_patterns[pn].start;
	}
	else
		tele_patterns[pn].i++;

	if(top == 0) {
		push(tele_patterns[pn].v[tele_patterns[pn].i]);
	}
	else {
		a = pop();
		tele_patterns[pn].v[tele_patterns[pn].i] = a;
	}
}

static void v_P_PREV(uint8_t n) {
	int a;
	if((tele_patterns[pn].i == 0) || (tele_patterns[pn].i == tele_patterns[pn].start)) {
		if(tele_patterns[pn].wrap)
			tele_patterns[pn].i = tele_patterns[pn].end;
	}
	else
		tele_patterns[pn].i--;
	
	if(top == 0) {
		push(tele_patterns[pn].v[tele_patterns[pn].i]);
	}
	else {
		a = pop();
		tele_patterns[pn].v[tele_patterns[pn].i] = a;
	}
}

static void v_P_WRAP(uint8_t n) {
	int a;
	if(top == 0) {
		push(tele_patterns[pn].wrap);
	}
	else {
		a = pop();
		if(a < 0) tele_patterns[pn].wrap = 0;
		else if(a > 1) tele_patterns[pn].wrap = 1;
		else tele_patterns[pn].wrap = a;
	}
}

static void v_P_START(uint8_t n) {
	int a;
	if(top == 0) {
		push(tele_patterns[pn].start);
	}
	else {
		a = pop();
		if(a < 0) tele_patterns[pn].start = 0;
		else if(a > 63) tele_patterns[pn].start = 1;
		else tele_patterns[pn].start = a;
	}
}

static void v_P_END(uint8_t n) {
	int a;
	if(top == 0) {
		push(tele_patterns[pn].end);
	}
	else {
		a = pop();
		if(a < 0) tele_patterns[pn].end = 0;
		else if(a > 63) tele_patterns[pn].end = 1;
		else tele_patterns[pn].end = a;
	}
}


static void a_TR(uint8_t);
static void a_CV(uint8_t);
static void a_CV_SLEW(uint8_t);
static void a_CV_OFF(uint8_t);
static void a_TR_TIME(uint8_t);

#define MAKEARRAY(name, func) {#name, {0,0,0,0}, func}
#define ARRAYS 5
static tele_array_t tele_arrays[ARRAYS] = {
	MAKEARRAY(TR,a_TR),
	MAKEARRAY(CV,a_CV),
	MAKEARRAY(CV.SLEW,a_CV_SLEW),
	MAKEARRAY(CV.OFF,a_CV_OFF),
	{"TR.TIME", {100,100,100,100}, a_TR_TIME}
};

static void a_TR(uint8_t i) {
	int a = pop();
	a = (a != 0);
	tele_arrays[0].v[i] = a;
	(*update_tr)(i, a);
}
static void a_CV(uint8_t i) {
	int a = pop();
	if(a<0) a = 0;
	else if(a > 16383) a = 16383;
	tele_arrays[1].v[i] = a;
	(*update_cv)(i, a);
}
static void a_CV_SLEW(uint8_t i) {
	int a = pop();
	if(a<1) a = 1;	// min slew = 1
	else if(a > 16383) a = 16383;
	tele_arrays[2].v[i] = a;
	(*update_cv_slew)(i, a);
}
static void a_CV_OFF(uint8_t i) {
	int a = pop();
	tele_arrays[3].v[i] = a;
	(*update_cv_off)(i, a);
	(*update_cv)(i, tele_arrays[1].v[i]);

}
static void a_TR_TIME(uint8_t i) {
	int a = pop();
	if(a<1) a = 1;
	tele_arrays[4].v[i] = a;
}



/////////////////////////////////////////////////////////////////
// DELAY ////////////////////////////////////////////////////////

static tele_command_t delay_c[D_SIZE];
static int delay_t[D_SIZE];
static uint8_t delay_count;
static uint16_t tr_pulse[4];

static void process_delays(uint8_t);
void clear_delays(void);

static void process_delays(uint8_t v) {
	for(int i=0;i<D_SIZE;i++) {
		if(delay_t[i]) {
			delay_t[i] -= v;
 			if(delay_t[i] <= 0) {
 				// sprintf(dbg,"\r\ndelay %d", i);
				// DBG
				process(&delay_c[i]);
				delay_t[i] = 0;
				delay_count--;
				if(delay_count == 0)
					(*update_delay)(0);
			}
		}
	}

	for(int i=0;i<4;i++) {
		if(tr_pulse[i]) {
			tr_pulse[i] -= v;
			if(tr_pulse[i] <= 0) {
				tr_pulse[i] = 0;
				if(tele_arrays[0].v[i]) tele_arrays[0].v[i] = 0;
				else tele_arrays[0].v[i] = 1;
				(*update_tr)(i,tele_arrays[0].v[i]);
			}
		}
	}
}

void clear_delays(void) {
	for(int i=0;i<D_SIZE;i++) {
		delay_t[i] = 0;
	}

	delay_count = 0;
}




/////////////////////////////////////////////////////////////////
// MOD //////////////////////////////////////////////////////////

static void mod_PROB(tele_command_t *);
static void mod_DEL(tele_command_t *);
static void mod_Q(tele_command_t *);
static void mod_IF(tele_command_t *);
static void mod_ELIF(tele_command_t *);
static void mod_ELSE(tele_command_t *);
static void mod_ITER (tele_command_t *);

#define MAKEMOD(name, params, doc) {#name, mod_ ## name, params, doc}
#define MODS 7
static const tele_mod_t tele_mods[MODS] = {
	MAKEMOD(PROB, 1, "PROBABILITY TO CONTINUE EXECUTING LINE"),
	MAKEMOD(DEL, 1, "DELAY THIS COMMAND"),
	MAKEMOD(Q, 0, "ADD COMMAND TO QUEUE"),
	MAKEMOD(IF, 1, "IF CONDITION FOR COMMAND"),
	MAKEMOD(ELIF, 1, "ELSE IF"),
	MAKEMOD(ELSE, 0, "ELSE"),
	MAKEMOD(ITER, 2, "LOOPED COMMAND WITH ITERATION")
};


void mod_PROB(tele_command_t *c) { 
	int a = pop();

	tele_command_t cc;
	if(rand() % 101 < a) {
		cc.l = c->l - c->separator - 1;
		cc.separator = -1;
		memcpy(cc.data, &c->data[c->separator+1], cc.l * sizeof(tele_data_t));
		// sprintf(dbg,"\r\nsub-length: %d", cc.l); 
		process(&cc);
	}
}
void mod_DEL(tele_command_t *c) {
	int i = 0;
	int a = pop();

	while(delay_t[i] != 0 && i != D_SIZE)
		i++;

	if(i < D_SIZE) {
		delay_count++;
		if(delay_count == 1)
			(*update_delay)(1);
		delay_t[i] = a;

		delay_c[i].l = c->l - c->separator - 1;
		delay_c[i].separator = -1;

		memcpy(delay_c[i].data, &c->data[c->separator+1], delay_c[i].l * sizeof(tele_data_t));
	}	
}
void mod_Q(tele_command_t *c) {
	if(q_top < Q_SIZE) {
		q[q_top].l = c->l - c->separator - 1;
		memcpy(q[q_top].data, &c->data[c->separator+1], q[q_top].l * sizeof(tele_data_t));
		q[q_top].separator = -1;
		q_top++;
		if(q_top == 1)
			(*update_q)(1);
	}
}
void mod_IF(tele_command_t *c) {
	condition = FALSE;
	tele_command_t cc;
	if(pop()) {
		condition = TRUE;
		cc.l = c->l - c->separator - 1;
		cc.separator = -1;
		memcpy(cc.data, &c->data[c->separator+1], cc.l * sizeof(tele_data_t));
		// sprintf(dbg,"\r\nsub-length: %d", cc.l); 
		process(&cc);
	}
}
void mod_ELIF(tele_command_t *c) {
	tele_command_t cc;
	if(!condition) {
		if(pop()) {
			condition = TRUE;
			cc.l = c->l - c->separator - 1;
			cc.separator = -1;
			memcpy(cc.data, &c->data[c->separator+1], cc.l * sizeof(tele_data_t));
			// sprintf(dbg,"\r\nsub-length: %d", cc.l); 
			process(&cc);
		}
	}
}
void mod_ELSE(tele_command_t *c) {
	tele_command_t cc;
	if(!condition) {
		condition = TRUE;
		cc.l = c->l - c->separator - 1;
		cc.separator = -1;
		memcpy(cc.data, &c->data[c->separator+1], cc.l * sizeof(tele_data_t));
		// sprintf(dbg,"\r\nsub-length: %d", cc.l); 
		process(&cc);
	}
}
void mod_ITER(tele_command_t *c) {
	int a, b, d, i;
	tele_command_t cc;
	a = pop();
	b = pop();

	if(a < b) {
		d = b - a + 1;
		for(i = 0; i<d; i++) {
			tele_vars[V_I].v = a + i;
			cc.l = c->l - c->separator - 1;
			cc.separator = -1;
			memcpy(cc.data, &c->data[c->separator+1], cc.l * sizeof(tele_data_t));
			// sprintf(dbg,"\r\nsub-length: %d", cc.l); 
			process(&cc);
		}
	}
	else {
		d = a - b + 1;
		for(i = 0; i<d; i++) {
			tele_vars[V_I].v = a - i;
			cc.l = c->l - c->separator - 1;
			cc.separator = -1;
			memcpy(cc.data, &c->data[c->separator+1], cc.l * sizeof(tele_data_t));
			// sprintf(dbg,"\r\nsub-length: %d", cc.l); 
			process(&cc);
		}
	}
}




/////////////////////////////////////////////////////////////////
// OPS //////////////////////////////////////////////////////////

static void op_ADD(void);
static void op_SUB(void);
static void op_MUL(void);
static void op_DIV(void);
static void op_RAND(void);
static void op_RRAND(void);
static void op_TOSS(void);
static void op_MIN(void);
static void op_MAX(void);
static void op_LIM(void);
static void op_WRAP(void);
static void op_QT(void);
static void op_AVG(void);
static void op_EQ(void);
static void op_NE(void);
static void op_LT(void);
static void op_GT(void);
static void op_TR_TOG(void);
static void op_N(void);
static void op_Q_ALL(void);
static void op_Q_POP(void);
static void op_Q_CLR(void);
static void op_DEL_CLR(void);
static void op_M_RESET(void);
static void op_V(void);
static void op_VV(void);
static void op_P(void);
static void op_P_INS(void);
static void op_P_RM(void);
static void op_P_PUSH(void);
static void op_P_POP(void);
static void op_PN(void);
static void op_TR_PULSE(void);
static void op_II(void);

#define MAKEOP(name, params, returns, doc) {#name, op_ ## name, params, returns, doc}
#define OPS 34
static const tele_op_t tele_ops[OPS] = {
	MAKEOP(ADD, 2, 1,"[A B] ADD A TO B"),
	MAKEOP(SUB, 2, 1,"[A B] SUBTRACT B FROM A"),
	MAKEOP(MUL, 2, 1,"[A B] MULTIPLY TWO VALUES"),
	MAKEOP(DIV, 2, 1,"[A B] DIVIDE FIRST BY SECOND"),
	MAKEOP(RAND, 1, 1,"[A] RETURN RANDOM NUMBER UP TO A"),
	MAKEOP(RRAND, 2, 1,"[A B] RETURN RANDOM NUMBER BETWEEN A AND B"),
	MAKEOP(TOSS, 0, 1,"RETURN RANDOM STATE"),
	MAKEOP(MIN, 2, 1,"RETURN LESSER OF TWO VALUES"),
	MAKEOP(MAX, 2, 1,"RETURN GREATER OF TWO VALUES"),
	MAKEOP(LIM, 3, 1,"[A B C] LIMIT C TO RANGE A TO B"),
	MAKEOP(WRAP, 3, 1,"[A B C] WRAP C WITHIN RANGE A TO B"),
	MAKEOP(QT, 2, 1,"[A B] QUANTIZE A TO STEP SIZE B"),
	MAKEOP(AVG, 2, 1,"AVERAGE TWO VALUES"),
	MAKEOP(EQ, 2, 1,"LOGIC: EQUAL"),
	MAKEOP(NE, 2, 1,"LOGIC: NOT EQUAL"),
	MAKEOP(LT, 2, 1,"LOGIC: LESS THAN"),
	MAKEOP(GT, 2, 1,"LOGIC: GREATER THAN"),
	{"TR.TOG", op_TR_TOG, 1, 0, "[A] TOGGLE TRIGGER A"},
	MAKEOP(N, 1, 1, "TABLE FOR NOTE VALUES"),
	{"Q.ALL", op_Q_ALL, 0, 0, "Q: EXECUTE ALL"},
	{"Q.POP", op_Q_POP, 0, 0, "Q: POP LAST"},
	{"Q.CLR", op_Q_CLR, 0, 0, "Q: FLUSH"},
	{"DEL.CLR", op_DEL_CLR, 0, 0, "DELAY: FLUSH"},
	{"M.RESET", op_M_RESET, 0, 0, "METRO: RESET"},
	MAKEOP(V, 1, 1, "TO VOLT"),
	MAKEOP(VV, 2, 1, "TO VOLT WITH PRECISION"),
	{"P", op_P, 1, 1, "PATTERN: GET/SET"},
	{"P.INS", op_P_INS, 2, 0, "PATTERN: INSERT"},
	{"P.RM", op_P_RM, 1, 0, "PATTERN: REMOVE"},
	{"P.PUSH", op_P_PUSH, 1, 0, "PATTERN: PUSH"},
	{"P.POP", op_P_POP, 0, 1, "PATTERN: POP"},
	{"PN", op_PN, 2, 1, "PATTERN: GET/SET N"},
	{"TR.PULSE", op_TR_PULSE, 1, 0, "PULSE TRIGGER"},
	{"II", op_II, 2, 0, "II"}
};

static void op_ADD() {
	push(pop() + pop());
}
static void op_SUB() { 
	push(pop() - pop());
}
static void op_MUL() { 
	push(pop() * pop());
}
static void op_DIV() { 
	push(pop() / pop());
}
static void op_RAND() { 
	int a = pop();
	if(a < 0)
		a = (a * -1);
	push(rand() % (a+1));
	
}
static void op_RRAND() {
	int a, b, min, max, range;
	a = pop();
	b = pop();
	if(a < b) {
		min = a;
		max = b; 
	}
	else {
		min = b;
		max = a;
	}
	range = max - min + 1;
	if(range == 0) push(a);
	else
		push(rand() % range + min);
}
static void op_TOSS() {
	push(rand() & 1);
}
static void op_MIN() { 
	int a, b;
	a = pop();
	b = pop();
	if(b > a) push(a);
	else push(b);
}
static void op_MAX() { 
	int a, b;
	a = pop();
	b = pop();
	if(a > b) push(a);
	else push(b);
}
static void op_LIM() {
	int a, b, i;
	a = pop();
	b = pop();
	i = pop();
	if(i < a) push(a);
	else if(i > b) push(b);
}
static void op_WRAP() {
	int a, b, i, c;
	a = pop();
	b = pop();
	i = pop();
	if(a < b) {
		c = b - a;
		while(i >= b)
			i -= c;
		while(i < a)
			i += c;
	}
	else {
		c = a - b;
		while(i >= a)
			i -= c;
		while(i < b)
			i += c;
	}
	push(i);
}
static void op_QT() {
	int a, b;
	a = pop();
	b = pop();

	// TODO
}
static void op_AVG() {
	push((pop() + pop()) / 2);
}
static void op_EQ() { 
	push(pop() == pop());
}
static void op_NE() {
	push(pop() != pop());
}
static void op_LT() { 
	push(pop() < pop());
}
static void op_GT() { 
	push(pop() > pop());
}
static void op_TR_TOG() {
	int a = pop();
	// saturate and shift
	if(a < 1) a = 1;
	else if(a > 4) a = 4;
	a--;
	if(tele_arrays[0].v[a]) tele_arrays[0].v[a] = 0;
	else tele_arrays[0].v[a] = 1;
	update_tr(a,tele_arrays[0].v[a]);
}
static void op_N() { 
	int a = pop();

	if(a < 0) {
		if(a < -127) a = -127;
		a *= -1;
		push(-1 * table_n[a]);
	}
	else {
		if(a > 127) a = 127;
		push(table_n[a]);
	}
}
static void op_Q_ALL() {
	for(int i = 0;i<q_top;i++)
		process(&q[q_top-i-1]);
	q_top = 0;
	(*update_q)(0);
}
static void op_Q_POP() {
	if(q_top) {
		q_top--;
		process(&q[q_top]);
		if(q_top == 0)
			(*update_q)(0);
	}
}
static void op_Q_CLR() {
	q_top = 0;
	(*update_q)(0);
}
static void op_DEL_CLR() {
	clear_delays();
}
static void op_M_RESET() {
	(*update_metro)(tele_vars[V_M].v, tele_vars[V_M_ACT].v, 1);
}
static void op_V() {
	int a = pop();
	if(a < 0) a = 0;
	else if(a > 10) a = 10;
	push(table_v[a]);
}
static void op_VV() {
	int a = pop();
	int b = pop();
	if(a < 0) a = 0;
	else if(a > 10) a = 10;
	if(b < 0) b = 0;
	else if(b > 99) b = 99;
	push(table_v[a] + table_vv[b]);
}
static void op_P() {
	int a, b;
	a = pop();
	if(a < 0) a = 0;
	else if(a > 63) a = 63;

	if(top == 0) {
		push(tele_patterns[pn].v[a]);
	}
	else if(top == 1) {
		b = pop();
		tele_patterns[pn].v[a] = b;
	}
}
static void op_P_INS() {
	int a, b, i;
	a = pop();
	b = pop();

	if(a<0) a = 0;
	else if(a>63) a =63;

	if(tele_patterns[pn].l >= a) {
		for(i = tele_patterns[pn].l;i>a;i--)
			tele_patterns[pn].v[i] = tele_patterns[pn].v[i-1];
		if(tele_patterns[pn].l < 63)
			tele_patterns[pn].l++;
	}

	tele_patterns[pn].v[a] = b;
}
static void op_P_RM() {
	int a, i;
	a = pop();

	if(a < 0) a = 0;
	else if(a > tele_patterns[pn].l) a = tele_patterns[pn].l;

	if(tele_patterns[pn].l > 0) {
		for(i = a;i<tele_patterns[pn].l;i++)
			tele_patterns[pn].v[i] = tele_patterns[pn].v[i+1];

		tele_patterns[pn].l--;
	}
}
static void op_P_PUSH() {
	int a;
	a = pop();

	if(tele_patterns[pn].l < 63) {
		tele_patterns[pn].v[tele_patterns[pn].l] = a;
		tele_patterns[pn].l++;
	}
}
static void op_P_POP() {
	if(tele_patterns[pn].l > 0) {
		tele_patterns[pn].l--;
 		push(tele_patterns[pn].v[tele_patterns[pn].l]);
	}
	else push(0);
}
static void op_PN() {
	int a, b, c;
	a = pop();
	b = pop();

	if(a < 0) a = 0;
	else if(a > 3) a = 33;
	if(a < 0) a = 0;
	else if(a > 63) a = 63;

	if(top == 0) {
		push(tele_patterns[a].v[b]);
	}
	else if(top == 1) {
		c = pop();
		tele_patterns[a].v[b] = c;
	}
}
static void op_TR_PULSE() {
	int a = pop();
	// saturate and shift
	if(a < 1) a = 1;
	else if(a > 4) a = 4;
	a--;
	if(tele_arrays[0].v[a]) tele_arrays[0].v[a] = 0;
	else tele_arrays[0].v[a] = 1;
	tr_pulse[a] = tele_arrays[4].v[a]; // set time
	update_tr(a,tele_arrays[0].v[a]);
}
static void op_II() {
	int a = pop();
	int b = pop();
	update_ii(a,b);
}


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// PROCESS //////////////////////////////////////////////////////

error_t parse(char *cmd) {
	char c[32];
	strcpy(c,cmd);
	const char *delim = " \n";
	const char *s = strtok(c,delim);

	uint8_t n = 0;
	temp.l = n;

	// sprintf(dbg,"\r\nparse: ");

    while(s) {
    	// CHECK IF NUMBER
 		if(isdigit(s[0]) || s[0]=='-') {
 			temp.data[n].t = NUMBER;
			temp.data[n].v = strtol(s, NULL, 0);		
		}
		else if(s[0]==':')
			temp.data[n].t = SEP;
		else {
			// CHECK AGAINST VARS
			int i = VARS - 1;

			do {
				// print_dbg("\r\nvar '");
				// print_dbg(tele_vars[i].name);
				// print_dbg("'");

				if(!strcmp(s,tele_vars[i].name)) {
					temp.data[n].t = VAR;
					temp.data[n].v = i;
					// sprintf(dbg,"v(%d) ", temp.data[n].v);
		            break;
				}
			} while(i--);

			if(i == -1) {
				// CHECK AGAINST ARRAYS
			    i = ARRAYS;

			    while(i--) {
			  //   	print_dbg("\r\narrays '");
					// print_dbg(tele_arrays[i].name);
					// print_dbg("'");

			        if(!strcmp(s,tele_arrays[i].name)) {
	 					temp.data[n].t = ARRAY;
						temp.data[n].v = i;
						// sprintf(dbg,"a(%d) ", temp.data[n].v);
			            break;
			        }
			    }
			}
			
			if(i == -1) {
				// CHECK AGAINST OPS
			    i = OPS;

			    while(i--) {
			  //   	print_dbg("\r\nops '");
					// print_dbg(tele_ops[i].name);
					// print_dbg("'");

			        if(!strcmp(s,tele_ops[i].name)) {
	 					temp.data[n].t = OP;
						temp.data[n].v = i;
						// sprintf(dbg,"f(%d) ", temp.data[n].v);
			            break;
			        }
			    }
			}

			if(i == -1) {
				// CHECK AGAINST MOD
			    i = MODS;

			    while(i--) {
			  //   	print_dbg("\r\nmods '");
					// print_dbg(tele_mods[i].name);
					// print_dbg("'");

			        if(!strcmp(s,tele_mods[i].name)) {
	 					temp.data[n].t = MOD;
						temp.data[n].v = i;
						// sprintf(dbg,"f(%d) ", temp.data[n].v);
			            break;
			        }
			    }
			}

			if(i == -1) {
				// CHECK AGAINST KEY
			    i = KEYS;

			    while(i--) {
			  //   	print_dbg("\r\nmods '");
					// print_dbg(tele_mods[i].name);
					// print_dbg("'");

			        if(!strcmp(s,tele_keys[i].name)) {
	 					temp.data[n].t = KEY;
						temp.data[n].v = i;
						// sprintf(dbg,"f(%d) ", temp.data[n].v);
			            break;
			        }
			    }
			}

		    if(i == -1) {
		    	strcpy(error_detail, s);
		    	return E_PARSE;
		    }
		}

	    s = strtok(NULL,delim);

	    n++;
	    temp.l = n;

	    if(n == COMMAND_MAX_LENGTH)
	    	return E_LENGTH;
	}

    // sprintf(dbg,"// length: %d", temp.l);

    return E_OK;
}


/////////////////////////////////////////////////////////////////
// VALIDATE /////////////////////////////////////////////////////

error_t validate(tele_command_t *c) {
	int h = 0;
	uint8_t n = c->l;
	c->separator = -1;

	while(n--) {
		if(c->data[n].t == OP) {
			h -= tele_ops[c->data[n].v].params;
			
			if(h < 0) {
				strcpy(error_detail, tele_ops[c->data[n].v].name);
				return E_NEED_PARAMS;
			}
			h += tele_ops[c->data[n].v].returns;
			// hack for var-length params for P
			if(c->data[n].v == 26 || c->data[n].v == 31) {
				if(n==0) 
					h--;
				else if(c->data[n-1].t == SEP)
					h--;
			}
					
		}
		else if(c->data[n].t == MOD) {
			strcpy(error_detail, tele_mods[c->data[n].v].name);
			if(n != 0)
				return E_NO_MOD_HERE;
			else if(c->separator == -1)
				return E_NEED_SEP;
			else if(h < tele_mods[c->data[n].v].params) 
				return E_NEED_PARAMS;
			else if(h > tele_mods[c->data[n].v].params) 
				return E_EXTRA_PARAMS;
			else h = 0;
		}
		else if(c->data[n].t == SEP) {
			if(c->separator != -1)
				return E_MANY_SEP;

			c->separator = n;
			if(h > 1) 
				return E_EXTRA_PARAMS;
			else h = 0;
		}

		// RIGHT (get)
		else if(n && c->data[n-1].t != SEP) {
			if(c->data[n].t == NUMBER || c->data[n].t == VAR ||  c->data[n].t == KEY) {
				h++;
			}
			else if(c->data[n].t == ARRAY) {
				if(h < 1) {
					strcpy(error_detail, tele_arrays[c->data[n].v].name);
					return E_NEED_PARAMS;
				}
				// h-- then h++
			}
		}
		// LEFT (set)
		else {
			if(c->data[n].t == NUMBER || c->data[n].t == KEY) {
				h++;
			}
			else if(c->data[n].t == VAR) {
				if(h == 0) h++;
				// else { 
				// 	h--;
				// 	if(h > 0)
				// 		return E_EXTRA_PARAMS;
				// }
			}
			else if(c->data[n].t == ARRAY) {
				if(h < 1) {
					strcpy(error_detail, tele_arrays[c->data[n].v].name);
					return E_NEED_PARAMS;
				}
				h--;
				if(h == 0) h++;
				// else if(h > 1)
					// return E_EXTRA_PARAMS;
			}
		}
	}

	if(h > 1)
		return E_EXTRA_PARAMS;
	else
		return E_OK;
}



/////////////////////////////////////////////////////////////////
// PROCESS //////////////////////////////////////////////////////

void process(tele_command_t *c) {
	top = 0;
	int i;
	int n;

	if(c->separator == -1)
		n = c->l;
	else
		n = c->separator;

	// sprintf(dbg,"\r\r\nprocess (%d): %s", n, print_command(c));
	// DBG;

	while(n--) {
		if(c->data[n].t == NUMBER)
			push(c->data[n].v);
		else if(c->data[n].t == OP)
			tele_ops[c->data[n].v].func();
		else if(c->data[n].t == MOD)
			tele_mods[c->data[n].v].func(c);
		else if(c->data[n].t == VAR) {
			if(tele_vars[c->data[n].v].func == NULL) {
				if(n || top == 0 )
					push(tele_vars[c->data[n].v].v);
				else
					tele_vars[c->data[n].v].v = pop();
			}
			else
				tele_vars[c->data[n].v].func(0);
		}
 		else if(c->data[n].t == ARRAY) {
			i = pop();

			// saturate for 1-4 indexing
			if(i<1) i=0;
			else if(i>3) i=4;
			i--;

			if(n || top == 0) {
					// sprintf(dbg,"\r\nget array %s @ %d : %d", tele_arrays[c->data[n].v].name, i, tele_arrays[c->data[n].v].v[i]);
					// DBG
				push(tele_arrays[c->data[n].v].v[i]);
			}
			else {
				// sprintf(dbg,"\r\nset array %s @ %d to %d", tele_arrays[c->data[n].v].name, i, tele_arrays[c->data[n].v].v[i]);
				// DBG
				if(tele_arrays[c->data[n].v].func)
					tele_arrays[c->data[n].v].func(i);
				else
					tele_arrays[c->data[n].v].v[i] = pop();
			}
		}
		else if(c->data[n].t == KEY) {
			push(tele_keys[c->data[n].v].v);
		}
	}

	// PRINT DEBUG OUTPUT IF VAL LEFT ON STACK
	if(top) {
		output = pop();
		output_new++;
		sprintf(dbg,"\r\n>>> %d", output);
		DBG
		// to_v(output);
	}
}



char * print_command(const tele_command_t *c) {
	int n = 0;
	char number[8];
	char *p = pcmd;

	*p = 0;

	while(n < c->l) {
		switch(c->data[n].t) {
			case OP:
				strcpy(p, tele_ops[c->data[n].v].name);
				p += strlen(tele_ops[c->data[n].v].name) - 1;
				break;
			case ARRAY:
				strcpy(p,tele_arrays[c->data[n].v].name);
				p += strlen(tele_arrays[c->data[n].v].name) - 1;
				break;
			case NUMBER:
				itoa(c->data[n].v,number,10);
				strcpy(p, number);
				p+=strlen(number) - 1;
				break;
			case VAR:
				strcpy(p,tele_vars[c->data[n].v].name);
				p += strlen(tele_vars[c->data[n].v].name) - 1;
				break;
			case MOD:
				strcpy(p, tele_mods[c->data[n].v].name);
				p += strlen(tele_mods[c->data[n].v].name) - 1;
				break;
			case SEP:
				*p = ':';
				break;
			case KEY:
				strcpy(p,tele_keys[c->data[n].v].name);
				p += strlen(tele_keys[c->data[n].v].name) - 1;
				break;
			default:
				break;
		}

		n++;
		p++;
		*p = ' ';
		p++; 
	}
	p--;
	*p = 0;

	return pcmd;
}



int tele_get_array(uint8_t a, uint8_t i) {
	return tele_arrays[a].v[i];
}

void tele_set_array(uint8_t a, uint8_t i, uint16_t v) {
	tele_arrays[a].v[i] = v;
}

void tele_set_val(uint8_t i, uint16_t v) {
	tele_vars[i].v = v;
}


void tele_tick(uint8_t i) {
	process_delays(i);

	// inc time
	if(tele_vars[V_TIME_ACT].v)
		tele_vars[V_TIME].v += i;
}

void tele_init() {
	u8 i;

	for(i=0;i<4;i++) {
		tele_patterns[i].i = 0;
		tele_patterns[i].l = 0;
		tele_patterns[i].wrap = 1;
		tele_patterns[i].start = 0;
		tele_patterns[i].end = 63;
	}
}


const char * to_v(int i) {
	static char n[3], v[7];
	int a=0, b=0;

	if(i > table_v[8]) {
		i -= table_v[8];
		a += 8;
	}

	if(i > table_v[4]) {
		i -= table_v[4];
		a += 4;
	}

	if(i > table_v[2]) {
		i -= table_v[2];
		a += 2;
	}

	if(i > table_v[1]) {
		i -= table_v[1];
		a += 1;
	}

	if(i > table_vv[64]) {
		i -= table_vv[64];
		b += 64;
	}

	if(i > table_vv[32]) {
		i -= table_vv[32];
		b += 32;
	}

	if(i > table_vv[16]) {
		i -= table_vv[16];
		b += 16;
	}

	if(i > table_vv[8]) {
		i -= table_vv[8];
		b += 8;
	}

	if(i > table_vv[4]) {
		i -= table_vv[4];
		b += 4;
	}

	if(i > table_vv[2]) {
		i -= table_vv[2];
		b++;
	}

	if(i > table_vv[1]) {
		i -= table_vv[1];
		b++;
	}

	b++;

	itoa(a,n,10);
	strcpy(v, n);
	strcat(v,".");
	itoa(b,n,10);
	strcat(v, n);
	strcat(v,"V");

	return v;
	// printf(" (VV %d %d)",a,b);
}
