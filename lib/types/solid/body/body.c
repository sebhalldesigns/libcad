/***************************************************************
**
** libcad Source File
**
** File         :  body.c
** Module       :  solid/body
** Author       :  SH
** Created      :  2026-02-15 (YYYY-MM-DD)
** License      :  MIT
** Description  :  libcad 3D solid body implementation
**
***************************************************************/

/***************************************************************
** MARK: INCLUDES
***************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <util/log/log.h>
#include <render/mesh/mesh.h>
#include <types/core/plane/plane.h>
#include <types/sketch/circle/circle.h>
#include <types/sketch/rectangle/rectangle.h>

#include "body.h"

/***************************************************************
** MARK: CONSTANTS
***************************************************************/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CYLINDER_SEGMENTS 32

/***************************************************************
** MARK: FORWARD DECLARATIONS
***************************************************************/

static void body_finalize(body_t* self);
static void body_regenerate_mesh(body_t* self);
static void body_generate_cylinder_mesh(body_t* self, vec3 center, float radius, float height, plane_t* plane);
static void body_generate_box_mesh(body_t* self, vec3 center, vec3 half_extents, plane_t* plane);
static void body_upload_mesh(body_t* self);
json_t* body_to_json(body_t* self);

/***************************************************************
** MARK: TYPE REGISTRATION
***************************************************************/

LIBCAD_DEFINE_TYPE(body, object_get_type())

/***************************************************************
** MARK: CLASS INITIALIZATION
***************************************************************/

static void body_class_init(body_class_t* cls)
{
    object_class_t* parent_class = (object_class_t*)cls;
    parent_class->to_json = (json_t*(*)(object_t*))body_to_json;

    cls->parent_class.parent_class.instance_finalize = (void(*)(void*))body_finalize;
    cls->regenerate_mesh = body_regenerate_mesh;

    log_info("body_class_t initialized");
}

/***************************************************************
** MARK: INSTANCE INITIALIZATION
***************************************************************/

static void body_init(body_t* self)
{
    self->source_profile = NULL;
    self->source_plane = NULL;
    self->height = 1.0f;
    glm_vec3_copy((vec3){0.0f, 0.0f, 1.0f}, self->direction);

    glm_vec4_copy((vec4){0.6f, 0.65f, 0.75f, 1.0f}, self->fill_color);
    glm_vec4_copy((vec4){0.15f, 0.15f, 0.15f, 1.0f}, self->edge_color);

    self->mesh_vertices = NULL;
    self->mesh_indices = NULL;
    self->edge_vertices = NULL;
    self->mesh_vertex_count = 0;
    self->mesh_index_count = 0;
    self->edge_vertex_count = 0;
    self->mesh_handle = MESH_INVALID_HANDLE;
}

/***************************************************************
** MARK: INSTANCE FINALIZATION
***************************************************************/

static void body_finalize(body_t* self)
{
    if (self->mesh_handle != MESH_INVALID_HANDLE) {
        mesh_destroy(self->mesh_handle);
        self->mesh_handle = MESH_INVALID_HANDLE;
    }

    free(self->mesh_vertices);
    free(self->mesh_indices);
    free(self->edge_vertices);
    self->mesh_vertices = NULL;
    self->mesh_indices = NULL;
    self->edge_vertices = NULL;
}

/***************************************************************
** MARK: JSON
***************************************************************/

json_t* body_to_json(body_t* self)
{
    if (!self) return json_null();

    json_t* json = object_to_json(BODY_AS_OBJECT(self));
    json_object_set_new(json, "type", json_string("body_t"));
    json_object_set_new(json, "height", json_real(self->height));

    json_t* fill_array = json_array();
    json_array_append_new(fill_array, json_real(self->fill_color[0]));
    json_array_append_new(fill_array, json_real(self->fill_color[1]));
    json_array_append_new(fill_array, json_real(self->fill_color[2]));
    json_array_append_new(fill_array, json_real(self->fill_color[3]));
    json_object_set_new(json, "fill_color", fill_array);

    return json;
}

/***************************************************************
** MARK: MESH REGENERATION (virtual)
***************************************************************/

