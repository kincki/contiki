
#include "lib/rand.h"
#include "dev/stm32w-radio.h"

int rand(void)
{
	u16_t rand_num;

	ST_RadioGetRandomNumbers(&rand_num, 1);

	rand_num &= RAND_MAX;

	return (int)rand_num;
}

/*
 *	It does nothing, since the rand already generates true random numbers.
 */
void srand(unsigned int seed)
{
}
