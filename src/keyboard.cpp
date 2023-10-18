#include "pch.h"

#include "keyboard.h"
#include "vst/aeffectx.h"
#include "display.h"
#include "config.h"
#include "song.h"
#include "export.h"

#include <map>

// auto generated keyup events
static std::multimap<byte, key_bind_t> key_up_map;

// keyboard status
byte keyboard_status[256] = { 0 }; //made it global
byte notes_status[256] = { 0 }; //stores each note number of times played

// keyboard lock
static thread_lock_t keyboard_lock;

// -----------------------------------------------------------------------------------------
// accessibility shortcut keys
// -----------------------------------------------------------------------------------------
static void AllowAccessibilityShortcutKeys(bool bAllowKeys, bool bInitialize) {
  static STICKYKEYS g_StartupStickyKeys = {sizeof(STICKYKEYS), 0};
  static TOGGLEKEYS g_StartupToggleKeys = {sizeof(TOGGLEKEYS), 0};
  static FILTERKEYS g_StartupFilterKeys = {sizeof(FILTERKEYS), 0};

  if (bInitialize) {
    SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &g_StartupStickyKeys, 0);
    SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(TOGGLEKEYS), &g_StartupToggleKeys, 0);
    SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &g_StartupFilterKeys, 0);
  }

  if (bAllowKeys) {
    // Restore StickyKeys/etc to original state and enable Windows key
    STICKYKEYS sk = g_StartupStickyKeys;
    TOGGLEKEYS tk = g_StartupToggleKeys;
    FILTERKEYS fk = g_StartupFilterKeys;

    SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &g_StartupStickyKeys, 0);
    SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &g_StartupToggleKeys, 0);
    SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &g_StartupFilterKeys, 0);
  } else {
    // Disable StickyKeys/etc shortcuts but if the accessibility feature is on,
    // then leave the settings alone as its probably being usefully used

    STICKYKEYS skOff = g_StartupStickyKeys;
    if ((skOff.dwFlags & SKF_STICKYKEYSON) == 0) {
      // Disable the hotkey and the confirmation
      skOff.dwFlags &= ~SKF_HOTKEYACTIVE;
      skOff.dwFlags &= ~SKF_CONFIRMHOTKEY;

      SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &skOff, 0);
    }

    TOGGLEKEYS tkOff = g_StartupToggleKeys;
    if ((tkOff.dwFlags & TKF_TOGGLEKEYSON) == 0) {
      // Disable the hotkey and the confirmation
      tkOff.dwFlags &= ~TKF_HOTKEYACTIVE;
      tkOff.dwFlags &= ~TKF_CONFIRMHOTKEY;

      SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &tkOff, 0);
    }

    FILTERKEYS fkOff = g_StartupFilterKeys;
    if ((fkOff.dwFlags & FKF_FILTERKEYSON) == 0) {
      // Disable the hotkey and the confirmation
      fkOff.dwFlags &= ~FKF_HOTKEYACTIVE;
      fkOff.dwFlags &= ~FKF_CONFIRMHOTKEY;

      SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &fkOff, 0);
    }
  }
}

