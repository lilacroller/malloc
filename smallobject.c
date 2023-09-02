#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "memory.h"

//#define ntimes 3000
#define ntimes 50

void __attribute__((noinline)) foo1(unsigned long *a, unsigned long *b, unsigned long *c, unsigned long *size) {
	for(unsigned long i = 0; i < *size; i++) {
		a[i] = b[i] + c[i];
	}
}

void dummy(unsigned long *a) {}

int main() {
	unsigned long *a = (unsigned long *)mymalloc(ntimes * sizeof(unsigned long));
	unsigned long *b = (unsigned long *)mymalloc(ntimes * sizeof(unsigned long));
	unsigned long *c = (unsigned long *)mymalloc(ntimes * sizeof(unsigned long));
	//unsigned long *size = (unsigned long *)mymalloc(ntimes * sizeof(unsigned long));
	unsigned long *size = (unsigned long *)mymalloc(sizeof(unsigned long));
	*size = ntimes;
	
	for(unsigned long i = 0; i < *size; i++) {
		a[i] = i;
		b[i] = i;
		c[i] = i;
	}
	
	for(unsigned long i = 0; i < *size; i++) {
		foo1(a, b, c, size);
	}

	dummy(a);

	myfree(a);
	myfree(b);
	myfree(c);
	myfree(size);
	
	return 0;
}
