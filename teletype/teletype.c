#include <string.h>
#include <stdio.h>		// printf
#include <stdlib.h>		// rand, strtol
#include <ctype.h>		// isdigit
#include <stdint.h>		// types

#include "teletype.h"
#include "table.h"

#ifdef SIM
#define DBG printf("%s",dbg);
#else
#include "print_funcs.h"
#define DBG print_dbg(dbg);
#endif


static const char * errordesc[] = {
	"OK",
	"UNKOWN WORD",
	"COMMAND TOO LONG",
	"NOT ENOUGH PARAMS",
	"TOO MANY PARAMS",
	"MOD NOT ALLOWED HERE",
	"MORE THAN ONE SEPARATOR",
	"NEED SEPARATOR"
};

const char * tele_error(error_t e) {
	return errordesc[e];
}

static char dbg[64];

uint8_t odirty;
int output;

tele_command_t temp;
static char condition;

static tele_command_t q[Q_SIZE];
static uint8_t q_top;


volatile update_metro_t update_metro;


/////////////////////////////////////////////////////////////////
// DELAY ////////////////////////////////////////////////////////

static tele_command_t delay_c[D_SIZE];
static uint delay_t[D_SIZE];

static void process_delays(void);

static void process_delays() {
	for(int i=0;i<D_SIZE;i++) {
		if(delay_t[i]) {
 			if(--delay_t[i] == 0) {
 				sprintf(dbg,"\r\ndelay %d", i);
				DBG
				process(&delay_c[i]);
			}
		}
	}
}

void clear_delays(void);

void clear_delays(void) {
	for(int i=0;i<D_SIZE;i++) {
		delay_t[i] = 0;
	}
}


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
// VARS ARRAYS //////////////////////////////////////////////////

// {NAME,VAL}

// ENUM IN HEADER

#define VARS 16
static tele_var_t tele_vars[VARS] = {
	{"I",0},	// gets overwritten by ITER
	{"TIME",0},
	{"TIME.ACT",1},
	{"IN",0},
	{"PARAM",0},
	{"PRESET",0},
	{"M",1000},
	{"M.ACT",1},
	{"X",0},
	{"Y",0},
	{"Z",0},
	{"T",0},
	{"A",1},
	{"B",2},
	{"C",3},
	{"D",4}
};


#define MAKEARRAY(name) {#name, {0,0,0,0}}
#define ARRAYS 6
static tele_array_t tele_arrays[ARRAYS] = {
	MAKEARRAY(TR),
	MAKEARRAY(CV),
	MAKEARRAY(CV.SLEW),
	MAKEARRAY(CV.OFFSET),
	MAKEARRAY(CV.TIME),
	MAKEARRAY(CV.NOW)
};



/////////////////////////////////////////////////////////////////
// MOD //////////////////////////////////////////////////////////

