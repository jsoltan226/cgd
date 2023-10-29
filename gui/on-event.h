#ifndef ON_EVENT_H
#define ON_EVENT_H

typedef int(*oe_OnEventFnPtr)(int argc, void** argv);

typedef struct {
    oe_OnEventFnPtr fn;
    int argc;
    void **argv;
} oe_OnEvent;

#define oe_executeOnEventfn(X)  X.fn(X.argc, X.argv)

#endif
