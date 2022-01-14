/*
    exprs_test.c - simple test code for lib_exprs.[ch].
    Copyright (C) 2022 David Shepperd

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

#include "lib_exprs.h"
#include "exprs_test.h"

typedef struct
{
	const char *expr;						/* Expression to test */
	ExprsTermTypes_t expectedResultType;	/* Expected type of result */
	long expectedInt;						/* If result type is int, then expect this value */
	double expectedFloat;					/* If result type is float, then expect this value */
	const char *expectedString;				/* If result type is string, then expect this value */
} TestExprs_t;

static const TestExprs_t TestExprs[] = 
{
	{ "\"plainString\"", EXPRS_TERM_STRING, 0, 0, "plainString" }, /* Text string (text delimited with quotes) */
	{ "\"plain\\\"S\\\"tring\"", EXPRS_TERM_STRING, 0, 0, "plain\"S\"tring" }, /* Text string (text delimited with quotes) */
	{ "'plainString'", EXPRS_TERM_STRING, 0, 0, "plainString" }, /* Text string (text delimited with quotes) */
	{ "3.14159", EXPRS_TERM_FLOAT, 0, 3.14159, NULL },	/* floating point number */
	{ "100", EXPRS_TERM_INTEGER, 100, 0, NULL },	/* integer number */
	{ "0xFF", EXPRS_TERM_INTEGER, 0xFF, 0, NULL },	/* integer number */
	{ "0d1234", EXPRS_TERM_INTEGER, 1234, 0, NULL },	/* integer number */
	{ "0b11111111", EXPRS_TERM_INTEGER, 255, 0, NULL },	/* integer number */
	{ "+21", EXPRS_TERM_INTEGER, 21, 0, NULL },	// EXPRS_TERM_PLUS, /* + (unary term) */
	{ "+21.0", EXPRS_TERM_FLOAT, 0, 21.0, NULL },	// EXPRS_TERM_PLUS, /* + (unary term) */
	{ "-22", EXPRS_TERM_INTEGER, -22, 0, NULL }, // EXPRS_TERM_MINUS,	/* - (unary term in this case) */
	{ "-22.0", EXPRS_TERM_FLOAT, 0, -22.0, NULL }, // EXPRS_TERM_MINUS,	/* - (unary term in this case) */
	{ "~3", EXPRS_TERM_INTEGER, ~3, 0, NULL }, 	// EXPRS_TERM_COM,		/* ~ */
	{ "~3.3", EXPRS_TERM_INTEGER, ~3, 0, NULL }, 	// EXPRS_TERM_COM,		/* ~ */
	{ "~+3", EXPRS_TERM_INTEGER, ~3, 0, NULL }, 	// EXPRS_TERM_COM,		/* ~ */
	{ "~+3.3", EXPRS_TERM_INTEGER, ~3, 0, NULL }, 	// EXPRS_TERM_COM,		/* ~ */
	{ "~-3", EXPRS_TERM_INTEGER, ~-3, 0, NULL }, 	// EXPRS_TERM_COM,		/* ~ */
	{ "~-3.3", EXPRS_TERM_INTEGER, ~-3, 0, NULL }, 	// EXPRS_TERM_COM,		/* ~ */
	{ "!100", EXPRS_TERM_INTEGER, !100, 0, NULL },	// EXPRS_TERM_NOT,		/* ! */
	{ "!100.100", EXPRS_TERM_INTEGER, !100, 0, NULL },	// EXPRS_TERM_NOT,		/* ! */
	{ "2**8", EXPRS_TERM_INTEGER, 1<<8, 0, NULL }, // EXPRS_TERM_POW,		/* ** */
	{ "2.0**8", EXPRS_TERM_FLOAT, 0, 256.0, NULL }, // EXPRS_TERM_POW,		/* ** */
	{ "2**8.0", EXPRS_TERM_FLOAT, 0, 256.0, NULL }, // EXPRS_TERM_POW,		/* ** */
	{ "2.0**8.0", EXPRS_TERM_FLOAT, 0, 256.0, NULL }, // EXPRS_TERM_POW,		/* ** */
	{ "2*3", EXPRS_TERM_INTEGER, 6, 0, NULL },	// EXPRS_TERM_MUL,		/* * */
	{ "2*3.0", EXPRS_TERM_FLOAT, 0, 6.0, NULL },	// EXPRS_TERM_MUL,		/* * */
	{ "2.0*3", EXPRS_TERM_FLOAT, 0, 6.0, NULL },	// EXPRS_TERM_MUL,		/* * */
	{ "2.0*3.0", EXPRS_TERM_FLOAT, 0, 6.0, NULL },	// EXPRS_TERM_MUL,		/* * */
	{ "100/2", EXPRS_TERM_INTEGER, 50, 0, NULL }, // EXPRS_TERM_DIV,		/* / */
	{ "100/2.0", EXPRS_TERM_FLOAT, 0, 50.0, NULL }, // EXPRS_TERM_DIV,		/* / */
	{ "100.0/2", EXPRS_TERM_FLOAT, 0, 50.0, NULL }, // EXPRS_TERM_DIV,		/* / */
	{ "100.0/2.0", EXPRS_TERM_FLOAT, 0, 50.0, NULL }, // EXPRS_TERM_DIV,		/* / */
	{ "110%25", EXPRS_TERM_INTEGER, 10, 0, NULL },	// EXPRS_TERM_MOD,		/* % */
	{ "110.0%25", EXPRS_TERM_FLOAT, 0, 10.0, NULL },	// EXPRS_TERM_MOD,		/* % */
	{ "110%25.0", EXPRS_TERM_FLOAT, 0, 10.0, NULL },	// EXPRS_TERM_MOD,		/* % */
	{ "110.0%25.0", EXPRS_TERM_FLOAT, 0, 10.0, NULL },	// EXPRS_TERM_MOD,		/* % */
	{ "1+2", EXPRS_TERM_INTEGER, 3, 0, NULL },	// EXPRS_TERM_ADD,	/* + (binary term in this case) */
	{ "1+2.1", EXPRS_TERM_FLOAT, 0, 3.1, NULL },	// EXPRS_TERM_ADD,	/* + (binary term in this case) */
	{ "1.2+2", EXPRS_TERM_FLOAT, 0, 3.2, NULL },	// EXPRS_TERM_ADD,	/* + (binary term in this case) */
	{ "1.2+2.3", EXPRS_TERM_FLOAT, 0, 3.5, NULL },	// EXPRS_TERM_ADD,	/* + (binary term in this case) */
	{ "1.2+\"2.3\"", EXPRS_TERM_STRING, 0, 0, "1.22.3" },	// EXPRS_TERM_PLUS,	/* + (binary term in this case) */
	{ "\"2.3\"+1.2", EXPRS_TERM_STRING, 0, 0, "2.31.2" },	// EXPRS_TERM_PLUS,	/* + (binary term in this case) */
	{ "2-1", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_SUB,	/* - (unary term in this case) */
	{ "2.0-1", EXPRS_TERM_FLOAT, 0, 1.0, NULL }, // EXPRS_TERM_SUB,	/* - (unary term in this case) */
	{ "2-1.0", EXPRS_TERM_FLOAT, 0, 1.0, NULL }, // EXPRS_TERM_SUB,	/* - (unary term in this case) */
	{ "2.0-1.0", EXPRS_TERM_FLOAT, 1, 1.0, NULL }, // EXPRS_TERM_SUB,	/* - (unary term in this case) */
	{ "1<<4", EXPRS_TERM_INTEGER, 16, 0, NULL },	// EXPRS_TERM_ASL,		/* << */
	{ "1.1<<4", EXPRS_TERM_INTEGER, 16, 0, NULL },	// EXPRS_TERM_ASL,		/* << */
	{ "1<<4.1", EXPRS_TERM_INTEGER, 16, 0, NULL },	// EXPRS_TERM_ASL,		/* << */
	{ "1.1<<4.1", EXPRS_TERM_INTEGER, 16, 0, NULL },	// EXPRS_TERM_ASL,		/* << */
	{ "32>>2", EXPRS_TERM_INTEGER, 8, 0, NULL },	// EXPRS_TERM_ASR,		/* >> */
	{ "32.1>>2", EXPRS_TERM_INTEGER, 8, 0, NULL },	// EXPRS_TERM_ASR,		/* >> */
	{ "32>>2.1", EXPRS_TERM_INTEGER, 8, 0, NULL },	// EXPRS_TERM_ASR,		/* >> */
	{ "32.1>>2.1", EXPRS_TERM_INTEGER, 8, 0, NULL },	// EXPRS_TERM_ASR,		/* >> */

	{ "1>2", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_GT,		/* > */
	{ "1.1>2", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_GT,		/* > */
	{ "1>2.2", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_GT,		/* > */
	{ "1.1>2.2", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_GT,		/* > */
	{ "2>1", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_GT,		/* > */
	{ "2.2>1", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_GT,		/* > */
	{ "2>1.1", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_GT,		/* > */
	{ "2.2>1.1", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_GT,		/* > */

	{ "1>=2", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_GE,		/* > */
	{ "1.1>=2", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_GE,		/* > */
	{ "1>=2.2", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_GE,		/* > */
	{ "1.1>=2.2", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_GE,		/* > */
	{ "2>=2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_GE,		/* > */
	{ "2.0>=2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_GE,		/* > */
	{ "2>=2.0", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_GE,		/* > */
	{ "2.2>=2.2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_GE,		/* > */
	{ "2>=1", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_GE,		/* > */
	{ "2.2>=1", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_GE,		/* > */
	{ "2>=1.1", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_GE,		/* > */
	{ "2.2>=1.1", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_GE,		/* > */
	
	{ "1<2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_LT,		/* > */
	{ "1.1<2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_LT,		/* > */
	{ "1<2.2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_LT,		/* > */
	{ "1.1<2.2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_LT,		/* > */
	{ "2<1", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_LT,		/* > */
	{ "2.2<1", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_LT,		/* > */
	{ "2<1.1", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_LT,		/* > */
	{ "2.2<1.1", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_LT,		/* > */

	{ "1<=2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_LE,		/* > */
	{ "1.1<=2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_LE,		/* > */
	{ "1<=2.2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_LE,		/* > */
	{ "1.1<=2.2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_LE,		/* > */
	{ "2<=2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_LE,		/* > */
	{ "2.0<=2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_LE,		/* > */
	{ "2<=2.0", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_LE,		/* > */
	{ "2.2<=2.2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_LE,		/* > */
	{ "2<=1", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_LE,		/* > */
	{ "2.2<=1", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_LE,		/* > */
	{ "2<=1.1", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_LE,		/* > */
	{ "2.2<=1.1", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_LE,		/* > */

	{ "2==2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_EQ,		/* > */
	{ "2.0==2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_EQ,		/* > */
	{ "2==2.0", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_EQ,		/* > */
	{ "2.2==2.2", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_EQ,		/* > */

	{ "2==1", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_EQ,		/* > */
	{ "2.0==1", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_EQ,		/* > */
	{ "2==1.0", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_EQ,		/* > */
	{ "2.2==1.1", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_EQ,		/* > */

	{ "2!=2", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_NE,		/* > */
	{ "2.0!=2", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_NE,		/* > */
	{ "2!=2.0", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_NE,		/* > */
	{ "2.2!=2.2", EXPRS_TERM_INTEGER, 0, 0, NULL },		// EXPRS_TERM_NE,		/* > */

	{ "2!=1", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_NE,		/* > */
	{ "2.0!=1", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_NE,		/* > */
	{ "2!=1.0", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_NE,		/* > */
	{ "2.2!=1.1", EXPRS_TERM_INTEGER, 1, 0, NULL },		// EXPRS_TERM_NE,		/* > */

	{ "0xFF&0x7F", EXPRS_TERM_INTEGER, 0x7F, 0, NULL }, // EXPRS_TERM_AND,		/* & */
	{ "255.5&0x7F", EXPRS_TERM_INTEGER, 0x7F, 0, NULL }, // EXPRS_TERM_AND,		/* & */
	{ "255&127.0", EXPRS_TERM_INTEGER, 0x7F, 0, NULL }, // EXPRS_TERM_AND,		/* & */
	{ "255.5&127.5", EXPRS_TERM_INTEGER, 0x7F, 0, NULL }, // EXPRS_TERM_AND,		/* & */
	
	{ "0xFF^0x7F", EXPRS_TERM_INTEGER, 0x80, 0, NULL }, // EXPRS_TERM_XOR,		/* ^ */
	{ "255.5^0x7F", EXPRS_TERM_INTEGER, 0x80, 0, NULL }, // EXPRS_TERM_XOR,		/* ^ */
	{ "255^127.0", EXPRS_TERM_INTEGER, 0x80, 0, NULL }, // EXPRS_TERM_XOR,		/* ^ */
	{ "255.5^127.5", EXPRS_TERM_INTEGER, 0x80, 0, NULL }, // EXPRS_TERM_XOR,	/* ^ */

	{ "128|64", EXPRS_TERM_INTEGER, 0xC0, 0, NULL }, // EXPRS_TERM_OR,		/* | */
	{ "128.5|64", EXPRS_TERM_INTEGER, 0xC0, 0, NULL }, // EXPRS_TERM_OR,		/* | */
	{ "128|64.5", EXPRS_TERM_INTEGER, 0xC0, 0, NULL }, // EXPRS_TERM_OR,		/* | */
	{ "128.9|64.9", EXPRS_TERM_INTEGER, 0xC0, 0, NULL }, // EXPRS_TERM_OR,		/* | */
	
	{ "1&&2", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LAND,	/* && */
	{ "1.9&&2", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LAND,	/* && */
	{ "1&&2.9", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LAND,	/* && */
	{ "1.9&&2.9", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LAND,	/* && */
	
	{ "0&&0", EXPRS_TERM_INTEGER, 0, 0, NULL }, // EXPRS_TERM_LAND,	/* && */
	{ "1&&0", EXPRS_TERM_INTEGER, 0, 0, NULL }, // EXPRS_TERM_LAND,	/* && */
	{ "1&&0.0", EXPRS_TERM_INTEGER, 0, 0, NULL }, // EXPRS_TERM_LAND,	/* && */
	{ "1.9&&0", EXPRS_TERM_INTEGER, 0, 0, NULL }, // EXPRS_TERM_LAND,	/* && */
	{ "1.9&&0.0", EXPRS_TERM_INTEGER, 0, 0, NULL }, // EXPRS_TERM_LAND,	/* && */

	{ "0&&1", EXPRS_TERM_INTEGER, 0, 0, NULL }, // EXPRS_TERM_LAND,	/* && */
	{ "0&&1.0", EXPRS_TERM_INTEGER, 0, 0, NULL }, // EXPRS_TERM_LAND,	/* && */
	{ "0.0&&1", EXPRS_TERM_INTEGER, 0, 0, NULL }, // EXPRS_TERM_LAND,	/* && */
	{ "0.0&&1.9", EXPRS_TERM_INTEGER, 0, 0, NULL }, // EXPRS_TERM_LAND,	/* && */

	{ "1||2", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LOR,	/* || */
	{ "1.9||2", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LOR,	/* || */
	{ "1||2.9", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LOR,	/* || */
	{ "1.9||2.9", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LOR,	/* || */

	{ "0||0", EXPRS_TERM_INTEGER, 0, 0, NULL }, // EXPRS_TERM_LOR,	/* || */
	{ "1||0", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LOR,	/* || */
	{ "1||0.0", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LOR,	/* || */
	{ "1.9||0", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LOR,	/* || */
	{ "1.9||0.0", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LOR,	/* || */
	{ "0||1", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LOR,	/* || */
	{ "0.0||1", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LOR,	/* || */
	{ "0||1.9", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LOR,	/* || */
	{ "0.0||1.9", EXPRS_TERM_INTEGER, 1, 0, NULL }, // EXPRS_TERM_LOR,	/* || */
	
	{ "1+(2+3)*4", EXPRS_TERM_INTEGER, 1+(2+3)*4, 0, NULL },	/* Test parenthesis */
	{ "1+2*3/4-6", EXPRS_TERM_INTEGER, 1+2*3/4-6, 0, NULL },	/* Test precedence */
	{ "2+2**2*4/2", EXPRS_TERM_INTEGER, 10, 0, NULL },			/* Test precedence */

};

static int getValue(char *buf, int bufLen, const char *title, const ExprsTerm_t *result, const TestExprs_t *tVal)
{
	int sLen;
	sLen = snprintf(buf,bufLen,"%s type ", title);
	if ( tVal )
	{
		sLen += snprintf(buf + sLen, bufLen - sLen, "%d, value ", tVal->expectedResultType);
		if ( tVal->expectedResultType == EXPRS_TERM_INTEGER )
			sLen += snprintf(buf + sLen, bufLen - sLen, "%ld", tVal->expectedInt);
		else if ( tVal->expectedResultType == EXPRS_TERM_FLOAT )
			sLen += snprintf(buf + sLen, bufLen - sLen, "%g", tVal->expectedFloat);
		else
			sLen += snprintf(buf + sLen, bufLen - sLen, "\"%s\"", tVal->expectedString);
	}
	else
	{
		sLen += snprintf(buf+sLen,bufLen-sLen,"%d, value ", result->termType);
		 if ( result->termType == EXPRS_TERM_INTEGER )
			 sLen += snprintf(buf+sLen,bufLen-sLen,"%ld", result->term.s64);
		 else if ( result->termType == EXPRS_TERM_FLOAT )
			 sLen += snprintf(buf+sLen,bufLen-sLen,"%g", result->term.f64);
		 else if ( result->termType == EXPRS_TERM_STRING )
		 {
			 char quote, *cp = result->term.string;
			 unsigned char cc;
			 quote = strchr(cp,'"') ? '\'':'"';
			 sLen += snprintf(buf+sLen,bufLen-sLen,"%c",quote);
			 while ( (cc = *cp) )
			 {
				 if ( isprint(cc) )
					 sLen += snprintf(buf+sLen,bufLen-sLen,"%c",cc);
				 else
					 sLen += snprintf(buf+sLen,bufLen-sLen,"\\x%02X",cc);
				 ++cp;
			 }
			 sLen += snprintf(buf+sLen,bufLen-sLen,"%c", quote);
		 }
		 else 
			 sLen += snprintf(buf+sLen,bufLen-sLen,"!! Type is not integer, float or string !!");
	}
	return sLen;
}

int exprsTest(int verbose)
{
	ExprsDef_t *exprs;
	ExprsTerm_t result;
	const TestExprs_t *pExp;
	ExprsErrs_t err;
	int ii, retV=0, fatal=0;
	char buf[256];
	int sLen;

	exprs = libExprsInit(NULL, 0, 0, 0);
	if ( !exprs )
	{
		fprintf(stderr,"Out of memory doing libExprsInit()\n");
		return 1;
	}
	libExprsSetVerbose(exprs,verbose);
	pExp = TestExprs;
	for (ii=0; ii < n_elts(TestExprs) && !fatal; ++ii, ++pExp)
	{
		err = libExprsEval(exprs, pExp->expr, &result, 0);
		if ( err )
		{
			printf("%3d: Expression '%s' returned error %d: %s\n", ii, pExp->expr, err, libExprsGetErrorStr(err));
			retV = 1;
			continue;
		}
		if ( result.termType != pExp->expectedResultType )
		{
			sLen  = snprintf(buf,sizeof(buf),"%3d: Type mismatch. Expression '%s' ", ii, pExp->expr);
			sLen += getValue(buf+sLen,sizeof(buf)-sLen, "expected",NULL,pExp);
			sLen += getValue(buf+sLen,sizeof(buf)-sLen, ". Got ",&result,NULL);
			printf("%s\n",buf);
		}
		else
		{
			int diff=0;
			switch (pExp->expectedResultType)
			{
			case EXPRS_TERM_INTEGER:
				if ( pExp->expectedInt != result.term.s64 )
					diff = 1;
				break;
			case EXPRS_TERM_FLOAT:
				if ( pExp->expectedFloat != result.term.f64 )
					diff = 1;
				break;
			case EXPRS_TERM_STRING:
				if ( strcmp(pExp->expectedString,result.term.string) )
					diff = 1;
				break;
			default:
				printf("FATAL: Exprs %d '%s' has undefined expected result type %d\n", ii, pExp->expr, pExp->expectedResultType);
				fatal = 1;
				retV = 1;
				break;
			}
			if ( diff )
			{
				sLen = snprintf(buf, sizeof(buf), "%3d: Value mismatch. Expression '%s' ", ii, pExp->expr);
				sLen += getValue(buf+sLen,sizeof(buf)-sLen, "expected",NULL,pExp);
				sLen += getValue(buf+sLen,sizeof(buf)-sLen, ". Got ",&result,NULL);
				printf("%s\n",buf);
				retV = 1;
			}
		}
	}
	libExprsDestroy(exprs);
	if ( !retV )
		printf("Passed all the tests.\n");
	return retV;
}

