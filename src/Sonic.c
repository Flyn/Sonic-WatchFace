#include <pebble.h>
#include <math.h>

#define KEY_TEMPERATURE 0
#define KEY_WEATHER 1
#define KEY_NIGHT 2

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_temp_layer;
static GFont s_time_font;
static GFont s_date_font;
static char time_buffer[] = "00:00";
static char date_buffer[] = "00 Jan";
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static BitmapLayer *s_sonic_layer;
static GBitmap *s_basepose_bitmap;
static BitmapLayer *s_rain_layer;
static bool animated = false;
static bool raining = false;
static bool thunder = false;
static bool snow = false;

static uint32_t current_weather = 0;
static bool current_night = false;

static uint32_t delayms = 200;
uint32_t current_frame = 0;
uint32_t current_rain_frame = 0;

#define MAX_FRAME 6
static GBitmap *s_anim_bitmap[MAX_FRAME];

#define MAX_RAIN_FRAME 3

static uint32_t animation[] = {0,1,3,4,3,4,3,4,3,3,5,5,5,5,3,3,4,3,4,3,4,3,1,0};

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
        if (!(thunder || snow)) {
            bitmap_layer_set_bitmap(s_sonic_layer, s_anim_bitmap[animation[current_frame]]);
            layer_mark_dirty(bitmap_layer_get_layer(s_sonic_layer));
        }
        current_frame ++;

        if (raining || snow) {
            layer_mark_dirty(bitmap_layer_get_layer(s_rain_layer));
            current_rain_frame ++;
            if (current_rain_frame == MAX_RAIN_FRAME) {
                current_rain_frame = 0;
            }
        }

        app_timer_register(delayms, animate, NULL);
    } else {
        current_frame = 0;
        animated = false;
        gbitmap_destroy(s_basepose_bitmap);
        if (raining) {
            if (thunder) {
                s_basepose_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_THUNDERPOSE);
            } else {
                s_basepose_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RAINPOSE);
            }
        } else if (snow) {
            s_basepose_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SNOWPOSE);
        } else {
            s_basepose_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BASEPOSE);
        }
        if (snow) {
            layer_set_frame(bitmap_layer_get_layer(s_sonic_layer), GRect(5, 24, 128, 94));
        } else {
            layer_set_frame(bitmap_layer_get_layer(s_sonic_layer), GRect(42, 34, 64, 80));
        }
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
    //s_anim_bitmap[2] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ANIM2);
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

static void draw_rainfall(GContext *ctx, int x, int y, int length) {
    graphics_context_set_stroke_width(ctx, 2);
    float angle = 2.0071286396850003;
    graphics_draw_line(ctx, GPoint(x,y), GPoint(x + length * cos(angle), y + length * sin(angle)));
}

static void draw_raindrop(GContext *ctx, int x, int y) {
    graphics_context_set_stroke_width(ctx, 3);
    graphics_draw_line(ctx, GPoint(x,y), GPoint(x - 4, y - 5));
    graphics_draw_line(ctx, GPoint(x,y), GPoint(x - 4, y - 5));
    graphics_draw_line(ctx, GPoint(x,y), GPoint(x + 2, y - 3));
}

static void draw_snowflake(GContext *ctx, int x, int y) {
    graphics_context_set_stroke_width(ctx, 1 + rand() % 3);
    graphics_draw_line(ctx, GPoint(x,y), GPoint(x, y - 1));
    graphics_draw_line(ctx, GPoint(x,y), GPoint(x, y + 1));
    graphics_draw_line(ctx, GPoint(x,y), GPoint(x + 1, y));
    graphics_draw_line(ctx, GPoint(x,y), GPoint(x - 1, y));
}

static void draw_rain(Layer *bg_layer, GContext *ctx) {
    if (raining) {
        if (thunder && (current_frame % 8) == 1) {
            graphics_context_set_fill_color(ctx, GColorWhite);
            graphics_fill_rect(ctx, GRect(0, 0, 144, 168), 0, 0);
            return;
        }
        graphics_context_set_stroke_color(ctx, GColorCobaltBlue);
        graphics_context_set_antialiased(ctx, false);
        if (current_rain_frame == 0) {
            draw_rainfall(ctx, 12, 42, 10);
            draw_rainfall(ctx, 28, 71, 15);
            draw_rainfall(ctx, 36, 34, 22);
            draw_rainfall(ctx, 42, 84, 16);
            draw_rainfall(ctx, 60, 32, 25);
            draw_rainfall(ctx, 96, 22, 27);
            draw_rainfall(ctx, 76, 50, 23);
            draw_rainfall(ctx, 84, 70, 15);
            draw_rainfall(ctx, 100, 50, 22);
            draw_rainfall(ctx, 122, 46, 18);
            draw_rainfall(ctx, 122, 84, 8);
            draw_rainfall(ctx, 142, 66, 5);
            draw_raindrop(ctx, 16,107);
            draw_raindrop(ctx, 100,108);
        }
        if (current_rain_frame == 1) {
            draw_rainfall(ctx, 7, 76, 10);
            draw_rainfall(ctx, 30, 68, 15);
            draw_rainfall(ctx, 36, 38, 22);
            draw_rainfall(ctx, 56, 51, 16);
            draw_rainfall(ctx, 54, 56, 25);
            draw_rainfall(ctx, 70, 83, 27);
            draw_rainfall(ctx, 82, 25, 23);
            draw_rainfall(ctx, 94, 84, 15);
            draw_rainfall(ctx, 116, 80, 22);
            draw_rainfall(ctx, 136, 38, 18);
            draw_rainfall(ctx, 140, 64, 25);
            draw_rainfall(ctx, 112, 55, 5);
            draw_raindrop(ctx, 21,109);
            draw_raindrop(ctx, 88,108);
            draw_raindrop(ctx, 135,104);
        }
        if (current_rain_frame == 2) {
            draw_rainfall(ctx, 8, 44, 10);
            draw_rainfall(ctx, 26, 61, 15);
            draw_rainfall(ctx, 54, 48, 22);
            draw_rainfall(ctx, 64, 47, 6);
            draw_rainfall(ctx, 94, 34, 25);
            draw_rainfall(ctx, 80, 79, 17);
            draw_rainfall(ctx, 134, 48, 23);
            draw_rainfall(ctx, 110, 25, 28);
            draw_rainfall(ctx, 44, 89, 12);
            draw_rainfall(ctx, 72, 47, 18);
            draw_rainfall(ctx, 104, 70, 15);
            draw_rainfall(ctx, 20, 91, 5);
            draw_raindrop(ctx, 16,104);
            draw_raindrop(ctx, 104,104);
            draw_raindrop(ctx, 127,103);
        }
        return;
    }
    if (snow) {
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_context_set_antialiased(ctx, false);
        for (int i = 0 ; i < 20 ; i++) {
            draw_snowflake(ctx, rand() % 144, rand() % 100);
        }
        return;
    }
}

