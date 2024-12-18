#ifndef MAIN_LOOP_H_
#define MAIN_LOOP_H_

#include "init.h"

void process_events(struct platform_ctx *p, struct gui_ctx *gui);
void update_gui(struct gui_ctx *gui);
void render_gui(struct gui_ctx *gui);

#endif /* MAIN_LOOP_H_ */
