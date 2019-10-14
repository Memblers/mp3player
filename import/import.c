#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAXCHAR 1000
#define BANK_SIZE 0x4000

unsigned char buffer[BANK_SIZE];

const char *ok[] = {"Cool!", "Nice!", "Right on!", "Hell yeah!", "Excellent!", "Awesome!", "Yess!", ":D"};

int main() {
	time_t t;
	srand((unsigned) time(0));
    FILE *fp_in, *fp_out, *fp_out_address, *fp_out_bank;
    char str[MAXCHAR];
	char str_name[MAXCHAR];
	
	int current_bank = 0;
	int last_bank = 0;

    fp_in = fopen("GTMP3 export data.txt", "r");
    fp_out = fopen("output.bin", "wb");
	fp_out_address = fopen("output-address.bin", "wb");
	fp_out_bank = fopen("output-bank.bin", "wb");

    if (fp_in == NULL)
    {
        printf("Could not open input file - GTMP3 export data.txt\n");
        return 1;
    }
    if (fp_out == NULL)
    {
      printf("Could not open output file\n");
      return 1;
    }
	if (fp_out_address == NULL)
    {
      printf("Could not open output file\n");
      return 1;
    }
	if (fp_out_bank == NULL)
    {
      printf("Could not open output file\n");
      return 1;
    }

    while (1)
    {
	  last_bank = current_bank;
	  if ((ftell(fp_out) % BANK_SIZE) >= (BANK_SIZE-0x80))
	  {
		  fseek(fp_out,BANK_SIZE - (ftell(fp_out) % BANK_SIZE), SEEK_CUR);
		  current_bank = (ftell(fp_out) / BANK_SIZE);
		  continue;
	  }
	  
	  memset(str,0,MAXCHAR);
      do
      {
        fgets(str, MAXCHAR, fp_in);
      } while (strncmp(str,"[FILE]",6) != 0);
	  
	  if (strncmp(str,"[FILE][END]",11) == 0)
		break;
	
	  memset(str_name,0,MAXCHAR);
      fgets(str_name, MAXCHAR, fp_in);	// filename
	  str_name[strcspn(str_name, "\n")] = 0;
      printf("Importing %s\n",str_name);
	  
	  short address = ((ftell(fp_out) % BANK_SIZE) + 0x8000);
	  fwrite(&address,sizeof(short),1,fp_out_address);
	  char bank = ((ftell(fp_out)) / BANK_SIZE);
	  fwrite(&bank,sizeof(char),1,fp_out_bank);
	  
      memset(str,0,MAXCHAR);
	  fgets(str, MAXCHAR, fp_in);
	  if (strlen(str) == 1)
	  {
		  printf("Empty title tag, substituting file name ...\n");
		  fwrite(str_name, sizeof(char),(strlen(str_name)+1),fp_out);   // filename
	  }
	  else
	  {
		  str[strcspn(str, "\n")] = 0;
		  fwrite(str, sizeof(char),(strlen(str)+1),fp_out);   // song title
	  }
	  
      memset(str,0,MAXCHAR);
	  fgets(str, MAXCHAR, fp_in);
	  str[strcspn(str, "\n")] = 0;
      fwrite(str, sizeof(char),(strlen(str)+1),fp_out);   // artist

      memset(str,0,MAXCHAR);
	  fgets(str, MAXCHAR, fp_in);
	  str[strcspn(str, "\n")] = 0;
      fwrite(str, sizeof(char),(strlen(str)+1),fp_out);   // album
	  
	  memset(str,0,MAXCHAR);
	  fgets(str, MAXCHAR, fp_in);
	  str[strcspn(str, "\n")] = 0;
      fwrite(str, sizeof(char),4,fp_out);   // year
	  
	  
      memset(str,0,MAXCHAR);
	  fgets(str, MAXCHAR, fp_in);	  
	  int timer,timer2,timer3,timer4;
      timer = atoi(str);
      timer2 = timer * 60;
      timer3 = (128 << 16) / timer2;
      timer4 = (192 << 16) / timer2;

      fwrite(&timer, sizeof(int),1,fp_out);  // number of seconds
      fwrite(&timer2, sizeof(int),1,fp_out); // number of NTSC frames
      fwrite(&timer3, sizeof(short),1,fp_out); // 8.16 progress bar NTSC pixel steps (128 pixels wide)
      fwrite(&timer4, sizeof(short),1,fp_out); // 8.16 progress bar NTSC pixel steps (192 pixels wide)

      timer2 = timer * 50;
      timer3 = (128 << 16) / timer2;
      timer4 = (192 << 16) / timer2;

      fwrite(&timer2, sizeof(int),1,fp_out); // number of PAL frames
      fwrite(&timer3, sizeof(short),1,fp_out); // 8.16 progress bar PAL pixel steps (128 pixels wide)
      fwrite(&timer4, sizeof(short),1,fp_out); // 8.16 progress bar PAL pixel steps (192 pixels wide) 
	  
	  current_bank = (ftell(fp_out) / BANK_SIZE);
	  if (current_bank != last_bank)
	  {
		  printf("Bank overflow error!  Tell Memblers the buffer is too damn small!  This error could also happen if you process an MP3 with a corrupt ID3 tag. %X\n", ftell(fp_out));
		  return 1;
	  }
    }
	
	printf("\n");
	printf("=========================\n");
	printf("MP3 ID3 Tag Importer V1.0\n");
	printf("=========================\n");
	
    fgets(str, MAXCHAR, fp_in);
    printf("\n%s",str);
	fgets(str, MAXCHAR, fp_in);	
    printf("%.2f MB of memory will be required for music data\n",(strtod(str, 0)) / 1048576);	
    printf("Data import completed! %s\n", ok[rand() % 8]);

    fclose(fp_in);
    fclose(fp_out);
	fclose(fp_out_address);
	fclose(fp_out_bank);
	
////////////////
// split files	
////////////////

	FILE *fp_out0,*fp_out1,*fp_out2,*fp_out3,*fp_out4,*fp_out5,*fp_out6,*fp_out7;

	fp_in = fopen("output.bin", "rb");
	fseek(fp_in,0,SEEK_SET);
	
	fp_out0 = fopen("output.bin.0", "wb");
	fp_out1 = fopen("output.bin.1", "wb");
	fp_out2 = fopen("output.bin.2", "wb");
	fp_out3 = fopen("output.bin.3", "wb");
	fp_out4 = fopen("output.bin.4", "wb");
	fp_out5 = fopen("output.bin.5", "wb");
	fp_out6 = fopen("output.bin.6", "wb");
	fp_out7 = fopen("output.bin.7", "wb");
	
#define	split(file) \
{ \
	memset(&buffer, 0, BANK_SIZE); \
	fread(&buffer, sizeof(unsigned char), BANK_SIZE, fp_in); \
	fwrite(&buffer, sizeof(unsigned char), BANK_SIZE, file); \
};
	split(fp_out0);
	split(fp_out1);
	split(fp_out2);
	split(fp_out3);
	split(fp_out4);
	split(fp_out5);
	split(fp_out6);
	split(fp_out7);
	
    return 0;
}