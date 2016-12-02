#include <pebble.h>

// Weather icon size
#define ICON_WIDTH  40
#define ICON_HEIGHT 20

// Keys for persistent data on watch
#define SETTINGS_KEY 1
#define KEY_WEATHER_TEMP 2
#define KEY_WEATHER_CODE 3

// Values for settings
#define DATE_FORMAT_METRIC 0
#define DATE_FORMAT_BACKWARDS 1

Window *watchface;
static GFont time_font;
static GFont date_font;
static GFont small_font;
static TextLayer *time_layer;
static TextLayer *date_layer;
static TextLayer *battery_layer;
static TextLayer *weather_layer;
BitmapLayer *weather_icon_layer;
GBitmap *meteoicons_all, *meteoicon_current;
static BitmapLayer *bt_icon_layer;
static GBitmap *bt_icon_bitmap;

// Holds current weather
static int temp;
static int code;
static int weather_saved = 0;

static bool s_js_ready;

// Struct to hold the app settings. Only includes settings that need to be saved on the watch (e.g. weather settings are grabbed directly from JS so don't need to be stored here)
typedef struct ClaySettings {
  int DateFormat;
} ClaySettings;
static ClaySettings settings;


/***************************************
                BATTERY
***************************************/
static void battery_callback(BatteryChargeState state) {
  // Set the battery level
  static char battery_buffer[5];
  snprintf(battery_buffer, sizeof(battery_buffer), "%d%%", state.charge_percent);
  
  // Set text layer if different
  text_layer_set_text(battery_layer, battery_buffer);
}

/***************************************
              BLUETOOTH
***************************************/
static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(bt_icon_layer), connected);

  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}

/***************************************
            WEATHER UPDATE
***************************************/
static void set_weather_icon(int w_icon) {
  if (meteoicon_current)  gbitmap_destroy(meteoicon_current);
  meteoicon_current =  gbitmap_create_as_sub_bitmap(meteoicons_all, GRect(0, ICON_HEIGHT*w_icon, ICON_WIDTH, ICON_HEIGHT)); 
  bitmap_layer_set_bitmap(weather_icon_layer, meteoicon_current);
}

static void set_weather_temp(int w_temp) {
  static char temperature_buffer[8];
  snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°", (int)w_temp);
  text_layer_set_text(weather_layer, temperature_buffer);
}

static void request_new_weather() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tick_handler() - Sending appmessage to update weather");
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_cstring(iter, MESSAGE_KEY_REQUEST_WEATHER, 0);
  app_message_outbox_send();
}

/***************************************
              TIME UPDATE
***************************************/
static void update_time(struct tm *tick_time) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "update_time()");
  static char time_buffer[8];
  if(clock_is_24h_style() == true) {
    strftime(time_buffer, sizeof(time_buffer), "%H:%M", tick_time);
    text_layer_set_text(time_layer, time_buffer);
  } else {
    strftime(time_buffer, sizeof(time_buffer), "%I:%M", tick_time);
    text_layer_set_text(time_layer, time_buffer+(('0' == time_buffer[0])?1:0));
  }
}


/***************************************
              DATE UPDATE
***************************************/
static void update_date(struct tm *tick_time) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "update_date()");
  static char date_buffer[16];
  switch (settings.DateFormat) {
    case DATE_FORMAT_METRIC:
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Date format: metric");
    strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);
    break;
    case DATE_FORMAT_BACKWARDS:
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Date format: backwards");
    strftime(date_buffer, sizeof(date_buffer), "%a %b %d", tick_time);
    break;
  }
  text_layer_set_text(date_layer, date_buffer);
}

/***************************************
             TICK HANDLER
***************************************/
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // Update time every minute
  if (units_changed & MINUTE_UNIT) {
    update_time(tick_time);
  }
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    request_new_weather();
  }
  
  // Update date every day
  if (units_changed & DAY_UNIT) {
    update_date(tick_time);
  }
}

/***************************************
              APPMESSAGE
***************************************/
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "AppMessage received");
  // Read incoming data
  Tuple *js_ready_tuple = dict_find(iterator, MESSAGE_KEY_JSReady);
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_WEATHER_TEMP);
  Tuple *code_tuple = dict_find(iterator, MESSAGE_KEY_WEATHER_CODE);
  // Settings
  Tuple *apikey_darksky_tuple = dict_find(iterator, MESSAGE_KEY_APIKey_DarkSky);
  Tuple *tempunits_tuple = dict_find(iterator, MESSAGE_KEY_TEMP_F);
  Tuple *dateformat_tuple = dict_find(iterator, MESSAGE_KEY_DATE_FORMAT);
  
  if(js_ready_tuple) {
    // PebbleKit JS is ready! Safe to send messages
    APP_LOG(APP_LOG_LEVEL_DEBUG, "JS is now ready");
    s_js_ready = true;
    
    // If temp and code not set, then must be first launch, therefore we want to request weather
    if (weather_saved == 0) {
      request_new_weather();
    }
  }
  
  // Update the weather temp and icon
  if(temp_tuple && code_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Weather data received");
    temp = temp_tuple->value->int32;
    code = code_tuple->value->int32;
    set_weather_temp(temp);
    set_weather_icon(code);
    weather_saved = 1;
  }
  
  // If api key or temp units changed, request new weather (JS accesses saved settings directly)
  if (apikey_darksky_tuple || tempunits_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "api or tempunits received");
    request_new_weather();
  }
  
  // If date format received, save it in watch storage
  if (dateformat_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "date format received");
    settings.DateFormat = atoi(dateformat_tuple->value->cstring);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Value: %d", settings.DateFormat);
    
    // For date to update
    time_t temp = time(NULL);
    struct tm *t = localtime(&temp);
    update_date(t);
  }
  
  // Save settings to persistent storage
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Saving settings");
  persist_write_data(SETTINGS_KEY, &settings, sizeof(ClaySettings));
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

