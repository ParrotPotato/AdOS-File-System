#include "debug.h"

int debug_depth = 0;

void add_debug_padding()
{
	int i = 0;

	for(i = 0 ; i < debug_depth ; i++)
	{
		printf("\t");
	}
}


void debug_add_call_layer()
{
	debug_depth += 1;
	printf("\n");
}


void debug_remove_call_layer()
{
	debug_depth -= 1;
	printf("\n");
}
