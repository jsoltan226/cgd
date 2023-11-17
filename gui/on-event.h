#ifndef ON_EVENT_H
#define ON_EVENT_H
#include <stdlib.h>

#define OE_ARGV_SIZE    4

typedef int(*oe_OnEventFnPtr)(int argc, void** argv);

typedef struct {
    oe_OnEventFnPtr fn;
    int argc;
    void* argv[4];
} oe_OnEvent;

#define oe_executeOnEventfn(oeObj)      oeObj.fn(oeObj.argc, oeObj.argv)

#endif
