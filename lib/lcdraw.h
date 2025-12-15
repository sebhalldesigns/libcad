#ifndef LCDRAW_H
#define LCDRAW_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include <cglm/cglm.h>

void make_context();
void render(int x, int y, int vpw, int vph, int w, int h);
void set_offset(vec2 vec);

#ifdef __cplusplus
}
#endif

#endif /* LCDRAW_H */