static void body_regenerate_mesh(body_t* self)
{
    if (!self) return;
    body_upload_mesh(self);
}

/***************************************************************
** MARK: CONSTRUCTORS
***************************************************************/

body_t* body_new_from_circle(circle_t* circle, plane_t* plane, float height)
{
    if (!circle || !plane) return NULL;

    body_t* body = body_new();
    if (!body) return NULL;

    body->source_profile = CIRCLE_AS_OBJECT(circle);
    body->source_plane = plane;
    body->height = height;
    glm_vec3_copy(plane->normal, body->direction);

    /* Compute world center from circle's 2D center on the plane */
    vec3 center_world;
    plane_local_to_world(plane, circle->center, center_world);

    body_generate_cylinder_mesh(body, center_world, circle->radius, height, plane);
    body_upload_mesh(body);

    return body;
}

body_t* body_new_from_rectangle(rectangle_t* rect, plane_t* plane, float height)
{
    if (!rect || !plane) return NULL;

    body_t* body = body_new();
    if (!body) return NULL;

    body->source_profile = RECTANGLE_AS_OBJECT(rect);
    body->source_plane = plane;
    body->height = height;
    glm_vec3_copy(plane->normal, body->direction);

    /* Compute center and half extents in world space */
    vec2 center_2d;
    rectangle_get_center(rect, center_2d);

    vec3 center_world;
    plane_local_to_world(plane, center_2d, center_world);

    float hw = rectangle_width(rect) * 0.5f;
    float hh = rectangle_height(rect) * 0.5f;
    vec3 half_extents = {hw, hh, height * 0.5f};

    body_generate_box_mesh(body, center_world, half_extents, plane);
    body_upload_mesh(body);

    return body;
}

/***************************************************************
** MARK: CYLINDER MESH GENERATION
***************************************************************/

