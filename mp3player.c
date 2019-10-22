//#resource "output-address.bin"
//#resource "output.bin"
//#resource "gtmp3.cfg"
//#resource "output.bin.0"
//#resource "output.bin.1"
//#resource "output.bin.2"
//#resource "output.bin.3"
//#resource "output.bin.4"
//#resource "output.bin.5"
//#resource "output.bin.6"
//#resource "output.bin.7"
//#define CFGFILE gtmp3.cfg

//#include <stdlib.h>
#include <string.h>
#include <stdio.h>


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

#define LIST_PAGE_V	23

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

typedef enum
{
  STATE_INIT_PLAY_SCREEN,
  STATE_RUN_PLAY_SCREEN,
  STATE_INIT_LIST_SCREEN,
  STATE_RUN_LIST_SCREEN
};


void __fastcall__ mp3_command(CMD command, unsigned char param1, unsigned char param2);
void select_tag(short tracknumber);



extern byte cv5000, reg5000, ROM_table[];
#pragma zpsym ("cv5000")
extern byte mp3_tags[];
extern unsigned int mp3_address[];
extern byte mp3_bank[];
extern byte chr_data[];

void update_time(void);
void hex_display(byte value, byte x_position, byte y_position);

byte state = STATE_INIT_PLAY_SCREEN;
byte ppu_buffer[128];
byte time_buffer[16];
byte pad1;
byte spr_id;
unsigned int current_track = 0;
unsigned int previous_track = 0;
unsigned int browse_track = 0;
byte playing = 0;
byte bank_select = 0;
unsigned int tag_data_index;
byte temp;
unsigned int temp16;
unsigned long frame_counter;
unsigned long tag_frame_counter;
byte auto_next = 1;	// start at track zero, auto-inc at beginning
unsigned long sprite_position = 0;
unsigned long sprite_velocity = 0;
unsigned long tag_seconds = 0;
byte i;

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
  
  vram_adr(0x0000);  
  vram_write(chr_data, 0x2000);
}

