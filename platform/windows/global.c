#include <stdlib.h>
#define P_INTERNAL_GUARD__
#include "global.h"
#undef P_INTERNAL_GUARD__

volatile HINSTANCE g_instance_handle = NULL;
volatile int g_n_cmd_show = 0;