#include <pebble.h>
#include "main.h"

// Background bitmap
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static BitmapLayer *s_logo_layer;
static GBitmap *s_logo_bitmap;

// Time TextLayer
static TextLayer *s_time_layer;

// Time gfx layers
Layer *minute_layer;
Layer *hour_layer;

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

static Window *s_main_window;

static const GPathInfo MINUTE_SEGMENT_PATH_POINTS = {
  3,
  (GPoint []) {
    {0, 0},
    {-6, -58}, // 58 = radius + fudge; 6 = 58*tan(6 degrees); 30 degrees per hour;
    {6,  -58},
  }
};

static GPath *minute_segment_path = NULL;

static const GPathInfo HOUR_SEGMENT_PATH_POINTS = {
  3,
  (GPoint []) {
    {0, 0},
    {-7, -68}, // 48 = radius + fudge; 5 = 48*tan(6 degrees); 6 degrees per second;
    {7,  -68},
  }
};

static GPath *hour_segment_path = NULL;

void minute_layer_update_callback(Layer *me, GContext* ctx) {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
	
  unsigned int angle = tick_time->tm_min * 6;
  GRect bounds = layer_get_bounds(me);
  GPoint center = grect_center_point(&bounds);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, 55);
  graphics_context_set_fill_color(ctx, GColorBlack);

  for(; angle < 355; angle += 6) {
    gpath_rotate_to(minute_segment_path, (TRIG_MAX_ANGLE / 360) * angle);
    gpath_draw_filled(ctx, minute_segment_path);
  }
  graphics_fill_circle(ctx, center, 50);
}

void hour_layer_update_callback(Layer *me, GContext* ctx) {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  unsigned int angle;

  if(clock_is_24h_style() == true) {
    angle = (tick_time->tm_hour * 15) + (tick_time->tm_min / 4);
  } else {
    angle = ((tick_time->tm_hour % 12 ) * 30) + (tick_time->tm_min / 2);
  }

  angle = angle - (angle % 6);
  GRect bounds = layer_get_bounds(me);
  GPoint center = grect_center_point(&bounds);
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, 65);
  graphics_context_set_fill_color(ctx, GColorBlack);

  for(; angle < 355; angle += 6) {
    gpath_rotate_to(hour_segment_path, (TRIG_MAX_ANGLE / 360) * angle);
    gpath_draw_filled(ctx, hour_segment_path);
  }
  graphics_fill_circle(ctx, center, 60);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
}

static void main_window_load(Window *window) {
  // Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
	
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 90, 144, 30));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);

  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  Layer *root_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(root_layer);
  	
  // Init the layer for the hour display
  hour_layer = layer_create(frame);
  layer_set_update_proc(hour_layer, &hour_layer_update_callback);
  layer_add_child(root_layer, hour_layer);

  // Init the hour segment path
  hour_segment_path = gpath_create(&HOUR_SEGMENT_PATH_POINTS);
  gpath_move_to(hour_segment_path, grect_center_point(&frame));

  // Init the layer for the display
  minute_layer = layer_create(frame);
  layer_set_update_proc(minute_layer, &minute_layer_update_callback);
  layer_add_child(root_layer, minute_layer);
	
  // Init the minute segment path
  minute_segment_path = gpath_create(&MINUTE_SEGMENT_PATH_POINTS);
  gpath_move_to(minute_segment_path, grect_center_point(&frame));

  // Create GBitmap, then set to created BitmapLayer
  s_logo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO);
  s_logo_layer = bitmap_layer_create(GRect(61, 43, 22, 35));
  bitmap_layer_set_bitmap(s_logo_layer, s_logo_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_logo_layer));	

  // Add the printed time as a child layer as well
  layer_add_child(root_layer, text_layer_get_layer(s_time_layer));
	
  update_time();
}

static void main_window_unload(Window *window) {
  gbitmap_destroy(s_background_bitmap); // Destroy GBitmap
  bitmap_layer_destroy(s_background_layer); // Destroy BitmapLayer
  text_layer_destroy(s_time_layer); // destroy digital clock layer
  layer_destroy(minute_layer); // destroy minute portal layer
  layer_destroy(hour_layer); // destroy the hour portal layer
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(minute_layer);
  layer_mark_dirty(hour_layer);
  update_time();
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
	
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}