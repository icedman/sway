#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <GLES2/gl2.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wlr/render/gles2.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_box.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output_damage.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/util/region.h>
#include "log.h"
#include "config.h"
#include "sway/config.h"
#include "sway/input/input-manager.h"
#include "sway/input/seat.h"
#include "sway/layers.h"
#include "sway/output.h"
#include "sway/server.h"
#include "sway/tree/arrange.h"
#include "sway/tree/container.h"
#include "sway/tree/root.h"
#include "sway/tree/view.h"
#include "sway/tree/workspace.h"

#include "blackbox/style.h"
#include "blackbox/render.h"

static void _scissor_output(struct wlr_output *wlr_output,
        pixman_box32_t *rect) {
    struct wlr_renderer *renderer = wlr_backend_get_renderer(wlr_output->backend);
    assert(renderer);

    struct wlr_box box = {
        .x = rect->x1,
        .y = rect->y1,
        .width = rect->x2 - rect->x1,
        .height = rect->y2 - rect->y1,
    };

    int ow, oh;
    wlr_output_transformed_resolution(wlr_output, &ow, &oh);

    enum wl_output_transform transform =
        wlr_output_transform_invert(wlr_output->transform);
    wlr_box_transform(&box, &box, transform, ow, oh);

    wlr_renderer_scissor(renderer, &box);
}

void render_rect(struct sway_output *output,
        pixman_region32_t *output_damage, const struct wlr_box *_box,
        float color[static 4]);

// _box.x and .y are expected to be layout-local
// _box.width and .height are expected to be output-buffer-local
void render_rect_texture(struct sway_output *output,
        pixman_region32_t *output_damage, const struct wlr_box *_box,
        struct wlr_texture* texture) {

    if (!texture) {
        return;
    }

    struct wlr_output *wlr_output = output->wlr_output;
    struct wlr_renderer *renderer =
        wlr_backend_get_renderer(wlr_output->backend);

    struct wlr_box box;
    memcpy(&box, _box, sizeof(struct wlr_box));
    box.x -= output->lx * wlr_output->scale;
    box.y -= output->ly * wlr_output->scale;

    pixman_region32_t damage;
    pixman_region32_init(&damage);
    pixman_region32_union_rect(&damage, &damage, box.x, box.y,
        box.width, box.height);
    pixman_region32_intersect(&damage, &damage, output_damage);
    bool damaged = pixman_region32_not_empty(&damage);
    if (!damaged) {
        goto damage_finish;
    }

    struct wlr_gles2_texture_attribs attribs;
    wlr_gles2_texture_get_attribs(texture, &attribs);
    glBindTexture(attribs.target, attribs.tex);
    glTexParameteri(attribs.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    float matrix[9];
    wlr_matrix_project_box(matrix, &box, WL_OUTPUT_TRANSFORM_NORMAL, 0.0,
        output->wlr_output->transform_matrix);

    int nrects;
    pixman_box32_t *rects = pixman_region32_rectangles(&damage, &nrects);
    for (int i = 0; i < nrects; ++i) {
        _scissor_output(wlr_output, &rects[i]);

        /*
        wlr_render_rect(renderer, &box, color,
            wlr_output->transform_matrix);
            */

        wlr_render_texture_with_matrix(renderer, texture, matrix, 1.0);
    }

damage_finish:
    pixman_region32_fini(&damage);
}

void blackbox_render_titlebar(struct sway_view *view, struct sway_output *output,
        pixman_region32_t *output_damage, struct sway_container *con,
        int x, int y, int width,
        struct border_colors *colors, struct wlr_texture *title_texture,
        struct wlr_texture *marks_texture) {
    struct wlr_box box;
    float color[4];
    float output_scale = output->wlr_output->scale;
    // double output_x = output->lx;
    // double output_y = output->ly;
    int titlebar_border_thickness = config->titlebar_border_thickness;
    // int titlebar_h_padding = config->titlebar_h_padding;
    // int titlebar_v_padding = config->titlebar_v_padding;
    // enum alignment title_align = config->title_align;

    float color1[4] = { 1.0, 0.0, 1.0, 1.0 };
    float color2[4] = { 1.0, 1.0, 1.0, 1.0 };
    
    // struct wlr_box texture_box;
    //     wlr_texture_get_size(title_texture,
    //         &texture_box.width, &texture_box.height);

    int titlebar_height = container_titlebar_height();
    int margin = titlebar_border_thickness;

    memcpy(&color, color1, sizeof(float) * 4);

    // borders
    box.x = x;
    box.y = y;
    box.width = width;
    box.height = titlebar_height;
    scale_box(&box, output_scale);
    render_rect(output, output_damage, &box, color);
    // save as hotspot

    // titlebar
    box.x = x + (titlebar_border_thickness * 2);
    box.y = y + (titlebar_border_thickness * 2);
    box.width = width - (titlebar_border_thickness * 4);
    box.height = titlebar_height - (titlebar_border_thickness * 4);
    scale_box(&box, output_scale);
    render_rect(output, output_damage, &box, color2);

    // label
    box.x = x + ((titlebar_border_thickness + margin) * 2);
    box.y = y + ((titlebar_border_thickness + margin) * 2);
    box.width = width - ((titlebar_border_thickness + margin) * 4);
    box.height = titlebar_height - ((titlebar_border_thickness + margin) * 4);
    scale_box(&box, output_scale);
    render_rect(output, output_damage, &box, color1);
}

void blackbox_render_frame(struct sway_output *output, pixman_region32_t *damage,
        struct sway_container *con, struct border_colors *colors) {
    // struct sway_view *view = con->view;
    if (!container_is_floating(con)) {
        return;
    }

    float color1[4] = { 1.0, 0.0, 1.0, 1.0 };
    // float color2[4] = { 1.0, 1.0, 1.0, 1.0 };

    struct wlr_box box;
    float output_scale = output->wlr_output->scale;
    float color[4];
    struct sway_container_state *state = &con->current;

    memcpy(&color, color1, sizeof(float) * 4);

    int bottom_height = 4 + (config->border_thickness * 2);
    box.x = state->x;
    box.y = state->content_y + state->content_height - bottom_height;
    box.width = state->width;
    box.height = state->border_thickness + bottom_height;
    scale_box(&box, output_scale);
    render_rect(output, damage, &box, color);
}