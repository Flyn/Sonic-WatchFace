#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static GFont s_time_font;
static GFont s_date_font;
static char time_buffer[] = "00:00";
static char date_buffer[] = "00 Jan";
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static BitmapLayer *s_sonic_layer;
static GBitmap *s_basepose_bitmap;
bool animated = false;

static uint32_t delayms = 200;
uint32_t current_frame = 0;
#define MAX_FRAME 6
static GBitmap *s_anim_bitmap[MAX_FRAME];
static uint32_t animation[] = {0,1,2,3,4,3,4,3,4,3,3,5,5,5,5,3,3,4,3,4,3,4,3,1,0};
#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

static void update_time() {
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // Create a long-lived buffer


    // Write the current hours and minutes into the buffer
    if(clock_is_24h_style() == true) {
        // Use 24 hour format
        strftime(time_buffer, sizeof("00:00"), "%H:%M", tick_time);
    } else {
        // Use 12 hour format
        strftime(time_buffer, sizeof("00:00"), "%I:%M", tick_time);
    }

    // Display this time on the TextLayer
    text_layer_set_text(s_time_layer, time_buffer);

    strftime(date_buffer, sizeof("00 Jan"), "%d %b", tick_time);

    text_layer_set_text(s_date_layer, date_buffer);
}


static void animate() {
    if (current_frame < NELEMS(animation)) {
        bitmap_layer_set_bitmap(s_sonic_layer, s_anim_bitmap[animation[current_frame]]);
        layer_mark_dirty(bitmap_layer_get_layer(s_sonic_layer));
        current_frame ++;
        app_timer_register(delayms, animate, NULL);
    } else {
        current_frame = 0;
        animated = false;
        bitmap_layer_set_bitmap(s_sonic_layer, s_basepose_bitmap);
        layer_mark_dirty(bitmap_layer_get_layer(s_sonic_layer));
    }
}

static void start_anim() {
    if (animated)
        return;
    animated = true;
    app_timer_register(1, animate, NULL);
}

static void load_anim() {
    s_anim_bitmap[0] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ANIM0);
    s_anim_bitmap[1] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ANIM1);
    s_anim_bitmap[2] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ANIM2);
    s_anim_bitmap[3] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ANIM3);
    s_anim_bitmap[4] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ANIM4);
    s_anim_bitmap[5] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ANIM5);
}

static void free_anim() {
    uint32_t i = 0;
    for (i = 0 ; i < MAX_FRAME; i++) {
        gbitmap_destroy(s_anim_bitmap[i]);
    }
}

static void main_window_load(Window *window) {
    // Create GBitmap, then set to created BitmapLayer
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
    s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));

    s_sonic_layer = bitmap_layer_create(GRect(42, 34, 64, 80));

    s_basepose_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BASEPOSE);
    bitmap_layer_set_compositing_mode(s_sonic_layer, GCompOpSet);
    bitmap_layer_set_bitmap(s_sonic_layer, s_basepose_bitmap);
    layer_add_child(bitmap_layer_get_layer(s_background_layer), bitmap_layer_get_layer(s_sonic_layer));

    load_anim();

    // Create time TextLayer
    s_time_layer = text_layer_create(GRect(2, 108, 139, 50));
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorBlack);
    text_layer_set_text(s_time_layer, "00:00");

    // Create GFont
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SONIC_40));

    // Apply to TextLayer
    text_layer_set_font(s_time_layer, s_time_font);
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

    //text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));

    // Add it as a child layer to the Window's root layer
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

    // Create time TextLayer
    s_date_layer = text_layer_create(GRect(2, 2, 139, 50));
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, GColorBlack);
    text_layer_set_text(s_date_layer, "21 JUIN");

    s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SONIC_14));

    text_layer_set_font(s_date_layer, s_date_font);
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));
    start_anim();
}

static void main_window_unload(Window *window) {
    // Destroy TextLayer
    text_layer_destroy(s_time_layer);
    text_layer_destroy(s_date_layer);
    // Unload GFont
    fonts_unload_custom_font(s_time_font);
    fonts_unload_custom_font(s_date_font);
    // Destroy GBitmap
    gbitmap_destroy(s_background_bitmap);
    gbitmap_destroy(s_basepose_bitmap);
    free_anim();

    // Destroy BitmapLayer
    bitmap_layer_destroy(s_background_layer);
    bitmap_layer_destroy(s_sonic_layer);
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
    start_anim();
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
}

static void init() {
    // Create main Window element and assign to pointer
    s_main_window = window_create();

    // Set handlers to manage the elements inside the Window
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });

    // Show the Window on the watch, with animated=true
    window_stack_push(s_main_window, true);

    // Make sure the time is displayed from the start
    update_time();

    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    accel_tap_service_subscribe(accel_tap_handler);
}

static void deinit() {
    // Destroy Window
    window_destroy(s_main_window);
    tick_timer_service_unsubscribe();
    accel_tap_service_unsubscribe();
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
