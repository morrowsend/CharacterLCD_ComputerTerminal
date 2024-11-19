/**
File Name	: terminal.c

Title		: Simple terminal combining an AT keyboard with a 4x20 LCD using an ATMEGA128
Target MCU	: ATmega128
Author		: Adam Harris


Notes		:	

The keyboard routine was inspired and adapted from the "Fun With Keyboards: Key Counter" project
at Sparkfun.com (http://www.sparkfun.com/commerce/present.php?p=Key-Counter)

Fun With Keyboards: Key Counter	2-19-2008
Copyright Spark Fun Electronics© 2008
by: Nathan Seidle

(Lfuse =0xef, Hfuse = 0x0F, extended fuse = 0xFF)


Special function keys are as follows:
			F1		used to replace "crtl+D"  (EOT)
			F2		used to replace "ctrl+Z"  (SUB)
			F3		used to replace "ctrl+C"  (ETX)
			F5		used to replace "ctrl+x"  (CAN)
			
			
			F9 		used directly (before ASCII conversion) for "Previous screen" button
			F10 		used directly (before ASCII conversion) for "Clear LCD"
	
This code was written using AVRlib in WinAVR. 
*/


//----- Include Files ---------------------------------------------------------
//WinAVR includes:
#include <avr/io.h>			// include I/O definitions (port names, pin names, etc)
#include <avr/interrupt.h>	// include interrupt support




//AVRlib Includes:



/*Local include (there's another that's not mentioned here but 
we still need lcdconf.h in the same folder.*/

#include "global.h"		// include our global settings (AVRlib)




/*
 You may need to replace the next 5 includes with these 5:


#include <AVRlib/uart.h>		// include uart function library
#include <AVRlib/rprintf.h>	// include printf function library
#include <AVRlib/timer.h">		// include timer function library
#include <AVRlib/extint.h>		// include external interrupt library
#include <AVRlib/lcd.h>		// lcd control for HD44780 LCD

*/

#include "uart.h"		// include uart function library
#include "rprintf.h"	// include printf function library
#include "timer.h"		// include timer function library
#include "extint.h"		// include external interrupt library
#include "lcd.h"		// lcd control for HD44780 LCD

#define CLK_IN		2 	// for the keyboard connections (PORTC pin2)
#define DATA_IN		1 	// for the keyboard connections (PORTB, pin1)


const uint8_t scancodes[] PROGMEM=  {	//unshifted characters list converting scancodes to ASCII
	'0',//00	
	'0',//01				F9
	'0',//02				
	0x18,//03				F5		used to replace "ctrl+x"  (CAN)
	0x03,//04				F3		used to replace "ctrl+C"  (ETX)
	0x04,//05				F1		used to replace "crtl+D"  (EOT)
	0x1A,//06				F2		used to replace "ctrl+Z"  (SUB)
	'0',//07				F12
	'0',//08				
	'0',//09				F10
	'0',//0A				F8
	'0',//0B				F6
	'0',//0C				F4
	9,//0D 					TAB
	'`',//0E
	'0',//0F	
	'0',//10	
	'0',//11				left ALT
	'0',//12				left SHIFT
	'0',//13	
	'0',//14				left CTRL
	'q',//15
	'1',//16 
	'0',//17	
	'0',//18	
	'0',//19
	'z',//1A
	's',//1B 
	'a',//1C 
	'w',//1D 
	'2',//1E 
	'0',//1F
	'0',//20
	'c',//21 
	'x',//22
	'd',//23 
	'e',//24
	'4',//25
	'3',//26
	'0',//27
	'0',//28
	' ',//29 
	'v',//2A 
	'f',//2B
	't',//2C 
	'r',//2D
	'5',//2E
	'0',//2f
	'0',//30
	'n',//31
	'b',//32
	'h',//33
	'g',//34 
	'y',//35
	'6',//36
	'0',//37
	'0',//38
	',',//39 
	'm',//3A
	'j',//3B
	'u',//3C 
	'7',//3D
	'8',//3E
	'0',//3f
	'0',//40
	',',//41
	'k',//42 
	'i',//43
	'o',//44 
	'0',//45
	'9',//46
	'0',//47
	'0',//48
	'.',//49 
	'/',//4A
	'l',//4B
	';',//4C
	'p',//4D
	'-',//4E
	'0',//4f
	'0',//50
	'0',//51
	'\'',//52 
	'0',//53
	'[',//54
	'=',//55 
	'0',//56
	'0',//57
	'0',//58				Caps Lock
	'0',//59
	0x0A,//5A 				Using LINEFEED here for "return"
	']',//5B
	'0',//5C
	'\\',//5D
	'0',//5E
	'0',//5F
	'0',//60
	'<',//61 
	'0',//62
	'0',//63
	'0',//64
	'0',//65
	8 ,//66 				Backspace
	'0',//67
	'0',//68
	'1',//69 
	'0',//6A
	'4',//6B
	'7',//6C
	'0',//6d
	'0',//6e
	'0',//6f
	'0',//70
	'.',//71
	'2',//72 
	'5',//73
	'6',//74
	'8',//75
	0x1B,//76				ESC
	'0',//77				Num Lock
	'0',//78				F11
	'+',//79 
	'3',//7A
	'-',//7B
	'*',//7C
	'9',//7D
	'0',//7E
	'0',//80
	'0',//81
	'0',//82
	'0',//83				F7

};




