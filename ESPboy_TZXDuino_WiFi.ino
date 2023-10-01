/*
                                      TZXduino
                               Written and tested by
                            Andrew Beer, Duncan Edwards
                            www.facebook.com/Arduitape/

                Designed for TZX files for Spectrum (and more later)
                Load TZX files onto an SD card, and play them directly
                without converting to WAV first!

                Directory system allows multiple layers,  to return to root
                directory ensure a file titles ROOT (no extension) or by
                pressing the Menu Select Button.

                Written using info from worldofspectrum.org
                and TZX2WAV code by Francisco Javier Crespo

 *              ***************************************************************
                Menu System:
                  TODO: add ORIC and ATARI tap support, clean up code, sleep

                V1.0
                  Motor Control Added.
                  High compatibility with Spectrum TZX, and Tap files
                  and CPC CDT and TZX files.

                  V1.32 Added direct loading support of AY files using the SpecAY loader
                  to play Z80 coded files for the AY chip on any 128K or 48K with AY
                  expansion without the need to convert AY to TAP using FILE2TAP.EXE.
                  Download the AY loader from http://www.specay.co.uk/download
                  and load the LOADER.TAP AY file loader on your spectrum first then
                  simply select any AY file and just hit play to load it. A complete
                  set of extracted and DEMO AY files can be downloaded from
                  http://www.worldofspectrum.org/projectay/index.htm
                  Happy listening!

                  V1.8.1 TSX support for MSX added by Natalia Pujol

                  V1.8.2 Percentage counter and timer added by Rafael Molina Chesserot along with a reworking of the OLED1306 library.
                  Many memory usage improvements as well as a menu for TSX Baud Rates and a refined directory controls.

                  V1.8.3 PCD8544 library changed to use less memory. Bitmaps added and Menu system reduced to a more basic level.
                  Bug fixes of the Percentage counter and timer when using motor control/

                  V1.11 Added unzipped UEF playback and turbo UEF to the Menu thatks to the kind work by kernal@kernalcrash.com
                  Supports Gunzipped UEFs only.

                  v1.13 HQ.UEF support added by Rafael Molina Chesserot of Team MAXDuino
                  v1.13.1 Removed digitalWrite in favour of a macro suggested by Ken Forster
                  Some MSX games will now work at higher Baudrates than before.
                  v1.13.2 Added a fix to some Gremlin Loaders which reverses the polarity of the block.
                  New menu Item added. "Gremlin Loader" turned on will allow Samurai Trilogy and Footballer of the Year II
                  CDTs to load properly.

                  1.14 ID15 code adapted from MAXDuino by Rafael Molina Chasserot.
                  Not working 100% with CPC Music Loaders but will work with other ID15 files.

                  1.14.2 Added an ID15 switch to Menu as ID15 being enabled was stopping some files loading properly.
                  1.14.3 Removed the switch in favour of an automatic system of switching ID15 routine on and off.

                  1.15 Added support for the Surenoo RGB Backlight LCD using an adapted version of the Grove RGBLCD library.
                       Second counter not currently working. Also some memory saving tweaks.

                  1.15.3 Adapted the MAXDuino ID19 code and TurboMode for ZX80/81
                         Also added UEF Chunk 117 which allows for differing baudrates in BBC UEFs.
                         Added a Spectrum Font for OLED 1306 users converted by Brendan Alford
                         Added File scrolling by holding up or down buttons. By Brendan Alford.

                  1.16 Fixed a bug that was stopping Head Over Heels (and probably others)loading on +2 Spectrum. Seems to have made
                       ZX80/81 playback alot more stable too.

                  1.17 Added ORIC TAP file playback from the Maxduino team.

                  1.18 Added a delay to IDCHUNKEOF before the stopFile(); as it was cutting off the last few milliseconds of playback on some UEF files.
                       Jungle Journey for Acorn Electron is now working.


*/

//IMPORTANT note: SPIFFS version somehow introduced a gap at 180 seconds into any file, breaking the loading. Moving to LittleFS fixed it
//IMPORTANT note2: upload TZX files to Little_FS using "Arduino IDE Little_FS uploader" plugin or using WiFi file uploading mode inside the TZX Duino

