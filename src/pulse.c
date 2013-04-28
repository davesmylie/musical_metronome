#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "mini-printf.h"


//#define MY_UUID { 0x66, 0xA9, 0xFF, 0x29, 0x49, 0x37, 0x41, 0x8E, 0x9E, 0xCC, 0x37, 0x3E, 0x77, 0x6D, 0x63, 0x3C }
#define MY_UUID { 0x1A, 0x6F, 0x65, 0x88, 0x27, 0x0C, 0x44, 0x8D, 0x95, 0x7D, 0x16, 0x63, 0x74, 0x0A, 0xC8, 0x52 }
PBL_APP_INFO(MY_UUID,
             "Musical Metronome", "Dave Inc",
             1, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_STANDARD_APP);

Window window;
AppContextRef g_ctx; // not sure how to get a handle back to this. will make global for nowo

TextLayer up_text_layer;   // display the up/down info help text
TextLayer down_text_layer;

TextLayer title_text_layer;
char      title_str[100];



TextLayer select_text_layer;
TextLayer select_text_layer2;
char      select_str[100];
char      select_str2[100];

TextLayer status_text_layer;
char      click_str[100];


TextLayer bpm_text_layer;
char      bpm_str[5];


TextLayer time_sig_text_layer;
char      time_sig_string[20];

int bpm;
//char str1[100]; // debug sort of string
int click_counter = 0;
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
#define DEBUG 0
#define MAX_BPM 120
#define MIN_BPM 20

const VibePattern short_vibe = {
      .durations = (uint32_t []) {50},
          .num_segments = 1
};

const VibePattern long_vibe = {
      .durations = (uint32_t []) {150},
          .num_segments = 1
};



// calculates the interval between counts based on bpm
// eg 60 bpm is 1000ms, 120bpm is 500ms
int get_interval() {
  return 60000 / bpm;
}

// does the vibration, deciding what sort to use depending on
// position in time sig (ie, short, long, none)
void do_vibe() {
  if (time_sig == 0) return;

  if (time_sig_counter % time_sig == 0 ) 
    vibes_enqueue_custom_pattern(long_vibe);
  else
    vibes_enqueue_custom_pattern(short_vibe);

}



void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
  (void)ctx;
  (void)handle;

  if (cookie == COOKIE_MY_TIMER) {
   
   if (running == TRUE) {
      do_vibe();
      timer_handle = app_timer_send_event(ctx, get_interval(), COOKIE_MY_TIMER); // and loop again
    }

    time_sig_counter++;
  }
}



void set_bpm_str() {
  mini_snprintf(bpm_str, 20, "%d bpm", bpm);
  text_layer_set_text(&bpm_text_layer, bpm_str);
}


// single click - increment bpm
void up_single_click_handler(ClickRecognizerRef recognizer, Window *window_2) {
  (void)recognizer;
  (void)window_2;
  if (bpm < MAX_BPM) {
    bpm = bpm + 2;
    set_bpm_str();
  }
}


// single click - decrement bpm
void down_single_click_handler(ClickRecognizerRef recognizer, Window *window) {
  (void)recognizer;
  (void)window;
  if (bpm > MIN_BPM) {
    bpm = bpm - 2;
    set_bpm_str();
  }
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

        vibes_enqueue_custom_pattern(short_vibe);
  switch (time_sig) {

    case 4:
      time_sig = 3;
      mini_snprintf(time_sig_string, 20, "3/3 Time");
      break;
    case 3:
      time_sig = 2;
      mini_snprintf(time_sig_string, 20, "2/2 Time");
      break;
    case 2:
      mini_snprintf(time_sig_string, 20, "No Time Sig");
      time_sig = 0;
      break;
    default:
      time_sig = 4;
      mini_snprintf(time_sig_string, 20, "4x4 Time");
  }

  text_layer_set_text(&time_sig_text_layer,time_sig_string);
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



void display_title_string() {
  text_layer_init(&title_text_layer, GRect(0,0,144,20));
  layer_add_child(&window.layer, &title_text_layer.layer);
  mini_snprintf(title_str, 20, "Musical Metronome", VERSION);
  text_layer_set_text(&title_text_layer,title_str);
}


void display_up_down_text() {
  text_layer_init(&up_text_layer, GRect(100,0,44,20));
  layer_add_child(&window.layer, &up_text_layer.layer);
  text_layer_set_text_alignment(&up_text_layer, GTextAlignmentRight);
  text_layer_set_font(&up_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(&up_text_layer,"UP ->");

  text_layer_init(&down_text_layer, GRect(144 - 60 ,130,60,20));
  layer_add_child(&window.layer, &down_text_layer.layer);
  text_layer_set_text_alignment(&down_text_layer, GTextAlignmentRight);
  text_layer_set_font(&down_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(&down_text_layer,"Down ->");

}


void display_select_text(){
  text_layer_init(&select_text_layer, GRect(0 ,60,144,20));
  layer_add_child(&window.layer, &select_text_layer.layer);
  text_layer_set_font(&select_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(&select_text_layer, GTextAlignmentRight);
  text_layer_set_text(&select_text_layer,"Toggle On/Off ->");

  text_layer_init(&select_text_layer2, GRect(0,77,144,20));
  text_layer_set_font(&select_text_layer2, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_add_child(&window.layer, &select_text_layer2.layer);
  text_layer_set_text_alignment(&select_text_layer2, GTextAlignmentRight);
  text_layer_set_text(&select_text_layer2,"Hold To Change Time Sig ->");

  if (DEBUG) {
    text_layer_set_background_color(&select_text_layer, GColorBlack);
    text_layer_set_text_color(&select_text_layer, GColorWhite);
    text_layer_set_background_color(&select_text_layer2, GColorBlack);
    text_layer_set_text_color(&select_text_layer2, GColorWhite);
  }
}

void handle_init(AppContextRef ctx) {
  (void)ctx;
  bpm = 90;
  click_counter = 0;
  running = FALSE;
  g_ctx  = ctx;

  window_init(&window, "Demo");
  window_stack_push(&window, true /* Animated */);

  
  //GFont custom_font =  fonts_get_system_font(FONT_KEY_GOTHAM_42_BOLD);
  //display_title_string();
  display_up_down_text();
  display_select_text();





  // Status layer (off/on)
  text_layer_init(&status_text_layer, GRect((144/2) - 30 ,130,60,20));
  text_layer_set_font(&status_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(&status_text_layer, GTextAlignmentCenter);
  layer_add_child(&window.layer, &status_text_layer.layer);




  // setup BPM
  text_layer_init(&bpm_text_layer, GRect((144/2) - 40 ,20, 80 ,30));
  text_layer_set_text_alignment(&bpm_text_layer, GTextAlignmentCenter);
  text_layer_set_font(&bpm_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  if (DEBUG) {
    text_layer_set_background_color(&bpm_text_layer, GColorBlack);
    text_layer_set_text_color(&bpm_text_layer, GColorWhite);
  }
  layer_add_child(&window.layer, &bpm_text_layer.layer);



  
  // setup Time Sig
  text_layer_init(&time_sig_text_layer, GRect(0 ,95, 144 , 32));
  text_layer_set_text_alignment(&time_sig_text_layer, GTextAlignmentCenter);
  text_layer_set_font(&time_sig_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  if (DEBUG) {
    text_layer_set_background_color(&time_sig_text_layer, GColorBlack);
    text_layer_set_text_color(&time_sig_text_layer, GColorWhite);
  }
  layer_add_child(&window.layer, &time_sig_text_layer.layer);
  

  text_layer_set_text(&status_text_layer,"OFF");
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
