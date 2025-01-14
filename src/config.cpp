#include "pch.h"
#include <dinput.h>
#include <Shlwapi.h>

#include "config.h"
#include "stdio.h"
#include "keyboard.h"
#include "output_asio.h"
#include "output_dsound.h"
#include "output_wasapi.h"
#include "synthesizer_vst.h"
#include "display.h"
#include "song.h"
#include "gui.h"
#include "language.h"
#include "utilities.h"

#include "../res/resource.h"

#include <string>
#include <map>

// -----------------------------------------------------------------------------------------
// config defines
// -----------------------------------------------------------------------------------------
#define BIND_TYPE_UNKNOWN     0
#define BIND_TYPE_KEYDOWN     1
#define BIND_TYPE_KEYUP       2
#define BIND_TYPE_LABEL       3
#define BIND_TYPE_COLOR       4

// -----------------------------------------------------------------------------------------
// config constants
// -----------------------------------------------------------------------------------------
struct name_t {
  const char *name;
  uint value;
  uint lang;
};

static name_t key_names[] = {
  { "Esc",            DIK_ESCAPE },
  { "F1",             DIK_F1 },
  { "F2",             DIK_F2 },
  { "F3",             DIK_F3 },
  { "F4",             DIK_F4 },
  { "F5",             DIK_F5 },
  { "F6",             DIK_F6 },
  { "F7",             DIK_F7 },
  { "F8",             DIK_F8 },
  { "F9",             DIK_F9 },
  { "F10",            DIK_F10 },
  { "F11",            DIK_F11 },
  { "F12",            DIK_F12 },
  { "~",              DIK_GRAVE },
  { "`",              DIK_GRAVE },
  { "1",              DIK_1 },
  { "2",              DIK_2 },
  { "3",              DIK_3 },
  { "4",              DIK_4 },
  { "5",              DIK_5 },
  { "6",              DIK_6 },
  { "7",              DIK_7 },
  { "8",              DIK_8 },
  { "9",              DIK_9 },
  { "0",              DIK_0 },
  { "-",              DIK_MINUS },
  { "=",              DIK_EQUALS },
  { "Backspace",      DIK_BACK },
  { "Back",           DIK_BACK },
  { "Tab",            DIK_TAB },
  { "Q",              DIK_Q },
  { "W",              DIK_W },
  { "E",              DIK_E },
  { "R",              DIK_R },
  { "T",              DIK_T },
  { "Y",              DIK_Y },
  { "U",              DIK_U },
  { "I",              DIK_I },
  { "O",              DIK_O },
  { "P",              DIK_P },
  { "{",              DIK_LBRACKET },
  { "[",              DIK_LBRACKET },
  { "}",              DIK_RBRACKET },
  { "]",              DIK_RBRACKET },
  { "|",              DIK_BACKSLASH },
  { "\\",             DIK_BACKSLASH },
  { "CapsLock",       DIK_CAPITAL },
  { "Caps",           DIK_CAPITAL },
  { "A",              DIK_A },
  { "S",              DIK_S },
  { "D",              DIK_D },
  { "F",              DIK_F },
  { "G",              DIK_G },
  { "H",              DIK_H },
  { "J",              DIK_J },
  { "K",              DIK_K },
  { "L",              DIK_L },
  { ":",              DIK_SEMICOLON },
  { ";",              DIK_SEMICOLON },
  { "\"",             DIK_APOSTROPHE },
  { "'",              DIK_APOSTROPHE },
  { "Enter",          DIK_RETURN },
  { "Shift",          DIK_LSHIFT },
  { "Z",              DIK_Z },
  { "X",              DIK_X },
  { "C",              DIK_C },
  { "V",              DIK_V },
  { "B",              DIK_B },
  { "N",              DIK_N },
  { "M",              DIK_M },
  { "<",              DIK_COMMA },
  { ",",              DIK_COMMA },
  { ">",              DIK_PERIOD },
  { ".",              DIK_PERIOD },
  { "?",              DIK_SLASH },
  { "/",              DIK_SLASH },
  { "RShift",         DIK_RSHIFT },
  { "Ctrl",           DIK_LCONTROL },
  { "Win",            DIK_LWIN },
  { "Alt",            DIK_LMENU },
  { "Space",          DIK_SPACE },
  { "RAlt",           DIK_RMENU },
  { "Apps",           DIK_APPS },
  { "RWin",           DIK_RWIN },
  { "RCtrl",          DIK_RCONTROL },

  { "Num0",           DIK_NUMPAD0 },
  { "Num.",           DIK_NUMPADPERIOD },
  { "NumEnter",       DIK_NUMPADENTER },
  { "Num1",           DIK_NUMPAD1 },
  { "Num2",           DIK_NUMPAD2 },
  { "Num3",           DIK_NUMPAD3 },
  { "Num4",           DIK_NUMPAD4 },
  { "Num5",           DIK_NUMPAD5 },
  { "Num6",           DIK_NUMPAD6 },
  { "Num7",           DIK_NUMPAD7 },
  { "Num8",           DIK_NUMPAD8 },
  { "Num9",           DIK_NUMPAD9 },
  { "Num+",           DIK_NUMPADPLUS },
  { "NumLock",        DIK_NUMLOCK },
  { "Num/",           DIK_NUMPADSLASH },
  { "Num*",           DIK_NUMPADSTAR },
  { "Num-",           DIK_NUMPADMINUS },

  { "Insert",         DIK_INSERT },
  { "Home",           DIK_HOME },
  { "PgUp",           DIK_PGUP },
  { "PageUp",         DIK_PGUP },
  { "Delete",         DIK_DELETE },
  { "End",            DIK_END },
  { "PgDn",           DIK_PGDN },
  { "PgDown",         DIK_PGDN },

  { "Up",             DIK_UP },
  { "Left",           DIK_LEFT },
  { "Down",           DIK_DOWN },
  { "Right",          DIK_RIGHT },

  { "PrintScreen",    DIK_SYSRQ },
  { "SysRq",          DIK_SYSRQ },
  { "ScrollLock",     DIK_SCROLL },
  { "Pause",          DIK_PAUSE },
  { "Break",          DIK_PAUSE },
};

static name_t bind_names[] = {
  { "Keydown",            BIND_TYPE_KEYDOWN },
  { "Keyup",              BIND_TYPE_KEYUP },
  { "Label",              BIND_TYPE_LABEL },
  { "Color",              BIND_TYPE_COLOR },

  { "Zapaldu",            BIND_TYPE_KEYDOWN,     FP_LANG_EUS },
  { "Askatu",            BIND_TYPE_KEYUP,       FP_LANG_EUS },
  { "Textua",                BIND_TYPE_LABEL,       FP_LANG_EUS },
  { "Kolorea",                BIND_TYPE_COLOR,       FP_LANG_EUS },

  // for compatibility
  { "Key",                BIND_TYPE_KEYDOWN },
};

static name_t action_names[] = {//keymapen jarri leizken hitzek
  { "KeySignature",       SM_KEY_SIGNATURE },
  { "Octave",             SM_OCTAVE },
  { "Velocity",           SM_VELOCITY },
  { "Channel",            SM_CHANNEL },
  { "Transpose",          SM_TRANSPOSE },
  { "FollowKey",          SM_FOLLOW_KEY },
  { "Volume",             SM_VOLUME },
  { "Play",               SM_PLAY },
  { "Record",             SM_RECORD },
  { "Stop",               SM_STOP },
  { "Group",              SM_SETTING_GROUP },
  { "GroupCount",         SM_SETTING_GROUP_COUNT },
  { "Note",               SM_NOTE_ON },
  { "NoteOn",             SM_NOTE_ON },
  { "NoteOff",            SM_NOTE_OFF },
  { "NotePressure",       SM_NOTE_PRESSURE },
  { "Pressure",           SM_PRESSURE },
  { "Pitch",              SM_PITCH },
  { "Program",            SM_PROGRAM },
  { "BankMSB",            SM_BANK_MSB },
  { "BankLSB",            SM_BANK_LSB },
  { "Sustain",            SM_SUSTAIN },
  { "Modulation",         SM_MODULATION },

  { "Klabea",               SM_KEY_SIGNATURE,       FP_LANG_EUS },
  { "Oktaba",               SM_OCTAVE,              FP_LANG_EUS },
  { "Abiadura",               SM_VELOCITY,            FP_LANG_EUS },
  { "Kanala",               SM_CHANNEL,             FP_LANG_EUS },
  { "Trasposatu",               SM_TRANSPOSE,           FP_LANG_EUS },
  { "JarraituGiltza",           SM_FOLLOW_KEY,          FP_LANG_EUS },
  { "Bolumena",             SM_VOLUME,              FP_LANG_EUS },
  { "Play",               SM_PLAY,                FP_LANG_EUS },
  { "REC",               SM_RECORD,              FP_LANG_EUS },
  { "STOP",               SM_STOP,                FP_LANG_EUS },
  { "Taldea",               SM_SETTING_GROUP,       FP_LANG_EUS },
  { "TaldeKopurua",           SM_SETTING_GROUP_COUNT, FP_LANG_EUS },
  { "NotaON",               SM_NOTE_ON,             FP_LANG_EUS },
  { "NotaOFF",               SM_NOTE_OFF,            FP_LANG_EUS },
  { "NotarenPresioa",           SM_NOTE_PRESSURE,       FP_LANG_EUS },
  { "Presioa",           SM_PRESSURE,            FP_LANG_EUS },
  { "Tonua",               SM_PITCH,               FP_LANG_EUS },
  { "Programa",               SM_PROGRAM,             FP_LANG_EUS },
  { "BankuaMSB",            SM_BANK_MSB,            FP_LANG_EUS },
  { "BankuaLSB",            SM_BANK_LSB,            FP_LANG_EUS },
  { "Sustain",               SM_SUSTAIN,             FP_LANG_EUS },
  { "Modulazioa",               SM_MODULATION,          FP_LANG_EUS },

  // pervious names
  { "Octshift",           SM_OCTAVE },
  { "KeyboardOctshift",   SM_OCTAVE },
  { "KeyboardVeolcity",   SM_VELOCITY },
  { "KeyboardChannel",    SM_CHANNEL },
  { "Controller",         SM_CONTROLLER_DEPRECATED },

  // for parse midi message
  { "MIDI",               0xff },
};