//01.10.23 file uploader upgraded to full file manager (you can upload/download/delete files on LittleFS - ESP8266 internal flash drive)
//11.04.23 minor fixes for percentage display and clicks at end of a file, moving to LittleFS, WiFi file transfer
//10.04.23 quick and dirty port to ESPboy by shiru8bit

#include "lib/ESPboyInit.h"
#include "lib/ESPboyInit.cpp"
//#include "lib/ESPboyTerminalGUI.h"
//#include "lib/ESPboyTerminalGUI.cpp"
//#include "lib/ESPboyOTA2.h"
//#include "lib/ESPboyOTA2.cpp"
#include <LittleFS.h>

ESPboyInit myESPboy;
ESPboyLED myESPboyLED;
//ESPboyTerminalGUI *terminalGUIobj = NULL;
//ESPboyOTA2 *OTA2obj = NULL;

#include "glcdfont.c"

#include "Timers.h"
#include "TZXDuino.h"
#include "version.h"

#define OUTPUT_PIN  D6
#define SPEAKER_PIN D3

volatile bool SoundEnable = true;

inline void LowWrite()
{
  GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, _BV(OUTPUT_PIN));
  if (SoundEnable) GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, _BV(SPEAKER_PIN));
}

inline void HighWrite()
{
  GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, _BV(OUTPUT_PIN));
  if (SoundEnable) GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, _BV(SPEAKER_PIN));
}

const int maxFilenameLength = 32;
const int nMaxPrevSubDirs = 16;

Dir dir;
File entry;

char fileName[maxFilenameLength + 1];     //Current filename
int fileNameLen;
volatile unsigned long filesize;             // filesize used for dimensioning AY files
byte pauseOn = 0;                   //Pause state

char PlayBytes[17];


#define FILE_HEIGHT 14

int file_cursor = 0;

uint8_t pad_state = 0;
uint8_t pad_state_prev = 0;
uint8_t pad_state_t = 0;


#define CFG_BYTE_1200_BAUD        0x01
#define CFG_BYTE_2400_BAUD        0x02
#define CFG_BYTE_3600_BAUD        0x04
#define CFG_BYTE_SPEAKER_ENABLE   0x10
#define CFG_BYTE_PAUSE_AT_START   0x20
#define CFG_BYTE_FLIP_POLARITY    0x40
#define CFG_BYTE_UEF_TURBO        0x80

unsigned char config_byte = CFG_BYTE_1200_BAUD | CFG_BYTE_SPEAKER_ENABLE;

#define CFG_FILENAME    "config.cfg"

bool play_file_active = false;

bool file_browser_ext(const char* name)
{
  while (1)
  {
    if (*name++ == '.') break;
  }

  if (strcasecmp(name, "tap") == 0) return true;
  if (strcasecmp(name, "tzx") == 0) return true;
  if (strcasecmp(name, "p") == 0) return true;
  if (strcasecmp(name, "o") == 0) return true;
  if (strcasecmp(name, "ay") == 0) return true;
  if (strcasecmp(name, "uef") == 0) return true;

  return false;
}



int check_key()
{
  pad_state_prev = pad_state;
  pad_state = 0;
  uint8_t pad_read = myESPboy.mcp.readGPIOAB()&255;

  for (int i = 0; i < 8; i++)
    if (!(pad_read & (1 << i))) pad_state |= (1 << i);

  pad_state_t = pad_state ^ pad_state_prev & pad_state;

  return pad_state;
}



void drawCharFast(int x, int y, int c, int16_t color, int16_t bg)
{
  static uint16_t buf[6 * 8];

  for (int i = 0; i < 6; ++i)
  {
    int line = i < 5 ? pgm_read_byte(&font[c * 5 + i]) : 0;

    for (int j = 0; j < 8; ++j)
    {
      buf[j * 6 + i] = ((line & 1) ? color : bg);
      line >>= 1;
    }
  }

  myESPboy.tft.pushImage(x, y, 6, 8, buf);
}



