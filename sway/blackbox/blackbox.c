#define _POSIX_C_SOURCE 200809L
#include <float.h>
#include <libevdev/libevdev.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_tablet_v2.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include "sway/input/cursor.h"
#include "sway/input/seat.h"
#include "sway/input/tablet.h"
#include "sway/tree/view.h"
#include "log.h"
#if HAVE_XWAYLAND
#include "sway/xwayland.h"
#endif

#include "blackbox/blackbox.h"

bool find_hotspot(struct sway_view* view, double lx, double ly)
{
    const int resizeEdges[] = {
        WLR_EDGE_TOP | WLR_EDGE_LEFT,
        WLR_EDGE_TOP | WLR_EDGE_RIGHT,
        WLR_EDGE_BOTTOM | WLR_EDGE_LEFT,
        WLR_EDGE_BOTTOM | WLR_EDGE_RIGHT,
        WLR_EDGE_TOP,
        WLR_EDGE_BOTTOM,
        WLR_EDGE_LEFT,
        WLR_EDGE_RIGHT
    };

    struct bb_view *bb = &view->blackbox;
    bb->hotspot = HS_NONE;
    bb->hotspot_edges = WLR_EDGE_NONE;
    for (int i = 0; i < (int)HS_COUNT; i++) {
        struct wlr_box* box = &bb->hotspots[i];

        if (!box->width || !box->height) {
            continue;
        }
        if (lx >= box->x && lx <= box->x + box->width && ly >= box->y && ly <= box->y + box->height) {
            bb->hotspot = i;
            if (i <= HS_EDGE_RIGHT) {
                bb->hotspot_edges = resizeEdges[i];
            }
            return true;
        }
    }

    return false;
}