#include <pebble.h>

//some code are from the sample code given by Chris
static Window *window;
static TextLayer *text_layer;
static Layer *s_canvas_layer; //to draw a temperature history curve

static char msg[100];
static double numbers[10];
int flag=0; //1-if tempHistory graph is required, 0-otherwise
int canvas=0; //0-if no canvas on window, 1-if canvas already on window
int vib=0; //1-if temperature is required, 0-other request




/**
//the following function transfer a string to a float number
//////////////////////////////////////////////////////////////////////////
///////////////////////////String to float////////////////////////////////
*/
double atof1(char* number_string) {
	double answer= 0;
	int digit_count = 0;
	int temp_digit = 0;
	double digit_shift = 1;

	for (unsigned int i = 0; i < strlen(number_string); i++) {		
		if (number_string[i] == '.') {
			digit_count = strlen(number_string)-1-i;			
		} else if (number_string[i]>='0'&& number_string[i]<='9'){
			temp_digit = number_string[i]-'0';
			answer = 10*answer+temp_digit;
		}		
	}
	if (digit_count == 0) {
		return (double)answer;
	}
	while (digit_count > 0) {
		digit_shift *= 10.0f;
		digit_count--;
	}
	return answer/digit_shift;
}
//////////////////////////////////////////////////////////////////////////
///////////////////////////String to float////////////////////////////////




/**
//the following function draw a curve line on the canvas layer given 10 temperature records
//////////////////////////////////////////////////////////////////////////
///////////////////////////temp history///////////////////////////////////
*/
void my_layer_update_proc(Layer *my_layer, GContext* ctx) {
	GRect bounds = layer_get_bounds(my_layer);
	int width = bounds.size.w;
	int height = bounds.size.h-10;
	int delta_width = (width-15)/10;
 	char current_number[15];
  GPoint points[10];
	double max = -100;
	double min = 100;
	int j = 0, k = 0;
	for (unsigned int i = 0; i < strlen(msg); i++) {
		if (msg[i] != ',') {
			current_number[j] = msg[i];
			j++;
		} else {
			current_number[j] = '\0';
			if (k == 0) {
				max = atof1(current_number);
			} else if (k == 1) {
				min = atof1(current_number);
			} else {
				numbers[k-2] = atof1(current_number);
			}			
			j = 0;
			k++;
		}
	}

	for (int i = 0; i < 10; i++) {
		points[i] = GPoint(20+i*delta_width, 10+(max-numbers[i])/(max-min)*(height-10));	
	}
  
  
	for (int i = 1; i < 10; i++) {
		graphics_context_set_stroke_color(ctx, GColorBlack);
		graphics_draw_line(ctx, points[i - 1], points[i]);	
	}
}
//////////////////////////////////////////////////////////////////////////
///////////////////////////temp history///////////////////////////////////


void out_sent_handler(DictionaryIterator *sent, void *context) {
// outgoing message was delivered -- do nothing
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
// outgoing message failed
  if(canvas) {
    layer_remove_from_parent(s_canvas_layer); 
    canvas=0;
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_layer));
  }
  text_layer_set_text(text_layer, "Error sending request.");
}


void in_received_handler(DictionaryIterator *received, void *context) {
// incoming message received 
// looks for key #0 in the incoming message
  int key = 0;
  Tuple *text_tuple = dict_find(received, key);
  if (text_tuple) {
    if (text_tuple->value) {
      // put it in this global variable
      strcpy(msg, text_tuple->value->cstring);
      if(flag&&strlen(msg)>0){
        if(canvas==0){
          //remove text layer from the window
          layer_remove_from_parent(text_layer_get_layer(text_layer)); 
          //add canvas layer to show on the window
          layer_add_child(window_get_root_layer(window), s_canvas_layer);
          //flag that currently there is a canvas layer on the window
          canvas=1;
        } else {
        layer_mark_dirty(s_canvas_layer);
        }
        layer_set_update_proc(s_canvas_layer, my_layer_update_proc);  
      } else{
        if(canvas) {
          //remove canvas layer from the window
          layer_remove_from_parent(s_canvas_layer); 
          //flag that currently there is no canvas layer on the window
          canvas=0;
          //text layer showing again
          layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_layer));
        }
        text_layer_set_text(text_layer, msg);
        
        /**
        //the following lines help to identify if the watch should report the alarm for a "too high" temperature
        //////////////////////////////////////////////////////////////////////////
        ///////////////////////////alarm function/////////////////////////////////
        */
        if(vib && msg[0] >= '0' && msg[0] <= '9'){         
          char current_number[15];
          double current_temp = 0;
          for (unsigned int i = 0; i < strlen(msg); i++) {
	          if (msg[i] != ' ') {
		          current_number[i] = msg[i];
	          } else {
		          current_number[i] = '\0';
	          }
          }
          current_temp = atof1(current_number);
          if (current_temp >= 60) {
	          current_temp = (current_temp-32)/1.8;
          }
          if(current_temp >= 33){
            vibes_short_pulse();
            vib=0;
          }           
        }
        //////////////////////////////////////////////////////////////////////////
        ///////////////////////////alarm function/////////////////////////////////
      }
    }
    else {
      if(canvas) {
        //remove canvas layer from the window
        layer_remove_from_parent(s_canvas_layer); 
        //flag that currently there is no canvas layer on the window
        canvas=0;
        //text layer showing again
        layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_layer));
        
      }
      strcpy(msg, "no value!");
      text_layer_set_text(text_layer, msg);
    }
  }
  else {
    if(canvas) {
      layer_remove_from_parent(s_canvas_layer);
      canvas=0;
      layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_layer));
    }
    text_layer_set_text(text_layer, "no message!");
  }
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  // incoming message dropped
  if(canvas) {
    layer_remove_from_parent(s_canvas_layer); 
    canvas=0;
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_layer));
  }
  text_layer_set_text(text_layer, "Error in!");
}