void printFast(int x, int y, const char* str, int16_t color)
{
  char c;

  while (1)
  {
    c = *str++;

    if (!c) break;

    drawCharFast(x, y, c, color, 0);
    x += 6;
  }
}



int file_browser(String path, const char* header, char* filename, int filename_len)
{
  char name[19 + 1];

  memset(filename, 0, filename_len);
  memset(name, 0, sizeof(name));

  myESPboy.tft.fillScreen(TFT_BLACK);

  fs::Dir dir = LittleFS.openDir(path);

  int file_count = 0;

  while (dir.next())
  {
    fs::File entry = dir.openFile("r");

    bool filter = file_browser_ext(entry.name());

    entry.close();

    if (filter) ++file_count;
  }

  printFast(4, 4, (char*)header, TFT_GREEN);
  myESPboy.tft.fillRect(0, 12, 128, 1, TFT_WHITE);

  bool change = true;
  int frame = 0;
  int ret = 0;

  while (1)
  {
    if (change)
    {
      if(file_count>0)
      {
      int pos = file_cursor - FILE_HEIGHT / 2;

      if (pos > file_count - FILE_HEIGHT) pos = file_count - FILE_HEIGHT;
      if (pos < 0) pos = 0;

      fs::Dir dir = LittleFS.openDir(path);
      int i = pos;

      while (dir.next())
      {
        fs::File entry = dir.openFile("r");

        bool filter = file_browser_ext(entry.name());

        entry.close();

        if (!filter) continue;

        --i;
        if (i <= 0) break;
      }

      int sy = 14;
      i = 0;

      while (1)
      {
        fs::File entry = dir.openFile("r");

        bool filter = file_browser_ext(entry.name());

        if (filter)
        {
          const char* str = entry.name();

          for (int j = 0; j < sizeof(name) - 1; ++j)
          {
            if (*str != 0 && *str != '.') name[j] = *str++;
            else name[j] = ' ';
          }

          printFast(8, sy, name, TFT_WHITE);

          drawCharFast(2, sy, ' ', TFT_WHITE, TFT_BLACK);

          if (pos == file_cursor)
          {
            strncpy(filename, entry.name(), filename_len);

            if (frame & 32) drawCharFast(2, sy, 0xdb, TFT_WHITE, TFT_BLACK);
          }
        }

        entry.close();

        if (!dir.next()) break;

        if (filter)
        {
          sy += 8;
          ++pos;
          ++i;
          if (i >= FILE_HEIGHT) break;
        }
      }
      }
      else
      {
        printFast(24, 60, "No files found", TFT_RED);
      }
      change = false;
    }

    check_key();

    if (pad_state_t & PAD_UP)
    {
      --file_cursor;

      if (file_cursor < 0) file_cursor = file_count - 1;

      change = true;
      frame = 32;
    }

    if (pad_state_t & PAD_DOWN)
    {
      ++file_cursor;

      if (file_cursor >= file_count) file_cursor = 0;

      change = true;
      frame = 32;
    }

    if (pad_state_t & PAD_ACT)
    {
      if(file_count>0)
      {
      ret = 0;
      break;
      }
    }

    if (pad_state_t & PAD_ESC)
    {
      ret = 1;
      break;
    }

    delay(1);

    ++frame;

    if (!(frame & 31)) change = true;
  }

  return ret;
}


void stopFile()
{
  play_file_active = false;
}