static void body_generate_cylinder_mesh(body_t* self, vec3 center, float radius, float height, plane_t* plane)
{
    const uint32_t seg = CYLINDER_SEGMENTS;

    /*
    ** Vertex layout:
    **   [0..seg-1]       = bottom cap center + ring
    **   [seg..2*seg-1]   = top cap center + ring
    **   Actually, let's use:
    **   - 1 bottom center + seg bottom ring = seg+1 verts for bottom cap
    **   - 1 top center + seg top ring = seg+1 verts for top cap
    **   - seg*2 side verts (each side quad has unique normals) = seg*4 verts (2 tris per quad, but sharing edges)
    **
    **   Simpler approach: separate verts for each face for hard normals.
    **   Bottom cap: 1 center + seg ring = seg+1, fan triangles = seg*3 indices
    **   Top cap: same = seg+1 verts, seg*3 indices
    **   Side: seg quads, each = 4 verts with outward normal, 6 indices = seg*4 verts, seg*6 indices
    **   Total verts: 2*(seg+1) + seg*4 = 2*seg + 2 + 4*seg = 6*seg + 2
    **   Total indices: 2*seg*3 + seg*6 = 12*seg
    */

    const uint32_t vert_count = 6 * seg + 2;
    const uint32_t idx_count = 12 * seg;
    const uint32_t edge_vert_count = seg * 2 + seg * 2 + 4 * 2; /* bottom ring + top ring + 4 vertical lines */

    self->mesh_vertices = (float*)malloc(vert_count * 6 * sizeof(float));
    self->mesh_indices = (uint32_t*)malloc(idx_count * sizeof(uint32_t));
    self->edge_vertices = (float*)malloc(edge_vert_count * 3 * sizeof(float));
    self->mesh_vertex_count = vert_count;
    self->mesh_index_count = idx_count;
    self->edge_vertex_count = edge_vert_count;

    if (!self->mesh_vertices || !self->mesh_indices || !self->edge_vertices) return;

    float* verts = self->mesh_vertices;
    uint32_t* indices = self->mesh_indices;
    float* edges = self->edge_vertices;

    vec3 normal;
    glm_vec3_copy(plane->normal, normal);

    vec3 u_axis, v_axis;
    glm_vec3_copy(plane->u_axis, u_axis);
    glm_vec3_copy(plane->v_axis, v_axis);

    /* Pre-compute ring positions */
    float cos_table[CYLINDER_SEGMENTS];
    float sin_table[CYLINDER_SEGMENTS];
    for (uint32_t i = 0; i < seg; i++) {
        float angle = (float)(2.0 * M_PI * i / seg);
        cos_table[i] = cosf(angle);
        sin_table[i] = sinf(angle);
    }

    uint32_t vi = 0; /* vertex index (counts of 6-float groups) */
    uint32_t ii = 0; /* index index */
    uint32_t ei = 0; /* edge vertex index (counts of 3-float groups) */

    /* Helper: write a vertex (pos + normal) */
    #define WRITE_VERT(px, py, pz, nx, ny, nz) do { \
        verts[vi*6+0] = (px); verts[vi*6+1] = (py); verts[vi*6+2] = (pz); \
        verts[vi*6+3] = (nx); verts[vi*6+4] = (ny); verts[vi*6+5] = (nz); \
        vi++; \
    } while(0)

    #define WRITE_EDGE(px, py, pz) do { \
        edges[ei*3+0] = (px); edges[ei*3+1] = (py); edges[ei*3+2] = (pz); \
        ei++; \
    } while(0)

    /* Bottom cap (faces downward, normal = -plane_normal) */
    vec3 neg_normal;
    glm_vec3_negate_to(normal, neg_normal);

    uint32_t bottom_center_idx = vi;
    WRITE_VERT(center[0], center[1], center[2],
               neg_normal[0], neg_normal[1], neg_normal[2]);

    uint32_t bottom_ring_start = vi;
    for (uint32_t i = 0; i < seg; i++) {
        float px = center[0] + radius * (cos_table[i] * u_axis[0] + sin_table[i] * v_axis[0]);
        float py = center[1] + radius * (cos_table[i] * u_axis[1] + sin_table[i] * v_axis[1]);
        float pz = center[2] + radius * (cos_table[i] * u_axis[2] + sin_table[i] * v_axis[2]);
        WRITE_VERT(px, py, pz, neg_normal[0], neg_normal[1], neg_normal[2]);
    }

    /* Bottom cap indices (CCW when viewed from below, which means CW from outside looking at -normal) */
    for (uint32_t i = 0; i < seg; i++) {
        uint32_t next = (i + 1) % seg;
        indices[ii++] = bottom_center_idx;
        indices[ii++] = bottom_ring_start + next;
        indices[ii++] = bottom_ring_start + i;
    }

    /* Top cap (faces upward, normal = plane_normal) */
    vec3 top_offset;
    glm_vec3_scale(normal, height, top_offset);

    uint32_t top_center_idx = vi;
    WRITE_VERT(center[0] + top_offset[0], center[1] + top_offset[1], center[2] + top_offset[2],
               normal[0], normal[1], normal[2]);

    uint32_t top_ring_start = vi;
    for (uint32_t i = 0; i < seg; i++) {
        float px = center[0] + top_offset[0] + radius * (cos_table[i] * u_axis[0] + sin_table[i] * v_axis[0]);
        float py = center[1] + top_offset[1] + radius * (cos_table[i] * u_axis[1] + sin_table[i] * v_axis[1]);
        float pz = center[2] + top_offset[2] + radius * (cos_table[i] * u_axis[2] + sin_table[i] * v_axis[2]);
        WRITE_VERT(px, py, pz, normal[0], normal[1], normal[2]);
    }

    /* Top cap indices (CCW when viewed from above) */
    for (uint32_t i = 0; i < seg; i++) {
        uint32_t next = (i + 1) % seg;
        indices[ii++] = top_center_idx;
        indices[ii++] = top_ring_start + i;
        indices[ii++] = top_ring_start + next;
    }

    /* Side quads */
    uint32_t side_start = vi;
    for (uint32_t i = 0; i < seg; i++) {
        /* Outward normal for this segment */
        float nx = cos_table[i] * u_axis[0] + sin_table[i] * v_axis[0];
        float ny = cos_table[i] * u_axis[1] + sin_table[i] * v_axis[1];
        float nz = cos_table[i] * u_axis[2] + sin_table[i] * v_axis[2];

        uint32_t next = (i + 1) % seg;
        float nx2 = cos_table[next] * u_axis[0] + sin_table[next] * v_axis[0];
        float ny2 = cos_table[next] * u_axis[1] + sin_table[next] * v_axis[1];
        float nz2 = cos_table[next] * u_axis[2] + sin_table[next] * v_axis[2];

        /* Average normal for smooth shading on the side */
        float snx0 = nx, sny0 = ny, snz0 = nz;
        float snx1 = nx2, sny1 = ny2, snz1 = nz2;

        /* Bottom-left */
        float bx0 = center[0] + radius * nx;
        float by0 = center[1] + radius * ny;
        float bz0 = center[2] + radius * nz;

        /* Bottom-right */
        float bx1 = center[0] + radius * nx2;
        float by1 = center[1] + radius * ny2;
        float bz1 = center[2] + radius * nz2;

        /* Top-left */
        float tx0 = bx0 + top_offset[0];
        float ty0 = by0 + top_offset[1];
        float tz0 = bz0 + top_offset[2];

        /* Top-right */
        float tx1 = bx1 + top_offset[0];
        float ty1 = by1 + top_offset[1];
        float tz1 = bz1 + top_offset[2];

        uint32_t v0 = vi; /* bottom-left */
        WRITE_VERT(bx0, by0, bz0, snx0, sny0, snz0);
        uint32_t v1 = vi; /* bottom-right */
        WRITE_VERT(bx1, by1, bz1, snx1, sny1, snz1);
        uint32_t v2 = vi; /* top-right */
        WRITE_VERT(tx1, ty1, tz1, snx1, sny1, snz1);
        uint32_t v3 = vi; /* top-left */
        WRITE_VERT(tx0, ty0, tz0, snx0, sny0, snz0);

        /* Two triangles */
        indices[ii++] = v0;
        indices[ii++] = v1;
        indices[ii++] = v2;
        indices[ii++] = v0;
        indices[ii++] = v2;
        indices[ii++] = v3;
    }

    /* Edge lines: bottom circle */
    for (uint32_t i = 0; i < seg; i++) {
        uint32_t next = (i + 1) % seg;
        float bx0 = center[0] + radius * (cos_table[i] * u_axis[0] + sin_table[i] * v_axis[0]);
        float by0 = center[1] + radius * (cos_table[i] * u_axis[1] + sin_table[i] * v_axis[1]);
        float bz0 = center[2] + radius * (cos_table[i] * u_axis[2] + sin_table[i] * v_axis[2]);
        float bx1 = center[0] + radius * (cos_table[next] * u_axis[0] + sin_table[next] * v_axis[0]);
        float by1 = center[1] + radius * (cos_table[next] * u_axis[1] + sin_table[next] * v_axis[1]);
        float bz1 = center[2] + radius * (cos_table[next] * u_axis[2] + sin_table[next] * v_axis[2]);
        WRITE_EDGE(bx0, by0, bz0);
        WRITE_EDGE(bx1, by1, bz1);
    }

    /* Edge lines: top circle */
    for (uint32_t i = 0; i < seg; i++) {
        uint32_t next = (i + 1) % seg;
        float tx0 = center[0] + top_offset[0] + radius * (cos_table[i] * u_axis[0] + sin_table[i] * v_axis[0]);
        float ty0 = center[1] + top_offset[1] + radius * (cos_table[i] * u_axis[1] + sin_table[i] * v_axis[1]);
        float tz0 = center[2] + top_offset[2] + radius * (cos_table[i] * u_axis[2] + sin_table[i] * v_axis[2]);
        float tx1 = center[0] + top_offset[0] + radius * (cos_table[next] * u_axis[0] + sin_table[next] * v_axis[0]);
        float ty1 = center[1] + top_offset[1] + radius * (cos_table[next] * u_axis[1] + sin_table[next] * v_axis[1]);
        float tz1 = center[2] + top_offset[2] + radius * (cos_table[next] * u_axis[2] + sin_table[next] * v_axis[2]);
        WRITE_EDGE(tx0, ty0, tz0);
        WRITE_EDGE(tx1, ty1, tz1);
    }

    /* Edge lines: 4 vertical lines (at 0, 90, 180, 270 degrees) */
    for (uint32_t k = 0; k < 4; k++) {
        uint32_t i = k * (seg / 4);
        float bx = center[0] + radius * (cos_table[i] * u_axis[0] + sin_table[i] * v_axis[0]);
        float by = center[1] + radius * (cos_table[i] * u_axis[1] + sin_table[i] * v_axis[1]);
        float bz = center[2] + radius * (cos_table[i] * u_axis[2] + sin_table[i] * v_axis[2]);
        WRITE_EDGE(bx, by, bz);
        WRITE_EDGE(bx + top_offset[0], by + top_offset[1], bz + top_offset[2]);
    }

    #undef WRITE_VERT
    #undef WRITE_EDGE

    (void)side_start;
}

