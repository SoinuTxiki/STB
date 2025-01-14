#include "pch.h"

#include "song.h"
#include "display.h"
#include "midi.h"
#include "keyboard.h"
#include "config.h"
#include "gui.h"
#include "language.h"
#include "utilities.h"

#include <dinput.h>
#include <Shlwapi.h>
#include <zlib.h>
#include <list>
#include <vector>


struct song_event_t {
  double time;
  byte a;
  byte b;
  byte c;
  byte d;
};

// record buffer (1M events should be enough.)
static song_event_t song_event_buffer[1024 * 1024];

// record or play position
static song_event_t *play_position = NULL;
static song_event_t *record_position = NULL;
static song_event_t *song_end = NULL;
static double song_timer = 0;
static double song_play_speed = 1;
static double song_auto_pedal_timer = 0;
static double song_clock = 0;

static song_info_t song_info;

// song thread lock
static thread_lock_t song_lock;

// -----------------------------------------------------------------------------------------
// SYNC and DELAY event
// -----------------------------------------------------------------------------------------
struct sync_event_t {
  uint flag;
  uint id;
  key_bind_t map;
};

struct delay_event_t {
  double timer;
  key_bind_t map;
};

static std::list<sync_event_t> sync_events;
static std::list<delay_event_t> delay_events;

// sync trigger
static uint sync_trigger = 0;
static uint sync_id = 0;

// fixed delay timer
static const double fixed_delay_timer = 20;
static const double fixed_smooth_timer = 2;

// reset sync events
static void sync_event_reset() {
  thread_lock lock(song_lock);
  sync_events.clear();
  sync_trigger = 0;
  sync_id = 0;
}

// add sync event
static void sync_event_add(uint flag, byte a, byte b, byte c, byte d) {
  thread_lock lock(song_lock);

  sync_event_t e;
  e.flag = flag;
  e.id = sync_id;
  e.map.a = a;
  e.map.b = b;
  e.map.c = c;
  e.map.d = d;
  sync_events.push_back(e);
}

// update sync events
static void sync_event_update(double time) {
  thread_lock lock(song_lock);

  // events to output
  std::vector<key_bind_t> output_events;

  // find triggered events
  for (auto it = sync_events.begin(); it != sync_events.end();) {
    auto e = it; ++it;
    if (e->id != sync_id && e->flag & sync_trigger) {
      output_events.push_back(e->map);
      sync_events.erase(e);
    }
  }

  // reset sync trigger
  if (sync_trigger) {
    sync_id ++;
    sync_trigger = 0;
  }

  // output events
  if (!output_events.empty()) {
    for (auto it = output_events.begin(); it != output_events.end(); ++it) {
      song_output_event(it->a, it->b, it->c, it->d);
    }
  }
}

// reset sync events
static void delay_event_reset() {
  thread_lock lock(song_lock);
  delay_events.clear();
}

// add delay event
static void delay_event_add(double timer, byte a, byte b, byte c, byte d) {
  thread_lock lock(song_lock);

  delay_event_t e;
  e.timer = timer;
  e.map.a = a;
  e.map.b = b;
  e.map.c = c;
  e.map.d = d;
  delay_events.push_back(e);
}

// update delay events
static void delay_event_update(double time) {
  thread_lock lock(song_lock);

  // events should output this time
  std::vector<key_bind_t> output_events;

  // find triggered events
  for (auto it = delay_events.begin(); it != delay_events.end();) {
    auto e = it; ++it;
    e->timer -= time;

    if (e->timer <= 0) {
      output_events.push_back(e->map);
      delay_events.erase(e);
    }
  }

  // output events
  if (!output_events.empty()) {
    for (auto it = output_events.begin(); it != output_events.end(); ++it) {
      song_output_event(it->a, it->b, it->c, it->d);
    }
  }
}

// -----------------------------------------------------------------------------------------
// event translator
// -----------------------------------------------------------------------------------------

static inline byte translate_channel(byte ch) {
  return config_get_output_channel(ch) & 0x0f;
}

static inline byte translate_note(byte ch, byte note) {
  int value = note;
  value += config_get_key_octshift(ch) * 12;
  value += config_get_key_transpose(ch);

  if (config_get_follow_key(ch))
    value += config_get_key_signature();

  return clamp_value(value, 0, 127);
}

static inline byte translate_pressure(byte ch, byte pressure) {
  return clamp_value((int)pressure * config_get_key_velocity(ch) / 127, 0, 127);
}

static inline int default_value(int v, int dv = 0) {
  if (v > 127) return dv;
  if (v < -127) return dv;
  return v;
}

static int translate_value(byte action, int value, int change) {
  switch (action & 0x0f) {
  case SM_VALUE_SET: value = change; break;
  case SM_VALUE_INC: value = value + change; break;
  case SM_VALUE_DEC: value = value - change; break;
  case SM_VALUE_FLIP: value = change - value; break;
  case SM_VALUE_PRESS: value = change; break;
  case SM_VALUE_SET10: value = change / 10 * 10 + (value % 10); break;
  case SM_VALUE_SET1: value = value / 10 * 10 + (change % 10); break;
  }
  return value;
}

