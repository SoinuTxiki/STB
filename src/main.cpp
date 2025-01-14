#include "pch.h"
#include "keyboard.h"
#include "display.h"
#include "config.h"
#include "gui.h"
#include "song.h"
#include "export_mp4.h"
#include "language.h"
#include "update.h"

#ifdef _DEBUG
int main()
#else
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
#endif
{
  //SetThreadUILanguage(LANG_ENGLISH);

  // initialize com
  if (FAILED(CoInitialize(NULL)))
    return 1;

  // config init
  if (config_init()) {
    MessageBox(NULL, lang_get_last_error(), APP_NAME, MB_OK);
    return 1;
  }

  // init language
  lang_init();

  // init gui
  if (gui_init()) {
    MessageBox(NULL, lang_get_last_error(), APP_NAME, MB_OK);
    return 1;
  }

  // init display
  if (display_init(gui_get_window())) {
    MessageBox(NULL, lang_get_last_error(), APP_NAME, MB_OK);
    return 1;
  }

  // intialize keyboard
  if (keyboard_init()) {
    MessageBox(NULL, lang_get_last_error(), APP_NAME, MB_OK);
    return 1;
  }

  // show gui
  gui_show();

  // load default config
  config_load("STB.cfg");

  // check for update
#ifndef _DEBUG
  update_check_async();
#endif

  
  //XAM init MIDI
	int result = -1;
	result = midi_open_output("IX10"); //try to open IX10

  MSG msg;
  while (GetMessage(&msg, NULL, NULL, NULL))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  config_save("STB.cfg");

  // shutdown keyboard
  keyboard_shutdown();

  // shutdown config
  config_shutdown();

  // shutdown display
  display_shutdown();

  // shutdown COM
  CoUninitialize();

  return 0;
}