/***************************************************************
** MARK: BOX MESH GENERATION
***************************************************************/

static void body_generate_box_mesh(body_t* self, vec3 center, vec3 half_extents, plane_t* plane)
{
    /*
    ** 6 faces, 4 verts each = 24 verts (hard normals at edges)
    ** 6 faces, 2 tris each = 36 indices
    ** 12 edges = 24 edge verts
    */

    const uint32_t vert_count = 24;
    const uint32_t idx_count = 36;
    const uint32_t edge_vert_count = 24;

    self->mesh_vertices = (float*)malloc(vert_count * 6 * sizeof(float));
    self->mesh_indices = (uint32_t*)malloc(idx_count * sizeof(uint32_t));
    self->edge_vertices = (float*)malloc(edge_vert_count * 3 * sizeof(float));
    self->mesh_vertex_count = vert_count;
    self->mesh_index_count = idx_count;
    self->edge_vertex_count = edge_vert_count;

    if (!self->mesh_vertices || !self->mesh_indices || !self->edge_vertices) return;

    float* verts = self->mesh_vertices;
    uint32_t* indices = self->mesh_indices;
    float* edges = self->edge_vertices;

    vec3 u_axis, v_axis, n_axis;
    glm_vec3_copy(plane->u_axis, u_axis);
    glm_vec3_copy(plane->v_axis, v_axis);
    glm_vec3_copy(plane->normal, n_axis);

    float hw = half_extents[0]; /* half width along u */
    float hh = half_extents[1]; /* half height along v */
    float depth = half_extents[2] * 2.0f; /* full depth along normal */

    /* Compute 8 corners of the box.
    ** Bottom 4 sit on the sketch plane (sn=0).
    ** Top 4 are offset by depth along the plane normal. */
    vec3 corners[8];
    for (int i = 0; i < 8; i++) {
        float su = (i & 1) ? hw : -hw;
        float sv = (i & 2) ? hh : -hh;
        float sn = (i & 4) ? depth : 0.0f;

        corners[i][0] = center[0] + su * u_axis[0] + sv * v_axis[0] + sn * n_axis[0];
        corners[i][1] = center[1] + su * u_axis[1] + sv * v_axis[1] + sn * n_axis[1];
        corners[i][2] = center[2] + su * u_axis[2] + sv * v_axis[2] + sn * n_axis[2];
    }

    /*
    ** Corner indices (bit pattern: bit0=u, bit1=v, bit2=n):
    **   0: (-u, -v, 0)    1: (+u, -v, 0)
    **   2: (-u, +v, 0)    3: (+u, +v, 0)
    **   4: (-u, -v, +n)   5: (+u, -v, +n)
    **   6: (-u, +v, +n)   7: (+u, +v, +n)
    */

    uint32_t vi = 0;
    uint32_t ii = 0;

    /* Helper: add a quad face (4 verts + 6 indices) */
    #define ADD_FACE(c0, c1, c2, c3, fnx, fny, fnz) do { \
        uint32_t base = vi; \
        verts[vi*6+0]=corners[c0][0]; verts[vi*6+1]=corners[c0][1]; verts[vi*6+2]=corners[c0][2]; \
        verts[vi*6+3]=(fnx); verts[vi*6+4]=(fny); verts[vi*6+5]=(fnz); vi++; \
        verts[vi*6+0]=corners[c1][0]; verts[vi*6+1]=corners[c1][1]; verts[vi*6+2]=corners[c1][2]; \
        verts[vi*6+3]=(fnx); verts[vi*6+4]=(fny); verts[vi*6+5]=(fnz); vi++; \
        verts[vi*6+0]=corners[c2][0]; verts[vi*6+1]=corners[c2][1]; verts[vi*6+2]=corners[c2][2]; \
        verts[vi*6+3]=(fnx); verts[vi*6+4]=(fny); verts[vi*6+5]=(fnz); vi++; \
        verts[vi*6+0]=corners[c3][0]; verts[vi*6+1]=corners[c3][1]; verts[vi*6+2]=corners[c3][2]; \
        verts[vi*6+3]=(fnx); verts[vi*6+4]=(fny); verts[vi*6+5]=(fnz); vi++; \
        indices[ii++]=base+0; indices[ii++]=base+1; indices[ii++]=base+2; \
        indices[ii++]=base+0; indices[ii++]=base+2; indices[ii++]=base+3; \
    } while(0)

    /* -N face (bottom): corners 0,1,3,2 facing -normal */
    ADD_FACE(0, 1, 3, 2, -n_axis[0], -n_axis[1], -n_axis[2]);
    /* +N face (top): corners 4,6,7,5 facing +normal */
    ADD_FACE(4, 6, 7, 5, n_axis[0], n_axis[1], n_axis[2]);
    /* -V face: corners 0,4,5,1 facing -v_axis */
    ADD_FACE(0, 4, 5, 1, -v_axis[0], -v_axis[1], -v_axis[2]);
    /* +V face: corners 2,3,7,6 facing +v_axis */
    ADD_FACE(2, 3, 7, 6, v_axis[0], v_axis[1], v_axis[2]);
    /* -U face: corners 0,2,6,4 facing -u_axis */
    ADD_FACE(0, 2, 6, 4, -u_axis[0], -u_axis[1], -u_axis[2]);
    /* +U face: corners 1,5,7,3 facing +u_axis */
    ADD_FACE(1, 5, 7, 3, u_axis[0], u_axis[1], u_axis[2]);

    #undef ADD_FACE

    /* 12 box edges */
    uint32_t ei = 0;
    #define WRITE_EDGE(c0, c1) do { \
        edges[ei*3+0]=corners[c0][0]; edges[ei*3+1]=corners[c0][1]; edges[ei*3+2]=corners[c0][2]; ei++; \
        edges[ei*3+0]=corners[c1][0]; edges[ei*3+1]=corners[c1][1]; edges[ei*3+2]=corners[c1][2]; ei++; \
    } while(0)

    /* Bottom face edges */
    WRITE_EDGE(0, 1);
    WRITE_EDGE(1, 3);
    WRITE_EDGE(3, 2);
    WRITE_EDGE(2, 0);
    /* Top face edges */
    WRITE_EDGE(4, 5);
    WRITE_EDGE(5, 7);
    WRITE_EDGE(7, 6);
    WRITE_EDGE(6, 4);
    /* Vertical edges */
    WRITE_EDGE(0, 4);
    WRITE_EDGE(1, 5);
    WRITE_EDGE(2, 6);
    WRITE_EDGE(3, 7);

    #undef WRITE_EDGE
}

/***************************************************************
** MARK: MESH UPLOAD
***************************************************************/

static void body_upload_mesh(body_t* self)
{
    if (!self || !self->mesh_vertices || !self->mesh_indices) return;

    mesh_instance_t instance;
    instance.vertices = self->mesh_vertices;
    instance.indices = self->mesh_indices;
    instance.edge_vertices = self->edge_vertices;
    instance.vertex_count = self->mesh_vertex_count;
    instance.index_count = self->mesh_index_count;
    instance.edge_vertex_count = self->edge_vertex_count;
    glm_vec4_copy(self->fill_color, instance.fill_color);
    glm_vec4_copy(self->edge_color, instance.edge_color);
    glm_mat4_identity(instance.model_matrix);

    if (self->mesh_handle == MESH_INVALID_HANDLE) {
        self->mesh_handle = mesh_create(&instance);
    } else {
        mesh_update(self->mesh_handle, &instance);
    }
}