const uint8_t shifted_scancodes[] PROGMEM=  {		//shifted characters list converting scancodes to ASCII
	'0',//00	
	'0',//01				F9
	'0',//02				
	0x18,//03				F5		used to replace "ctrl+x"  (CAN)
	0x03,//04				F3		used to replace "ctrl+C"  (ETX)
	0x04,//05				F1		used to replace "crtl+D"  (EOT)
	0x1A,//06				F2		used to replace "ctrl+Z"  (SUB)
	'0',//07				F12
	'0',//08				
	'0',//09				F10
	'0',//0A				F8
	'0',//0B				F6
	'0',//0C				F4
	9,//0D 					TAB
	'`',//0E
	'0',//0F	
	'0',//10	
	'0',//11				left ALT
	'0',//12				left SHIFT
	'0',//13	
	'0',//14				left CTRL
	'Q',//15
	'!',//16 
	'0',//17	
	'0',//18	
	'0',//19
	'Z',//1A
	'S',//1B 
	'A',//1C 
	'W',//1D 
	'@',//1E 
	'0',//1F
	'0',//20
	'C',//21 
	'X',//22
	'D',//23 
	'E',//24
	'$',//25
	'#',//26
	'0',//27
	'0',//28
	' ',//29 
	'V',//2A 
	'F',//2B
	'T',//2C 
	'R',//2D
	'%',//2E
	'0',//2f
	'0',//30
	'N',//31
	'B',//32
	'H',//33
	'G',//34 
	'Y',//35
	'^',//36
	'0',//37
	'0',//38
	',',//39 				//
	'M',//3A
	'J',//3B
	'U',//3C 
	'&',//3D
	'*',//3E
	'0',//3f
	'0',//40
	'<',//41
	'K',//42 
	'I',//43
	'O',//44 
	'0',//45
	'(',//46
	'0',//47
	'0',//48
	'>',//49 
	'?',//4A
	'L',//4B
	':',//4C
	'P',//4D
	'_',//4E
	'0',//4f
	'0',//50
	'0',//51
	'\'',//52 				//
	'0',//53
	'{',//54
	'+',//55 
	'0',//56
	'0',//57
	'0',//58				Caps Lock
	'0',//59
	0x0A,//5A 				Using LINEFEED here for "return" 
	'}',//5B
	'0',//5C
	'|',//5D
	'0',//5E
	'0',//5F
	'0',//60
	'<',//61 				//
	'0',//62
	'0',//63
	'0',//64
	'0',//65
	8 ,//66  				Backspace
	'0',//67
	'0',//68
	'1',//69 
	'0',//6A
	'4',//6B
	'7',//6C
	'0',//6d
	'0',//6e
	'0',//6f
	'0',//70
	'.',//71
	'2',//72 
	'5',//73
	'6',//74
	'8',//75
	0x1B,//76				ESC
	'0',//77				Num Lock
	'0',//78				F11
	'+',//79 
	'3',//7A
	'-',//7B
	'*',//7C
	'9',//7D
	'0',//7E
	'0',//80
	'0',//81
	'0',//82
	'0',//83				F7

};