static name_t note_names[] = {
  { "C-1",                0 },
  { "C#-1",               1 },
  { "D-1",                2 },
  { "D#-1",               3 },
  { "E-1",                4 },
  { "F-1",                5 },
  { "F#-1",               6 },
  { "G-1",                7 },
  { "G#-1",               8 },
  { "A-1",                9 },
  { "A#-1",               10 },
  { "B-1",                11 },
  { "C0",                 12 },
  { "C#0",                13 },
  { "D0",                 14 },
  { "D#0",                15 },
  { "E0",                 16 },
  { "F0",                 17 },
  { "F#0",                18 },
  { "G0",                 19 },
  { "G#0",                20 },
  { "A0",                 21 },
  { "A#0",                22 },
  { "B0",                 23 },
  { "C1",                 24 },
  { "C#1",                25 },
  { "D1",                 26 },
  { "D#1",                27 },
  { "E1",                 28 },
  { "F1",                 29 },
  { "F#1",                30 },
  { "G1",                 31 },
  { "G#1",                32 },
  { "A1",                 33 },
  { "A#1",                34 },
  { "B1",                 35 },
  { "C2",                 36 },
  { "C#2",                37 },
  { "D2",                 38 },
  { "D#2",                39 },
  { "E2",                 40 },
  { "F2",                 41 },
  { "F#2",                42 },
  { "G2",                 43 },
  { "G#2",                44 },
  { "A2",                 45 },
  { "A#2",                46 },
  { "B2",                 47 },
  { "C3",                 48 },
  { "C#3",                49 },
  { "D3",                 50 },
  { "D#3",                51 },
  { "E3",                 52 },
  { "F3",                 53 },
  { "F#3",                54 },
  { "G3",                 55 },
  { "G#3",                56 },
  { "A3",                 57 },
  { "A#3",                58 },
  { "B3",                 59 },
  { "C4",                 60 },
  { "C#4",                61 },
  { "D4",                 62 },
  { "D#4",                63 },
  { "E4",                 64 },
  { "F4",                 65 },
  { "F#4",                66 },
  { "G4",                 67 },
  { "G#4",                68 },
  { "A4",                 69 },
  { "A#4",                70 },
  { "B4",                 71 },
  { "C5",                 72 },
  { "C#5",                73 },
  { "D5",                 74 },
  { "D#5",                75 },
  { "E5",                 76 },
  { "F5",                 77 },
  { "F#5",                78 },
  { "G5",                 79 },
  { "G#5",                80 },
  { "A5",                 81 },
  { "A#5",                82 },
  { "B5",                 83 },
  { "C6",                 84 },
  { "C#6",                85 },
  { "D6",                 86 },
  { "D#6",                87 },
  { "E6",                 88 },
  { "F6",                 89 },
  { "F#6",                90 },
  { "G6",                 91 },
  { "G#6",                92 },
  { "A6",                 93 },
  { "A#6",                94 },
  { "B6",                 95 },
  { "C7",                 96 },
  { "C#7",                97 },
  { "D7",                 98 },
  { "D#7",                99 },
  { "E7",                 100 },
  { "F7",                 101 },
  { "F#7",                102 },
  { "G7",                 103 },
  { "G#7",                104 },
  { "A7",                 105 },
  { "A#7",                106 },
  { "B7",                 107 },
  { "C8",                 108 },
  { "C#8",                109 },
  { "D8",                 110 },
  { "D#8",                111 },
  { "E8",                 112 },
  { "F8",                 113 },
  { "F#8",                114 },
  { "G8",                 115 },
  { "G#8",                116 },
  { "A8",                 117 },
  { "A#8",                118 },
  { "B8",                 119 },
  { "C9",                 120 },
  { "C#9",                121 },
  { "D9",                 122 },
  { "D#9",                123 },
  { "E9",                 124 },
  { "F9",                 125 },
  { "F#9",                126 },
  { "G9",                 127 },
  { "A9",                 128 },
  { "A#9",                129 },
  { "B9",                 130 },
  { "C10",                131 },
  { "C#10",               132 },
  { "D10",                133 },
  { "D#10",               134 },
  { "E10",                135 },
  { "F10",                136 },
  { "F#10",               137 },
  { "G10",                138 },
  { "G#10",               139 },
  { "A10",                140 },
};

static name_t controller_names[] = {//TODO investigate
  { "BankSelect",         0x0 },
  { "Modulation",         0x1 },
  { "BreathControl",      0x2 },
  { "FootPedal",          0x4 },
  { "Portamento",         0x5 },
  { "DataEntry",          0x6 },
  { "MainVolume",         0x7 },
  { "Balance",            0x8 },
  { "Pan",                0xa },
  { "Expression",         0xb },
  { "EffectSelector1",    0xc },
  { "EffectSelector2",    0xd },

  { "GeneralPurpose1",    0x10 },
  { "GeneralPurpose2",    0x11 },
  { "GeneralPurpose3",    0x12 },
  { "GeneralPurpose4",    0x13 },

  { "SustainPedal",       0x40 },
  { "PortamentoPedal",    0x41 },
  { "SostenutoPedal",     0x42 },
  { "SoftPedal",          0x43 },
  { "LegatoPedal",        0x44 },
  { "Hold2",              0x45 },
  { "SoundController1",   0x46 },
  { "SoundController2",   0x47 },
  { "SoundController3",   0x48 },
  { "SoundController4",   0x49 },
  { "SoundController5",   0x4a },
  { "SoundController6",   0x4b },
  { "SoundController7",   0x4c },
  { "SoundController8",   0x4d },
  { "SoundController9",   0x4e },
  { "SoundController10",  0x4f },

  { "DataIncrement",      0x60 },
  { "DataDecrement",      0x61 },
  { "NRPNLSB",            0x62 },
  { "NRPNMSB",            0x63 },
  { "RPNLSB",             0x64 },
  { "RPNMSB",             0x65 },

  { "AllSoundsOff",       0x78 },
  { "ResetAllControllers", 0x79 },
  { "LocalControlOnOff",  0x7a },
  { "AllNotesOff",        0x7b },
  { "OmniModeOff",        0x7c },
  { "OmniModeOn",         0x7d },
  { "MonoModeOn",         0x7e },
  { "PokyModeOn",         0x7f },
};

static name_t value_action_names[] = {
  { "Set",            0x00 },
  { "Inc",            0x01 },
  { "Dec",            0x02 },
  { "Flip",           0x03 },
  { "Press",          0x04 },
  { "Set10",          0x0a },
  { "Set1",           0x0b },
  { "SyncSet",        0x10 },
  { "SyncInc",        0x11 },
  { "SyncDec",        0x12 },
  { "SyncFlip",       0x13 },
  { "SyncPress",      0x14 },

  { "Ezarri",         0x00,      FP_LANG_EUS },
  { "Handitu",           0x01,      FP_LANG_EUS },
  { "Txikitu",           0x02,      FP_LANG_EUS },
  { "Aldatu",           0x03,      FP_LANG_EUS },
  { "Zapaldu",           0x04,      FP_LANG_EUS },
  { "Ezarri10",       0x0a,      FP_LANG_EUS },
  { "Ezarri1",       0x0b,      FP_LANG_EUS },
  { "SyncEzarri",       0x10,      FP_LANG_EUS },
  { "SyncHanditu",       0x11,      FP_LANG_EUS },
  { "SyncTxikitu",       0x12,      FP_LANG_EUS },
  { "SyncAldatu",       0x13,      FP_LANG_EUS },
  { "SyncZapaldu",       0x14,      FP_LANG_EUS },
};

static name_t instrument_type_names[] = {
  { "None",           INSTRUMENT_TYPE_MIDI },
  { "VSTI",           INSTRUMENT_TYPE_VSTI },
};

static name_t output_type_names[] = {
  { "Auto",           OUTPUT_TYPE_AUTO },
  { "DirectSound",    OUTPUT_TYPE_DSOUND },
  { "Wasapi",         OUTPUT_TYPE_WASAPI },
  { "ASIO",           OUTPUT_TYPE_ASIO },
};

static name_t boolean_names[] = {
  { "disable",        0 },
  { "no",             0 },
  { "false",          0 },
  { "enable",         1 },
  { "yes",            1 },
  { "true",           1 },
};

static name_t channel_names[] = {
  { "In_0",      0x00 },
  { "In_1",      0x01 },
  { "In_2",      0x02 },
  { "In_3",      0x03 },
  { "In_4",      0x04 },
  { "In_5",      0x05 },
  { "In_6",      0x06 },
  { "In_7",      0x07 },
  { "In_8",      0x08 },
  { "In_9",      0x09 },
  { "In_10",     0x0a },
  { "In_11",     0x0b },
  { "In_12",     0x0c },
  { "In_13",     0x0d },
  { "In_14",     0x0e },
  { "In_15",     0x0f },

  { "Out_0",     0x10 },
  { "Out_1",     0x11 },
  { "Out_2",     0x12 },
  { "Out_3",     0x13 },
  { "Out_4",     0x14 },
  { "Out_5",     0x15 },
  { "Out_6",     0x16 },
  { "Out_7",     0x17 },
  { "Out_8",     0x18 },
  { "Out_9",     0x19 },
  { "Out_10",    0x1a },
  { "Out_11",    0x1b },
  { "Out_12",    0x1c },
  { "Out_13",    0x1d },
  { "Out_14",    0x1e },
  { "Out_15",    0x1f },

  { "Ch_L",      0x00 },
  { "Ch_R",      0x01 },
  { "Ch_0",      0x00 },
  { "Ch_1",      0x01 },
  { "Ch_2",      0x02 },
  { "Ch_3",      0x03 },
  { "Ch_4",      0x04 },
  { "Ch_5",      0x05 },
  { "Ch_6",      0x06 },
  { "Ch_7",      0x07 },
  { "Ch_8",      0x08 },
  { "Ch_9",      0x09 },
  { "Ch_10",     0x0a },
  { "Ch_11",     0x0b },
  { "Ch_12",     0x0c },
  { "Ch_13",     0x0d },
  { "Ch_14",     0x0e },
  { "Ch_15",     0x0f },

  { "In_0",     0x00,     FP_LANG_EUS },
  { "In_1",     0x01,     FP_LANG_EUS },
  { "In_2",     0x02,     FP_LANG_EUS },
  { "In_3",     0x03,     FP_LANG_EUS },
  { "In_4",     0x04,     FP_LANG_EUS },
  { "In_5",     0x05,     FP_LANG_EUS },
  { "In_6",     0x06,     FP_LANG_EUS },
  { "In_7",     0x07,     FP_LANG_EUS },
  { "In_8",     0x08,     FP_LANG_EUS },
  { "In_9",     0x09,     FP_LANG_EUS },
  { "In_10",    0x0a,     FP_LANG_EUS },
  { "In_11",    0x0b,     FP_LANG_EUS },
  { "In_12",    0x0c,     FP_LANG_EUS },
  { "In_13",    0x0d,     FP_LANG_EUS },
  { "In_14",    0x0e,     FP_LANG_EUS },
  { "In_15",    0x0f,     FP_LANG_EUS },

  { "Out_0",     0x10,     FP_LANG_EUS },
  { "Out_1",     0x11,     FP_LANG_EUS },
  { "Out_2",     0x12,     FP_LANG_EUS },
  { "Out_3",     0x13,     FP_LANG_EUS },
  { "Out_4",     0x14,     FP_LANG_EUS },
  { "Out_5",     0x15,     FP_LANG_EUS },
  { "Out_6",     0x16,     FP_LANG_EUS },
  { "Out_7",     0x17,     FP_LANG_EUS },
  { "Out_8",     0x18,     FP_LANG_EUS },
  { "Out_9",     0x19,     FP_LANG_EUS },
  { "Out_10",    0x1a,     FP_LANG_EUS },
  { "Out_11",    0x1b,     FP_LANG_EUS },
  { "Out_12",    0x1c,     FP_LANG_EUS },
  { "Out_13",    0x1d,     FP_LANG_EUS },
  { "Out_14",    0x1e,     FP_LANG_EUS },
  { "Out_15",    0x1f,     FP_LANG_EUS },
};