// translate message
bool song_translate_note(byte &a, byte &b, byte &c, byte &d) {
  switch (a) {
  case SM_NOTE_ON:
    {
      byte ch = b;
      byte note = c;
      byte pressure = d;
      a = SM_MIDI_NOTEON | translate_channel(ch);
      b = translate_note(ch, note);
      c = translate_pressure(ch, pressure);
      d = 0;
      return true;
    }
    break;
  case SM_NOTE_OFF:
    {
      a = SM_MIDI_NOTEOFF | translate_channel(b);
      b = translate_note(b, c);
      c = 0;
      d = 0;
      return true;
    }
    break;

  case SM_NOTE_PRESSURE:
    {
      byte ch = b;
      byte note = c;
      byte pressure = d;
      a = SM_MIDI_PRESSURE | translate_channel(ch);
      b = translate_note(ch, note);
      c = translate_pressure(ch, pressure);
      d = 0;
      return true;
    }
    break;
  }
  return false;
}

// -----------------------------------------------------------------------------------------
// smoothd parameters
// -----------------------------------------------------------------------------------------
struct smooth_param_t {
  char target;
  char current;
  double timer;
};

static inline bool smooth_param_update(smooth_param_t &param, double time_elapsed) {
  param.timer -= time_elapsed;

  if (param.timer < 0) {
    char cur = param.current;

    if (param.target != param.current) {
      param.timer = fixed_smooth_timer;

      if (param.current > param.target) --param.current;
      if (param.current < param.target) ++param.current;

      return true;
    }
  }

  return false;
}

static smooth_param_t pitch_smooth[16] = {0};

static void pitch_update(double time) {
  for (int ch = 0; ch < 16; ch++) {
    if (smooth_param_update(pitch_smooth[ch], time)) {
      byte a = SM_MIDI_PITCH_BEND | ch;
      byte c = 64 + clamp_value<char>(pitch_smooth[ch].current, -64, 63);
      midi_output_event(a, 0, c, 0);
    }
  }
}

// -----------------------------------------------------------------------------------------
// event parser
// -----------------------------------------------------------------------------------------

// add event
static void song_add_event(double time, byte a, byte b, byte c, byte d);

// dynamic mapping
static byte keyboard_map_key_code = 0;
static byte keyboard_map_key_type = 0;
static byte keyboard_label_key_code = 0;
static byte keyboard_label_key_size = 0;
static char keyboard_label_text[256];
static byte keyboard_color_key_code = 0;

// keyboard event map
static void keyboard_event_map(int code, int type) {
  keyboard_map_key_code = code;
  keyboard_map_key_type = type;
}

// keyboard event label
static void keyboard_event_label(int code, int length) {
  keyboard_label_key_code = code;
  keyboard_label_key_size = length;
  keyboard_label_text[0] = 0;
}

// keyboard event color
static void keyboard_event_color(int code, int length) {
  keyboard_color_key_code = code;
}

// event message
extern byte keyboard_status[256];
extern uint current_setting;
void song_send_event(byte a, byte b, byte c, byte d, bool record) {
   //song_send_event(SM_SYSTEM, SMS_KEY_EVENT, code, keydown, true);
	byte c2;
  thread_lock lock(song_lock);

  // record event
  if (record) {
    // HACK: don't record playback control commands
    if (a != SM_PLAY &&
        a != SM_RECORD &&
        a != SM_STOP)
      song_add_event(song_timer, a, b, c, d);
  }

  // setting a key label
  if (keyboard_label_key_size) {
    char *ch = keyboard_label_text + strlen(keyboard_label_text);
    char data[4] = { a, b, c, d };

    for (int i = 0; i < 4; i++) {
      ch[i] = data[i];
      ch[i + 1] = 0;

      if (--keyboard_label_key_size == 0) {
        config_bind_set_label(keyboard_label_key_code, keyboard_label_text);
        break;
      }
    }
    return;
  }

  // mapping a key.
  if (keyboard_map_key_code) {
    switch (keyboard_map_key_type) {
     case 0: config_bind_add_keydown(keyboard_map_key_code, key_bind_t(a, b, c, d)); break;
     case 1: config_bind_add_keyup(keyboard_map_key_code, key_bind_t(a, b, c, d)); break;
    }

    keyboard_map_key_code = 0;
    return;
  }

  // color
  if (keyboard_color_key_code) {
    uint color = (a << 24) | (b << 16) | (c << 8) | d;
    config_bind_set_color(keyboard_color_key_code, color); //XAM TODO proatzea?);
    keyboard_color_key_code = 0;
    return;
  }


  // special events
  if (a == SM_SYSTEM) {
    switch (b) {
	 case SMS_KEY_EVENT: {
		 byte lednum = 0;
		//keyboard_status[code] = keydown;
		if ((c == 57) || (c == 184)) { //if any space pressed/depressed     //&& keyboard_status[c] != d
			c2 = 57;
			keyboard_send_event(57, d); //do action coresponding to space button
						for (uint i = 0; i < 255; i++){
				if (keyboard_status[i] == 1) {//for all the pressed keys, turn them off and back on with new color
					lednum = trans_kc_led(i); //get lednum for code
					LED_event(SM_MIDI_NOTEOFF + current_setting, lednum, 0, 0);//itzali 
					LED_event(SM_MIDI_NOTEOFF + current_setting, lednum, 0, 0);
					LED_event(SM_MIDI_NOTEON + current_setting, lednum, 255, 0);//piztu cuurent setting koloren
					LED_event(SM_MIDI_NOTEON + current_setting, lednum, 255, 0);
				}
			}
		}
		else {
			keyboard_send_event(c, d);
			c2 = c;
		}

		lednum = trans_kc_led(c2);
		switch (d) {
		case 1: {//key pressed -> turn on
			LED_event(SM_MIDI_NOTEON + current_setting, lednum, 255, 0);
			LED_event(SM_MIDI_NOTEON + current_setting, lednum, 255, 0);
		}
				break;
		default: {//key depressed -> turn off
			LED_event(SM_MIDI_NOTEOFF + current_setting, lednum, 0, 0);
			LED_event(SM_MIDI_NOTEOFF + current_setting, lednum, 0, 0);
		}
				break;
		/*default: {
			LED_event(0, 0, c, d);
		}*/
		}
		
	}
	break;
     case SMS_KEY_MAP:   keyboard_event_map(c, d); break;
     case SMS_KEY_LABEL: keyboard_event_label(c, d); break;
     case SMS_KEY_COLOR: keyboard_event_color(c, d); break;
    }
    return;
  }

  song_output_event(a, b, c, d);
}