/***************************************
             LOAD/UNLOAD
***************************************/
static void watchface_load(Window *window) {
  time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BIGNOODLE_78));
  small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BIGNOODLE_19));
  date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BIGNOODLE_32));

  // Get information about the Window
  Layer *root_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root_layer);
  
  // Create layers and populate with initial values
  // TIME
  time_layer = text_layer_create(
      GRect(0, 25, bounds.size.w, 80));
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
  text_layer_set_font(time_layer, time_font);
  layer_add_child(root_layer, text_layer_get_layer(time_layer));
  
  // DATE
  date_layer = text_layer_create(
      GRect(0, 105, bounds.size.w, 35));
  text_layer_set_background_color(date_layer, GColorClear);
  text_layer_set_text_color(date_layer, GColorWhite);
  text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
  text_layer_set_font(date_layer, date_font);
  layer_add_child(root_layer, text_layer_get_layer(date_layer));

  // TEMP
  weather_layer = text_layer_create(GRect(3, 0, 43, 21));
  text_layer_set_background_color(weather_layer, GColorClear);
  text_layer_set_text_color(weather_layer, GColorWhite);
  text_layer_set_text_alignment(weather_layer, GTextAlignmentLeft);
  text_layer_set_font(weather_layer, small_font);
  text_layer_set_text(weather_layer, "-°");
  layer_add_child(root_layer, text_layer_get_layer(weather_layer));
  
  // WEATHER ICON
  meteoicons_all = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_METEOICONS);
  weather_icon_layer =  bitmap_layer_create(GRect(51, 1, 41, 20));
  bitmap_layer_set_compositing_mode(weather_icon_layer, GCompOpSet);
  layer_add_child(root_layer, bitmap_layer_get_layer(weather_icon_layer));

  // BATTERY
  battery_layer = text_layer_create(GRect(98, 0, 43, 21));
  text_layer_set_background_color(battery_layer, GColorClear);
  text_layer_set_text_color(battery_layer, GColorWhite);
  text_layer_set_text_alignment(battery_layer, GTextAlignmentRight);
  text_layer_set_font(battery_layer, small_font);
  text_layer_set_text(battery_layer, "-%");
  layer_add_child(root_layer, text_layer_get_layer(battery_layer));
  
  // Bluetooth Icon
  bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_DISCONNECT);
  bt_icon_layer = bitmap_layer_create(GRect(59, 0, 30, 30));
  bitmap_layer_set_bitmap(bt_icon_layer, bt_icon_bitmap);
  layer_add_child(root_layer, bitmap_layer_get_layer(bt_icon_layer));
}

static void watchface_unload(Window *window) {
  fonts_unload_custom_font(time_font);
  fonts_unload_custom_font(date_font);
  fonts_unload_custom_font(small_font);
  
  text_layer_destroy(time_layer);
  text_layer_destroy(date_layer);
  text_layer_destroy(weather_layer);
  text_layer_destroy(battery_layer);
  gbitmap_destroy(meteoicons_all);
  gbitmap_destroy(meteoicon_current);
  bitmap_layer_destroy(weather_icon_layer);
  gbitmap_destroy(bt_icon_bitmap);
  bitmap_layer_destroy(bt_icon_layer);
  
}

/***************************************
             INIT/DEINIT
***************************************/
void init(void) {
  // Configure main window
  watchface = window_create();
  window_set_window_handlers(watchface, (WindowHandlers) {
    .load = watchface_load,
    .unload = watchface_unload
  });
  window_set_background_color(watchface, GColorBlack);
  window_stack_push(watchface, true);
  
  // Retrieve settings from watch storage
  if (persist_exists(SETTINGS_KEY)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Retrieving saved settings");
    persist_read_data(SETTINGS_KEY, &settings, sizeof(ClaySettings));
  }

  // Initial updates
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);
  update_time(t);
  update_date(t);
  battery_callback(battery_state_service_peek());
  bluetooth_callback(connection_service_peek_pebble_app_connection());
  
  // Get temp from storage if it exists
  if (persist_exists(KEY_WEATHER_TEMP)) {
    temp = persist_read_int(KEY_WEATHER_TEMP);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Retrieving saved temp... setting temp to %d", temp);
    set_weather_temp(temp);
  }
  if (persist_exists(KEY_WEATHER_CODE)) {
    code = persist_read_int(KEY_WEATHER_CODE);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Retrieving saved weather code... code is: %d", code);
    set_weather_icon(code);
  }
  
  // Subscriptions
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
}

void deinit(void) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "deinit()");
  persist_write_int(KEY_WEATHER_TEMP, temp);
  persist_write_int(KEY_WEATHER_CODE, code);
  
  // Save settings to persistent storage
  persist_write_data(SETTINGS_KEY, &settings, sizeof(ClaySettings));
  
  window_destroy(watchface);
}

/***************************************
                 MAIN
***************************************/
int main(void) {
  init();
  app_event_loop();
  deinit();
}
