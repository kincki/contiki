/*******************************************************************************
 *  _____       ______   ____
 * |_   _|     |  ____|/ ____|  Institute of Embedded Systems
 *   | |  _ __ | |__  | (___    Wireless Group
 *   | | | '_ \|  __|  \___ \   Zuercher Hochschule Winterthur
 *  _| |_| | | | |____ ____) |  (University of Applied Sciences)
 * |_____|_| |_|______|_____/   8401 Winterthur, Switzerland
 *
 *******************************************************************************
 *
 * Copyright (c) 2010, Institute Of Embedded Systems at Zurich University
 * of Applied Sciences. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
 /**
 * \file
 *						Defines some dummy methods. The method "_sbrk" is
 *						needed by the malloc routine
 * \author
 *						Silvan Fischer
 * \version
 *						0.1
 * \since
 *						05.04.2009
 */
/*---------------------------------------------------------------------------*/


#include <sys/types.h>

extern unsigned int _HEAP_START;
extern unsigned int _HEAP_END;

static caddr_t heap = NULL;

int _exit(int x) {
	while (1);
}

caddr_t _sbrk ( int increment )
{
	caddr_t prevHeap;
	caddr_t nextHeap;

	if (heap == NULL) {
		// first allocation
		heap = (caddr_t)&_HEAP_START;
	}

	prevHeap = heap;

	// Always return data aligned on a 8 byte boundary 
	nextHeap = (caddr_t)(((unsigned int)(heap + increment) + 7) & ~7); 

	// get current stack pointer 
	register caddr_t stackPtr asm ("sp");

	// Check enough space and there is no collision with stack coming the other way
	// if stack is above start of heap
	if ( (((caddr_t)&_HEAP_START < stackPtr) && (nextHeap > stackPtr)) || 
		(nextHeap >= (caddr_t)&_HEAP_END)) { 
		return NULL; // error - no more memory 
	} else {
		heap = nextHeap;
		return (caddr_t) prevHeap; 
	}
}