byte trans_kc_led(byte c) {//translate keycode to lednum
	byte led;
	switch (c) {
	case 59:
	case 60:
	case 61:
	case 62:
	case 63:
	case 64:
	case 65:
	case 66:
	case 67:
	case 68: {
		led = c - 58;
		break;
	  }
	case 87:
	case 88: {
		led = c - 76;
		break;
	  }
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13: {
		led = c + 14;
		break;
	  }
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	case 38: {
		led = c + 16;
		break;
	  }
	case 46:
	case 47:
	case 48:
	case 49:
	case 50:
	case 51: {
		led = c + 16;
		break;
	  }
	case 57: {//space
		led = 76;
		break;
	}
	default: { led = 0;
		break;
	  }
	}
	return led;
}

static void output_controller(byte a, byte b, byte c, byte d, byte id) {
  byte ch = b;
  byte op = c;
  int change = (char)d;
  int value = 0;

  value = config_get_controller(ch, id);
  value = default_value(value);
  value = translate_value(op, value, change);
  value = clamp_value(value, 0, 127);

  // sync event
  if (op & SM_VALUE_SYNC) {
    sync_event_add(SONG_SYNC_FLAG_KEYDOWN, a, b, c & ~SM_VALUE_SYNC, d);
    return;
  }

  // restore
  if ((op & 0x0f) == SM_VALUE_PRESS) {
    delay_event_add(fixed_delay_timer, a, b, SM_VALUE_SET, config_get_controller(ch, id));
  }

  midi_output_event(SM_MIDI_CONTROLLER | translate_channel(ch), id, value, 0);
}

