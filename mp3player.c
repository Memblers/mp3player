//#resource "output-address.bin"
//#resource "output-bank.bin"
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
//#link "oamfx.s"
//#link "mp3.s"
//#link "chr_generic.s"

#include <string.h>
#include <stdio.h>

// include NESLIB header
#include "neslib.h"

// include CC65 NES Header (PPU)
#include <nes.h>

// offsets inside the binary data section of tag structure
#define TDB_SECONDS		0
#define	TDB_NTSC_FRAMES		4
#define TDB_NTSC_BAR_128 	8
#define TDB_NTSC_BAR_192 	10
#define TDB_PAL_FRAMES 		12
#define TDB_PAL_BAR_128 	16
#define TDB_PAL_BAR_192 	18

#define LIST_PAGE_V	23
#define LIST_TOP	23
#define LIST_BOTTOM	((LIST_PAGE_V) * 8)
#define MAX_TRACK	102
#define MAX_PAGE	MAX_TRACK / LIST_PAGE_V
#define COOLDOWN_LENGTH 30
#define SCROLLER_SPEED	0x000049

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
  STATE_RUN_LIST_SCREEN,
  STATE_INIT_VIS_SCREEN,
  STATE_RUN_VIS_SCREEN
};

void __fastcall__ mp3_command(CMD command, unsigned char param1, unsigned char param2);
void __fastcall__ beep(void);
void __fastcall__ oam_bar_init(void);
void __fastcall__ oam_bar_run(void);
void __fastcall__ vis_stars_init(void);
void __fastcall__ vis_stars_run(void);
void select_tag(short tracknumber);
void select_tag_time(short tracknumber);

extern byte cv5000, reg5000, UNROM_table[];
#pragma zpsym ("cv5000")
extern byte mp3_tags[];
extern unsigned int mp3_address[];
extern byte mp3_bank[];
extern byte chr_data[];
extern byte sine[];

void update_time(void);
void hex_display(byte value, byte x_position, byte y_position);

byte state = STATE_INIT_PLAY_SCREEN;
byte ppu_buffer[128];
byte str_buffer[128];
byte time_buffer[16];
byte pad1 = 0;
byte spr_id = 0;
word cooldown_timer = 0;
byte auto_trigger = 0;
unsigned int current_track = 0;
unsigned int previous_track = 0;
unsigned int browse_track = 0;
byte playing = 0;
byte bank_select = 0;
unsigned int tag_data_index = 0;
byte temp = 0;
unsigned int temp16 = 0;
unsigned long frame_counter = 0;
unsigned long tag_frame_counter = 0;
byte auto_next = 1;	// start at track zero, auto-inc at beginning
unsigned long sprite_position = 0;
unsigned long sprite_velocity = 0;
unsigned long tag_seconds = 0;
byte i;
byte selector_y_position = LIST_TOP;
byte list_page = 0;
unsigned int index;
word h_scroll = 0;
unsigned long h_scroll_ext = 0;

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
      if (frame_counter >= tag_frame_counter)
      {
        auto_next = 1;
        frame_counter = 0;
      }
      else
      {
        auto_next = 0;
      }
    }
    if (cooldown_timer)
      --cooldown_timer;
  }
          
}