void play_file(const char* filename)
{
  char str[21];

  myESPboy.myLED.setRGB(0, 5, 0);
  myESPboy.myLED.on();

  myESPboy.tft.fillScreen(TFT_BLACK);
  myESPboy.tft.fillRect(0, 24, 128, 1, TFT_WHITE);
  printFast(4, 32, filename, TFT_WHITE);
  printFast(8, 104, "Arrow to mute sound", 0x7bef);

  config_load();

  TZXSetup();

  strncpy(fileName, filename, sizeof(fileName));
  fileNameLen = strlen(fileName);

  entry.close();
  entry = LittleFS.open(fileName, "r");
  filesize = entry.size();
  ayblklen = filesize + 3;  // add 3 file header, data byte and chksum byte to file length
  currpct = 100;

  Serial.println(fileName);
  Serial.println(filesize);

  TZXPlay();

  for (int i = 0; i < buffsize; ++i) TZXLoop(); //prebuffering

  //lcdsegs = 0;

  if (PauseAtStart == true)
  {
    pauseOn = 1;
    TZXPause();
  }
  else
  {
    pauseOn = 0;
  }

  int prev_newpct = -1;
  unsigned long bytesRead_prev = -1;
  int frame = 0;
  bool change = true;

  play_file_active = true;

  char buf[32];

  while (play_file_active)
  {
    if (change)
    {
      change = true;

      if (!pauseOn)
      {
        printFast(4, 16, "Playing", TFT_YELLOW);
      }
      else
      {
        printFast(4, 16, "Paused  ", TFT_YELLOW);
      }
    }

    if (newpct != prev_newpct)
    {
      prev_newpct = newpct;

      sprintf(buf, "%3.3i%/100%%", newpct);
      printFast(4, 48, buf, TFT_WHITE);
    }

    if (bytesRead != bytesRead_prev)
    {
      bytesRead_prev = bytesRead;

      sprintf(buf, "%i/%i%b", bytesRead, filesize);
      printFast(4, 64, buf, TFT_WHITE);
    }

    check_key();

    if (pad_state_t&PAD_ACT)
    {
      pauseOn ^= 1;
      TZXPause();
    }

    if (pad_state_t&PAD_ESC)
    {
      play_file_active = false;
    }

    if (pad_state_t&(PAD_LEFT | PAD_RIGHT | PAD_UP | PAD_DOWN))
    {
      SoundEnable ^= true;
    }

    delay(1000 / 60);
  }

  TZXStop();
  LowWrite();

  myESPboy.myLED.off();
}



void config_load()
{
  File f = LittleFS.open(CFG_FILENAME, "r");

  if (f)
  {
    if (f.size() == sizeof(config_byte))
    {
      f.readBytes((char*)&config_byte, sizeof(config_byte));
    }

    f.close();
  }

  uefTurboMode = config_byte & CFG_BYTE_UEF_TURBO ? true : false;
  FlipPolarity = config_byte & CFG_BYTE_FLIP_POLARITY ? true : false;
  PauseAtStart = config_byte & CFG_BYTE_PAUSE_AT_START ? true : false;
  SoundEnable = config_byte & CFG_BYTE_SPEAKER_ENABLE ? true : false;

  if (config_byte & CFG_BYTE_1200_BAUD) BAUDRATE = 1200;
  if (config_byte & CFG_BYTE_2400_BAUD) BAUDRATE = 2400;
  if (config_byte & CFG_BYTE_3600_BAUD) BAUDRATE = 3600;
}



void config_save()
{
  File f = LittleFS.open(CFG_FILENAME, "w");

  if (f)
  {
    f.write((char*)&config_byte, sizeof(config_byte));

    f.close();
  }
}