static void update_weather(uint32_t weather, bool night) {

    raining = false;
    thunder = false;

    if (weather == current_weather && night == current_night) {
        return;
    }

    current_weather = weather;
    current_night = night;

    GColor text_color = GColorBlack;

    if (night) {
        text_color = GColorWhite;
    }

    gbitmap_destroy(s_background_bitmap);

    if (weather == 1) {
        if (night) {
            s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLEARNIGHT);
        } else {
            s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SUNNY);
        }
    } else if (weather == 2 || weather == 3 || weather == 4) {
        if (night) {
            s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLOUDYNIGHT);
        } else {
            s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CLOUDY);
        }
    } else if (weather == 9 || weather == 10 || weather == 11) {
        if (night) {
            s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RAINYNIGHT);
        } else {
            s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RAINY);
        }
        raining = true;
        if (weather == 11) {
            thunder = true;
        }
    } else if (weather == 13) {
        if (night) {
            s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SNOWYNIGHT);
        } else {
            s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SNOWY);
        }
        snow = true;
    }  else {
        s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
    }

    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    text_layer_set_text_color(s_date_layer, text_color);
    text_layer_set_text_color(s_time_layer, text_color);
    text_layer_set_text_color(s_temp_layer, text_color);
    layer_mark_dirty(bitmap_layer_get_layer(s_background_layer));
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

    s_rain_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
    layer_add_child(bitmap_layer_get_layer(s_background_layer), bitmap_layer_get_layer(s_rain_layer));
    layer_set_update_proc(bitmap_layer_get_layer(s_rain_layer), draw_rain);

    load_anim();

    // Create time TextLayer
    s_time_layer = text_layer_create(GRect(2, 108, 139, 50));
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorBlack);
    text_layer_set_text(s_time_layer, "00:00");

    // Create GFont
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SONIC_38));

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
    text_layer_set_text(s_date_layer, "SONIC");

    s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SONIC_14));

    text_layer_set_font(s_date_layer, s_date_font);
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

    // Create temperature Layer
    s_temp_layer = text_layer_create(GRect(2, 4, 139, 50));
    text_layer_set_background_color(s_temp_layer, GColorClear);
    text_layer_set_text_color(s_temp_layer, GColorBlack);

    //text_layer_set_font(s_temp_layer, s_date_font);
    text_layer_set_font(s_temp_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_text_alignment(s_temp_layer, GTextAlignmentRight);

    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_temp_layer));

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
    bitmap_layer_destroy(s_rain_layer);
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
    start_anim();
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
    /*if(tick_time->tm_min == 0) {
        vibes_short_pulse();
    }*/
    if(tick_time->tm_min % 30 == 0) {
      // Begin dictionary
      DictionaryIterator *iter;
      app_message_outbox_begin(&iter);

      // Add a key-value pair
      dict_write_uint8(iter, 0, 0);

      // Send the message!
      app_message_outbox_send();
    }
    if(tick_time->tm_min % 5 == 0) {
        start_anim();
    }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    static char temperature_buffer[8];
    static uint32_t weather;
    static bool night;

    // Read first item
    Tuple *t = dict_read_first(iterator);

    // For all items
    while(t != NULL) {
        // Which key was received?
        switch(t->key) {
            case KEY_TEMPERATURE:
            snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°C", (int)t->value->int32);
            break;
            case KEY_WEATHER:
            weather = t->value->int32;
            break;
            case KEY_NIGHT:
            night = (t->value->int32 == 1);
            break;
            default:
            APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
            break;
        }

        // Look for next item
        t = dict_read_next(iterator);
    }
    text_layer_set_text(s_temp_layer, temperature_buffer);
    update_weather(weather, night);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
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

    // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    // Open AppMessage
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

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