// -----------------------------------------------------------------------------------------
// configurations
// -----------------------------------------------------------------------------------------
struct key_label_t {
  char text[16];
  char always_0;
  uint color;
};

struct global_setting_t {
  uint instrument_type;
  char instrument_path[256];
  uint instrument_show_midi;
  uint instrument_show_vsti;

  uint output_type;
  uint output_type_current;
  char output_device[256];
  uint output_delay;
  char keymap[256];
  uint output_volume;

  uint enable_hotkey;
  uint enable_inactive;
  uint enable_resize;
  uint midi_transpose;
  uint fixed_doh;
  uint key_fade;
  byte gui_transparency;
  byte auto_color;
  byte preview_color;
  byte note_display;
  uint update_version;

  std::map<std::string, midi_input_config_t> midi_inputs;

  global_setting_t() {
    instrument_type = INSTRUMENT_TYPE_MIDI;
    instrument_path[0] = 0;
    instrument_show_midi = 1;
    instrument_show_vsti = 1;

    output_type = OUTPUT_TYPE_AUTO;
    output_type_current = OUTPUT_TYPE_AUTO;
    output_device[0] = 0;
    output_delay = 10;
    keymap[0] = 0;

    gui_transparency = 255;
    output_volume = 100;
    enable_hotkey = false;
    enable_inactive = false;
    enable_resize = true;
    midi_transpose = false;
    fixed_doh = false;
    auto_color = 2;
    preview_color = 0;
    note_display = 2;
    update_version = 0;

    key_fade = 0;
  }
};

struct setting_t {
  // keymap
  std::multimap<byte, key_bind_t> keydown_map;
  std::multimap<byte, key_bind_t> keyup_map;

  // key label
  key_label_t key_label[256];

  // key properties
  char key_octshift[16];
  char key_transpose[16];
  char key_velocity[16];
  char key_channel[16];
  char key_signature;
  char follow_key[16];
  char midi_program[16];
  char midi_controller[16][256];

  void clear() {
    key_signature = 0;

    keydown_map.clear();
    keyup_map.clear();

    for (int i = 0; i < 256; i++) {
      memset(key_label[i].text, 0, sizeof(key_label[i].text));
      key_label[i].color = 0;
    }

    for (int i = 0; i < 16; i++) {
      key_octshift[i] = 0;
      key_transpose[i] = 0;
      key_velocity[i] = 127;
      key_channel[i] = i;//In_x -> Midi ch_x
      follow_key[i] = 1;

      midi_program[i] = -1;

      for (int j = 0; j < 128; j++)
        midi_controller[i][j] = -1;
    }
  }
};

// settings
static global_setting_t global;
static setting_t settings[256];
uint current_setting = 0; //global
static uint setting_count = 1;

// song thread lock
static thread_lock_t config_lock;

// verison
static uint map_language = FP_LANG_ENGLISH;
static uint map_version = 0;
static const uint map_current_version = APP_VERSION;


int config_bind_get_keydown(byte code, key_bind_t *buff, int size) {
  thread_lock lock(config_lock);

  int result = 0;
  if (buff) {
    auto it = settings[current_setting].keydown_map.find(code);
    auto end = settings[current_setting].keydown_map.end();

    while (it != end && it->first == code) {
      if (result < size) {
        buff[result] = it->second;
        ++result;
      }
      ++it;
    }
  }
  return result;
}

int config_bind_get_keyup(byte code, key_bind_t *buff, int size) {
  thread_lock lock(config_lock);

  int result = 0;
  if (buff) {
    auto it = settings[current_setting].keyup_map.find(code);
    auto end = settings[current_setting].keyup_map.end();

    while (it != end && it->first == code) {
      if (result < size) {
        buff[result] = it->second;
        ++result;
      }
      ++it;
    }
  }
  return result;
}

void config_bind_clear_keydown(byte code) {
  thread_lock lock(config_lock);

  settings[current_setting].keydown_map.erase(code);
}

void config_bind_clear_keyup(byte code) {
  thread_lock lock(config_lock);

  settings[current_setting].keyup_map.erase(code);
}

void config_bind_add_keydown(byte code, key_bind_t bind) {
  thread_lock lock(config_lock);

  if (bind.a) {
    settings[current_setting].keydown_map.insert(std::pair<byte, key_bind_t>(code, bind));
  }
}

void config_bind_add_keyup(byte code, key_bind_t bind) {
  thread_lock lock(config_lock);

  if (bind.a) {
    settings[current_setting].keyup_map.insert(std::pair<byte, key_bind_t>(code, bind));
  }
}

void config_bind_set_label(byte code, const char *label) {
  thread_lock lock(config_lock);

  strncpy(settings[current_setting].key_label[code].text, label ? label : "", sizeof(settings[current_setting].key_label[code].text));
}

const char* config_bind_get_label(byte code) {
  thread_lock lock(config_lock);

  return settings[current_setting].key_label[code].text;
}

// set bind color
void config_bind_set_color(byte code, uint color) {
  thread_lock lock(config_lock);
  settings[current_setting].key_label[code].color = color;
}

// get bind color
uint config_bind_get_color(byte code) {
  return settings[current_setting].key_label[code].color;
}

// set key signature
void config_set_key_signature(char key) {
  thread_lock lock(config_lock);
  settings[0].key_signature = key; //grupo 0na grupo guztintzat

}

// get key signature
char config_get_key_signature() {
  thread_lock lock(config_lock);
  return settings[0].key_signature;//grupo 0na grupo guztintzat 
}

// set transpose
void config_set_key_transpose(byte channel, char transpose) {
  thread_lock lock(config_lock);

  if (channel < ARRAY_COUNT(settings[current_setting].key_transpose)) {
    settings[current_setting].key_transpose[channel] = transpose;
  }
}

// get transpose
char config_get_key_transpose(byte channel) {
  thread_lock lock(config_lock);

  if (channel < ARRAY_COUNT(settings[current_setting].key_transpose))
    return settings[current_setting].key_transpose[channel];
  else
    return 0;
}

// set octave shift
void config_set_key_octshift(byte channel, char shift) {
  thread_lock lock(config_lock);

  if (channel < ARRAY_COUNT(settings[current_setting].key_octshift)) {
    settings[current_setting].key_octshift[channel] = shift;
  }
}

// get octave shift
char config_get_key_octshift(byte channel) {
  thread_lock lock(config_lock);

  if (channel < ARRAY_COUNT(settings[current_setting].key_octshift))
    return settings[current_setting].key_octshift[channel];
  else
    return 0;
}

// set velocity
void config_set_key_velocity(byte channel, byte velocity) {
  thread_lock lock(config_lock);

  if (channel < ARRAY_COUNT(settings[current_setting].key_velocity)) {
    settings[current_setting].key_velocity[channel] = velocity;
  }
}

// get velocity
byte config_get_key_velocity(byte channel) {
  thread_lock lock(config_lock);

  if (channel < ARRAY_COUNT(settings[current_setting].key_velocity))
    return settings[current_setting].key_velocity[channel];
  else
    return 127;
}

// set follow key
void config_set_follow_key(byte channel, byte value) {
  thread_lock lock(config_lock);

  if (channel < ARRAY_COUNT(settings[current_setting].follow_key)) {
    settings[current_setting].follow_key[channel] = value;
  }
}

// get follow key
byte config_get_follow_key(byte channel) {
  if (channel < ARRAY_COUNT(settings[current_setting].follow_key))
    return settings[current_setting].follow_key[channel];
  else
    return 0;
}

// get channel
int config_get_output_channel(byte channel) {
  thread_lock lock(config_lock);

  if (channel <= SM_INPUT_MAX)
    return settings[current_setting].key_channel[channel] & 0x0f;
  else
    return channel & 0x0f;
}

// get channel
void config_set_output_channel(byte channel, byte value) {
  thread_lock lock(config_lock);

  if (channel < ARRAY_COUNT(settings[current_setting].key_channel)) {
    settings[current_setting].key_channel[channel] = value & 0x0f;
  }
}

// get midi program
byte config_get_program(byte channel) {
  thread_lock lock(config_lock);

  channel = config_get_output_channel(channel);

  if (channel < ARRAY_COUNT(settings[current_setting].midi_program))
    return settings[current_setting].midi_program[channel];
  else
    return 0;
}

// set midi program
void config_set_program(byte channel, byte value) {
  thread_lock lock(config_lock);

  channel = config_get_output_channel(channel);

  if (channel < ARRAY_COUNT(settings[current_setting].midi_program)) {
    settings[current_setting].midi_program[channel] = value;
  }
}

// get midi program
byte config_get_controller(byte channel, byte id) {
  thread_lock lock(config_lock);

  channel = config_get_output_channel(channel);

  if (channel < ARRAY_COUNT(settings[current_setting].midi_controller))
    return settings[current_setting].midi_controller[channel][id];
  else
    return -1;
}