void song_output_event(byte a, byte b, byte c, byte d) {
  // midi events
  if (a >= SM_MIDI_MESSAGE_START) {
    midi_output_event(a, b, c, d);
    return;
  }

  // keyboard control events
  switch (a) {
   case SM_KEY_SIGNATURE: {
     byte op = b;
     char change = c;
     int value = config_get_key_signature();

     // sync event
     if (op & SM_VALUE_SYNC) {
       sync_event_add(SONG_SYNC_FLAG_KEYDOWN, a, op & ~SM_VALUE_SYNC, c, 0);
       return;
     }

     // restore
     if ((op & 0x0f) == SM_VALUE_PRESS) {
       delay_event_add(fixed_delay_timer, a, SM_VALUE_SET, value, 0);
     }

     value = translate_value(op, value, change);
     value = wrap_value(value, -4, 7);
     config_set_key_signature(value);
   }
   break;

   case SM_TRANSPOSE: {
     int ch = b;
     byte op = c;
     char change = d;
     int value = config_get_key_transpose(ch);

     // sync event
     if (op & SM_VALUE_SYNC) {
       sync_event_add(SONG_SYNC_FLAG_KEYDOWN, a, b, op & ~SM_VALUE_SYNC, d);
       return;
     }

     // restore
     if ((op & 0x0f) == SM_VALUE_PRESS) {
       delay_event_add(fixed_delay_timer, a, b, SM_VALUE_SET, value);
     }

     value = translate_value(op, value, change);
     value = clamp_value(value, -64, 64);
     config_set_key_transpose(ch, value);
   }
   break;

   case SM_FOLLOW_KEY: {
     int ch = b;
     byte op = c;
     char change = d;
     int value = config_get_follow_key(ch);

     // sync event
     if (op & SM_VALUE_SYNC) {
       sync_event_add(SONG_SYNC_FLAG_KEYDOWN, a, b, op & ~SM_VALUE_SYNC, d);
       return;
     }

     // restore
     if ((op & 0x0f) == SM_VALUE_PRESS) {
       delay_event_add(fixed_delay_timer, a, b, SM_VALUE_SET, value);
     }

     value = translate_value(op, value, change);
     value = clamp_value(value, 0, 1);
     config_set_follow_key(ch, value);
   }
   break;

   case SM_OCTAVE: {
     int ch = b;
     byte op = c;
     char change = d;
     int value = config_get_key_octshift(ch);

     // sync event
     if (op & SM_VALUE_SYNC) {
       sync_event_add(SONG_SYNC_FLAG_KEYDOWN, a, b, op & ~SM_VALUE_SYNC, d);
       return;
     }

     // restore
     if ((op & 0x0f) == SM_VALUE_PRESS) {
       delay_event_add(fixed_delay_timer, a, b, SM_VALUE_SET, value);
     }

     value = translate_value(op, value, change);
     value = wrap_value(value, -1, 3); //XAM oktabak -1etik 3aino
     config_set_key_octshift(ch, value);
   }
   break;

   case SM_VELOCITY: {
     int ch = b;
     byte op = c;
     char change = d;
     int value = config_get_key_velocity(ch);

     // sync event
     if (op & SM_VALUE_SYNC) {
       sync_event_add(SONG_SYNC_FLAG_KEYDOWN, a, b, op & ~SM_VALUE_SYNC, d);
       return;
     }

     // restore
     if ((op & 0x0f) == SM_VALUE_PRESS) {
       delay_event_add(fixed_delay_timer, a, b, SM_VALUE_SET, value);
     }

     value = translate_value(op, value, change);
     value = clamp_value(value, 0, 127);
     config_set_key_velocity(ch, value);
   }
   break;

   case SM_CHANNEL: {
     int ch = b;
     byte op = c;
     char change = d;
     int value = config_get_output_channel(ch);

     // sync event
     if (op & SM_VALUE_SYNC) {
       sync_event_add(SONG_SYNC_FLAG_KEYDOWN, a, b, op & ~SM_VALUE_SYNC, d);
       return;
     }

     // restore
     if ((op & 0x0f) == SM_VALUE_PRESS) {
       delay_event_add(fixed_delay_timer, a, b, SM_VALUE_SET, value);
     }

     value = translate_value(op, value, change);
     value = clamp_value(value, 0, 15);
     config_set_output_channel(ch, value);
   }
   break;

   case SM_VOLUME: {
     byte op = b;
     byte change = c;
     int value = config_get_output_volume();

     // sync event
     if (op & SM_VALUE_SYNC) {
       sync_event_add(SONG_SYNC_FLAG_KEYDOWN, a, op & ~SM_VALUE_SYNC, c, 0);
       return;
     }

     // restore
     if ((op & 0x0f) == SM_VALUE_PRESS) {
       delay_event_add(fixed_delay_timer, a, SM_VALUE_SET, value, 0);
     }

     value = translate_value(op, value, change);
     value = clamp_value(value, 0, 200);
     config_set_output_volume(value);
   }
   break;

   case SM_PLAY:
     if (song_allow_input())
       song_start_playback();
     break;

   case SM_RECORD:
     if (song_allow_input()) {
       if (song_is_recording())
         song_stop_record();
       else
         song_start_record();
     }
     break;

   case SM_STOP:
     if (song_is_recording())
       song_stop_record();

     if (song_is_playing())
       song_stop_playback();
     break;

   case SM_SETTING_GROUP: {
     byte op = b;
     char change = c;
     int value = config_get_setting_group();

     // sync event
     if (op & SM_VALUE_SYNC) {
       sync_event_add(SONG_SYNC_FLAG_KEYDOWN, a, op & ~SM_VALUE_SYNC, c, 0);
       return;
     }

     // restore
     if ((op & 0x0f) == SM_VALUE_PRESS) {
       delay_event_add(fixed_delay_timer, a, SM_VALUE_SET, value, 0);
     }

     value = translate_value(op, value, change);
     value = wrap_value(value, 0, (int)config_get_setting_group_count() - 1);
     config_set_setting_group(value);
   }
   break;

   case SM_SETTING_GROUP_COUNT: {
     config_set_setting_group_count(b);
   }
   break;

   case SM_NOTE_ON:
   case SM_NOTE_OFF:
   case SM_NOTE_PRESSURE:
     if (song_translate_note(a, b, c, d)) {
       midi_output_event(a, b, c, d);
     }
     break;

  case SM_PRESSURE:
    {
      byte ch = translate_channel(b);
      byte op = c;
      char change = d;
      int value = 0;

      // sync event
      if (op & SM_VALUE_SYNC) {
        sync_event_add(SONG_SYNC_FLAG_KEYDOWN, a, b, op & ~SM_VALUE_SYNC, d);
        return;
      }

      // restore
      if ((op & 0x0f) == SM_VALUE_PRESS) {
        delay_event_add(fixed_delay_timer, a, b, SM_VALUE_SET, value);
      }

      value = translate_value(op, value, change);
      value = translate_pressure(b, value);

      a = SM_MIDI_CHANNEL_PRESSURE | ch;
      b = value;
      midi_output_event(a, b, 0, 0);
    }
    break;

  case SM_PITCH:
    {
      byte ch = translate_channel(b);
      byte op = c;
      char change = d;
      int value = pitch_smooth[ch].target;
      value = default_value(value);

      // sync event
      if (op & SM_VALUE_SYNC) {
        sync_event_add(SONG_SYNC_FLAG_KEYDOWN, a, b, op & ~SM_VALUE_SYNC, d);
        return;
      }

      // restore
      if ((op & 0x0f) == SM_VALUE_PRESS) {
        delay_event_add(fixed_delay_timer, a, b, SM_VALUE_SET, value);
      }

      value = translate_value(op, value, change);
      value = clamp_value(value, -64, 63);

      pitch_smooth[ch].target = value;
    }
    break;

  case SM_PROGRAM:
    {
      byte ch = b;
      byte op = c;
      char change = d;
      int value = config_get_program(ch);
      value = default_value(value);

      // sync event
      if (op & SM_VALUE_SYNC) {
        sync_event_add(SONG_SYNC_FLAG_KEYDOWN, a, b, op & ~SM_VALUE_SYNC, d);
        return;
      }

      // restore
      if ((op & 0x0f) == SM_VALUE_PRESS) {
        delay_event_add(fixed_delay_timer, a, b, SM_VALUE_SET, value);
      }

      value = translate_value(op, value, change);
      value = clamp_value(value, 0, 127);

      a = SM_MIDI_PROGRAM | translate_channel(ch);
      b = value;
      c = 0;
      d = 0;
      midi_output_event(a, b, c, d);
    }
    break;

  case SM_BANK_LSB:
    output_controller(a, b, c, d, 32);
    break;

  case SM_BANK_MSB:
    output_controller(a, b, c, d, 0);
    break;

  case SM_SUSTAIN:
    output_controller(a, b, c, d, 64);
    break;

  case SM_MODULATION:
    output_controller(a, b, c, d, 1);
    break;
  }
}

