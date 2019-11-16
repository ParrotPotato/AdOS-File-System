#ifndef DEBUG_HEADER
#define DEBUG_HEADER


#include <stdio.h>
#include <stdlib.h>

void debug_add_call_layer();
void debug_remove_call_layer();
void add_debug_padding();

#define debug(...) add_debug_padding();printf("    ");printf(__VA_ARGS__)
#define debug_func_init() add_debug_padding();printf("%s:\n", __FUNCTION__)


#endif