/** 
This keyboard routine was inspired and adapted from the "Fun With Keyboards: Key Counter" project
at Sparkfun.com (http://www.sparkfun.com/commerce/present.php?p=Key-Counter)

Fun With Keyboards: Key Counter	2-19-2008
Copyright Spark Fun Electronics© 2008
Nathan Seidle

I first attempted using AVRlib's external interrupts to catch the scancodes, however, the timing 
was strange and I couldn't get any clear scancodes that way. This routine is much more elegant 
than the way I was using external interrupts. This version is also much easier to explain.
*/


u08 getkey(void)					//simplified keyboatd getkey routine
{
	u08 incoming_byte;				//declare variable

while( (PINB & (1<<CLK_IN)) == 0); //Wait for clock line to go high

	incoming_byte = 0;				//reset variable
	
	for(int index = 0 ; index < 10 ; index++)	//for the start bit to the stop bit, do the following:
	{
		while( (PINB & (1<<CLK_IN)) ); //Wait for clock line to go low, telling you data is clean


		if(index<8)					//only counts the start bit and data bits, parity (9) and stop bit (10) are ignored by this
		{
			/*
			Pre-rotate the variable:
			incoming_byte starts as 00000000 in binary,
			so we can rotate here to keep the zeros when 
			data isn't '1'
			*/
			incoming_byte >>= 1; 		//Rotate incoming_byte  -->  left to right
			
			if( PINC & (1<<DATA_IN) ) 	//If the data is '1'
			{
			/*
			Record any incoming data. by ORing it with the 
			most significant bit in incoming_Byte.
			This will be shifted on the next iteration of the 
			for loop so it will not be overwritten.
			*/
				incoming_byte |= 0x80; 
			}
		}
		
		while( (PINB & (1<<CLK_IN)) == 0); //Wait for clock line to go high again
	}
	
	/*
	The parity bit and stop bits are caught by the for loop above and ignored.
	*/


	return(incoming_byte);			//return the variable 
}








