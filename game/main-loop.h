#ifndef MAIN_LOOP_H_
#define MAIN_LOOP_H_

#include "init.h"

void process_events(struct main_ctx *ctx);
void update_gui(const struct main_ctx *ctx, struct gui_ctx *gui);
void render_gui(struct main_ctx *ctx, const struct gui_ctx *gui);

#endif /* MAIN_LOOP_H_ */
