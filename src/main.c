#include <pebble.h>

static Window *main_window;
static Layer *ticks, *hands;
static TextLayer *date;
static GPath *minute_hand, *hour_hand;

static const GPathInfo MINUTE_HAND_POINTS = {
  .num_points = 6,
  .points = (GPoint []) {
	  {2, -72},
	  {5, -20},
	  {2, 0},
	  {-2, 0},
	  {-5, -20},
	  {-2, -72}}
};

static const GPathInfo HOUR_HAND_POINTS = {
  .num_points = 5,
  .points = (GPoint []) {
	  {0, -50},
	  {7, -20},
	  {3, 0},
	  {-3, 0},
	  {-7, -20}}
};

typedef struct {
	GPoint lower_middle;
	int min_size;
	int half_min_size;
} LayerInfo;

static LayerInfo get_layer_info(Layer *layer) {
	GRect bounds = layer_get_bounds(layer);
	int max_size = bounds.size.w < bounds.size.h ?
		bounds.size.h :
		bounds.size.w;
	int min_size = bounds.size.w > bounds.size.h ?
		bounds.size.h :
		bounds.size.w;
	GPoint middle = GPoint(bounds.size.w/2, bounds.size.h/2);

	int16_t size_difference = max_size - min_size;

	return (LayerInfo) {
		.lower_middle = GPoint(middle.x, middle.y + size_difference/2),
		.min_size = min_size,
		.half_min_size = min_size/2
	};
}

static int is_special_tick(int tick, int special_tick_modulo) {
	if (special_tick_modulo == -1) return false;
	if ((tick % special_tick_modulo) != 0) return false;
	return true;
}

static GPoint get_point_on_circle(int16_t radius, int16_t degree, GPoint offset) {
		int32_t angle = (degree * TRIG_MAX_ANGLE) / 360;
		int32_t sin = sin_lookup(angle);
		int32_t cos = cos_lookup(angle);

		// the y coordinate has to be negative because on the pebble y
		// grows to the bottom, while in mathematical coordinate system,
		// y grows to the top
		return GPoint(
			radius * cos / TRIG_MAX_RATIO + offset.x,
			- radius * sin / TRIG_MAX_RATIO + offset.y 
		);
}

static void draw_ticks(Layer *layer, GContext *ctx, int ticks, int size_percent, int special_ticks) {
	LayerInfo info = get_layer_info(layer);
	int16_t big_length = info.min_size / 10;
	int16_t small_length = info.min_size / 20;

	for (int i = 0; i < ticks; i++) {
		int32_t length = is_special_tick(i, special_ticks)
			? big_length
			: small_length;
		int32_t degree = i * (360/ticks);

		int16_t radius_outer = (info.half_min_size - 0     ) * size_percent / 100;
		int16_t radius_inner = (info.half_min_size - length) * size_percent / 100;

		GPoint from = get_point_on_circle(radius_inner, degree, info.lower_middle);
		GPoint to   = get_point_on_circle(radius_outer, degree, info.lower_middle);

		graphics_context_set_stroke_color(ctx, GColorWhite);
		graphics_draw_line(ctx, from, to);
	}
}

static void update_ticks(Layer *layer, GContext *ctx) {
	draw_ticks(layer, ctx, 60, 100, 5);
	draw_ticks(layer, ctx, 24, 80, -1);
}

static int32_t clock_degree(int32_t degree) {
	return (360 + (90 - degree)) % 360;
}

static void draw_seconds(Layer *layer, GContext *ctx, struct tm *t) {
	LayerInfo info = get_layer_info(layer);
	int32_t degree = clock_degree(360 * t->tm_sec / 60);
	int16_t radius_outer = info.half_min_size;

	GPoint from = info.lower_middle;
	GPoint to   = get_point_on_circle(radius_outer, degree, info.lower_middle);

	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_draw_line(ctx, from, to);
}

static void draw_minutes(Layer *layer, GContext *ctx, struct tm *t) {
	LayerInfo info = get_layer_info(layer);
	int32_t degree = 360 * t->tm_min / 60;
	int32_t angle = (degree * TRIG_MAX_ANGLE) / 360;

	gpath_move_to(minute_hand, info.lower_middle);
	gpath_rotate_to(minute_hand, angle);

	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_context_set_fill_color(ctx, GColorWhite);
	gpath_draw_filled(ctx, minute_hand);
}

static void draw_hours(Layer *layer, GContext *ctx, struct tm *t) {
	LayerInfo info = get_layer_info(layer);
	int32_t degree = 360 * (t->tm_hour * 60 + t->tm_min) / (24 * 60);
	int32_t angle = (degree * TRIG_MAX_ANGLE) / 360;

	gpath_move_to(hour_hand, info.lower_middle);
	gpath_rotate_to(hour_hand, angle);

	graphics_context_set_stroke_color(ctx, GColorBlack);
	graphics_context_set_fill_color(ctx, GColorWhite);
	gpath_draw_filled(ctx, hour_hand);
	gpath_draw_outline(ctx, hour_hand);
}

static void update_hands(Layer *layer, GContext *ctx) {
	time_t now = time(NULL);
	struct tm *t = localtime(&now);

	draw_seconds(layer, ctx, t);
	draw_minutes(layer, ctx, t);
	draw_hours(layer, ctx, t);
}

static void draw_date() {
	static char buffer[] = "2015-01-01";
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	strftime(buffer, sizeof("2015-01-01"), "%F", t);
	text_layer_set_text(date, buffer);
}

static void main_window_load(Window *window) {
	window_set_background_color(window, GColorBlack);
	GRect bounds = layer_get_bounds(window_get_root_layer(window));

	ticks = layer_create(bounds);
	layer_set_update_proc(ticks, update_ticks);
	layer_add_child(window_get_root_layer(window), ticks);

	hands = layer_create(bounds);
	layer_set_update_proc(hands, update_hands);
	layer_add_child(window_get_root_layer(window), hands);

	date = text_layer_create(GRect(0, 0, 144, 60));
	text_layer_set_background_color(date, GColorClear);
	text_layer_set_text_color(date, GColorWhite);
	text_layer_set_font(date, fonts_get_system_font(FONT_KEY_GOTHIC_14));
	text_layer_set_text_alignment(date, GTextAlignmentLeft);
	draw_date();
	layer_add_child(window_get_root_layer(window), text_layer_get_layer(date));
}

static void main_window_unload(Window *window) {
	layer_destroy(ticks);
	layer_destroy(hands);
	text_layer_destroy(date);
}

static void seconds_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	layer_mark_dirty(window_get_root_layer(main_window));
	if (units_changed & DAY_UNIT) {
		draw_date();
	}
}

static void init() {
	minute_hand = gpath_create(&MINUTE_HAND_POINTS);
	hour_hand = gpath_create(&HOUR_HAND_POINTS);
	main_window = window_create();

	window_set_window_handlers(main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});

	window_stack_push(main_window, true);

	tick_timer_service_subscribe(SECOND_UNIT, seconds_tick_handler);
}

static void deinit() {
	gpath_destroy(minute_hand);
	gpath_destroy(hour_hand);
	window_destroy(main_window);
	tick_timer_service_unsubscribe();
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