// -----------------------------------------------------------------------------------------
// record and playback
// -----------------------------------------------------------------------------------------

// add event
static void song_add_event(double time, byte a, byte b, byte c, byte d) {
  if (record_position) {
    record_position->time = time;
    record_position->a = a;
    record_position->b = b;
    record_position->c = c;
    record_position->d = d;

    ++record_position;

    // just simple set song end to record posision
    song_end = record_position;

    // auto stop record
    if (record_position == ARRAY_END(song_event_buffer) - 1) {
      song_stop_record();
    }
  }
}

// init record
static void song_init_record() {
  song_timer = 0;
  song_clock = 0;
  song_auto_pedal_timer = 0;
  record_position = song_event_buffer;
  play_position = NULL;
  song_end = song_event_buffer;
  song_info.version = APP_VERSION;
  song_info.author[0] = 0;
  song_info.title[0] = 0;
  song_info.comment[0] = 0;
  song_info.write_protected = false;
  song_info.compatibility = true;

  song_play_speed = 1;
}

// reset
static void song_reset_event() {
  // exit key map mode
  keyboard_map_key_code = 0;

  // exit key label mode
  keyboard_label_key_size = 0;

  // reset keyboard
  keyboard_reset();

  // reset sync and delay events
  sync_event_reset();
  delay_event_reset();

  // reset midi
  midi_reset();
}