int main(void)					//Main program
{
// Port Designations


DDRB= 0x00;						//PORTB = inputs
PORTB=0;						//with no pullups

DDRC= 0x00;						//PORTC = inputs 
PORTC=0;						//with no pullups

//variable declarations
u08 var;					//varaible grabs scancode from keyboard
u08 stdin; 					//variable grabs character from UART
u08 address=0; 				//keeps track of position on LCD
int row=0;					//keeps track of row number on LCD
u08 column=0;				//keeps track of column number on LCD

//u08 e_flag=0;					//extended key flag (not implemented)			
u08 s_flag=0;				//Shift key flag
u08 c_flag=0;				//Caps Lock flag
	
u08 buff;					//variable stores keyboard character after converted to ASCII
u08 screen_buffer [4][20];	//this is a 2D array to store the previous 4 lines of the screen. a 3 D array could hold even more "Screenshots" easily

	
	
	//Initializing libraries:
	uartInit();				//initialize the UART
	uartSetBaudRate(9600);	//setup our baud rate
	
	timerInit();				//init the system timer

	lcdInit();					//Init the LCD screen (based off information in lcdconf.h)
	lcdControlWrite(0x0F);	   //underline and blink cursor

	rprintfInit(lcdDataWrite);	//send output to LCD screen

	while(1)
	{
		
		
		if (uartReceiveByte(&stdin))		// get input from UART
		{
		rprintfInit(lcdDataWrite);			//switch back to LCD
		
		
		/*
		//backspace routine below  needs a bit of work
		
		if (stdin== 0x08 )			//if backspace ("delete" not yet supported as it is an extended key)
				{
					address--;						//take off character count to correct line position on LCD
					column--;
				rprintfInit(lcdDataWrite);			//switch back to printing on the LCD
				rprintf("%d", address);		//prints to LCD
					// This is formatting for the 4-line LCD)
					
					if (column == 0)				//if first column			
					{
						row--;						//go to the previous row
						if (row<0)					//if you are on the top row...
						{		
						row=3;						//go to bottom row
						}
						lcdGotoXY(20,row);			//send position command to LCD
						column = 20;				//reset column counter
					}
					
					
					lcdControlWrite(0x10);			//shift cursor to the left (backspace)
					rprintfInit(lcdDataWrite);			//switch back to printing on the LCD
				}	
				
				*/
				
		if (stdin==0x0D)					//if "enter"	
		{	
		row++;								//go to the next row
		if (row>3)							//if on the last row, 
		{
		row=0;								//go to the first row
		}
		lcdGotoXY(0,row);					//send LCD command for position
		column = 0;							//reset column counter
		rprintfInit(lcdDataWrite);			//switch back to LCD (if in weird state from before)
		}
		
		
		else if( stdin <0x20)				//if stdin is a non-printable character
		{
		/*
		Do nothing at all for now, This must be handled funyn because it has the possibility of printing actual characters to the LCD from CG RAM
		*/
		}
		
		else
		{
		rprintfInit(lcdDataWrite);			//switch back to LCD
		rprintf("%c",stdin);				//send character to LCD
		address++;							//increase address counter
		column++;							//increase column counter
			
			if (address<79)					//if we haven't fille the LCD screen up yet,			
			{
				if (column >=20)			//and if we have filled the current line up, (kinda redundant on the last line...)
				{
				row++;						//go to the next line
					if (row>3)				//if on the last line
					{
					row=0;					//go back to the first line
					}
				lcdGotoXY(0,row);			//send position command to LCD
				column = 0;					//reset column counter
				}	
			}
			if (address == 80)  //if end of the screen, clear screen and start at top left position
			{
				lcdClear();		//clear LCD screen
				lcdHome();		//send cursor back home
				address = 0;	//reset address counter
				column = 0;		//reset column counter
			}
		screen_buffer[row][column]= stdin;		//store in the "previous screen" buffer
		}
		
		}
		
		
		
		/** This keyboard routine was inspired and adapted from the "Fun With Keyboards: Key Counter" project
		at Sparkfun.com (http://www.sparkfun.com/commerce/present.php?p=Key-Counter)
		
		
		I first attempted using AVRlib's external interrupts to catch teh scancodes, however, the timing was 
		strange, and I couldn't get any scancodes that way.  
		
		This method uses a simulated interrupt by waiting for the CLK line to go low before doing anythign int he loop.
		*/
		
		if( (PINB & (1<<CLK_IN)) == 0) //Check to see if clock line is low
		{
			var=getkey();					//scan the keyboard and load the resut into "Var"
			timerPause(10);					//this pause helps with timing
			
			if (var!=0xf0)					//if the scancode isn't "end of character" do the following:
			{
				
				
				if (var== 0x83)				//if "F7"
				{
				/*
				This serves as a template for using the function keys. 
				remove the following two line and put your code in here:
				*/				
				rprintfInit(lcdDataWrite);	//switch back to printing on the LCD
				rprintf("%d",column);		//print the column number 
				
				}	
				
				
				else if (var== 0x01)			//if F9
				{
				rprintfInit(lcdDataWrite);			//switch back to printing on the LCD
					
					for(int row_buffer=0; row_buffer<4;row_buffer++)		//for each row (starting at the first row)
					{
					
						for( int column_buffer=0;column_buffer<21;column_buffer++)		//print each character in order (starting at the first character)
						{
						rprintf("%c",screen_buffer[row_buffer][column_buffer]);			//this is the print to screen command
						
						}
					}					
					
				}	
				
				
				else if (var== 0x09)			//if F10
				{
				rprintfInit(lcdDataWrite);			//switch back to printing on the LCD
				lcdClear();				//clear screen
				lcdHome();				//go to home position (0,0)
				}	
				
				
				
				
				
				else if (var== 0x59 || var == 0x12)			//if shifted character
				{
				s_flag=1;									//set flag
				}
				
				
				
				else if (var== 0x58)			//if Caps Lock character
				{
				c_flag=!c_flag;					//turn caps lock either on or off based on last state
				}	
				
				
				
				else if (var== 0x66 )			//if backspace ("delete" not yet supported as it is an extended key)
				{
					address--;						//take off character count to correct line position on LCD
					column--;
					rprintfInit(uartSendByte);		//send backspace key to UART
					
					
					/** This is formatting for the 4-line LCD*/
					
					if (column == 0)				//if first column			
					{
						row--;						//go to the previous row
						if (row<0)					//if you are on the top row...
						{		
						row=3;						//go to bottom row
						}
						lcdGotoXY(20,row);			//send position command to LCD
						column = 20;				//reset column counter
					}
					
					
					lcdControlWrite(0x10);			//shift cursor to the left (backspace)
					rprintfInit(uartSendByte);		//send backspace key to UART:
					buff=pgm_read_byte(&scancodes[var]);	//get ASCII value
					
					rprintf("%c",buff);					//print to UART
					buff=0x00;								//reset character
					rprintfInit(lcdDataWrite);			//switch back to printing on the LCD
				}	
				
				
				
				else									//if normal key
				{
					
					if (s_flag==1 || c_flag==1)			//see if it is shifted or caps lock
					{												//if so, 
					buff=pgm_read_byte(&shifted_scancodes[var]);	//look up ASCII
					s_flag=0;								//reset the "shifted" flag
					}
					else									//otherwise its not a shifted key
					buff=pgm_read_byte(&scancodes[var]);	//get ASCII code for value
					
					
					if (address<79)							//if not the end of the screen
					{
					
						if (column >=30 )				//if at the end of the screen
						{
						row++;									//go to the next row
						if (row>3)							//if on the last row, 
						{
						row=0;								//go to the first row
						}
						lcdGotoXY(0,row);						//send LCD command for position
						column = 0;								//reset column counter
						}	
						
						
						
						if (buff==0x0A)				//if at the end of the screen
						{
						//row++;									//go to the next row
						if (row>3)							//if on the last row, 
						{
						row=0;								//go to the first row
						}
						lcdGotoXY(0,row);						//send LCD command for position
						column = 0;								//reset column counter
						rprintfInit(uartSendByte);				//get ready to print to the UART
						/* *Based on which shell you are using, different commands here work better,
						Some shells prefer carriage return some like linefeed, and some like both.*/
						rprintf("\r");						//print CR character to UART
						buff=0x00;								//reset character
						lcdClear();				//clear screen
						lcdHome();				//go to home position (0,0)
						}	
						
							rprintfInit(uartSendByte);				//get ready to print to the UART
							rprintf("%c",buff);						//print character to UART
							rprintfInit(lcdDataWrite);			//switch back to printing on the LCD
						
							if (buff!=0x0A )							//don't print weird character from "enter" button
							{
							//Do nothing in here for now, can be handled better later	
							}
					}
					
					else					//if end of the screen, clear screen and start at top left position
					{
						lcdClear();				//clear screen
						lcdHome();				//go to home position (0,0)
						address = 0;			//reset address counter
						column = 0;				//reset column counter
						row = 0;				//reset row counter
					}
					
				}//end else (normal key)
				
			}//end if var != 0xf0 (character sent command)
			
		}//end if clk line goes low
		
	}// end while(1)
	return 0;
} //end main()

/** =================Additions=================

 --extended key support
 
 --separate caps lock and shift scancode lists 
 
 --make shift work while holding it down (more than one character at a time)
 
 --handle linefeeds and carriage returns better
 
 --allow for faster typing speed?
 
 --handle special characters from the UART
*/





