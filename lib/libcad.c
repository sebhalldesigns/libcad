/***************************************************************
**
** libcad Source File
**
** File         :  libcad.c
** Module       :  libcad
** Author       :  SH
** Created      :  2026-01-08 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad core API
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <libcad/libcad.h>

/***************************************************************
** MARK: CONSTANTS & MACROS
***************************************************************/

/***************************************************************
** MARK: TYPEDEFS
***************************************************************/

/***************************************************************
** MARK: STATIC VARIABLES
***************************************************************/
/***************************************************************
** MARK: STATIC FUNCTION DEFS
***************************************************************/

/***************************************************************
** MARK: PUBLIC FUNCTIONS
***************************************************************/


cad_ctx_t  cad_create_context()
{
    printf("cad_create_context called\n");
    
    /* Register core types */
    object_get_type();
    document_get_type();
    text_field_get_type();

    return (cad_ctx_t)1;
}

void cad_destroy_context(cad_ctx_t ctx)
{

}

void cad_set_cursor_pos(int x, int y)
{
    
    
}

void cad_cursor_lost()
{
   
}

void cad_set_cursor_button_state(int button, bool pressed)
{
  
}

void cad_set_modifier_state(int modifier, bool state)
{
   
}

void cad_set_viewport(int x, int y, int vpw, int vph, int w, int h)
{
   
}

void cad_render_viewport()
{

   

}

void cad_init_viewport()
{
   

}

void cad_axis_delta(int axis, float delta)
{
   
}

int cad_get_cursor_type()
{
    return 0;
}

void cad_start_modal_tool(int tool_id)
{
    
}

void cad_clear_modal_tool()
{

}

void cad_save_json(const char *path)
{

}

void cad_load_json(const char *path)
{

}

/***************************************************************
** MARK: STATIC FUNCTIONS
***************************************************************/