void __fastcall__ blank_callback(void)
{
  return;          
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
          vram_write(__DATE__ " - "__TIME__"  V0.9", 28);
          vram_adr(NTADR_A(2,6));
          vram_write("\x1E/\x1F to change tracks",20);
          vram_adr(NTADR_A(2,7));
          vram_write("\x1C/\x1D to change volume",20);
          vram_adr(NTADR_A(2,8));
          vram_write("Select to change screen mode",28);
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

        #define play_command(void)	\
        {	\
              if (cooldown_timer == 0)	\
              {	\
                mp3_command(CMD_SELECT_MP3_FOLDER,(current_track >> 8),(current_track & 0xFF));	\
              	ppu_wait_nmi();\
              	mp3_command(CMD_SELECT_MP3_FOLDER,(current_track >> 8),(current_track & 0xFF));	\
              	playing = 1;	\
                cooldown_timer = COOLDOWN_LENGTH;	\
              }	\
              else	\
              {	\
                auto_trigger = 1;	\
              }	\
	    };
        

      case STATE_RUN_PLAY_SCREEN:
        {
          if (pad1 & PAD_RIGHT)
          {
            if (current_track != MAX_TRACK)
            {
              browse_track = ++current_track;            
              select_tag(current_track-1);
              play_command();              
              if (((browse_track) % LIST_PAGE_V) == 1)
                ++list_page;
            }
          }
          if (pad1 & PAD_LEFT)
          {
            if (current_track >= 2)
            {        
              browse_track = --current_track;
              select_tag(current_track-1);              
              play_command();              
            }
          }
          if (pad1 & PAD_SELECT)
            state = STATE_INIT_LIST_SCREEN;
          if (pad1 & PAD_START)
            mp3_command(CMD_ADVERT,0,4);
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
          spr_id = oam_spr((sprite_position >> 16),0x9F,0x01,2,spr_id);	// progress bar indicator
          oam_hide_rest(spr_id);          
          break;
        }
      
      case STATE_INIT_LIST_SCREEN:
        {          
          set_vram_update(NULL);
          ppu_wait_nmi();
          ppu_off();          
          oam_clear();
          nmi_set_callback(blank_callback);
          
          selector_y_position = (((browse_track - 1) % LIST_PAGE_V) * 8) + LIST_TOP;
          list_page = ((browse_track - 1) / LIST_PAGE_V);
          
          vram_adr(0x2000);
          vram_fill(0x00, 0x400);
          
          temp16 = 0x2062;
          temp = list_page * LIST_PAGE_V;
                             
          for (i = 0; i < 23; i++)
          {
            index = mp3_address[temp];  
            bank_select = mp3_bank[temp];
            UNROM_table[bank_select] = bank_select;  // UNROM bus conflict

            cv5000 &= 0xF0;			// GTROM
            cv5000 |= bank_select;
            reg5000 = cv5000;

            memset(ppu_buffer, 0, 32);
            strncpy((unsigned char *)ppu_buffer, ((unsigned char *) index),29);
            vram_adr(temp16);
            vram_write((unsigned char *)ppu_buffer, 29);
            temp16 += 0x20;
            if (temp >= MAX_TRACK-1)
              break;
            ++temp;
          }
          
          nmi_set_callback(irq_nmi_callback);
          
          oam_bar_init();          
          
          vram_adr(0x0000);
          ppu_wait_nmi();
          ppu_on_all();

          state = STATE_RUN_LIST_SCREEN;
          break;      
        }
        
      case STATE_RUN_LIST_SCREEN:
        {
          if ((pad1 & PAD_SELECT) || (pad1 & PAD_B))
          {
            oam_clear();
            ppu_wait_nmi();
            ppu_off();

            vram_adr(0x2000);
            vram_fill(0x00,0x400);

            ppu_on_all();
            state = STATE_INIT_VIS_SCREEN;
          }
          
          if (pad1 & PAD_LEFT)
          {
            if (list_page == 0)
            {
              browse_track = 1;
              list_page = 0;
              selector_y_position = LIST_TOP;
            }
            else
            {
              browse_track -= LIST_PAGE_V;
              --list_page;
              state = STATE_INIT_LIST_SCREEN;
            }            
          }
          
          if (pad1 & PAD_RIGHT)
          {
            if (list_page < MAX_PAGE)
            {
              browse_track += LIST_PAGE_V;
              if (browse_track > MAX_TRACK)
              {                
                selector_y_position = ((MAX_TRACK % LIST_PAGE_V) * 8) + 15;
                browse_track = MAX_TRACK;
              }
              ++list_page;
              state = STATE_INIT_LIST_SCREEN;
            }
            else	// on last page, move to last track
            {
              selector_y_position = ((MAX_TRACK % LIST_PAGE_V) * 8) + 15;
              browse_track = MAX_TRACK;
            }
          }
          if (pad1 & PAD_DOWN)
          {
            if (browse_track >= MAX_TRACK)
            	;
              
            else if (selector_y_position < LIST_BOTTOM)
            {
              selector_y_position += 8;
              ++browse_track;
            }
          }
	  if (pad1 & PAD_UP)
          {
            if (selector_y_position > LIST_TOP)
            {
              selector_y_position -= 8;
              --browse_track;
            }
          }
          if ((pad1 & PAD_A) || (pad1 & PAD_START))
          {            
            if (current_track != browse_track)	// don't retrigger same track in list mode
            {              
              current_track = browse_track;
              select_tag(current_track-1);
              play_command();
            }

              ppu_wait_nmi();
              ppu_off();            
              oam_clear();

              vram_adr(0x2000);
              vram_fill(0x00,0x400);

              ppu_on_all();
            
              state = STATE_INIT_PLAY_SCREEN;            
          }

          spr_id = 0;
          oam_bar_run();       
          
          /*

          hex_display((current_track >> 8),0x10,0xD0);
          hex_display((current_track & 0xFF),0x20,0xD0);
          hex_display((browse_track >> 8),0x10,0xD8);
          hex_display((browse_track & 0xFF),0x20,0xD8);

          */
          
          oam_hide_rest(spr_id);          

          break;
        }
        
      case STATE_INIT_VIS_SCREEN:
        {
          set_rand(frame_counter);
          vis_stars_init();
          
          h_scroll_ext = 0;
          
          
          ppu_wait_nmi();
          ppu_off();                   
          
          index = mp3_address[current_track-1];  
          bank_select = mp3_bank[current_track-1];
          UNROM_table[bank_select] = bank_select;  // UNROM bus conflict

          cv5000 &= 0xF0;			// GTROM
          cv5000 |= bank_select;
          reg5000 = cv5000;
          
          memset(str_buffer, 0, 128);
          strncpy((unsigned char *)str_buffer, ((unsigned char *) index),64);

          vram_adr(0x2600);
          vram_write(&str_buffer[0], 32);
          vram_adr(0x2200);
          vram_write(&str_buffer[0x20], 32);
          
          ppu_wait_nmi();
          ppu_on_all();   
          state = STATE_RUN_VIS_SCREEN;
          break;
        }
      case STATE_RUN_VIS_SCREEN:
        {
          if ((pad1 & PAD_SELECT) || (pad1 & PAD_B))
          {

            oam_clear();
            ppu_wait_nmi();
            ppu_off();

            vram_adr(0x2000);
            vram_fill(0x00,0x0800);
            scroll(0,0);            

            ppu_on_all();
            
            state = STATE_INIT_PLAY_SCREEN;
            break;
          }
          
          h_scroll_ext += SCROLLER_SPEED; 
          scroll((h_scroll_ext >> 8),0);

          vis_stars_run();
          break;
        }
      default:
        break;        
    }	// end of - switch (state)
    
    if (auto_next)
    {
      if (current_track != MAX_TRACK)
      {
        browse_track = ++current_track;
        switch (state)
        {
          case STATE_RUN_PLAY_SCREEN:
            {
              select_tag(current_track-1);
              break;
            }
	  case STATE_INIT_VIS_SCREEN:
          case STATE_RUN_VIS_SCREEN:
            {
              select_tag_time(current_track-1);
              state = STATE_INIT_VIS_SCREEN;
              break;
            }
          case STATE_INIT_LIST_SCREEN:
          case STATE_RUN_LIST_SCREEN:
            {
              select_tag_time(current_track-1);
              state = STATE_INIT_LIST_SCREEN;
            }
          default:
            {
              select_tag_time(current_track-1);
              state = STATE_INIT_LIST_SCREEN;
              break;
            }
        }
        play_command();
        if (((browse_track) % LIST_PAGE_V) == 1)
          ++list_page;
      }
    }
    if (auto_trigger)
    {
      browse_track = current_track;
      select_tag(current_track-1);
      mp3_command(CMD_SELECT_MP3_FOLDER,(current_track >> 8),(current_track & 0xFF));
      ppu_wait_nmi();
      mp3_command(CMD_SELECT_MP3_FOLDER,(current_track >> 8),(current_track & 0xFF));
      playing = 1;
      cooldown_timer = COOLDOWN_LENGTH;
      auto_trigger = 0;
    }
  }
}