/**
 * This is called when the select button is clicked
 * Show the current temperature
 */
void select_click_handler(ClickRecognizerRef recognizer, void *context) {
   flag=0;//check if the curve layer is needed
   vib=1; //check if it might have a condition to report the warning (vibration)
   DictionaryIterator *iter;
   app_message_outbox_begin(&iter);
   int key = 0;
   // send the message "temperature" to the phone, using key #0
   Tuplet value = TupletCString(key, "temperature");
   dict_write_tuplet(iter, &value);
   app_message_outbox_send();
}


/**
 * This is called when select is hold
 * Show temperature in F/C in turn
 */
void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  flag=0;//check if the curve layer is needed
   DictionaryIterator *iter;
   app_message_outbox_begin(&iter);
   int key = 0;
   // send the message "F/C" to the phone, using key #0
   Tuplet value = TupletCString(key, "F/C");
   dict_write_tuplet(iter, &value);
   app_message_outbox_send();
}

/**
 * This is called when up is clicked
 * turn the user interface and sensor to standby mode
 */
void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  flag=0;//check if the curve layer is needed
   DictionaryIterator *iter;
   app_message_outbox_begin(&iter);
   int key = 0;
   Tuplet value = TupletCString(key, "standby");
   dict_write_tuplet(iter, &value);
   app_message_outbox_send();
}


/**
 * This is called when double-triple up button is clicked
 * turn the user interface and sensor back to normal operation
 */
void up_multi_click_handler(ClickRecognizerRef recognizer, void *context) {
  flag=0;//check if the curve layer is needed
   DictionaryIterator *iter;
   app_message_outbox_begin(&iter);
   int key = 0;
   Tuplet value = TupletCString(key, "wakeup");
   dict_write_tuplet(iter, &value);
   app_message_outbox_send();
}


/**
 * This is called when down buttom is clicked
 * a temperature curve for last one minute is shown
 */
void down_click_handler(ClickRecognizerRef recognizer, void *context) {
   flag=1; //check if the curve layer is needed
   DictionaryIterator *iter;
   app_message_outbox_begin(&iter);
   int key = 0;
   Tuplet value = TupletCString(key, "tempHistory");
   dict_write_tuplet(iter, &value);
   app_message_outbox_send();
}


/**
 * This is called when up long buttom is hold
 * calibrate++
 */
void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
   flag=0; //check if the curve layer is needed
   DictionaryIterator *iter;
   app_message_outbox_begin(&iter);
   int key = 0;
   // send the message "+" to the phone, using key #0
   Tuplet value = TupletCString(key, "+");
   dict_write_tuplet(iter, &value);
   app_message_outbox_send();
}

/**
 * This is called when down long buttom is hold
 * calibrate--
 */
void down_long_click_handler(ClickRecognizerRef recognizer, void *context) {
   flag=0; //check if the curve layer is needed
   DictionaryIterator *iter;
   app_message_outbox_begin(&iter);
   int key = 0;
   // send the message "-" to the phone, using key #0
   Tuplet value = TupletCString(key, "-");
   dict_write_tuplet(iter, &value);
   app_message_outbox_send();
}


/* this registers the appropriate function to the appropriate button */
void config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, NULL, select_long_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 0, NULL, up_long_click_handler);
  window_long_click_subscribe(BUTTON_ID_DOWN, 0, NULL, down_long_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_multi_click_subscribe(BUTTON_ID_UP,2, 3, 0, true, up_multi_click_handler);
}

/**
 * The following functions are called when the app is initilized
 * window and layer is created and ready to use
 */
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  //canvas layer created
  s_canvas_layer = layer_create(bounds);
  text_layer = text_layer_create((GRect) 
  { .origin = { 0, 72 }, 
    .size = { bounds.size.w, bounds.size.h } });
  text_layer_set_text(text_layer, "Welcome to the Pebble Temperature APP");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  // need this for adding the listener
  window_set_click_config_provider(window, config_provider);
  
  // for registering AppMessage handlers
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  const uint32_t inbound_size = 64;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);
  
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}