// start record
void song_start_record() {
  thread_lock lock(song_lock);

  song_stop_playback();
  song_stop_record();
  song_init_record();

  int current_group = config_get_setting_group();
  int total_groups = config_get_setting_group_count();

  // record config groups
  song_add_event(0, SM_SETTING_GROUP_COUNT, total_groups, 0, 0);

  // record group settings
  for (int i = 0; i < total_groups; i++) {
    // change current setting group
    config_set_setting_group(i);
    song_add_event(0, SM_SETTING_GROUP, SM_VALUE_SET, i, 0);

    // record current configs
    song_add_event(0, SM_KEY_SIGNATURE, SM_VALUE_SET, config_get_key_signature(), 0);

    // record input settings
    for (int ch = SM_INPUT_0; ch <= SM_INPUT_MAX; ch++) {
      if (config_get_key_octshift(ch))
        song_add_event(0, SM_OCTAVE, ch, SM_VALUE_SET, config_get_key_octshift(ch));

      if (config_get_key_transpose(ch))
        song_add_event(0, SM_TRANSPOSE, ch, SM_VALUE_SET, config_get_key_transpose(ch));

      if (config_get_key_velocity(ch) != 127)
        song_add_event(0, SM_VELOCITY, ch, SM_VALUE_SET, config_get_key_velocity(ch));

      if (config_get_output_channel(ch))
        song_add_event(0, SM_CHANNEL, ch, SM_VALUE_SET, config_get_output_channel(ch));
    }

    // record output settings
    for (int ch = SM_OUTPUT_0; ch <= SM_OUTPUT_MAX; ch++) {
      // program
      if (config_get_program(ch) < 128)
        song_add_event(0, SM_PROGRAM, ch, SM_VALUE_SET, config_get_program(ch));

      // bank msb
      if (config_get_controller(ch, 0) < 128)
        song_add_event(0, SM_BANK_MSB, ch, SM_VALUE_SET, config_get_controller(ch, 0));

      // bank lsb
      if (config_get_controller(ch, 32) < 128)
        song_add_event(0, SM_BANK_LSB, ch, SM_VALUE_SET, config_get_controller(ch, 32));

      // sustain
      if (config_get_controller(ch, 64) < 128)
        song_add_event(0, SM_SUSTAIN, ch, SM_VALUE_SET, config_get_controller(ch, 64));

      // modulation
      if (config_get_controller(ch, 1) < 128)
        song_add_event(0, SM_MODULATION, ch, SM_VALUE_SET, config_get_controller(ch, 1));
    }

    // record keymaps
    for (int i = 0; i < 256; i++) {
      int count;
      key_bind_t bind[256];

      count = config_bind_get_keydown(i, bind, 256);
      for (int j = 0; j < count; j++) {
        if (bind[j].a) {
          song_add_event(0, SM_SYSTEM, SMS_KEY_MAP, i, 0);
          song_add_event(0, bind[j].a, bind[j].b, bind[j].c, bind[j].d);
        }
      }

      count = config_bind_get_keyup(i, bind, 256);
      for (int j = 0; j < count; j++) {
        if (bind[j].a) {
          song_add_event(0, SM_SYSTEM, SMS_KEY_MAP, i, 1);
          song_add_event(0, bind[j].a, bind[j].b, bind[j].c, bind[j].d);
        }
      }

      if (*config_bind_get_label(i)) {
        const char *label = config_bind_get_label(i);
        int size = strlen(label);

        song_add_event(0, SM_SYSTEM, SMS_KEY_LABEL, i, size);

        for (int i = 0; i < (size + 3) / 4; i++) {
          song_add_event(0, label[0], label[1], label[2], label[3]);
          label += 4;
        }
      }

      if (config_bind_get_color(i)) {
        uint color = config_bind_get_color(i);
        song_add_event(0, SM_SYSTEM, SMS_KEY_COLOR, i, 1);
        song_add_event(0, color >> 24, color >> 16, color >> 8, color);
      }
    }
  }

  // restore current group
  config_set_setting_group(current_group);
  song_add_event(0, SM_SETTING_GROUP, 0, current_group, 0);
}

// stop record
void song_stop_record() {
  thread_lock lock(song_lock);

  if (record_position) {
    song_add_event(song_timer, SM_STOP, 0, 0, 0);
    song_reset_event();
    record_position = NULL;
  }
}

// is recoding
bool song_is_recording() {
  thread_lock lock(song_lock);

  return record_position != NULL;
}

// start playback
void song_start_playback() {
	thread_lock lock(song_lock);
	//HID XAM
	/*int res;
	unsigned char buf[33];
	//wchar_t wstr[255];
	hid_device *handle;
	hid_device *dev = NULL; // HIDAPI device we will open

	uint16_t vid = 0;        // productId
	uint16_t pid = 0;        // vendorId
	uint16_t usage_page = 0; // usagePage to search for, if any
	uint16_t usage = 0;      // usage to search for, if any
	wchar_t serial_wstr[1024 / 4] = { L'\0' }; // serial number string rto search for, if any
	char devpath[1024];   // path to open, if filter by usage*/
	//  int i;

	// Initialize the hidapi library
	/*res = hid_init();
	// Open the device using the VID, PID,
	// and optionally the Serial number.
	struct hid_device_info *devs, *cur_dev;
	devs = hid_enumerate(0x0C45, 0x5004);
	while (cur_dev) {
		if ((!vid || cur_dev->vendor_id == vid) &&
			(!pid || cur_dev->product_id == pid) &&
			(!usage_page || cur_dev->usage_page == usage_page) &&
			(!usage || cur_dev->usage == usage) &&
			(serial_wstr[0] == L'\0' || wcscmp(cur_dev->serial_number, serial_wstr) == 0)) {
			strncpy(devpath, cur_dev->path, 1024); // save it!
		}
		cur_dev = cur_dev->next;
	}
	hid_free_enumeration(devs);
	if (devpath[0]) {
		dev = hid_open_path(devpath);
		if (dev == NULL) {

			 //msg("Error: could not open device\n");
		}
		else {
			//msg("Device opened\n");
			buf[0] = 0x0;//DUMMY
			buf[1] = 0x08;//LEDS OFF
			buf[2] = 0x00;//
			res = hid_write(dev, buf, 32);
			hid_close(dev); 
		}
	}
	else {

		//msg("Error: no matching devices\n");
	}
	hid_exit();*/
	//XAM: MIDI bidez Ledak kontolatzeko saiakera 
	int result = -1;
	result = midi_open_output("IX10");//IX10

	LED_event(SM_MIDI_CONTROLLER , 0, 0, 0);//All off
	

#ifdef _DEBUG
	fprintf(stdout, "MIDI_opened!: %02x\n", result);
#endif
	//XAM: result = 0 if opened correctly
	//if (result == 0) {////hau hemen in partez berezko lekun in funtzio bat ad_hoc ta horrei dettu
		//global.instrument_type = INSTRUMENT_TYPE_MIDI;
		//strncpy(global.instrument_path, "IX10", sizeof(global.instrument_path));
	//}

	/*handle = hid_open(0x0C45, 0x5004, NULL);
	if (!handle) {
		//printf("Unable to open device\n");
		hid_exit();
	} else {
		buf[0] = 0x0;//DUMMY
		buf[1] = 0x08;//LEDS OFF
		buf[2] = 0x00;//
		res = hid_write(handle, buf, 32);
		hid_close(handle);
	}
    hid_exit();*/

  if (song_end) {
    song_stop_record();
    song_stop_playback();

    song_timer = 0;
    song_clock = 0;
    song_auto_pedal_timer = 0;
    play_position = song_event_buffer;

    // clear current setting
    config_set_setting_group_count(1);
    config_clear_key_setting();

    song_update(0);
  }
}

