#ifndef LIBCAD_H
#define LIBCAD_H

#include <stdint.h>
#include <stdbool.h>

typedef uintptr_t cad_ctx_t;
typedef uintptr_t cad_model_t;

cad_ctx_t   cad_create_context();
void        cad_destroy_context(cad_ctx_t ctx);

bool        cad_load_model_file(cad_ctx_t ctx, cad_model_t* p_model, const char* filepath);
bool        cad_load_model_data(cad_ctx_t ctx, cad_model_t* p_model, const uint8_t* data, size_t size);
void        cad_unload_model(cad_ctx_t ctx);

bool        cad_write_model_file(cad_ctx_t ctx, cad_model_t model, const char* format, const char* filepath);
bool        cad_write_model_data(cad_ctx_t ctx, cad_model_t model, const char* format, uint8_t** p_data, size_t* p_size);





#endif /* LIBCAD_H */