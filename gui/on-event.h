#ifndef ON_EVENT_H
#define ON_EVENT_H
#include <stdlib.h>

typedef int(*oe_OnEventFnPtr)(int argc, void** argv);

typedef struct {
    oe_OnEventFnPtr fn;
    int argc;
    void **argv;
} oe_OnEvent;

#define oe_executeOnEventfn(oeObj)      oeObj.fn(oeObj.argc, oeObj.argv)
#define oe_destroyOnEventObj(oeObj)     free(oeObj.argv); oeObj.argv = NULL

#endif
