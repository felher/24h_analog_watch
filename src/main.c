#include <pebble.h>
#include <math.h>

static Window *main_window;
static Layer *canvas;

typedef struct {
	GPoint middle;
	int sw;
	int sh;
	int sw2;
	int sh2;
	int max_size;
	int max_size2;
} LayerInfo;

static LayerInfo get_layer_info(Layer *layer) {
	GRect bounds = layer_get_bounds(layer);
	return (LayerInfo) {
		.middle = GPoint(bounds.size.w/2, bounds.size.h/2),
		.sw = bounds.size.w,
		.sh = bounds.size.h,
		.sw2 = bounds.size.w/2,
		.sh2 = bounds.size.h/2,
		.max_size = bounds.size.w > bounds.size.h ?
			bounds.size.h :
			bounds.size.w,
		.max_size2 = bounds.size.w > bounds.size.h ?
			bounds.size.h/2 :
			bounds.size.w/2
	};
}

static void draw_hours(Layer *layer, GContext *ctx) {
	LayerInfo info = get_layer_info(layer);

	for (int i = 0; i < 24; i++) {
		int32_t angle = (i * 15 * TRIG_MAX_ANGLE)/360;
		int32_t sin = sin_lookup(angle) * 8 / 10;
		int32_t cos = cos_lookup(angle) * 8 / 10;
		int32_t length = info.max_size / 15;
		int32_t small_length = info.max_size / 20;

		GPoint from = GPoint(
			(info.max_size2 - length) * cos / TRIG_MAX_RATIO + info.middle.x,
			(info.max_size2 - length) * sin / TRIG_MAX_RATIO + info.middle.y
		);
		GPoint to = GPoint(
			info.max_size2 * cos / TRIG_MAX_RATIO + info.middle.x,
			info.max_size2 * sin / TRIG_MAX_RATIO + info.middle.y
		);
		graphics_context_set_stroke_color(ctx, GColorBlack);
		graphics_draw_line(ctx, from, to);
	}

}

static GPathInfo minute_tick = {
	.num_points = 4,
	.points = (GPoint []) {
		{-3, -144/2},
		{+3, -144/2},
		{+3, -144/2 + 20},
		{-3, -144/2 + 20}
	},
};

static void draw_minutes(Layer *layer, GContext *ctx) {
	LayerInfo info = get_layer_info(layer);
	int32_t length = info.max_size / 15;
	int32_t small_length = info.max_size / 20;

	for (int i = 0; i < 60; i++) {
		int32_t angle = (i * 6 * TRIG_MAX_ANGLE)/360;
		GPath *path = gpath_create(&minute_tick);
		gpath_move_to(path, info.middle);
		gpath_rotate_to(path, angle);
		gpath_draw_filled(ctx, path);
		gpath_draw_outline(ctx, path);
		gpath_destroy(path);
	}

}

static void update_canvas(Layer *layer, GContext *ctx) {
	draw_hours(layer, ctx);
	draw_minutes(layer, ctx);
}

static void main_window_load(Window *window) {
	GRect bounds = layer_get_bounds(window_get_root_layer(window));
	canvas = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
	layer_set_update_proc(canvas, update_canvas);

	layer_add_child(window_get_root_layer(window), canvas);
}

static void main_window_unload(Window *window) {
	layer_destroy(canvas);
}

static void init() {
	main_window = window_create();

	window_set_window_handlers(main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});

	window_stack_push(main_window, true);
}

static void deinit() {
	window_destroy(main_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