void __fastcall__ irq_nmi_callback(void)
{
  if (__A__ & 0x80)
    ;	// IRQ
  else	// NMI
  {     
    if (playing)
    {
      update_time();
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
  }
          
}

void main(void)
{
  cv5000 = 0xA0;
  reg5000 = 0xA0;
  setup_graphics();
  
  nmi_set_callback(irq_nmi_callback);

  // infinite loop
  while(1)
  {
    ppu_wait_nmi();
    pad1 = pad_trigger(0);
    
    switch (state)
    {      
      case STATE_INIT_PLAY_SCREEN:
        {
          ppu_wait_nmi();
          ppu_off();
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
          vram_adr(NTADR_A(4,20));
          vram_write("\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02\x02",26);  
                    
          // enable rendering
          ppu_on_all();
          
          select_tag(current_track - 1);
          set_vram_update(time_buffer);
          update_time();
          
          state = STATE_RUN_PLAY_SCREEN;          
          break;
        }

      case STATE_RUN_PLAY_SCREEN:
        {
          if ((pad1 & PAD_RIGHT) || auto_next )
          {
            browse_track = ++current_track;
            select_tag(current_track-1);
            mp3_command(CMD_SELECT_MP3_FOLDER,(current_track >> 8),(current_track & 0xFF));
            ppu_wait_nmi();
            mp3_command(CMD_SELECT_MP3_FOLDER,(current_track >> 8),(current_track & 0xFF));
            playing = 1;
          }
          if (pad1 & PAD_LEFT)
          {
            if (current_track >= 2)
            {        
              browse_track = --current_track;
              select_tag(current_track-1);
              mp3_command(CMD_SELECT_MP3_FOLDER,(current_track >> 8),(current_track & 0xFF));
              ppu_wait_nmi();
              mp3_command(CMD_SELECT_MP3_FOLDER,(current_track >> 8),(current_track & 0xFF));
              playing = 1;
            }
          }
          if (pad1 & PAD_SELECT)
            state = STATE_INIT_LIST_SCREEN;
            //mp3_command(CMD_SHUFFLE_PLAY,0,0);
          if (pad1 & PAD_UP)
            mp3_command(CMD_VOLUME_UP,0,0);
          if (pad1 & PAD_DOWN)
            mp3_command(CMD_VOLUME_DOWN,0,0);
          if (pad1 & PAD_A)
          {      
            mp3_command(CMD_PLAY,1,1);
            playing = 1;
          }
          if (pad1 & PAD_B)
          {
            mp3_command(CMD_PAUSE,2,2);
            playing = 0;
          }


          spr_id = 0;

          spr_id = oam_spr((sprite_position >> 16),0x9F,0x01,2,spr_id);

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
          break;
        }
      
      case STATE_INIT_LIST_SCREEN:
        {
          unsigned int index;
          set_vram_update(NULL);
          ppu_wait_nmi();
          ppu_off();
          
          oam_clear();
          
          temp16 = 0x2042;
          temp = browse_track-1;
          
          for (i = 0; i < 23; i++)
          {
            index = mp3_address[temp];  
            bank_select = mp3_bank[temp];
            ROM_table[bank_select] = bank_select;  // UNROM bus conflict

            cv5000 &= 0xF0;			// GTROM
            cv5000 |= bank_select;
            reg5000 = cv5000;

            memset(ppu_buffer, 0, 32);
            strcpy((unsigned char *)ppu_buffer, ((unsigned char *) index));
            vram_adr(temp16);
            vram_write((unsigned char *)ppu_buffer, 30);
            temp16 += 0x20;
            ++temp;
          }
          vram_adr(0x0000);
          ppu_wait_nmi();
          ppu_on_all();

          state = STATE_RUN_LIST_SCREEN;
          break;      
        }
        
      case STATE_RUN_LIST_SCREEN:
        {
          if (pad1 & PAD_SELECT)
          {
            ppu_wait_nmi();
            ppu_off();
            vram_adr(0x2000);
            vram_fill(0x00,0x400);
            ppu_wait_nmi();
            ppu_on_all();
            state = STATE_INIT_PLAY_SCREEN;                    
          }
          if (pad1 & PAD_LEFT)
          {
            if (browse_track < LIST_PAGE_V)
            {
              browse_track = 1;
              state = STATE_INIT_LIST_SCREEN;
            }
            else
            {
              browse_track -= LIST_PAGE_V;
              state = STATE_INIT_LIST_SCREEN;
            }
            
          }
          if (pad1 & PAD_RIGHT)
          {
            browse_track += LIST_PAGE_V;
            state = STATE_INIT_LIST_SCREEN;
          }
          break;
        }
        
      default:
        break;
    }    
  }
}


#define print_tag(line, length) \
  {    \
  \
    set_vram_update(NULL); \
    memset(ppu_buffer, 0, 34); \
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
  
  bank_select = mp3_bank[tracknumber];
  ROM_table[bank_select] = bank_select;  // UNROM bus conflict
  
  cv5000 &= 0xF0;			// GTROM
  cv5000 |= bank_select;
  reg5000 = cv5000;
    
  print_tag(15,30);
  index += text_length;
  print_tag(16,30);
  index += text_length;
  print_tag(17,30);
  index += text_length;  
  print_tag(18,4);  
  index += 4;
  tag_data_index = index;
  
  memcpy(&tag_seconds, &mp3_tags[(tag_data_index - 0x8000) + TDB_SECONDS],4);

  set_vram_update(NULL);
  memset(ppu_buffer, 0, 34);
  ppu_buffer[0] = MSB(NTADR_A(7,22))|NT_UPD_HORZ;
  ppu_buffer[1] = LSB(NTADR_A(7,22));
  ppu_buffer[2] = 20;
  sprintf(&ppu_buffer[3], "0:00:00 / %1.1ld:", tag_seconds / 3600);
  sprintf(&ppu_buffer[15], "%02.2ld:", tag_seconds / 60);
  sprintf(&ppu_buffer[18], "%02.2ld", tag_seconds % 60);
  ppu_buffer[3 + 20] = NT_UPD_EOF;
  set_vram_update(ppu_buffer);
  ppu_wait_nmi();
  set_vram_update(NULL);

  if (previous_track != tracknumber)
  {
    memcpy(&tag_frame_counter, &mp3_tags[(tag_data_index - 0x8000) + TDB_NTSC_FRAMES], 4);
    frame_counter = 0;  

    memcpy(&sprite_velocity, &mp3_tags[(tag_data_index - 0x8000) + TDB_NTSC_BAR_192], 2);
    sprite_velocity &= 0x0000FFFF;
    sprite_position = 0x00200000;
    
    memcpy(&time_buffer[0],&ppu_buffer[0],10);
    time_buffer[2] = 7;
    time_buffer[10] = NT_UPD_EOF;
  }    
  previous_track = tracknumber;         
}

void update_time()
{
  static byte timer = 0;
  if (++timer != 60)
    return;
  timer = 0;
  set_vram_update(NULL);
  
  if (++time_buffer[9] == 0x3A)
  {
    time_buffer[9] = '0';
    if (++time_buffer[8] == '6')
    {
      time_buffer[8] = '0';
      if (++time_buffer[6] == 0x3A)
      {
        time_buffer[6] = '0';
        if (++time_buffer[5] == '6')
        {
          time_buffer[5] = '0';
          if (++time_buffer[3] == 0x3A)
          {
            time_buffer[3] = '0';
          }
        }
      }
    }
  }
  if (state == STATE_RUN_PLAY_SCREEN)
    set_vram_update(time_buffer);  
}
  
void hex_display(byte value, byte x_position, byte y_position)
{
    temp = (value >> 4);
    spr_id = oam_spr(x_position,y_position,hex_table[temp],2,spr_id);
    temp = (value & 0x0F);
    spr_id = oam_spr((x_position + 8),y_position,hex_table[temp],2,spr_id);
}

//#link "mp3.s"