// set midi program
void config_set_controller(byte channel, byte id, byte value) {
  thread_lock lock(config_lock);

  channel = config_get_output_channel(channel);

  if (channel < ARRAY_COUNT(settings[0].midi_controller)) {//XAM group 0ko sustain hartu ta aldatu beti
    settings[0].midi_controller[channel][id] = value;
  }
}

// reset config
static void config_reset() {
  thread_lock lock(config_lock);

  // keep only one setting group
  config_set_setting_group_count(1);

  // clear current setting group
  config_clear_key_setting();
}

// get setting group
uint config_get_setting_group() {
  thread_lock lock(config_lock);
  return current_setting;
}

// set setting group
void config_set_setting_group(uint id) {
  thread_lock lock(config_lock);

  if (id < setting_count)
    current_setting = id;

  // update status
  for (int ch = 0; ch < 16; ch++) {
    for (int i = 0; i < 127; i++) {
      if (config_get_controller(SM_OUTPUT_0 + ch, i) < 128)
        song_output_event(0xb0 | ch, i, config_get_controller(SM_OUTPUT_0 + ch, i), 0);
    }

    if (config_get_program(SM_OUTPUT_0 + ch) < 128)
      song_output_event(0xc0 | ch, config_get_program(SM_OUTPUT_0 + ch), 0, 0);
  }
}

// delete settting group
void config_delete_setting_group(uint id) {
  // move groups
  for (uint i = id + 1; i < setting_count; ++i) {
    settings[i - 1] = settings[i];
  }

  // remove last group
  config_set_setting_group_count(setting_count - 1);

  // refresh group settings.
  config_set_setting_group(current_setting);
}

// insert setting group
void config_insert_setting_group(uint pos) {
  if (pos > setting_count)
    pos = setting_count;

  // increase group count
  config_set_setting_group_count(setting_count + 1);

  // move groups
  for (uint i = setting_count - 1; i > pos; --i) {
    settings[i] = settings[i - 1];
  }

  // set current setting
  current_setting = pos;

  // clear key setting
  config_clear_key_setting();
  config_set_setting_group(pos);
}

// get setting group count
uint config_get_setting_group_count() {
  thread_lock lock(config_lock);
  return setting_count;
}

// set setting group count
void config_set_setting_group_count(uint count) {
  thread_lock lock(config_lock);

  if (count < 1)
    count = 1;

  if (count > ARRAY_COUNT(settings))
    count = ARRAY_COUNT(settings);

  // new setting id
  uint setting_id = current_setting < count ? current_setting : count - 1;

  // clear newly added settings.
  for (current_setting = setting_count; current_setting < count; current_setting++)
    config_clear_key_setting();

  current_setting = setting_id;
  setting_count = count;
}

// -----------------------------------------------------------------------------------------
// configuration save and load
// -----------------------------------------------------------------------------------------
static bool match_space(char **str) {
  char *s = *str;
  while (*s == ' ' || *s == '\t') s++;

  if (s == *str)
    return false;

  *str = s;
  return true;
}

static bool match_end(char **str) {
  bool result = false;
  char *s = *str;
  while (*s == ' ' || *s == '\t') s++;
  if (*s == '\r' || *s == '\n' || *s == '\0') result = true;
  while (*s == '\r' || *s == '\n') s++;

  if (result)
    *str = s;
  return result;
}

static bool match_word(char **str, const char *match) {
  char *s = *str;

  while (*match) {
    if (tolower(*s) != tolower(*match))
      return false;

    match++;
    s++;
  }

  if (match_space(&s) || match_end(&s)) {
    *str = s;
    return true;
  }

  return false;
}

// match hex
static bool match_hex(char **str, uint *value) {
  char *s = *str;
  uint v = 0;

  for (;;) {
    if (*s >= '0' && *s <= '9')
      v = v * 16 + (*s++ - '0');
    else if (*s >= 'a' && *s <= 'f') {
      v = v * 16 + (10 + *s++ - 'a');
    }
    else if (*s >= 'A' && *s <= 'F') {
      v = v * 16 + (10 + *s++ - 'A');
    }
    else break;
  }

  if (s > *str) {
    if (!match_space(&s) && !match_end(&s))
      return false;

    *value = v;
    *str = s;
    return true;
  }

  return false;
}

// match number
static bool match_number(char **str, uint *value) {
  uint v = 0;
  char *s = *str;

  // hex mode
  if (*s == '$') {
    s++;
    return match_hex(&s, value);
  }

  else {
    if (*s == '-' || *s == '+')
      s++;

    if (*s >= '0' && *s <= '9') {
      while (*s >= '0' && *s <= '9')
        v = v * 10 + (*s++ - '0');

      if (!match_space(&s) && !match_end(&s))
        return false;

      *value = (**str == '-') ? -(int)v : v;
      *str = s;
      return true;
    }
  }

  return false;
}

static bool match_number(char **str, int *value) {
  int v = 0;
  char *s = *str;

  if (*s == '-' || *s == '+')
    s++;

  if (*s >= '0' && *s <= '9') {
    while (*s >= '0' && *s <= '9')
      v = v * 10 + (*s++ - '0');

    if (!match_space(&s) && !match_end(&s))
      return false;

    *value = (**str == '-') ? -v : v;
    *str = s;
    return true;
  }

  return false;
}

