//#resource "output-bank.bin"
//#resource "output-address.bin"
//#resource "output.bin"
//#resource "gtmp3.cfg"
//#define CFGFILE gtmp3.cfg

//#include <stdlib.h>
#include <string.h>


// include NESLIB header
#include "neslib.h"

// include CC65 NES Header (PPU)
#include <nes.h>

// link the pattern table into CHR ROM
//#link "chr_generic.s"

// offsets inside the binary data section of tag structure
#define TDB_SECONDS		0
#define	TDB_NTSC_FRAMES		4
#define TDB_NTSC_BAR_128 	8
#define TDB_NTSC_BAR_192 	10
#define TDB_PAL_FRAMES 		12
#define TDB_PAL_BAR_128 	16
#define TDB_PAL_BAR_192 	18

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
void select_tag(short tracknumber);



extern byte cv5000, reg5000;
#pragma zpsym ("cv5000")
extern byte mp3_tags[];
extern unsigned int mp3_address[];
extern byte mp3_bank[];

void hex_display(byte value, byte x_position, byte y_position);


byte ppu_buffer[128];
byte pad1;
byte spr_id;
unsigned int current_track = 1;
byte playing = 1;
unsigned int tag_data_index;
byte temp;
unsigned long frame_counter;
unsigned long tag_frame_counter;
byte auto_next = 0;
unsigned long sprite_position = 0;
unsigned long sprite_velocity = 0;

const char hex_table[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8',
                           '9', 'A', 'B', 'C', 'D', 'E', 'F' };
  

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
  reg5000 = 0xA0;
  setup_graphics();

  // draw message  
  vram_adr(NTADR_A(2,3));
  vram_write("MI MEDIA PLAYER",15);
  vram_adr(NTADR_A(2,4));
  vram_write(__DATE__ " - "__TIME__, 22);
  vram_adr(NTADR_A(2,6));
  vram_write("L/R to change tracks",20);
  vram_adr(NTADR_A(2,7));
  vram_write("U/D to change volume",20);
  vram_adr(NTADR_A(2,8));
  vram_write("Select to enter shuffle mode",28);
  vram_adr(NTADR_A(4,12));
  vram_write("\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\0x02",25);
  
  
  // enable rendering
  ppu_on_all();
  
  // start playing
  select_tag(0);
  mp3_command(CMD_SELECT_MP3_FOLDER,0,1);  

  // infinite loop
  while(1)
  {
    ppu_wait_nmi();
    pad1 = pad_trigger(0);    
    
    if ((pad1 & PAD_RIGHT) || auto_next )
    {
      ++current_track;
      select_tag(current_track-1);
      mp3_command(CMD_SELECT_MP3_FOLDER,(current_track >> 8),(current_track & 0xFF));
      ppu_wait_nmi();
      mp3_command(CMD_SELECT_MP3_FOLDER,(current_track >> 8),(current_track & 0xFF));
    }
    if (pad1 & PAD_LEFT)
    {
      if (current_track >= 2)
      {        
        --current_track;
        select_tag(current_track-1);
        mp3_command(CMD_SELECT_MP3_FOLDER,(current_track >> 8),(current_track & 0xFF));
        ppu_wait_nmi();
        mp3_command(CMD_SELECT_MP3_FOLDER,(current_track >> 8),(current_track & 0xFF));
      }
    }
    if (pad1 & PAD_SELECT)
      mp3_command(CMD_SHUFFLE_PLAY,0,0);    
    if (pad1 & PAD_UP)
      mp3_command(CMD_VOLUME_UP,0,0);
    if (pad1 & PAD_DOWN)
      mp3_command(CMD_VOLUME_DOWN,0,0);
    if (pad1 & PAD_A)
      mp3_command(CMD_PLAY,1,1);
    if (pad1 & PAD_B)
    {
      mp3_command(CMD_PAUSE,2,2);
    }
    
    if (playing)
    {
      sprite_position += sprite_velocity;
      ++frame_counter;
      if (frame_counter == tag_frame_counter)
      {
        auto_next = 1;
        frame_counter = 0;
      }
      else
      {
        auto_next = 0;
      }
    }
    
    spr_id = 0;

    spr_id = oam_spr((sprite_position >> 16),95,0x01,2,spr_id);
    
    /*
    hex_display((sprite_velocity >> 8),0x20,0xa0);
    hex_display((sprite_velocity & 0xFF),0x10,0xa0);
    hex_display(mp3_tags[tag_data_index-0x8000],0x38,0xa0);
    hex_display((sprite_position),0x40,0xB0);
    hex_display((sprite_position >> 8),0x30,0xB0);
    hex_display((sprite_position >> 16),0x20,0xB0);
    hex_display((sprite_position >> 24),0x10,0xB0);
    hex_display((frame_counter),0x40,0xB8);
    hex_display((frame_counter >> 8),0x30,0xB8);
    hex_display((frame_counter >> 16),0x20,0xB8);
    hex_display((frame_counter >> 24),0x10,0xB8);
    */
    oam_hide_rest(spr_id);
    
  }
}


#define print_tag(line, length) \
  {    \
    memset(ppu_buffer, 0, 34); \
    set_vram_update(NULL); \
    ppu_buffer[0] = MSB(NTADR_A(2,line))|NT_UPD_HORZ; \
    ppu_buffer[1] = LSB(NTADR_A(2,line)); \
    text_length = strlen((unsigned char *)index) + 1; \
    ppu_buffer[2] = length; \
    strcpy((unsigned char *)ppu_buffer+3, ((unsigned char *) index)); \
    ppu_buffer[3 + length] = NT_UPD_EOF; \
    set_vram_update(ppu_buffer); \
    ppu_wait_nmi(); \
  };

void select_tag(short tracknumber)
{    
  byte text_length;
  unsigned int index;  
  
  index = mp3_address[tracknumber];  
    
  print_tag(15,30);
  index += text_length;
  print_tag(16,30);
  index += text_length;
  print_tag(17,30);
  index += text_length;  
  print_tag(18,4);  
  index += 4;
  tag_data_index = index;
  memcpy(&tag_frame_counter, &mp3_tags[(tag_data_index - 0x8000) + TDB_NTSC_FRAMES], 4);
  frame_counter = 0;  
  memcpy(&sprite_velocity, &mp3_tags[(tag_data_index - 0x8000) + TDB_NTSC_BAR_192], 2);
  sprite_velocity &= 0x0000FFFF;

  sprite_position = 0x00200000;
}
  
void hex_display(byte value, byte x_position, byte y_position)
{
    temp = (value >> 4);
    spr_id = oam_spr(x_position,y_position,hex_table[temp],2,spr_id);
    temp = (value & 0x0F);
    spr_id = oam_spr((x_position + 8),y_position,hex_table[temp],2,spr_id);
}

//#link "mp3.s"