#define print_tag(line, length) \
  {    \
    set_vram_update(NULL); \
    memset(ppu_buffer, 0, 34); \
    ppu_buffer[0] = MSB(NTADR_A(2,line))|NT_UPD_HORZ; \
    ppu_buffer[1] = LSB(NTADR_A(2,line)); \
    text_length = strlen((unsigned char *)index) + 1; \
    ppu_buffer[2] = length; \
    strncpy((unsigned char *)ppu_buffer+3, ((unsigned char *) index), 29); \
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
  UNROM_table[bank_select] = bank_select;  // UNROM bus conflict
  
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

void select_tag_time(short tracknumber)
{  
  byte text_length;
  unsigned int index;  
  
  index = mp3_address[tracknumber];
  
  bank_select = mp3_bank[tracknumber];
  UNROM_table[bank_select] = bank_select;  // UNROM bus conflict
  
  cv5000 &= 0xF0;			// GTROM
  cv5000 |= bank_select;
  reg5000 = cv5000;
    
  text_length = strlen((unsigned char *)index) + 1;
  index += text_length;
  text_length = strlen((unsigned char *)index) + 1;
  index += text_length;
  text_length = strlen((unsigned char *)index) + 1;
  index += text_length;  
  text_length = strlen((unsigned char *)index) + 1;
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
  //set_vram_update(ppu_buffer);
  
  //set_vram_update(NULL);

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