// match version 
bool match_version(char **str, int *value) {
  int version = 0;
  int digit = 3;
  char *s = *str;

  for (;;) {
    if (*s >= '0' && *s <= '9') {
      int tmp = 0;
      while (*s >= '0' && *s <= '9')
        tmp = tmp * 10 + (*s++ - '0');

      // number too large
      if (tmp > 255) {
        return false;
      }

      version |= tmp << (digit * 8);

      // next digit
      if (*s == '.') {
        if (--digit < 0) {
          return false;
        }
        s++;
      } else if (match_space(&s) || match_end(&s)) {
        *str = s;
        if (value) *value = version;
        return true;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  return false;
}

// match line
static bool match_line(char **str, char *buff, uint size) {
  char *s = *str;
  while (*s == ' ' || *s == '\t') s++;

  char *e = s;
  while (*e != '\0' && *e != '\r' && *e != '\n') e++;

  char *f = e;
  while (f > s && (f[-1] == ' ' || f[-1] == '\t')) f--;

  if (f > s && f - s < (int)size) {
    memcpy(buff, s, f - s);
    buff[f - s] = 0;

    while (*e == '\r' || *e == '\n') e++;
    *str = e;
    return true;
  }

  return false;
}

// match string
static bool match_string(char **str, char *buf, uint size) {
  char *s = *str;

  while (*s == ' ' || *s == '\t') s++;

  if (*s != '"') {
    return false;
  }

  s++;
  char *d = buf;

  for (;;) {
    switch (s[0]) {
    case '\\':
      switch (s[1]) {
      case 'n': *d = '\n'; s++; break;
      case 'r': *d = '\r'; s++; break;
      case 't': *d = '\t'; s++; break;
      case '\"': *d = '\"'; s++; break;
      case '\0': *d = '\0'; s++; break;
      }
      break;

    case '\r':
    case '\n':
    case '\0':
      return false;

    case '\"':
      *d = '\0';
      s++;

      if (!match_space(&s) && !match_end(&s))
        return false;

      *str = s;
      return true;

    default:
      *d = s[0];
    }

    if (++d >= buf + size)
      return false;

    ++s;
  }
}

static bool match_name(char **str, name_t *names, uint count, uint *value) {
  for (name_t *n = names; n < names + count; n++) {
    if (match_word(str, n->name)) {
      *value = n->value;
      return true;
    }
  }
  return false;
}

// match config value
static bool match_value(char **str, name_t *names, uint count, uint *value) {
  if (match_name(str, names, count, value)) {
    return true;
  }
  return match_number(str, value);
}

// match change value action
static bool match_change_value(char **str, uint *op, uint *value, name_t *names, uint names_size) {
  char *s = *str;

  // there are 2 methods to change a value:
  // 1: op change
  // 2: change [op]
  uint tmp;

  // got a op first, expect for value
  if (match_name(&s, value_action_names, ARRAY_COUNT(value_action_names), &tmp)) {
    if (match_value(&s, names, names_size, value)) {
      *op = tmp;
      *str = s;
      return true;
    }
  }
  else
  {
    // match value first
    if (match_value(&s, names, names_size, value)) {
      if (!match_value(&s, value_action_names, ARRAY_COUNT(value_action_names), op)) {
        *op = SM_VALUE_SET;
      }
      *str = s;
      return true;
    }
  }

  return false;
}

// match action
static bool match_event(char **str, key_bind_t *e) {
  uint action = 0;
  uint channel = 0;
  uint arg1 = 0;
  uint arg2 = 0;
  uint arg3 = 0;

  // match action
  if (!match_value(str, action_names, ARRAY_COUNT(action_names), &action))
    return false;

  if (action == 0)
    return false;

  // make action
  e->a = action;
  e->b = 0;
  e->c = 0;
  e->d = 0;

  bool parsed = false;

  switch (action) {
  case SM_SETTING_GROUP_COUNT:
    if (!match_value(str, 0, NULL, &arg1))
      return false;

    break;

  case SM_KEY_SIGNATURE:
  case SM_VOLUME:
  case SM_SETTING_GROUP:
    if (!match_change_value(str, &arg1, &arg2, NULL, 0))
      return false;

    break;

  case SM_CONTROLLER_DEPRECATED:
      return false;
    break;

  case SM_PROGRAM:
  case SM_TRANSPOSE:
  case SM_FOLLOW_KEY:
  case SM_OCTAVE:
  case SM_VELOCITY:
  case SM_BANK_MSB:
  case SM_BANK_LSB:
  case SM_SUSTAIN:
  case SM_PRESSURE:
  case SM_PITCH:
  case SM_MODULATION:
    if (!match_value(str, channel_names, ARRAY_COUNT(channel_names), &arg1))
      return false;

      if (!match_change_value(str, &arg2, &arg3, NULL, 0))
        return false;

    break;

  case SM_CHANNEL:
    if (!match_value(str, channel_names, ARRAY_COUNT(channel_names), &arg1))
      return false;

    if (!match_change_value(str, &arg2, &arg3, channel_names, ARRAY_COUNT(channel_names)))
        return false;

    break;

  case SM_NOTE_ON:
  case SM_NOTE_OFF:
  case SM_NOTE_PRESSURE:
    if (!match_value(str, channel_names, ARRAY_COUNT(channel_names), &arg1))
      return false;

    if (!match_value(str, note_names, ARRAY_COUNT(note_names), &arg2))
      return false;


    // default pressure is 127
    if (!match_number(str, &arg3))
      arg3 = 127;

    break;

  default:
    // a hex value
    if (**str == '$') {
      uint msg;
      match_number(str, &msg);

      action = (msg >> 24) & 0xff;
      arg1 = (msg >> 16) & 0xff;
      arg2 = (msg >> 8) & 0xff;
      arg3 = (msg >> 0) & 0xff;
    }
    else {
      match_hex(str, &action);
      match_hex(str, &arg1);
      match_hex(str, &arg2);
      match_hex(str, &arg3);
    }
    break;
  }

  e->a = action;
  e->b = arg1;
  e->c = arg2;
  e->d = arg3;
  return true;
}

static int config_parse_keymap_line(char *s, byte override_key = 0) {
  // skip comment
  if (*s == '#')
    return 0;

  uint action;
  key_bind_t bind;

  if (match_name(&s, bind_names, ARRAYSIZE(bind_names), &action)) {
    if (action == BIND_TYPE_KEYDOWN) {
      uint key = 0;

      // match key name
      if (!match_value(&s, key_names, ARRAY_COUNT(key_names), &key))
        return -1;

      // override key
      if (override_key)
        key = override_key;

      // match midi event
      if (match_event(&s, &bind)) {
        // set that action
        config_bind_add_keydown(key, bind);
        return 0;
      }
    }

    else if (action == BIND_TYPE_KEYUP) {
      uint key = 0;

      // match key name
      if (!match_value(&s, key_names, ARRAY_COUNT(key_names), &key))
        return -1;

      // override key
      if (override_key)
        key = override_key;

      // match midi event
      if (match_event(&s, &bind)) {
        // set that action
        config_bind_add_keyup(key, bind);
        return 0;
      }
    }

    else if (action == BIND_TYPE_LABEL) {
      uint key = 0;
      char buff[256] = " ";

      // match key name
      if (!match_value(&s, key_names, ARRAY_COUNT(key_names), &key))
        return -1;

      // override key
      if (override_key)
        key = override_key;

      // text
      match_line(&s, buff, sizeof(buff));

      // set key label
      config_bind_set_label(key, buff);
      return 0;
    }

    else if (action == BIND_TYPE_COLOR) { //botoian kolorea
      uint key = 0;
      int r = 0;
      int g = 0;
      int b = 0;
      int a = 255;

      // match key name
      if (!match_value(&s, key_names, ARRAY_COUNT(key_names), &key))
        return -1;

      // override key
      if (override_key)
        key = override_key;

      match_number(&s, &r);
      match_number(&s, &g);
      match_number(&s, &b);
      match_number(&s, &a);

      // set key color
      config_bind_set_color(key, (a << 24) | (r << 16) | (g << 8) | b);
      return 0;
    }
  }
  else if (match_event(&s, &bind)) {
    // execute event here
    song_output_event(bind.a, bind.b, bind.c, bind.d);

    // clear all settings here
    if (bind.a == SM_SETTING_GROUP_COUNT) {
      for (int i = config_get_setting_group_count() - 1; i >= 0; i--) {
        config_set_setting_group(i);
        config_clear_key_setting();
      }
    }
    return 0;
  }
  else if (match_word(&s, "SoinuTxikiBirtuala")) {
    int version = 0;
    if (match_version(&s, &version)) {
      map_version = version;
      return 0;
    }
  }

  return -1;
}

int config_parse_keymap(const char *command, byte override_key, uint version) {
  thread_lock lock(config_lock);

  char line[1024];
  char *line_end = line;
  int result = 0;

  map_version = version == -1 ? map_current_version : version;

  for (;; ) {
    switch (*command) {
     case 0:
     case '\n':
       if (line_end > line) {
         *line_end = 0;
         result = config_parse_keymap_line(line, override_key);
         line_end = line;
       }

       if (*command == 0)
         return result;
       break;

     default:
       if (line_end < line + sizeof(line) - 1)
         *line_end++ = *command;
       break;
    }

    command++;
  }
}

// load keymap
int config_load_keymap(const char *filename) {
  thread_lock lock(config_lock);

  char line[256];
  config_get_media_path(line, sizeof(line), filename);

  FILE *fp = fopen(line, "r");
  if (!fp)
    return -1;

  // undefined version
  map_version = 0;

  // clear settings
  config_set_setting_group_count(0);
  config_clear_key_setting();

  // read lines
  while (fgets(line, sizeof(line), fp)) {
    config_parse_keymap_line(line);
  }

  fclose(fp);

  // restore current group
  config_set_setting_group(0);
  return 0;
}

static int print_format(char *buff, int buff_size, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int len = _vsnprintf(buff, buff_size, format, args);
  va_end(args);
  return len;
}

static int print_hex(char *buff, int buff_size, int value, const char *sep = "\t") {
  return print_format(buff, buff_size, "%s%02x", sep, value);
}

static int print_value(char *buff, int buff_size, int value, name_t *names = NULL, int name_count = 0, const char *sep = "\t") {
  const char* result = NULL;

  for (int i = 0; i < name_count; i++) {
    if (names[i].value == value) {
      if (names[i].lang == map_language) {
        result = names[i].name;
        break;
      }
      else if (!result) {
        result = names[i].name;
      }
    }
  }

  if (result)
    return print_format(buff, buff_size, "%s%s", sep, result);
  else
    return print_format(buff, buff_size, "%s%d", sep, value);
}

int print_version(char *buff, int buff_size, uint value, const char *sep = "\t") {
  int digit[4] = {
    (value >> 24) & 0xff,
    (value >> 16) & 0xff,
    (value >> 8) & 0xff,
    (value >> 0) & 0xff,
  };

  char *s = buff;
  char *end = buff + buff_size;

  s += print_format(s, end - s, "%s%d.%d", sep, digit[0], digit[1]);
  for (int i = 2; i < 4; i++) {
    if (digit[i]) {
      s += print_format(s, end - s, ".%d", digit[i]);
    }
  }

  return s - buff;
}

static int print_event(char *buff, int buffer_size, key_bind_t &e, const char *sep = "\t") {
  char *s = buff;
  char *end = buff + buffer_size;

  if (e.a < SM_MIDI_MESSAGE_START) {
    s += print_value(s, end - s, e.a, action_names, ARRAY_COUNT(action_names), sep);

    switch (e.a) {
     case SM_KEY_SIGNATURE:
     case SM_VOLUME:
     case SM_SETTING_GROUP:
       s += print_value(s, end - s, e.b, value_action_names, ARRAY_COUNT(value_action_names));
       s += print_value(s, end - s, (char)e.c, NULL, 0);
       break;

     case SM_SETTING_GROUP_COUNT:
       s += print_value(s, end - s, e.b, NULL, 0);
       break;

     case SM_OCTAVE:
     case SM_VELOCITY:
     case SM_TRANSPOSE:
     case SM_FOLLOW_KEY:
     case SM_PROGRAM:
     case SM_BANK_MSB:
     case SM_BANK_LSB:
     case SM_SUSTAIN:
     case SM_MODULATION:
     case SM_PRESSURE:
     case SM_PITCH:
       s += print_value(s, end - s, e.b, channel_names, ARRAY_COUNT(channel_names));
       s += print_value(s, end - s, e.c, value_action_names, ARRAY_COUNT(value_action_names));
       s += print_value(s, end - s, (char)e.d);
       break;

     case SM_NOTE_ON:
     case SM_NOTE_OFF:
     case SM_NOTE_PRESSURE:
       s += print_value(s, end - s, e.b, channel_names, ARRAY_COUNT(channel_names));
       s += print_value(s, end - s, e.c, note_names, ARRAY_COUNT(note_names));
       if (e.d != 127)
         s += print_value(s, end - s, e.d);
       break;

     case SM_CHANNEL:
       s += print_value(s, end - s, e.b, channel_names, ARRAY_COUNT(channel_names));
       s += print_value(s, end - s, e.c, value_action_names, ARRAY_COUNT(value_action_names));
       s += print_value(s, end - s, (char)e.d);

     case SM_PLAY:
     case SM_RECORD:
     case SM_STOP:
       break;

     default:
       s += print_value(s, end - s, e.b, NULL, 0);
       s += print_value(s, end - s, e.c, NULL, 0);
       s += print_value(s, end - s, e.d, NULL, 0);
       break;
    }
  }
  else
  {
    uint midi_msg = (e.a << 24) | (e.b << 16) | e.c << 8 | e.d;
    s += print_format(s, end - s, "%sMIDI", sep);
    const char *tab = "\t";
    const char *spc = " ";

    if (e.d) {
      s += print_hex(s, end - s, e.a, tab);
      s += print_hex(s, end - s, e.b, spc);
      s += print_hex(s, end - s, e.c, spc);
      s += print_hex(s, end - s, e.d, spc);
    }
    else if (e.c) {
      s += print_hex(s, end - s, e.a, tab);
      s += print_hex(s, end - s, e.b, spc);
      s += print_hex(s, end - s, e.c, spc);
    }
    else if (e.b) {
      s += print_hex(s, end - s, e.a, tab);
      s += print_hex(s, end - s, e.b, spc);
    }
    else if (e.a) {
      s += print_hex(s, end - s, e.a, tab);
    }
  }

  return s - buff;
}

static int print_keyboard_event(char *buff, int buffer_size, int key, key_bind_t &e) {
  char *s = buff;
  char *end = buff + buffer_size;

  s += print_value(s, end - s, key, key_names, ARRAY_COUNT(key_names));
  s += print_event(s, end - s, e);

  return s - buff;
}

// save key bind
static int config_save_keybind(int key, char *buff, int buffer_size) {
  char *end = buff + buffer_size;
  char *s = buff;

  key_bind_t temp[256];
  bool first;
  int count;

  count = config_bind_get_keydown(key, temp, ARRAYSIZE(temp));
  first = true;
  for (int i = 0; i < count; i++) {
    if (temp[i].a) {
      s += print_value(s, end - s, BIND_TYPE_KEYDOWN, bind_names, ARRAYSIZE(bind_names), "");
      s += print_keyboard_event(s, end - s, key, temp[i]);
      s += print_format(s, end - s, "\r\n");
      first = false;
    }
  }

  count = config_bind_get_keyup(key, temp, ARRAYSIZE(temp));
  first = true;
  for (int i = 0; i < count; i++) {
    if (temp[i].a) {
      s += print_value(s, end - s, BIND_TYPE_KEYUP, bind_names, ARRAYSIZE(bind_names), "");
      s += print_keyboard_event(s, end - s, key, temp[i]);
      s += print_format(s, end - s, "\r\n");
      first = false;
    }
  }

  if (*config_bind_get_label(key)) {
    s += print_value(s, end - s, BIND_TYPE_LABEL, bind_names, ARRAYSIZE(bind_names), "");
    s += print_value(s, end - s, key, key_names, ARRAY_COUNT(key_names));
    s += print_format(s, end - s, "\t%s\r\n", config_bind_get_label(key));
  }

  if (config_bind_get_color(key)) {
    uint color = config_bind_get_color(key);
    byte a = color >> 24;
    byte r = color >> 16;
    byte g = color >> 8;
    byte b = color;

    s += print_value(s, end - s, BIND_TYPE_COLOR, bind_names, ARRAYSIZE(bind_names), "");
    s += print_value(s, end - s, key, key_names, ARRAY_COUNT(key_names));

    if (a != 0xff)
      s += print_format(s, end - s, "\t%d %d %d %d\r\n", r, g, b, a);
    else
      s += print_format(s, end - s, "\t%d %d %d\r\n", r, g, b);
  }

  return s - buff;
}

// save controller
static int config_save_controller(char *buff, int buffer_size, int action, int controller) {
  char *end = buff + buffer_size;
  char *s = buff;
  for (int ch = SM_OUTPUT_0; ch <= SM_OUTPUT_MAX; ch++) {
    int value = config_get_controller(ch, controller);
    if (value < 128) {
      key_bind_t bind(action, ch, SM_VALUE_SET, value);
      s += print_event(s, end - s, bind, "");
      s += print_format(s, end - s, "\r\n");
    }
  }
  return s - buff;
}

// save current key settings
static int config_save_key_settings(char *buff, int buffer_size) {
  char *end = buff + buffer_size;
  char *s = buff;

  // save key signature
  s += print_event(s, end - s, key_bind_t(SM_KEY_SIGNATURE, SM_VALUE_SET, config_get_key_signature(), 0), "");
  s += print_format(s, end - s, "\r\n");

  // save keyboard status
  for (int ch = SM_INPUT_0; ch <= SM_INPUT_MAX; ch++) {
    int value = config_get_key_octshift(ch);
    if (value != 0) {
      key_bind_t bind(SM_OCTAVE, ch, SM_VALUE_SET, value);
      s += print_event(s, end - s, bind, "");
      s += print_format(s, end - s, "\r\n");
    }
  }

  // transpose
  for (int ch = SM_INPUT_0; ch <= SM_INPUT_MAX; ch++) {
    int value = config_get_key_transpose(ch);
    if (value != 0) {
      key_bind_t bind(SM_TRANSPOSE, ch, SM_VALUE_SET, value);
      s += print_event(s, end - s, bind, "");
      s += print_format(s, end - s, "\r\n");
    }
  }

  // follow key
  for (int ch = SM_INPUT_0; ch <= SM_INPUT_MAX; ch++) {
    int value = config_get_follow_key(ch);
    if (value == 0) {
      key_bind_t bind(SM_FOLLOW_KEY, ch, SM_VALUE_SET, value);
      s += print_event(s, end - s, bind, "");
      s += print_format(s, end - s, "\r\n");
    }
  }

  // velocity
  for (int ch = SM_INPUT_0; ch <= SM_INPUT_MAX; ch++) {
    int value = config_get_key_velocity(ch);
    if (value != 127) {
      key_bind_t bind(SM_VELOCITY, ch, SM_VALUE_SET, value);
      s += print_event(s, end - s, bind, "");
      s += print_format(s, end - s, "\r\n");
    }
  }

  // channel
  for (int ch = SM_INPUT_0; ch <= SM_INPUT_MAX; ch++) {
    int value = config_get_output_channel(ch);
    if (value != 0) {
      key_bind_t bind(SM_CHANNEL, ch, SM_VALUE_SET, value);
      s += print_event(s, end - s, bind, "");
      s += print_format(s, end - s, "\r\n");
    }
  }

  // program
  for (int ch = SM_OUTPUT_0; ch <= SM_OUTPUT_MAX; ch++) {
    int value = config_get_program(ch);
    if (value < 128) {
      key_bind_t bind(SM_PROGRAM, ch, SM_VALUE_SET, value);
      s += print_event(s, end - s, bind, "");
      s += print_format(s, end - s, "\r\n");
    }
  }

  // bank msb
  s += config_save_controller(s, end - s, SM_BANK_MSB, 0);

  // bank lsb
  s += config_save_controller(s, end - s, SM_BANK_LSB, 32);

  // sustain
  s += config_save_controller(s, end - s, SM_SUSTAIN, 64);

  // modulation
  s += config_save_controller(s, end - s, SM_MODULATION, 1);

  // save key bindings
  for (int key = 0; key < 256; key++) {
    s += config_save_keybind(key, s, end - s);
  }

  return s - buff;
}

// save keymap
char* config_save_keymap(uint lang) {
  thread_lock lock(config_lock);
  size_t buffer_size = 4096 * 10;
  char * buffer = (char*)malloc(buffer_size);

  char *s = buffer;
  char *end = buffer + buffer_size;

  map_language = lang < FP_LANG_COUNT ? lang : lang_get_current();

  // save map version
  s += print_format(s, end - s, "SoinuTxikiBirtuala");
  s += print_version(s, end - s, map_current_version, " ");
  s += print_format(s, end - s, "\r\n\r\n");

  // save group count
  s += print_event(s, end - s, key_bind_t(SM_SETTING_GROUP_COUNT, config_get_setting_group_count(), 0, 0), "");
  s += print_format(s, end - s, "\r\n");

  // current setting group
  int current_setting = config_get_setting_group();

  // for each group
  for (uint i = 0; i < config_get_setting_group_count(); i++) {
    size_t start = s - buffer;

    for(;;) {
      // change group
      s += print_event(s, end - s, key_bind_t(SM_SETTING_GROUP, SM_VALUE_SET, i, 0), "");
      s += print_format(s, end - s, "\r\n");

      // change to group i
      config_set_setting_group(i);

      // save current key settings
      s += config_save_key_settings(s, end - s);

      if (s < end - 4096)
        break;

      buffer_size += 4096 * 10;
      buffer = (char*)realloc(buffer, buffer_size);
      
      s = buffer + start;
      end = buffer + buffer_size;
    }
  }

  // restore current setting group
  config_set_setting_group(current_setting);

  return buffer;
}

// dump key bind
char* config_dump_keybind(byte code, uint lang) {
  size_t buffer_size = 8192;
  char * buffer = (char*)malloc(buffer_size);
  buffer[0] = 0;

  map_language = lang < FP_LANG_COUNT ? lang : lang_get_current();

  for (;;) {
    config_save_keybind(code, buffer, buffer_size);

    if (buffer < buffer + buffer_size - 4096)
      break;

    buffer_size += 4096;
    buffer = (char*)realloc(buffer, buffer_size);
  }

  return buffer;
}

// clear keyboard setting
void config_clear_key_setting() {
  thread_lock lock(config_lock);

  settings[current_setting].clear();
}

// copy key setting
void config_copy_key_setting() {
  thread_lock lock(config_lock);

  if (OpenClipboard(NULL)) {
    // temp buffer
    uint buff_size = 1024 * 1024;
    char *buff = new char[buff_size];

    // save key settings
    int size = config_save_key_settings(buff, buff_size);

    if (size > 0) {
      EmptyClipboard();

      // Allocate a global memory object for the text.
      HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (size + 1) * sizeof(char));
      if (hglbCopy) {
        char *lptstrCopy = (char *)GlobalLock(hglbCopy);
        memcpy(lptstrCopy, buff, size);
        lptstrCopy[size] = 0;
        GlobalUnlock(hglbCopy);
        SetClipboardData(CF_TEXT, hglbCopy);
      }
    }

    delete[] buff;
    CloseClipboard();
  }
}

// paste key setting
void config_paste_key_setting() {
  thread_lock lock(config_lock);

  if (OpenClipboard(NULL)) {
    if (IsClipboardFormatAvailable(CF_TEXT)) {
      HGLOBAL hglb = GetClipboardData(CF_TEXT);
      if (hglb != NULL) {
        char *lptstr = (char *)GlobalLock(hglb);

        if (lptstr != NULL) {
          config_clear_key_setting();
          config_parse_keymap(lptstr, 0, -1);
          GlobalUnlock(hglb);
        }
      }
    }
    CloseClipboard();
  }
}

static int config_apply() {
  // set keymap
  config_set_keymap(global.keymap);

  // load instrument
  config_select_instrument(global.instrument_type, global.instrument_path);

  // open output
  if (config_select_output(global.output_type, global.output_device))
    global.output_device[0] = 0;

  config_set_output_volume(global.output_volume);
  config_set_output_delay(global.output_delay);

  // reset default key setting
  if (global.keymap[0] == 0)
    config_default_key_setting();

  // open midi inputs
  midi_open_inputs();

  return 0;
}

// load
int config_load(const char *filename) {
  thread_lock lock(config_lock);

  char line[256];
  config_get_media_path(line, sizeof(line), filename);

  // reset config
  config_reset();

  FILE *fp = fopen(line, "r");
  if (fp) {
    // read lines
    while (fgets(line, sizeof(line), fp)) {
      char *s = line;

      // comment
      if (*s == '#')
        continue;

      // instrument
      if (match_word(&s, "instrument")) {
        // instrument type
        if (match_word(&s, "type")) {
          match_value(&s, instrument_type_names, ARRAY_COUNT(instrument_type_names), &global.instrument_type);
        } else if (match_word(&s, "path")) {
          match_line(&s, global.instrument_path, sizeof(global.instrument_path));
        } else if (match_word(&s, "showui")) {
          uint showui = 1;
          match_value(&s, NULL, 0, &showui);
          vsti_show_editor(showui != 0);
        } else if (match_word(&s, "showmidi")) {
          match_value(&s, NULL, 0, &global.instrument_show_midi);
        } else if (match_word(&s, "showvsti")) {
          match_value(&s, NULL, 0, &global.instrument_show_vsti);
        }
      }
      // output
      else if (match_word(&s, "output")) {
        // instrument type
        if (match_word(&s, "type")) {
          match_value(&s, output_type_names, ARRAY_COUNT(output_type_names), &global.output_type);
        } else if (match_word(&s, "device")) {
          match_line(&s, global.output_device, sizeof(global.output_device));
        } else if (match_word(&s, "delay")) {
          match_number(&s, &global.output_delay);
        } else if (match_word(&s, "volume")) {
          match_number(&s, &global.output_volume);
        }
      }
      // keyboard
      else if (match_word(&s, "keyboard")) {
        if (match_word(&s, "map")) {
          match_line(&s, global.keymap, sizeof(global.keymap));
        }
      }
      // midi
      else if (match_word(&s, "midi")) {
        if (match_word(&s, "input")) {
          char buff[256];
          if (match_string(&s, buff, sizeof(buff))) {
            midi_input_config_t config;
            config.enable = true;
            config.remap = 0;
            match_number(&s, &config.remap);
            config_set_midi_input_config(buff, config);
          }

        } else if (match_word(&s, "transpose")) {
          match_value(&s, NULL, 0, &global.midi_transpose);
        }
      }
      // resize window
      else if (match_word(&s, "resize")) {
        uint enable = 0;
        match_value(&s, boolean_names, ARRAY_COUNT(boolean_names), &enable);
        config_set_enable_resize_window(enable != 0);
      }
      // hotkey
      else if (match_word(&s, "hotkey")) {
        uint enable = 0;
        match_value(&s, boolean_names, ARRAY_COUNT(boolean_names), &enable);
        config_set_enable_hotkey(enable != 0);
      }
	  // INACTIVE
      else if (match_word(&s, "inactive")) {
        uint enable = 0;
        match_value(&s, boolean_names, ARRAY_COUNT(boolean_names), &enable);
        config_set_enable_inactive(enable != 0);
      }
      else if (match_word(&s, "key-fade")) {
        uint value = 0;
        match_number(&s, &value);
        config_set_key_fade(value);
      }
      else if (match_word(&s, "gui-transparency")) {
        uint value = 0;
        match_number(&s, &value);
        if (value > 255)
          value = 255;
        config_set_gui_transparency(value);
      }
      else if (match_word(&s, "gui-auto-color")) {
        uint value = 0;
        match_number(&s, &value);
        config_set_auto_color(value);
      }
      else if (match_word(&s, "gui-preview-color")) {
        uint value = 0;
        match_number(&s, &value);
        config_set_preview_color(value);
      }
      else if (match_word(&s, "gui-note-display")) {
        uint value = 0;
        match_number(&s, &value);
        config_set_note_display(value);
      }
      else if (match_word(&s, "update-version")) {
        int value = 0;
        match_version(&s, &value);
        config_set_update_version(value);
      }
      else if (match_word(&s, "language")) {
        uint value = 0;
        match_number(&s, &value);
        gui_set_language(value);

      }
    }

    fclose(fp);
  }

  return config_apply();
}

// save
int config_save(const char *filename) {
  thread_lock lock(config_lock);

  char file_path[256];
  config_get_media_path(file_path, sizeof(file_path), filename);

  FILE *fp = fopen(file_path, "wb");
  if (!fp)
    return -1;

  if (global.instrument_type)
    fprintf(fp, "instrument type %s\r\n", instrument_type_names[global.instrument_type]);

  if (global.instrument_path[0])
    fprintf(fp, "instrument path %s\r\n", global.instrument_path);

  if (!vsti_is_show_editor())
    fprintf(fp, "instrument showui %d\r\n", vsti_is_show_editor());

  if (!global.instrument_show_midi)
    fprintf(fp, "instrument showmidi %d\r\n", global.instrument_show_midi);

  if (!global.instrument_show_vsti)
    fprintf(fp, "instrument showvsti %d\r\n", global.instrument_show_vsti);

  if (global.output_type)
    fprintf(fp, "output type %s\r\n", output_type_names[global.output_type]);

  if (global.output_device[0])
    fprintf(fp, "output device %s\r\n", global.output_device);

  if (global.output_delay != 10)
    fprintf(fp, "output delay %d\r\n", global.output_delay);

  if (global.output_volume != 100)
    fprintf(fp, "output volume %d\r\n", global.output_volume);

  if (global.keymap[0])
    fprintf(fp, "keyboard map %s\r\n", global.keymap);

  if (global.midi_transpose)
    fprintf(fp, "midi transpose %d\r\n", global.midi_transpose);
    
  fprintf(fp, "language %d\r\n", lang_get_current());

  for (auto it = global.midi_inputs.begin(); it != global.midi_inputs.end(); ++it) {
    if (it->second.enable) {
      fprintf(fp, "midi input \"%s\" %d\r\n", it->first.c_str(), it->second.remap);
    }
  }

  if (!config_get_enable_hotkey())
    fprintf(fp, "hotkey disable\r\n");
    
  //XAM inactive
  if (!config_get_enable_inactive())
    fprintf(fp, "inactive disable\r\n");

  if (!config_get_enable_resize_window())
    fprintf(fp, "resize disable\r\n");

  if (config_get_key_fade()) {
    fprintf(fp, "key-fade %d\r\n", config_get_key_fade());
  }

  if (config_get_gui_transparency() != 255) {
    //fprintf(fp, "gui-transparency %d\r\n", config_get_gui_transparency());
  }

  if (config_get_auto_color()) {
    fprintf(fp, "gui-auto-color %d\r\n", config_get_auto_color());
  }

  if (config_get_preview_color()) {
    fprintf(fp, "gui-preview-color %d\r\n", config_get_preview_color());
  }

  if (config_get_note_display()) {
    fprintf(fp, "gui-note-display %d\r\n", config_get_note_display());
  }


  // update version
  if (config_get_update_version()) {
    char buff[256];
    print_version(buff, sizeof(buff), config_get_update_version(), "");
    fprintf(fp, "update-version %s\r\n", buff);
  }

  fclose(fp);
  return 0;
}

// initialize config
int config_init() {
  thread_lock lock(config_lock);

  config_reset();
  return 0;
}

// close config
void config_shutdown() {
  midi_close_inputs();
  midi_close_output();
  vsti_unload_plugin();
  asio_close();
  dsound_close();
  wasapi_close();
}


// select instrument
int config_select_instrument(int type, const char *name) {
  // unload previous instrument
  vsti_unload_plugin();
  midi_close_output();

  // load new instrument
  int result = -1;
  thread_lock lock(config_lock);

  if (type == INSTRUMENT_TYPE_MIDI) {
    result = midi_open_output(name);
    if (result == 0) {
      global.instrument_type = type;
      strncpy(global.instrument_path, name, sizeof(global.instrument_path));
    }
  }
  else if (type == INSTRUMENT_TYPE_VSTI) {
    if (PathIsFileSpec(name)) {
      struct select_instrument_cb : vsti_enum_callback {
        void operator () (const char *value) {
          if (!found) {
            if (_stricmp(PathFindFileName(value), name) == 0) {
              strncpy(name, value, sizeof(name));
              found = true;
            }
          }
        }

        char name[256];
        bool found;
      };

      select_instrument_cb callback;
      print_format(callback.name, sizeof(callback.name), "%s.dll", name);
      callback.found = false;

      vsti_enum_plugins(callback);

      if (callback.found) {
        // load plugin
        result = vsti_load_plugin(callback.name);
        if (result == 0) {
          global.instrument_type = type;
          strncpy(global.instrument_path, name, sizeof(global.instrument_path));
        }
      }
    } else {
      // load plugin
      result = vsti_load_plugin(name);
      if (result == 0) {
        global.instrument_type = type;
        strncpy(global.instrument_path, name, sizeof(global.instrument_path));
      }
    }
  }

  midi_reset();
  return result;
}

// get instrument type
int config_get_instrument_type() {
  thread_lock lock(config_lock);

  return global.instrument_type;
}

const char* config_get_instrument_path() {
  thread_lock lock(config_lock);

  return global.instrument_path;
}

// set enable resize window
void config_set_enable_resize_window(bool enable) {
  thread_lock lock(config_lock);

  global.enable_resize = enable;
}

// get enable resize window
bool config_get_enable_resize_window() {
  thread_lock lock(config_lock);

  return global.enable_resize != 0;
}

void config_set_enable_hotkey(bool enable) {
  thread_lock lock(config_lock);

  global.enable_hotkey = enable;
}

bool config_get_enable_hotkey() {
  thread_lock lock(config_lock);

  return global.enable_hotkey != 0;
}

//XAM inactive
void config_set_enable_inactive(bool enable) {
  thread_lock lock(config_lock);

  global.enable_inactive = enable;
}

bool config_get_enable_inactive() {
  thread_lock lock(config_lock);

  return global.enable_inactive != 0;
}

bool config_get_midi_transpose() {
  thread_lock lock(config_lock);

  return global.midi_transpose != 0;
}

void config_set_midi_transpose(bool value) {
  thread_lock lock(config_lock);

  global.midi_transpose = value;
}

// select output
int config_select_output(int type, const char *device) {
  asio_close();
  dsound_close();
  wasapi_close();

  int result = -1;
  int auto_selected_type = 0;

  // open output
  switch (type) {
   case OUTPUT_TYPE_AUTO:
     // try wasapi first
     result = wasapi_open(NULL);
     auto_selected_type = OUTPUT_TYPE_WASAPI;

     if (result) {
       result = dsound_open(device);
       auto_selected_type = OUTPUT_TYPE_DSOUND;
     }

     if (result == 0) {
       device = "";
     }
     break;

   case OUTPUT_TYPE_DSOUND:
     result = dsound_open(device);
     break;

   case OUTPUT_TYPE_WASAPI:
     result = wasapi_open(device);
     break;

   case OUTPUT_TYPE_ASIO:
     result = asio_open(device);
     break;
  }

  // success
  if (result == 0) {
    thread_lock lock(config_lock);
    global.output_type = type;
    global.output_type_current = auto_selected_type;
    strncpy(global.output_device, device, sizeof(global.output_device));
  }

  return result;
}


void config_set_midi_input_config(const char *device, const midi_input_config_t &config) {
  thread_lock lock(config_lock);

  if (device[0]) {
    global.midi_inputs[std::string(device)] = config;
  }
}

bool config_get_midi_input_config(const char *device, midi_input_config_t *config) {
  thread_lock lock(config_lock);

  if (device[0]) {
    auto it = global.midi_inputs.find(std::string(device));
    if (it != global.midi_inputs.end()) {
      *config = it->second;
      return true;
    }
  }

  config->enable = false;
  config->remap = 0;
  return false;
}

// get output delay
int config_get_output_delay() {
  thread_lock lock(config_lock);

  return global.output_delay;
}

// get output delay
void config_set_output_delay(int delay) {
  thread_lock lock(config_lock);

  if (delay < 0)
    delay = 0;
  else if (delay > 500)
    delay = 500;

  global.output_delay = delay;

  // open output
  switch (global.output_type) {
   case OUTPUT_TYPE_AUTO:
     dsound_set_buffer_time(global.output_delay);
     wasapi_set_buffer_time(global.output_delay);
     break;

   case OUTPUT_TYPE_DSOUND:
     dsound_set_buffer_time(global.output_delay);
     break;

   case OUTPUT_TYPE_WASAPI:
     wasapi_set_buffer_time(global.output_delay);
     break;

   case OUTPUT_TYPE_ASIO:
     break;
  }
}

// get output type
int config_get_output_type() {
  thread_lock lock(config_lock);
  return global.output_type;
}

int config_get_current_output_type() {
  return global.output_type_current;
}

// get output device
const char* config_get_output_device() {
  thread_lock lock(config_lock);
  return global.output_device;
}

// get keymap
const char* config_get_keymap() {
  thread_lock lock(config_lock);
  return global.keymap;
}

// set keymap
int config_set_keymap(const char *mapname) {
  thread_lock lock(config_lock);

  char path[MAX_PATH];
  config_get_media_path(path, sizeof(path), mapname);

  int result;
  if (result = config_load_keymap(mapname)) {
    global.keymap[0] = 0;
  } else {
    config_get_relative_path(global.keymap, sizeof(global.keymap), path);
  }
  return result;
}


// get exe path
void config_get_media_path(char *buff, int buff_size, const char *path) {
  if (PathIsRelative(path)) {
    char base[MAX_PATH];
    char temp[MAX_PATH];

    GetModuleFileNameA(NULL, base, sizeof(base));

    // remove file spec
    PathRemoveFileSpec(base);

    // to media path
#ifdef _DEBUG
    PathAppend(base, "\\..\\..\\data\\");
#else
    PathAppend(base, "\\.\\");
#endif

    PathCombine(temp, base, path);
    strncpy(buff, temp, buff_size);
  } else {
    strncpy(buff, path, buff_size);
  }

}

// get relative path
void config_get_relative_path(char *buff, int buff_size, const char *path) {
  if (PathIsRelative(path)) {
    strncpy(buff, path, buff_size);
  } else {
    char base[MAX_PATH];
    config_get_media_path(base, sizeof(base), "");

    if (_strnicmp(path, base, strlen(base)) == 0) {
      strncpy(buff, path + strlen(base), buff_size);
    } else {
      strncpy(buff, path, buff_size);
    }
  }
}


// get volume
int config_get_output_volume() {
  thread_lock lock(config_lock);
  return global.output_volume;
}

// set volume
void config_set_output_volume(int volume) {
  thread_lock lock(config_lock);
  if (volume < 0) volume = 0;
  if (volume > 200) volume = 200;
  global.output_volume = volume;
}

// reset settings
void config_default_key_setting() {
  thread_lock lock(config_lock);
  config_clear_key_setting();

  if (lang_text_open(IDR_TEXT_DEFAULT_SETTING)) {
    map_version = 0;

    char line[4096];
    while (lang_text_readline(line, sizeof(line)))
      config_parse_keymap_line(line);

    lang_text_close();
  }
}

// get key name
const char* config_get_key_name(byte code) {
  for (name_t *name = key_names; name < key_names + ARRAY_COUNT(key_names); name++)
    if (name->value == code)
      return name->name;

  static char buff[16];
  print_format(buff, sizeof(buff), "%d", code);
  return buff;
}

// get note name
const char* config_get_note_name(byte note) {
  static bool initialized = false;
  static const char* name_map[128] = {0};

  if (!initialized) {
    for (int i = 0; i < 128; i++) {
      name_map[i] = "";
    }
    for (int i = 0; i < ARRAYSIZE(note_names); i++) {
      if (note_names[i].value < 128) {
        name_map[note_names[i].value] = note_names[i].name;
      }
    }
    initialized = true;
  }

  return note < 128 ? name_map[note] : "";
}

// get channel name
const char* config_get_channel_name(byte ch) {
  const char* result = NULL;

  for (int i = 0; i < ARRAYSIZE(channel_names); i++) {
    if (channel_names[i].value == ch) {
      if (channel_names[i].lang == lang_get_current()) {
        result = channel_names[i].name;
        break;
      }
      else if (!result) {
        result = channel_names[i].name;
      }
    }
  }

  return result ? result : "";
}

// print default action
static int print_default_action(char *buff, size_t size, int action, int value) {
  char *s = buff;
  char *end = buff + size;

  switch (action & 0x0f) {
  case SM_VALUE_SET:
  case SM_VALUE_SET10:
  case SM_VALUE_SET1:
    s += print_format(s, end - s, "%d", value);
    break;

  case SM_VALUE_INC:
    if (value > 0)
      s += print_format(s, end - s, "+");
    if (value != 1)
      s += print_format(s, end - s,  "%d", value);
    break;

  case SM_VALUE_DEC:
    if (value > 0)
      s += print_format(s, end - s, "-");
    if (value != 1)
      s += print_format(s, end - s,  "%d", value);
    break;

  case SM_VALUE_FLIP:
    s += print_format(s, end - s,  "~");
    break;

  case SM_VALUE_PRESS:
    s += print_format(s, end - s, "*%d", value);
    break;
  }

  return s - buff;
}


// translate message to key display
int config_default_keylabel(char* buff, size_t size, key_bind_t bind) {
  char *s = buff;
  char *end = buff + size;

  if (bind.a < SM_MIDI_MESSAGE_START) {
    switch (bind.a)  {
    case SM_KEY_SIGNATURE:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_KEYSIGNATURE));
      s += print_default_action(s, end - s, bind.b, (char)bind.c);
      break;

    case SM_TRANSPOSE:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_TRANSPOSE));
      s += print_default_action(s, end - s, bind.c, (char)bind.d);
      break;

    case SM_FOLLOW_KEY:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_FOLLOW_KEY));
      break;

    case SM_OCTAVE:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_OCTAVE));
      s += print_default_action(s, end - s, bind.c, (char)bind.d);
      break;

    case SM_VELOCITY:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_VELOCITY));
      s += print_default_action(s, end - s, bind.c, (char)bind.d);
      break;

    case SM_CHANNEL:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_CHANNEL));
      s += print_default_action(s, end - s, bind.c, (char)bind.d);
      break;

    case SM_VOLUME:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_VOLUME));
      s += print_default_action(s, end - s, bind.b, (char)bind.c);
      break;

    case SM_PLAY:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_PLAY));
      break;

    case SM_RECORD:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_RECORD));
      break;

    case SM_STOP:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_STOP));
      break;

    case SM_SETTING_GROUP:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_GROUP));
      s += print_default_action(s, end - s, bind.b, (char)bind.c);
      break;

    case SM_SETTING_GROUP_COUNT:
      break;

    case SM_NOTE_ON:
    case SM_NOTE_OFF:
      s += print_format(s, end - s, "%s", config_get_note_name(bind.c));
      break;

    case SM_NOTE_PRESSURE:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_NOTE_PRESSURE));
      break;

    case SM_PRESSURE:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_PRESSURE));
      break;

    case SM_PITCH:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_PITCH));
      break;

    case SM_MODULATION:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_MODULATION));
      break;

    case SM_PROGRAM:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_PROGRAM));
      s += print_default_action(s, end - s, bind.c, (char)bind.d);
      break;

    case SM_BANK_LSB:
    case SM_BANK_MSB:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_BANK));
      s += print_default_action(s, end - s, bind.c, (char)bind.d);
      break;

    case SM_SUSTAIN:
      s += print_format(s, end - s, "%s", lang_load_string(IDS_KEY_LABEL_SUSTAIN));
      s += print_default_action(s, end - s, bind.c, (char)bind.d);
      break;
    }
  }
  else {
    switch (bind.a & 0xf0) {
    case SM_MIDI_NOTEON:
    case SM_MIDI_NOTEOFF:
      if (bind.b < 128) {
        s += print_format(s, end - s, "%s", config_get_note_name(bind.b));
      }
      break;
    }
  }
    
  return s - buff;
}


