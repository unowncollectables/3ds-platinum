#include <3ds.h>
#include <stdio.h>
int main(void){gfxInitDefault();consoleInit(GFX_TOP,NULL);printf("Platinum 3DS CIA pipeline test: OK\nPress START\n");
FILE*f=fopen("sdmc:/3ds/platinum3ds/hello_cia.txt","w");if(f){fputs("hello cia ran\n",f);fclose(f);}
while(aptMainLoop()){hidScanInput();if(hidKeysDown()&KEY_START)break;gspWaitForVBlank();}gfxExit();return 0;}