// stop playback
void song_stop_playback() {
  thread_lock lock(song_lock);

  song_reset_event();
  play_position = NULL;
}

// is recoding
bool song_is_playing() {
  thread_lock lock(song_lock);

  return play_position != NULL;
}


// get record length
int song_get_length() {
  thread_lock lock(song_lock);

  if (song_end && song_end > song_event_buffer)
    return (int)song_end[-1].time;

  return 0;
}

// allow save
bool song_allow_save() {
  thread_lock lock(song_lock);
  return song_end != NULL && !song_info.write_protected && !song_is_recording();
}

// allow input
bool song_allow_input() {
  thread_lock lock(song_lock);
  return play_position == NULL;
}

// song has data
bool song_is_empty() {
  thread_lock lock(song_lock);
  return song_end == NULL;
}


// get time
int song_get_time() {
  thread_lock lock(song_lock);
  return (int)song_timer;
}

// get clock
double song_get_clock() {
  thread_lock lock(song_lock);
  return song_clock;
}

// song get play speed
double song_get_play_speed() {
  return song_play_speed;
}

// song get play speed
void song_set_play_speed(double speed) {
  song_play_speed = speed;
  if (song_play_speed < 0)
    song_play_speed = 0;
}

// update
void song_update(double time_elapsed) {
  thread_lock lock(song_lock);

#ifdef _DEBUG
  if (GetAsyncKeyState(VK_ESCAPE))
    return;
#endif

  // adjust playing speed
  if (song_is_playing())
    time_elapsed *= song_play_speed;

  if (song_is_playing() || song_is_recording())
    song_timer += time_elapsed;

  // playback
  while (play_position && song_end) {
    if (play_position->time <= song_timer) {
      // send event to keyboard
      song_send_event(play_position->a, play_position->b, play_position->c, play_position->d);

      if (play_position) {
        if (++play_position >= song_end) {
          song_stop_playback();
		  LED_event(SM_MIDI_CONTROLLER, 0, 0, 0);//XAM all LEDS OFF
        }
      }
    } else break;
  }

  // adjust clock
  song_clock += time_elapsed;

  // update keyboard
  keyboard_update(time_elapsed);

  // update controllers
  sync_event_update(time_elapsed);
  delay_event_update(time_elapsed);

  // update smooth pitch
  pitch_update(time_elapsed);
}

song_info_t* song_get_info() {
  return &song_info;
}

void song_trigger_sync(uint flags) {
  thread_lock lock(song_lock);
  sync_trigger |= flags;
}

// -----------------------------------------------------------------------------------------
// load and save functions
// -----------------------------------------------------------------------------------------

static void read(void *buff, int size, FILE *fp) {
  int ret = fread(buff, 1, size, fp);
  if (ret != size)
    throw -1;
}

static void write(const void *buff, int size, FILE *fp) {
  int ret = fwrite(buff, 1, size, fp);
  if (ret != size)
    throw -1;
}

static void read_string(char *buff, size_t buff_size, FILE *fp) {
  uint count = 0;

  read(&count, sizeof(count), fp);

  if (count < buff_size) {
    read(buff, count, fp);
    buff[count] = 0;
  } else   {
    throw -1;
  }
}

static void write_string(const char *buff, FILE *fp) {
  uint count = strlen(buff);

  write(&count, sizeof(count), fp);
  write(buff, count, fp);
}

static void read_idp_string(char *buff, size_t buff_size, FILE *fp) {
  byte count = 0;
  read(&count, sizeof(count), fp);

  if (count < buff_size) {
    read(buff, buff_size, fp);
    buff[count] = 0;
  } else   {
    throw -1;
  }
}


