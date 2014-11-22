#include "mbed.h"

#include "SPI_TFT_ILI9341.h"
#include "GraphicsDisplay.h"
#include "Arial12x12.h"
#include "Arial24x23.h"
#include "Arial28x28.h"

#include "SDFileSystem.h"

#define CLEAR_STATUS 6
#define REPETITIONS_PER_DAY 432000         //24*60*60*5

SDFileSystem sd(PTE3, PTE1, PTE2, PTE4, "sd"); //MOSI, MISO, SCK, CS

Serial pc(USBTX, USBRX);                        //PC-Connection via UART(Serial)
Serial bt(PTC15,PTC14);                         //Bluetooth-Connection via UART to a HC-06                                                     
SPI_TFT_ILI9341 tft(PTD2,PTD3,PTD1,PTD0,PTE26,PTC4,"TFT");  //Screen (Mosi,Miso,Sclk,CS,Reset,DC)
DigitalOut vcc(PTC10);                          //Power Supply
AnalogIn adc(A0);                               //Controller (Potentiometer)
DigitalIn btn(PTC5);                            //Confirm-button                                        

int repetition_counter = 0;                     //Counter of repetitions to measure one day
int last_status = CLEAR_STATUS;                 //Which container is selected
int expiration_dates[]= {0,0,0,0,0,0};

void printText(int x, int y, char txt[])
{
    tft.locate(x,y);
    for(int counter=0; counter<strlen(txt); counter++)
        tft.putc((int)txt[counter]);
}
void setMarker(int status)  //Draw the yellow bar at the top of each container
{
    if(status==last_status)
        return;
    tft.fillrect(((int)(last_status / 2)) * 108, (last_status % 2) * 122, ((int)(last_status / 2)) * 108 + 104, (last_status % 2) * 122 + 8, Black);
    if(status!=CLEAR_STATUS)  
        tft.fillrect(((int)(status / 2)) * 108, (status % 2) * 122, ((int)(status / 2)) * 108 + 104, (status % 2) * 122 + 8, Yellow); 
    last_status=status; 
}

void setTime(int time, int index)   //Draw the remaining time
{
   tft.locate(((int)(index / 2)) * 108, (index % 2) * 122+96);
   tft.putc(time+'0');
}

inline void setWarning(int index, int color)    //Set the red or orange bar at the bottom of each container
{
    tft.fillrect(((int)(index / 2)) * 108, (index % 2) * 122+94, ((int)(index / 2)) * 108 + 104, (index % 2) * 122 + 118, color);
    
}

int drawBitmap(int index, int fruit)
{
    switch(fruit)
    {
        case 1:
            return tft.BMP_16(((int)(index / 2)) * 108, (index % 2) * 122,"/sd/apple.bmp");
        case 2:
            return tft.BMP_16(((int)(index / 2)) * 108, (index % 2) * 122,"/sd/banana.bmp");
        case 3:
            return tft.BMP_16(((int)(index / 2)) * 108, (index % 2) * 122,"/sd/blueberry.bmp");
        case 4:
            return tft.BMP_16(((int)(index / 2)) * 108, (index % 2) * 122,"/sd/kiwi.bmp");
        case 5:
            return tft.BMP_16(((int)(index / 2)) * 108, (index % 2) * 122,"/sd/lime.bmp");
        case 6:
            return tft.BMP_16(((int)(index / 2)) * 108, (index % 2) * 122,"/sd/peach.bmp");
        case 7:
            return tft.BMP_16(((int)(index / 2)) * 108, (index % 2) * 122,"/sd/strawberry.bmp");
        case 0:
            return tft.BMP_16(((int)(index / 2)) * 108, (index % 2) * 122,"/sd/watermelon.bmp");
        default:
            return -1;
    }
}


int main()
{
    vcc=true;                                       //Enable the power supply
    //Set the baud rates
    bt.baud(9600);                                   
    pc.baud(9600);
    //Set the params of the screen
    tft.set_orientation(1);
    tft.set_font((unsigned char *)Arial24x23);   
    //Draw the containers
    tft.fillrect(0,0,320,240,White);
    tft.fillrect(0,0,104,118,Black);
    tft.fillrect(108,0,212,118,Black);
    tft.fillrect(216,0,320,118,Black);
    tft.fillrect(0,122,104,240,Black);
    tft.fillrect(108,122,212,240,Black);
    tft.fillrect(216,122,320,240,Black);
        
    while (true) 
    {
        setMarker(CLEAR_STATUS);                        //remove marker   
        if(!btn)                                        //if confirm-button is prssed:
        {
             while(!btn);
             while(btn)                                 //Choose container
             {
                setMarker((int)(adc.read()*6));
                wait_ms(200);
                repetition_counter++;
             }
             while(!btn);
             while(btn)                                 //set expiration date
             {
                setWarning(last_status, Black);
                setTime((int)(adc.read()*9), last_status);
                expiration_dates[last_status]=(int)(adc.read()*9)+1;
                wait_ms(200);
                repetition_counter++;
             }
             while(!btn);
             while(btn)                                 //choose fruit icon
             {
                drawBitmap(last_status, (int)(adc.read()*8));
                setTime(expiration_dates[last_status], last_status);
                wait_ms(200);
                repetition_counter++;
             }
             while(!btn);
        }
        wait_ms(200);                       
        repetition_counter++;
        
        if(repetition_counter >= REPETITIONS_PER_DAY)                               //check if day is over
        {
            repetition_counter -= REPETITIONS_PER_DAY;
            for(int i = 0; i < 6; i++) 
            {
                if(expiration_dates[i] > 0)
                {
                    expiration_dates[i]--;                                          //refresh time until the product expires
                    setTime(expiration_dates[i], i);
                    if(expiration_dates[i]==0)
                    {
                        setWarning(i, Orange);
                        bt.printf("Number %d is expiring tomorrow!\n", i);
                    }
                }
                else                                                                //warn if product is expired
                {
                    setWarning(i, Red);
                    bt.printf("Number %d expired today!\n", i);
                }
            }
        }
    }
}