void config_menu()
{
  config_load();

  myESPboy.tft.fillScreen(TFT_BLACK);

  printFast(4, 4, "Configuration", TFT_YELLOW);
  myESPboy.tft.fillRect(0, 12, 128, 1, TFT_WHITE);

  int option_cursor = 0;
  bool change = true;
  int frame = 0;
  unsigned char prev_config_byte = config_byte;

  while (1)
  {
    if (change)
    {

      int sy = 14;

      for (int i = 0; i < 6; ++i)
      {
        drawCharFast(2, sy, ' ', TFT_WHITE, TFT_BLACK);

        if ((i == option_cursor) && (frame & 32)) drawCharFast(2, sy, 0xdb, TFT_WHITE, TFT_BLACK);

        switch (i)
        {
          case 0:
            printFast(8, sy, "WiFi transfer", TFT_WHITE);
            break;
          case 1:
            printFast(8, sy, "Baud Rate", TFT_WHITE);
            if (config_byte & CFG_BYTE_1200_BAUD) printFast(100, sy, "1200", TFT_WHITE);
            if (config_byte & CFG_BYTE_2400_BAUD) printFast(100, sy, "2400", TFT_WHITE);
            if (config_byte & CFG_BYTE_3600_BAUD) printFast(100, sy, "3600", TFT_WHITE);
            break;
          case 2:
            printFast(8, sy, "Pause at start", TFT_WHITE);
            printFast(108, sy, (config_byte & CFG_BYTE_PAUSE_AT_START) ? " On" : "Off", TFT_WHITE);
            break;
          case 3:
            printFast(8, sy, "Flip polarity", TFT_WHITE);
            printFast(108, sy, (config_byte & CFG_BYTE_FLIP_POLARITY) ? " On" : "Off", TFT_WHITE);
            break;
          case 4:
            printFast(8, sy, "UEF turbo mode", TFT_WHITE);
            printFast(108, sy, (config_byte & CFG_BYTE_UEF_TURBO) ? " On" : "Off", TFT_WHITE);
            break;
          case 5:
            printFast(8, sy, "Sound speaker", TFT_WHITE);
            printFast(108, sy, (config_byte & CFG_BYTE_SPEAKER_ENABLE) ? " On" : "Off", TFT_WHITE);
            break;
        }

        sy += 8;
      }

      change = false;
    }

    check_key();

    if (pad_state_t & PAD_UP)
    {
      --option_cursor;

      if (option_cursor < 0) option_cursor = 5;

      change = true;
      frame = 32;
    }

    if (pad_state_t & PAD_DOWN)
    {
      ++option_cursor;

      if (option_cursor >= 6) option_cursor = 0;

      change = true;
      frame = 32;
    }

    if (pad_state_t&PAD_ACT)
    {
      switch (option_cursor)
      {
        case 0: WiFiFileManager(); break;

        case 1:
          {
            unsigned char prev_cfg = config_byte;
            config_byte &= ~(CFG_BYTE_1200_BAUD | CFG_BYTE_2400_BAUD | CFG_BYTE_3600_BAUD);
            if (prev_cfg & CFG_BYTE_1200_BAUD) config_byte |= CFG_BYTE_2400_BAUD; else if (prev_cfg & CFG_BYTE_2400_BAUD) config_byte |= CFG_BYTE_3600_BAUD; else if (prev_cfg & CFG_BYTE_3600_BAUD) config_byte |= CFG_BYTE_1200_BAUD;
          }
          break;

        case 2: config_byte ^= CFG_BYTE_PAUSE_AT_START; break;
        case 3: config_byte ^= CFG_BYTE_FLIP_POLARITY; break;
        case 4: config_byte ^= CFG_BYTE_UEF_TURBO; break;
        case 5: config_byte ^= CFG_BYTE_SPEAKER_ENABLE; break;
      }

      frame = 32;
      change = true;
    }

    if (pad_state_t&PAD_ESC) break;

    delay(1);

    ++frame;

    if (!(frame & 31)) change = true;
  }

  if (prev_config_byte != config_byte) config_save();
}



void setup()
{
  Serial.begin(115200);
  myESPboy.begin("TZX Duino");
  myESPboyLED.begin(&myESPboy.mcp);
  myESPboyLED.off();
/*
  //Check OTA2
  if (myESPboy.getKeys()&PAD_ACT || myESPboy.getKeys()&PAD_ESC) { 
     terminalGUIobj = new ESPboyTerminalGUI(&myESPboy.tft, &myESPboy.mcp);
     OTA2obj = new ESPboyOTA2(terminalGUIobj);
  }
*/
  pinMode(OUTPUT_PIN, OUTPUT);
  LowWrite();
  LittleFS.begin();
  Serial.println("startmsg");
}



void loop(void)
{
  char filename[64];

  file_cursor = 0;

  while (1)
  {
    if (file_browser("/", "ACT:Play    ESC:Menu", filename, sizeof(filename)) == 0)
    {
      play_file(filename);
    }
    else
    {
      config_menu();
    }
  }
}


void printtextF(const char* text, int l)
{ //Print text to screen.
  Serial.println(reinterpret_cast <const __FlashStringHelper *> (text));
}

void printtext(char* text, int l)
{ //Print text to screen.
  Serial.println(text);
}
