/*****************************************************************************************************
* TFT1_8 1,8" Display from Sainsmart with a ST7735 controller and a RaspberryPi
*
* This is a test program to control the 1.8" ST7735 TFT with the RaspberryPi and tft_st7735 library along
* with the wiringPi library
*/

#include "wiringPi.h"
#include "wiringPiSPI.h"

#include "tft_st7735.h"
#include "tft_manager.h"
#include "tft_field.h"


int main (void)
{
int j = 0;
int i = 0;
bool direction = false;
TFT_ST7735 tft      = *new TFT_ST7735(0, 24, 25, 48000000);

  wiringPiSetupGpio();      // initialize wiringPi and wiringPiGpio

  tft.commonInit();         // initialize SPI and reset display
  tft.initR();              // initialize display
  tft.setRotation(DEGREE_0);
  tft.setBackground(TFT_BLACK);
  tft.clearScreen();        // reset Display

  for(; j < 1000; j++)
  {
tft.clearScreen();
    if(!direction){
	tft.drawString(++i, 10, "GL-BaseCamp!", TFT_WHITE, 1);
	//tft.fillRect(0, 10, TFT_width, 10, TFT_WHITE);
	if(i > 30)
	    direction = true;
    }
    else{
	tft.drawString(--i, 10, "GL-BaseCamp!", TFT_WHITE, 1);
	if(i < 1)
	    direction = false;
    }

    delay(15);
  }
return 0;

}
