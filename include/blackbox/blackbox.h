#ifndef BLACKBOX_H
#define BLACKBOX_H

enum bb_hotspot_type {
    HS_EDGE_TOP_LEFT,
    HS_EDGE_TOP_RIGHT,
    HS_EDGE_BOTTOM_LEFT,
    HS_EDGE_BOTTOM_RIGHT,
    HS_EDGE_TOP,
    HS_EDGE_BOTTOM,
    HS_EDGE_LEFT,
    HS_EDGE_RIGHT,
    HS_TITLEBAR,
    HS_HANDLE,
    HS_COUNT,
    HS_NONE = -1
};

struct bb_view {
    struct wlr_box hotspots[HS_COUNT];
    enum bb_hotspot_type hotspot;
    uint32_t hotspot_edges;
};

bool find_hotspot(struct sway_view* view, double lx, double ly);

#endif // BLACKBOX_H