// instrument show midi 
bool config_get_instrument_show_midi() {
  thread_lock lock(config_lock);
  return global.instrument_show_midi != 0;
}

void config_set_instrument_show_midi(bool value) {
  thread_lock lock(config_lock);
  global.instrument_show_midi = value;
}

// instrument show midi 
bool config_get_instrument_show_vsti() {
  thread_lock lock(config_lock);
  return global.instrument_show_vsti != 0;
}

void config_set_instrument_show_vsti(bool value) {
  thread_lock lock(config_lock);
  global.instrument_show_vsti = value;
}

// key fade speed
int config_get_key_fade() {
  return global.key_fade;
}

void config_set_key_fade(int value) {
  global.key_fade = value;
}

void config_set_gui_transparency(byte value) {
  global.gui_transparency = value;
  HWND hwnd = gui_get_window();

  if (global.gui_transparency == 255) {
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
  }
  else {
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, global.gui_transparency, LWA_ALPHA);
  }
}

byte config_get_gui_transparency() {
  return global.gui_transparency;
}

// get auto color mode
byte config_get_auto_color() {
  return global.auto_color;
}

// set auto color mode
void config_set_auto_color(byte value) {
  global.auto_color = value;
}

// get preview color mode
uint config_get_preview_color() {
  return global.preview_color;
}

// set preview color mode
void config_set_preview_color(uint value) {
  global.preview_color = clamp_value<int>(value, 0, 100);
}

// get auto color mode
uint config_get_note_display() {
  return global.note_display;
}

// set auto color mode
void config_set_note_display(uint value) {
  global.note_display = value;
}

// get neweset version
uint config_get_update_version() {
  return global.update_version;
}

// set update version
void config_set_update_version(uint version) {
  global.update_version = version;
}