// close
void song_close() {
  thread_lock lock(song_lock);

  song_stop_record();
  song_stop_playback();
  song_end = NULL;
}


// check compatibility
static void check_compatibility() {
  // upgrade from 1.7 to 1.8
  if (song_info.version <= 0x01070000) {
    song_event_t *e;

    // foreach event
    for (e = song_event_buffer; e < song_end; e++) {
      byte a = e->a;
      byte b = e->b;
      byte c = e->c;
      byte d = e->d;

      if (a == SM_SYSTEM) {
        if (b == SMS_KEY_LABEL) {
          e += (d + 3) / 4;
        }
        else if (b == SMS_KEY_COLOR) {
          e ++;
        }
      }

      else if (a == SM_AUTO_PEDAL_OBSOLETE) {
        if (b != 0 || c != 0)
          song_info.compatibility = false;
      }

      else if (a == SM_DELAY_KEYUP_OBSOLETE) {
        if (c != 0 || d != 0)
          song_info.compatibility = false;
      }
      else
      {
        switch (a & 0xf0) {
        case SM_MIDI_NOTEON:
          e->a = SM_NOTE_ON;
          e->b = a & 0x0f;
          e->c = b;
          e->d = c;
          break;

        case SM_MIDI_NOTEOFF:
          e->a = SM_NOTE_OFF;
          e->b = a & 0x0f;
          e->c = b;
          e->d = 0;
          break;

        case SM_MIDI_PROGRAM:
          e->a = SM_PROGRAM;
          e->b = a & 0x0f;
          e->c = c;
          e->d = b;
          break;

        case SM_MIDI_CONTROLLER:
          if (b == 64) {
            e->a = SM_SUSTAIN;
            e->b = a & 0x0f;
            e->c = d;
            e->d = c;

            // flip needs a value since 1.8
            if (d == SM_VALUE_FLIP) {
              e->d = 127;
            }
          }
          break;
        }
      }
    }
  }

}

// open song
int song_open(const char *filename) {
  thread_lock lock(song_lock);

  song_close();

  FILE *fp = fopen(filename, "rb");

  try {
    if (!fp)
      throw -1;

    char magic[sizeof("SoinuTxikiBirtualaSong")];
    char instrument[256];

    // magic
    read(magic, sizeof("SoinuTxikiBirtualaSong"), fp);
    if (memcmp(magic, "SoinuTxikiBirtualaSong", sizeof("SoinuTxikiBirtualaSong")) != 0)
      throw -1;

    // start record
    song_init_record();

    // version
    read(&song_info.version, sizeof(song_info.version), fp);
    if (song_info.version > APP_VERSION)
      throw -2;

    // song info
    read_string(song_info.title, sizeof(song_info.title), fp);
    read_string(song_info.author, sizeof(song_info.author), fp);
    read_string(song_info.comment, sizeof(song_info.comment), fp);

    // instrument
    read_string(instrument, sizeof(instrument), fp);


    // read events
    uint temp_size;
    read(&temp_size, sizeof(temp_size), fp);
    byte *temp_buffer = new byte[temp_size];

    read(temp_buffer, temp_size, fp);

    uint data_size = sizeof(song_event_buffer);
    uncompress((byte *)song_event_buffer, (uLongf *)&data_size, (Bytef *)temp_buffer, temp_size);

    song_end = record_position = song_event_buffer + data_size / sizeof(song_event_t);

    record_position = NULL;

    delete[] temp_buffer;
    fclose(fp);

    // mark song protected
    song_info.write_protected = true;

    return 0;
  } catch (int err) {
    song_end = NULL;
    if (fp) fclose(fp);
    return err;
  }
}


// save song
int song_save(const char *filename) {
  thread_lock lock(song_lock);

  song_stop_record();

  if (song_end) {
    FILE *fp = fopen(filename, "wb");
    if (!fp)
      return -1;
    try {
      // magic
      write("SoinuTxikiBirtualaSong", sizeof("SoinuTxikiBirtualaSong"), fp);

      // version
      song_info.version = APP_VERSION;
      write(&song_info.version, sizeof(song_info.version), fp);

      // song info
      write_string(song_info.title, fp);
      write_string(song_info.author, fp);
      write_string(song_info.comment, fp);

      // instrument
      {
        char buff[256];
        strncpy(buff, PathFindFileName(config_get_instrument_path()), sizeof(buff));
        PathRenameExtension(buff, "");
        write_string(buff, fp);
      }

      // write events
      uint temp_size = sizeof(song_event_buffer);
      byte *temp_buffer = new byte[temp_size];

      compress((Bytef *)temp_buffer, (uLongf *)&temp_size, (byte *)song_event_buffer, (song_end - song_event_buffer) * sizeof(song_event_t));

      write(&temp_size, sizeof(temp_size), fp);
      write(temp_buffer, temp_size, fp);

      delete[] temp_buffer;

      fclose(fp);
      return 0;
    } catch (int err) {
      fclose(fp);
      return err;
    }
  }
  return -1;
}