// -----------------------------------------------------------------------------------------
// disable windows key (works only in dll)
// -----------------------------------------------------------------------------------------
static HHOOK keyboard_hook = NULL;
static byte keydown_status[256] = {0}; //ustez honek gordetzeu aber zapalduta daon o ez

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {//ustet honek saltatzeula tekladutik zeoze zapaltzea bakoitzen
  if (nCode == HC_ACTION && !export_rendering()) {
    KBDLLHOOKSTRUCT *p = (KBDLLHOOKSTRUCT *)lParam;

    uint extent = p->flags & 1;
    uint code = (p->scanCode & 0xFF);
    uint keydown = !(p->flags & LLKHF_UP);
	uint ximor = 0;

    // special case for scan code
    switch (code) {
     case 0x45:  extent = !extent;   break;
     case 0x36:  extent = 0;
    }

    // translate extented key.
    if (extent) code |= 0x80;

    // trigger event only when key changed
    if (keydown_status[code] != keydown) { //zapalduta ez bazeon ta oain bai:
		keydown_status[code] = keydown; //egoera berrie gorde
		if ((code == 57 && keydown_status[184] == 1) || \
		    (code == 184 && keydown_status[57] == 1))	 //XAM espaziotako bat izan bada ta bestea zapalduta badao ignoratu
			ximor = 1;
		else {//beste eozin kasutan in beharrekoa in
			song_send_event(SM_SYSTEM, SMS_KEY_EVENT, code, keydown, true); //keyboard_send_event 
		}
	  	//in beharrekoa in da geo (espazio bazan ya grupoa aldatuta ber luke):
	  if ((code == 57 && keydown_status[184] == 1) || \
		  (code == 184 && keydown_status[57] == 1))
		  ximor = 1;//XAM espazio izan bada ta beste zapalduta badao, ignoratu
	  else if (code == 57 || code == 184) {//espaziotako bat izan bada ta bestea zapaldu gabe:
			//zapalduta zeuden eskubiko botoiai off eman ta gero berriz on
			for ( uint i = 0x02; i <= 0x0e; i++) {//2. fila: 1-etik backaspacea
				if (keydown_status[i] == 1){//zapalduta bazeon
				  song_send_event(SM_SYSTEM, SMS_KEY_EVENT, i, 0, true);//askatzeko esan
				  song_send_event(SM_SYSTEM, SMS_KEY_EVENT, i, 1, true);//berriz zapaltzeko esan
				}
			}
			for ( uint i = 0x3b; i <= 0x44; i++) {//1. fila: F1-F10
				if (keydown_status[i] == 1){//zapalduta bazeon
				  song_send_event(SM_SYSTEM, SMS_KEY_EVENT, i, 0, true);//askatzeko esan
				  song_send_event(SM_SYSTEM, SMS_KEY_EVENT, i, 1, true);//berriz zapaltzeko esan
				}
			}
		
			for ( uint i = 0x57; i <= 0x58; i++) {//1. fila: F11-F12
				if (keydown_status[i] == 1){//zapalduta bazeon
				  song_send_event(SM_SYSTEM, SMS_KEY_EVENT, i, 0, true);//askatzeko esan
				  song_send_event(SM_SYSTEM, SMS_KEY_EVENT, i, 1, true);//berriz zapaltzeko esan
				}
			}
			//TODO: 3. filan re berdine in
			//BAJUKIN RE BAI
			for ( uint i = 0x21; i <= 0x26; i++) {//4. fila: F-L
				if (keydown_status[i] == 1){//zapalduta bazeon
				  song_send_event(SM_SYSTEM, SMS_KEY_EVENT, i, 0, true);//askatzeko esan
				  song_send_event(SM_SYSTEM, SMS_KEY_EVENT, i, 1, true);//berriz zapaltzeko esan
				}
			}
			for ( uint i = 0x2e; i <= 0x33; i++) {//5. fila: c-,
				if (keydown_status[i] == 1){//zapalduta bazeon
				  song_send_event(SM_SYSTEM, SMS_KEY_EVENT, i, 0, true);//askatzeko esan
				  song_send_event(SM_SYSTEM, SMS_KEY_EVENT, i, 1, true);//berriz zapaltzeko esan
				}
			}
		}
    }

    if (!config_get_enable_hotkey() || p->vkCode == VK_LWIN || p->vkCode == VK_RWIN)
      return 1;
  }

  return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// initialize keyboard
int keyboard_init() {
  thread_lock lock(keyboard_lock);

  // disable accessibility key
  AllowAccessibilityShortcutKeys(false, true);

  return 0;
}

// shutdown keyboard system
void keyboard_shutdown() {
  thread_lock lock(keyboard_lock);

  // enable accessibility key
  AllowAccessibilityShortcutKeys(true, false);

  // disable keyboard hook
  keyboard_enable(false);
}


// enable or disable keyboard
void keyboard_enable(bool enable) {
  thread_lock lock(keyboard_lock);

  if (enable) {
    // install keyboard hook
    if (keyboard_hook == NULL) {
      keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL,  LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    }
  }
  else {
    // uninstall keyboard hook
    if (keyboard_hook) {
      UnhookWindowsHookEx(keyboard_hook);
      keyboard_hook = NULL;
    }

    keyboard_reset();
  }
}

// reset keyboard
void keyboard_reset() {
  for (int i = 0; i < ARRAY_COUNT(keyboard_status); i++) {
	notes_status[i] = 0;
    if (keyboard_status[i] || keydown_status[i]) {
      // send keyup event
      keyboard_send_event(i, 0);
      keyboard_status[i] = 0;
      keydown_status[i] = 0;
    }
  }
}

// enum keymap
void keyboard_enum_keymap(keymap_enum_callback &callback) {
  char path[256];
  char buff[256];
  config_get_media_path(path, sizeof(path), "");

  _snprintf(buff, sizeof(buff), "%s\\*.map", path);

  WIN32_FIND_DATAA data;
  HANDLE finddata = FindFirstFileA(buff, &data);

  if (finddata != INVALID_HANDLE_VALUE) {
    do {
      _snprintf(buff, sizeof(buff), "%s\\%s", path, data.cFileName);
      callback(buff);
    } while (FindNextFileA(finddata, &data));
  }

  FindClose(finddata);
}

static inline byte clamp_value(int data, byte min = 0, byte max = 127) {
  if (data < min) return min;
  if (data > max) return max;
  return data;
}

// keyboard event
void keyboard_send_event(int code, int keydown) {
  thread_lock lock(keyboard_lock);

  // keep state
  if (code < ARRAY_COUNT(keyboard_status))
    keyboard_status[code] = keydown;

  // keydown event
  if (keydown) {
    key_bind_t temp[256];

    // get key bind
    int num_keydown = config_bind_get_keydown(code, temp, ARRAYSIZE(temp));

#ifdef _DEBUG
	fprintf(stdout, "num_keydown: %u \n", code);
#endif
    for (int i = 0; i < num_keydown; i++) {
      key_bind_t down = temp[i];

      // translate to real midi event
      song_translate_note(down.a, down.b, down.c, down.d);
	  
	//notes_status[down.b] = 0;//XAM store how many times that note is being played

      // note on
      if ((down.a & 0xf0) == SM_MIDI_NOTEON) {

        // auto generate note off event
        key_bind_t up;

        up.a = SM_MIDI_NOTEOFF | (down.a & 0x0f);
        up.b = down.b;
        up.c = down.c;
        up.d = down.d;
        key_up_map.insert(std::pair<byte, key_bind_t>(code, up));

		  // send event to song
		  if (midi_get_note_status(down.a & 0xf, down.b & 0x7f)) //if not already playing that note on that channel
			midi_inc_note_status(down.a & 0xf, down.b & 0x7f);
		  else
			song_output_event(down.a, down.b, down.c, down.d);
      } else {//note midi note on -> output always
        song_output_event(down.a, down.b, down.c, down.d);
	  }

    }

    // add key up events.
    int num_keyup = config_bind_get_keyup(code, temp, ARRAYSIZE(temp));
    for (int i = 0; i < num_keyup; i++) {
      key_bind_t up = temp[i];

      // translate to real midi event
      song_translate_note(up.a, up.b, up.c, up.d);

      // insert keyup event to keyup map
      key_up_map.insert(std::pair<byte, key_bind_t>(code, up));

      // generate a keyup event
      if ((up.a & 0xf0) == SM_MIDI_NOTEON) {
        up.a = SM_MIDI_NOTEOFF | (up.a & 0x0f);
        key_up_map.insert(std::pair<byte, key_bind_t>(code, up));
      }
    }

    // trigger keyboard sync
    song_trigger_sync(SONG_SYNC_FLAG_KEYDOWN);
  }
  // keyup event
  else {
    auto it = key_up_map.find(code);
    while (it != key_up_map.end() && it->first == code) {
      key_bind_t &up = it->second;

      if (up.a) {
        if ((up.a & 0xf0) == SM_MIDI_NOTEOFF) {
          if (midi_get_note_status(up.a & 0xf, up.b & 0x7f) == 1) //if only one instace is pressed, set it off
            song_output_event(up.a, up.b, up.c, up.d);
	  	  else //decrease number of presses
		    midi_dec_note_status(up.a & 0xf, up.b & 0x7f);
		} else
			song_output_event(up.a, up.b, up.c, up.d);
      }

      ++it;
    }
    key_up_map.erase(code);
  }
}

// keyboard event
void keyboard_update(double time_elapsed) {

}
// get keyboard status
byte keyboard_get_status(byte code) {
  return keyboard_status[code];
}