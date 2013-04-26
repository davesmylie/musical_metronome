#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "mini-printf.h"


#define MY_UUID { 0x66, 0xA9, 0xFF, 0x29, 0x49, 0x37, 0x41, 0x8E, 0x9E, 0xCC, 0x37, 0x3E, 0x77, 0x6D, 0x63, 0x3C }
PBL_APP_INFO(MY_UUID,
             "Musical Metronome", "Dave Inc",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_STANDARD_APP);

Window window;
AppContextRef g_ctx; // not sure how to get a handle back to this. will make global for nowo


//TextLayer title_text_layer;
//char      title_str[100];

TextLayer timer_text_layer;
char      timer_str[100];


TextLayer select_text_layer;
char      select_str[100];

TextLayer status_text_layer;
char      click_str[100];


TextLayer bpm_text_layer;
char      bpm_str[5];


TextLayer time_sig_text_layer;
char      time_sig_string[20];

int bpm;
//char str1[100]; // debug sort of string
int click_counter = 0;
int timer_counter = 0;
int time_sig_counter = 0; // position in current time beat
int time_sig = 4;        // actual time sig

int running; 


// Can be used to cancel timer via `app_timer_cancel_event()`
AppTimerHandle timer_handle;
// Can be used to distinguish between multiple timers in your app
#define COOKIE_MY_TIMER 1
#define VERSION 8
#define FALSE 0
#define TRUE 1

const VibePattern short_vibe = {
      .durations = (uint32_t []) {50},
          .num_segments = 1
};

const VibePattern long_vibe = {
      .durations = (uint32_t []) {150},
          .num_segments = 1
};



// return the interval between counts based on bpm
// eg 60 bpm is 1000ms, 120bpm is 500ms
int get_interval() {
  //return 1000;
  return 60000 / bpm;
}

void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
  (void)ctx;
  (void)handle;

  if (cookie == COOKIE_MY_TIMER) {
   
   if (running == TRUE) {
      if (time_sig_counter % time_sig == 0 ) 
        vibes_enqueue_custom_pattern(long_vibe);
      else
        vibes_enqueue_custom_pattern(short_vibe);
      
      timer_handle = app_timer_send_event(ctx, get_interval(), COOKIE_MY_TIMER); // and loop again
      mini_snprintf(timer_str, 20, "TImer VIBE ON clk %d, tmr %d", click_counter, timer_counter);
    } else {
      mini_snprintf(timer_str, 20, "TImer VIBE OFF clk %d, tmr %d", click_counter, timer_counter);
    }


    // always resubmit. can't see a good way to get ctx (is it get_graphics_context?) so 
    // we always run the timer - what changes is whether or not we vibe
    //text_layer_set_text(&timer_text_layer, timer_str);
    timer_counter++;
    time_sig_counter++;
  }

  //layer_mark_dirty(window_get_root_layer(&window));
}



void set_bpm_str() {

  mini_snprintf(bpm_str, 20, "%d bpm", bpm);
  text_layer_set_text(&bpm_text_layer, bpm_str);
}


// single click - increment bpm
void up_single_click_handler(ClickRecognizerRef recognizer, Window *window_2) {
  (void)recognizer;
  (void)window_2;
  bpm = bpm + 2;
  set_bpm_str();


}


// single click - decrement bpm
void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  bpm = bpm - 2;
  set_bpm_str();
}


// select stop/start bpm 
void select_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  click_counter++;
 
  app_timer_cancel_event(g_ctx, timer_handle); // stop any existing timers before starting new ones

  if (running == TRUE) {
    running = FALSE;
    strcpy(click_str, "OFF");
  } else {
    running = TRUE;

    timer_handle = app_timer_send_event(g_ctx, 100, COOKIE_MY_TIMER); // fire 1st timer immediately
    strcpy(click_str, "ON");
  }


  text_layer_set_text(&status_text_layer,click_str);

}


void select_long_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;

}

// set up buttons
void click_config_provider(ClickConfig **config, Window *window) {
  (void)window;

  config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;

  config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;

  config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
  config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;

  config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
  config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;

}






void handle_init(AppContextRef ctx) {
  (void)ctx;
  bpm = 60;
  click_counter = 0;
  running = FALSE;
  g_ctx  = ctx;

  window_init(&window, "Demo");
  window_stack_push(&window, true /* Animated */);

  
  //GFont custom_font =  fonts_get_system_font(FONT_KEY_GOTHAM_42_BOLD);




  //text_layer_init(&title_text_layer, GRect(0,0,144,20));
  //layer_add_child(&window.layer, &title_text_layer.layer);

  text_layer_init(&status_text_layer, GRect(0,130,40,20));
  //text_layer_set_font(&status_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_add_child(&window.layer, &status_text_layer.layer);

  text_layer_init(&timer_text_layer, GRect(0,50,144,20));
  layer_add_child(&window.layer, &timer_text_layer.layer);

  text_layer_init(&select_text_layer, GRect(30,65,100,20));
  layer_add_child(&window.layer, &select_text_layer.layer);

  text_layer_init(&bpm_text_layer, GRect(60,90,144,30));
  text_layer_set_font(&bpm_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  //text_layer_set_background_color(&bpm_text_layer, GColorBlack);
  //text_layer_set_text_color(&bpm_text_layer, GColorWhite);
  layer_add_child(&window.layer, &bpm_text_layer.layer);

  text_layer_init(&time_sig_text_layer, GRect(144-60,130, 60,20));
  //text_layer_set_background_color(&time_sig_text_layer, GColorBlack);
  //text_layer_set_text_color(&time_sig_text_layer, GColorWhite);
  layer_add_child(&window.layer, &time_sig_text_layer.layer);
  

  //mini_snprintf(title_str, 20, "Musical Metronome", VERSION);
  //text_layer_set_text(&title_text_layer,title_str);
  text_layer_set_text(&status_text_layer,"OFF");
  text_layer_set_text(&select_text_layer,"Toggle On/Off ->");
  text_layer_set_text(&time_sig_text_layer,"4/4 Time");
  set_bpm_str();

   timer_handle = app_timer_send_event(ctx, get_interval(), COOKIE_MY_TIMER); // and loop again
  window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);
}




void pbl_main(void *params) {



  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .timer_handler = &handle_timer
  };

  app_event_loop(params, &handlers);
}