static void mod_PROB(tele_command_t *);
static void mod_DELAY(tele_command_t *);
static void mod_Q(tele_command_t *);
static void mod_IF(tele_command_t *);
static void mod_ELIF(tele_command_t *);
static void mod_ELSE(tele_command_t *);
static void mod_ITER (tele_command_t *);

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
void mod_DELAY(tele_command_t *c) {
	int a = pop();

	int i = 0;
	while(delay_t[i] != 0 && i != D_SIZE)
		i++;

	if(i < D_SIZE) {
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
	tele_command_t cc;
	int a, b, d, i;
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

#define MAKEMOD(name, params, doc) {#name, mod_ ## name, params, doc}
#define MODS 7
static const tele_mod_t tele_mods[MODS] = {
	MAKEMOD(PROB, 1, "PROBABILITY TO CONTINUE EXECUTING LINE"),
	MAKEMOD(DELAY, 1, "DELAY THIS COMMAND"),
	MAKEMOD(Q, 0, "ADD COMMAND TO QUEUE"),
	MAKEMOD(IF, 1, "IF CONDITION FOR COMMAND"),
	MAKEMOD(ELIF, 1, "ELSE IF"),
	MAKEMOD(ELSE, 0, "ELSE"),
	MAKEMOD(ITER, 2, "LOOPED COMMAND WITH ITERATION")
};



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
static void op_TR_TOGGLE(void);
static void op_N(void);
static void op_Q_ALL(void);
static void op_Q_POP(void);
static void op_Q_FLUSH(void);
static void op_DELAY_FLUSH(void);
static void op_M_RESET(void);

void op_ADD() { push(pop() + pop()); }
void op_SUB() { push(pop() - pop()); }
void op_MUL() { push(pop() * pop()); }
void op_DIV() { push(pop() / pop()); }
void op_RAND() { 
	int a;
	a = pop();
	if(a < 0) a = (a * -1);
	push(rand() % (a+1));
}
void op_RRAND() { 
	int a,b, min, max, range;
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
	range = max - min;
	if(range == 0) push(a);
	else push(rand() % range + min); 
}
void op_TOSS() { push(rand() & 1); }
void op_MIN() { 
	int a, b;
	a = pop();
	b = pop();
	if(b > a) push(a);
	else push(b);
}
void op_MAX() { 
	int a, b;
	a = pop();
	b = pop();
	if(a > b) push(a);
	else push(b);
}
void op_LIM() {
	int a, b, i;
	a = pop();
	b = pop();
	i = pop();
	if(i < a) i = a;
	else if(i > b) i = b;
	push(i);
}
void op_WRAP() {
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
void op_QT() {
	int a, b;
	a = pop();
	b = pop();

	// HERE
}
void op_AVG() { push((pop() + pop()) / 2); }
void op_EQ() { push(pop() == pop()); }
void op_NE() { push(pop() != pop()); }
void op_LT() { push(pop() < pop()); }
void op_GT() { push(pop() > pop()); }
void op_TR_TOGGLE() {
	int a = pop();
	// saturate and shift
	if(a < 1) a = 1;
	else if(a > 4) a = 4;
	a--;
	if(tele_arrays[0].v[a]) tele_arrays[0].v[a] = 0;
	else tele_arrays[0].v[a] = 1;
}
void op_N() { 
	int a;
	a = pop();

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
void op_Q_ALL() {
	for(int i = 0;i<q_top;i++)
		process(&q[q_top-i-1]);
	q_top = 0;
}
void op_Q_POP() {
	if(q_top) {
		q_top--;
		process(&q[q_top]);
	}
}
void op_Q_FLUSH() {
	q_top = 0;
}
void op_DELAY_FLUSH() {
	clear_delays();
}
void op_M_RESET() {
	(*update_metro)(tele_vars[V_M].v, tele_vars[V_M_ACT].v, 1);
}


#define MAKEOP(name, params, doc) {#name, op_ ## name, params, doc}
#define OPS 24
static const tele_op_t tele_ops[OPS] = {
	MAKEOP(ADD, 2, "[A B] ADD A TO B"),
	MAKEOP(SUB, 2, "[A B] SUBTRACT B FROM A"),
	MAKEOP(MUL, 2, "[A B] MULTIPLY TWO VALUES"),
	MAKEOP(DIV, 2, "[A B] DIVIDE FIRST BY SECOND"),
	MAKEOP(RAND, 1, "[A] RETURN RANDOM NUMBER UP TO A"),
	MAKEOP(RRAND, 2, "[A B] RETURN RANDOM NUMBER BETWEEN A AND B"),
	MAKEOP(TOSS, 0, "RETURN RANDOM STATE"),
	MAKEOP(MIN, 2, "RETURN LESSER OF TWO VALUES"),
	MAKEOP(MAX, 2, "RETURN GREATER OF TWO VALUES"),
	MAKEOP(LIM, 3, "[A B C] LIMIT C TO RANGE A TO B"),
	MAKEOP(WRAP, 3, "[A B C] WRAP C WITHIN RANGE A TO B"),
	MAKEOP(QT, 2, "[A B] QUANTIZE A TO STEP SIZE B"),
	MAKEOP(AVG, 2, "AVERAGE TWO VALUES"),
	MAKEOP(EQ, 2, "LOGIC: EQUAL"),
	MAKEOP(NE, 2, "LOGIC: NOT EQUAL"),
	MAKEOP(LT, 2, "LOGIC: LESS THAN"),
	MAKEOP(GT, 2, "LOGIC: GREATER THAN"),
	{"TR.TOGGLE", op_TR_TOGGLE, 1, "[A] TOGGLE TRIGGER A"},
	MAKEOP(N, 1, "TABLE FOR NOTE VALUES"),
	{"Q.ALL", op_Q_ALL, 0, "Q: EXECUTE ALL"},
	{"Q.POP", op_Q_POP, 0, "Q: POP LAST"},
	{"Q.FLUSH", op_Q_FLUSH, 0, "Q: FLUSH"},
	{"DELAY.FLUSH", op_DELAY_FLUSH, 0, "DELAY: FLUSH"},
	{"M.RESET", op_M_RESET, 0, "METRO: RESET"}
};



/////////////////////////////////////////////////////////////////
// PROCESS //////////////////////////////////////////////////////

error_t parse(char *cmd) {
	const char *delim = " \n";
	const char* s = strtok(cmd,delim);

	uint8_t n = 0;
	temp.l = n;

	// sprintf(dbg,"\r\nparse: ");

    while(s) {
    	// CHECK IF NUMBER
 		if(isdigit(s[0]) || s[0]=='-') {
 			temp.data[n].t = NUMBER;
			temp.data[n].v = strtol(s, NULL, 0);
			// sprintf(dbg,"n:%d ", temp.data[n].v);
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

		    if(i == -1)
		    	return E_PARSE;
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
			if(h < 0)
				return E_NEED_PARAMS;
			h++;
		}
		else if(c->data[n].t == MOD) {
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
			if(c->data[n].t == NUMBER || c->data[n].t == VAR) {
				h++;
			}
			else if(c->data[n].t == ARRAY) {
				if(h < 0)
					return E_NEED_PARAMS;
				// h-- then h++
			}
		}
		// LEFT (set)
		else {
			if(c->data[n].t == NUMBER) {
				h++;
			}
			else if(c->data[n].t == VAR) {
				if(h == 0) h++;
				else { 
					h--;
					if(h > 0)
						return E_EXTRA_PARAMS;
				}
			}
			else if(c->data[n].t == ARRAY) {
				h--;
				if(h < 0)
					return E_NEED_PARAMS;
				if(h == 0) h++;
				else {
					h--;
					if(h > 0)
						return E_EXTRA_PARAMS;
				}
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

	sprintf(dbg,"\r\r\nprocess (%d):", n);
	DBG
	print_command(c);

	while(n--) {
		if(c->data[n].t == OP)
			tele_ops[c->data[n].v].func();
		// MODS
		else if(c->data[n].t == MOD) {
			tele_mods[c->data[n].v].func(c);
		}
		// RIGHT (get)
		else if(n) {
			if(c->data[n].t == NUMBER)
				push(c->data[n].v);
			else if(c->data[n].t == VAR) {
				sprintf(dbg,"\r\nget var %s : %d", tele_vars[c->data[n].v].name, tele_vars[c->data[n].v].v);
				DBG
				push(tele_vars[c->data[n].v].v);
			}
			else if(c->data[n].t == ARRAY) {
				i = pop();

				// saturate for 1-4 indexing
				if(i<1) i=0;
				else if(i>3) i=4;
				i--;

				sprintf(dbg,"\r\nget array %s @ %d : %d", tele_arrays[c->data[n].v].name, i, tele_arrays[c->data[n].v].v[i]);
				DBG
				push(tele_arrays[c->data[n].v].v[i]);
			}
		}

		// LEFTMOST (set/get)
		else {
			if(c->data[n].t == NUMBER)
				push(c->data[n].v);
			else if(c->data[n].t == VAR) {
				// LONE GET JUST PRINTS
				if(top == 0) {
					sprintf(dbg,"\r\nget var %s : %d", tele_vars[c->data[n].v].name, tele_vars[c->data[n].v].v);
					DBG
					push(tele_vars[c->data[n].v].v);
				}
				// OTHERWISE SET
				else {
					tele_vars[c->data[n].v].v = pop();
					sprintf(dbg,"\r\nset var %s to %d", tele_vars[c->data[n].v].name, tele_vars[c->data[n].v].v);
					DBG

					if(c->data[n].v == V_PRESET) {
						;;
					}
					else if(c->data[n].v == V_M) {
						(*update_metro)(tele_vars[V_M].v, tele_vars[V_M_ACT].v, 0);
					} 
					else if(c->data[n].v == V_M_ACT) {
						(*update_metro)(tele_vars[V_M].v, tele_vars[V_M_ACT].v, 0);
					}
				}
			}
			else if(c->data[n].t == ARRAY) {
				i = pop();
				// saturate for 1-4 indexing
				if(i<1) i=1;
				else if(i>4) i=4;
				i--;

				if(top == 0) {
 					sprintf(dbg,"\r\nget array %s @ %d : %d", tele_arrays[c->data[n].v].name, i, tele_arrays[c->data[n].v].v[i]);
 					DBG
					push(tele_arrays[c->data[n].v].v[i]);
				}
				else {
					tele_arrays[c->data[n].v].v[i] = pop();
					sprintf(dbg,"\r\nset array %s @ %d to %d", tele_arrays[c->data[n].v].name, i, tele_arrays[c->data[n].v].v[i]);
					DBG
					odirty++;
				}
			}
		}
	}

	// PRINT DEBUG OUTPUT IF VAL LEFT ON STACK
	if(top) {
		output = pop();
		sprintf(dbg,"\r\n>>> %d", output);
		DBG
	}
}



void print_command(const tele_command_t *c) {
	int n = 0;
	while(n < c->l) {
		sprintf(dbg," ");
		DBG
		switch(c->data[n].t) {
			case OP:
				strcpy(dbg,tele_ops[c->data[n].v].name);
				DBG
				break;
			case MOD:
				strcpy(dbg,tele_mods[c->data[n].v].name);
				DBG
				break;
			case SEP:
				strcpy(dbg,":");
				DBG
				break;
			case NUMBER:
				sprintf(dbg, "%d", c->data[n].v);
				DBG
				break;
			case VAR:
				strcpy(dbg,tele_vars[c->data[n].v].name);
				DBG
				break;
			case ARRAY:
				strcpy(dbg,tele_arrays[c->data[n].v].name);
				DBG
				break;
			default:
				break;
		}

		n++;
	}
}



int tele_get_array(uint8_t a, uint8_t i) {
	return tele_arrays[a].v[i];
}

void tele_set_array(uint8_t a, uint8_t i, uint16_t v) {
	tele_arrays[a].v[i] = v;
	odirty++;
}

void tele_set_val(uint8_t i, uint16_t v) {
	tele_vars[i].v = v;
}


void tele_tick() {
	process_delays();

	// inc time
	if(tele_vars[2].v)
		tele_vars[1].v++;
}
