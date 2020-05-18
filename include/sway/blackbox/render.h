#ifndef BLACKBOX_RENDER_H
#define BLACKBOX_RENDER_H

void blackbox_render_titlebar(struct sway_view *view, struct sway_output *output,
        pixman_region32_t *output_damage, struct sway_container *con,
        int x, int y, int width,
        struct border_colors *colors, struct wlr_texture *title_texture,
        struct wlr_texture *marks_texture);

void blackbox_render_frame(struct sway_output *output, pixman_region32_t *damage,
        struct sway_container *con, struct border_colors *colors);

#endif // BLACKBOX_RENDER_H