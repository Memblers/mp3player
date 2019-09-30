
//#include <stdlib.h>
//#include <string.h>


// include NESLIB header
#include "neslib.h"

// include CC65 NES Header (PPU)
#include <nes.h>

// link the pattern table into CHR ROM
//#link "chr_generic.s"


typedef enum
{
  CMD_NEXT_SONG		= 0x01,
  CMD_PREVIOUS_SONG,
  CMD_TRACK_SELECT,
  CMD_VOLUME_UP,
  CMD_VOLUME_DOWN,
  CMD_SELECT_VOLUME,
  CMD_SELECT_EQ,
  CMD_SELECT_LOOPED_TRACK,
  CMD_SELECT_TF,
  CMD_SLEEP_MODE,
  CMD_WAKE_UP,
  CMD_CHIP_RESET,
  CMD_PLAY,
  CMD_PAUSE,
  CMD_SELECT_FILE,
  CMD_LOOP			= 0x11,
  CMD_SELECT_MP3_FOLDER	= 0x12,
  CMD_ADVERT			= 0x13,
  CMD_STOP			= 0x16,
  CMD_PLAY_FOLDER_LOOP,
  CMD_SHUFFLE_PLAY,
  CMD_SINGLE_LOOP_ENABLE,
  CMD_PLAY_WITH_VOLUME	= 0x22
  } CMD;

void __fastcall__ mp3_command(CMD command, unsigned char param1, unsigned char param2);

extern byte cv5000;
#pragma zpsym ("cv5000")

byte pad1;
byte spr_id;
  

/*{pal:"nes",layout:"nes"}*/
const char PALETTE[32] = { 
  0x03,			// screen color

  0x11,0x30,0x33,0x00,	// background palette 0
  0x1C,0x20,0x2C,0x00,	// background palette 1
  0x00,0x10,0x20,0x00,	// background palette 2
  0x06,0x16,0x26,0x00,   // background palette 3

  0x16,0x35,0x24,0x00,	// sprite palette 0
  0x00,0x37,0x25,0x00,	// sprite palette 1
  0x0D,0x2D,0x3A,0x00,	// sprite palette 2
  0x0D,0x27,0x2A	// sprite palette 3
};

// setup PPU and tables
void setup_graphics() {
  // clear sprites
  oam_clear();
  // set palette colors
  pal_all(PALETTE);
}

void main(void)
{
  cv5000 = 0xA0;
  setup_graphics();

  // draw message  
  vram_adr(NTADR_A(2,3));
  vram_write("MI MEDIA PLAYER",15);
  vram_adr(NTADR_A(2,4));
  vram_write(__DATE__ " - "__TIME__, 22);
  // enable rendering
  ppu_on_all();
  // infinite loop
  while(1)
  {
    ppu_wait_nmi();
    pad1 = pad_trigger(0);    
    
    if (pad1 & PAD_RIGHT)
      mp3_command(CMD_NEXT_SONG,0,0);    
    if (pad1 & PAD_LEFT)    
      mp3_command(CMD_PREVIOUS_SONG,0,0);    
    if (pad1 & PAD_SELECT)
      mp3_command(CMD_SHUFFLE_PLAY,0,0);    
    if (pad1 & PAD_UP)
      mp3_command(CMD_VOLUME_UP,0,0);
    if (pad1 & PAD_DOWN)
      mp3_command(CMD_VOLUME_DOWN,0,0);
    if (pad1 & PAD_A)
      mp3_command(CMD_SELECT_EQ,1,1);
    if (pad1 & PAD_B)
    {
      spr_id = 0;
      mp3_command(CMD_SELECT_EQ,2,2);
      spr_id = oam_spr(0x18,0x90,'C',2,spr_id);
      mp3_command(CMD_SELECT_EQ,2,2);        
    }     
    
  }
}

//#link "mp3.s"
