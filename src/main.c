#include <pebble.h>
#include "effect_layer.h"

Window *my_window;

TextLayer *text_time, *text_date, *text_dow;
BitmapLayer *background_layer;
GBitmap *bitmap_background;
EffectLayer *effect_layer, *effect_background_layer;
EffectMask* mask;

char s_date[] = "HELLO"; //test
char s_time[] = "HOWRE"; //test
char s_dow[] = "YOU"; //test


static void battery_handler(BatteryChargeState state) {
   layer_set_frame(effect_layer_get_layer(effect_layer), GRect(96,139,26*state.charge_percent/100,21));
}


TextLayer* create_datetime_layer(GRect coords, int font) {
  TextLayer *text_layer = text_layer_create(coords);
  text_layer_set_font(text_layer, fonts_load_custom_font(resource_get_handle(font)));
  text_layer_set_text_color(text_layer, GColorWhite);  
  text_layer_set_background_color(text_layer, GColorClear);  
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(text_layer));
  return text_layer;
}



void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  
   if (!clock_is_24h_style()) {
    
        if( tick_time->tm_hour > 11 ) {   // YG Jun-25-2014: 0..11 - am 12..23 - pm
            if( tick_time->tm_hour > 12 ) tick_time->tm_hour -= 12;
        } else {
            if( tick_time->tm_hour == 0 ) tick_time->tm_hour = 12;
        }        
    }
  
    if (units_changed & MINUTE_UNIT) { // on minutes change - change time
      strftime(s_time, sizeof(s_time), "%H:%M", tick_time);
      text_layer_set_text(text_time, s_time);
    }  
    
    if (units_changed & DAY_UNIT) { // on day change - change date
      strftime(s_date, sizeof(s_date), "%b%d", tick_time);
      text_layer_set_text(text_date, s_date);
    
      strftime(s_dow, sizeof(s_dow), "%a", tick_time);
      text_layer_set_text(text_dow, s_dow);
    }
  
}

void handle_init(void) {
  my_window = window_create();
  window_set_background_color(my_window, GColorBlack);
  window_stack_push(my_window, true);
  
  //creating background - but layer only on applite, basalt will create background via mask below
  bitmap_background = gbitmap_create_with_resource(RESOURCE_ID_BITMAP_BG);
  #ifndef PBL_COLOR
    background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
    bitmap_layer_set_bitmap(background_layer, bitmap_background);
    layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(background_layer));
  #endif
  
  //creating texts
  text_date = create_datetime_layer(GRect(0,4,144,52), RESOURCE_ID_BRADY_44);
  text_time = create_datetime_layer(GRect(0,42,144,52), RESOURCE_ID_BRADY_44);
  text_dow = create_datetime_layer(GRect(0,78,144,52), RESOURCE_ID_BRADY_44);
  
  //if this is Basalt - creating layer with blur effect and background mask
  #ifdef PBL_COLOR
    //creating mask to show background bitmap thru
    mask = malloc(sizeof(EffectMask));
    mask->bitmap_background = bitmap_background;
    mask->mask_color = GColorBlack;
    mask->bitmap_mask = NULL;
    mask->text = NULL;
    mask->background_color = GColorClear;
    
    //adding effect of blur & mask
    effect_background_layer = effect_layer_create(GRect(0,0,144,168));
    effect_layer_add_effect(effect_background_layer, effect_blur, (void *)1);
    effect_layer_add_effect(effect_background_layer, effect_mask, mask);
    layer_add_child(window_get_root_layer(my_window), effect_layer_get_layer(effect_background_layer));
  #endif
  
  //creating battery effect layer with inverter effect
  effect_layer = effect_layer_create(GRect(96,139,26,21));
  effect_layer_add_effect(effect_layer, effect_invert, NULL);
  layer_add_child(window_get_root_layer(my_window), effect_layer_get_layer(effect_layer));
  
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
   //Get a time structure so that the face doesn't start blank
  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);
 
  //Manually call the tick handler when the window is loading
  tick_handler(t, DAY_UNIT | MINUTE_UNIT);
  
  //getting battery info
  battery_state_service_subscribe(battery_handler);
  battery_handler(battery_state_service_peek());
  
    
}

void handle_deinit(void) {
  
  //clearnup
  gbitmap_destroy(bitmap_background);
  #ifndef PBL_COLOR
    bitmap_layer_destroy(background_layer);
  #else
    effect_layer_destroy(effect_background_layer);
    free(mask);
  #endif
  effect_layer_destroy(effect_layer);
  text_layer_destroy(text_date);
  text_layer_destroy(text_time);
  text_layer_destroy(text_dow);
  
  window_destroy(my_window);
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
