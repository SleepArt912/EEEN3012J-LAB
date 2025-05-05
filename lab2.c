#include <c8051f020.h>
#include<intrins.h>
#include<stdio.h>

typedef unsigned char uchar;
sbit SS = P1^0;
sbit SDI = P1^2;
sbit SCK = P1^3;


unsigned int counter = 0;
uchar dat ;
uchar tempString[5];

void init_Clock(void){
	unsigned int i;
	OSCICN |= 0x03;                     // Configure internal oscillator for
                                       // its highest frequency (16 MHz)

   OSCICN |= 0x80;                     // Enable missing clock detector
	 OSCXCN = 0x67;                      // External Oscillator is an external
                                       // crystal (no divide by 2 stage)
	 for (i = 9000; i > 0; i--);         // at 16 MHz, 1 ms = 16000 SYSCLKs
                                       // DJNZ = 2 SYSCLKs
    while ((OSCXCN & 0x80) != 0x80);


   // Step 4. Switch the system clock to the external oscillator.
   OSCICN |= 0x08;

}

void init_Port(void){
	XBR2 |= 0x40;  //使能P1~P3口输出驱动
	P1 = 0XFF;
	P1MDOUT = 0XFF;  //配置P1口所有IO为推完输出模式
	
	P74OUT |= 0x08;		// 1: P5.[7:4] configured as Push-Pull.
	P74OUT &= 0xFB;		// 0: P5.[3:0] configured as Open-Drain.
	P5 = 0x0F;
}

void init_sys(){
	WDTCN = 0xDE;		// disable watchdog timer
	WDTCN = 0xAD;
	init_Clock();
	init_Port();
}

void delay(long s){
	while(s--);
}

void SPI_WriteByte(uchar dat)
{
	uchar i;	
	for( i=0x80;i != 0; i >>=1)
	{
		if((i & dat) ==0)  SDI = 0;
		else SDI = 1;    
        delay(1);
		SCK = 1;  delay(1);
		SCK = 0;
	}
	

}

void Timer0_Init(void)		//5毫秒@8MHz
{
	
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x01;			//设置定时器模式
	TL0 = 0xC0;				//设置定时初始值
	TH0 = 0x63;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	EA = 1;
	ET0 = 1;
	CKCON &= 0xf7;//T0M = 0;SYSCLK时钟12分频 
	TR0 = 1;				//定时器0开始计时
}
void Timer1_Init(void)
{
	XBR0      = 0x04;
    XBR1      = 0x08;//配置 UART0 P0.0 P0.1; T1 P0.2
	TMOD &= 0x0f;			//设置定时器模式
	TMOD |= 0x50;			//设置定时器模式
	TL1 = 0;				//设置定时初始值
	TH1 = 0;				//设置定时初始值
    TF1 = 0;
	ET1 = 0;
	TR1 = 1;

}

void T0_ISR() interrupt 1  //25毫秒中断1次
{
	static uchar t =0;
	TL0 = 0x00;				//重装定时T0初始值
	TH0 = 0x4c;				
	t++;
	if(t>=40)
	{
		t = 0;
		P5 ^=0X80;
		TR1 = 0;
		counter = TH1*256 + TL1;
		TL1 = 0;
		TH1 = 0;
		TR1 = 1;
	}

}

void setup(void);
void cleanDisplaySPI(void);
void s7sSendStringSPI(uchar *p);
void setDecimalsSPI(uchar decimal);
void setBrightnessSPI(uchar value);

//以下两个函数printf重定向到SEG

uchar seg_I = 0;
void sendByte(unsigned char dat){
	
	
//   SBUF = dat;//写入发送缓冲寄存器
//   while(!TI);//等待发送完成，TI发送溢出标志位 置1
//   TI = 0;      //对溢出标志位清零	
     tempString[seg_I] = dat;
	 seg_I++;
}

char putchar(char c){
	//输出重定向到串口
	sendByte(c);
	return c;  //返回给函数的调用者printf
}

void displayConvert(unsigned int num)
{
	if(num<10)
	{
		tempString[3] = tempString[0];
		tempString[0] = ' ';
		tempString[1] = ' ';
		tempString[2] = ' ';
	}
	else if(num<100)
	{
		tempString[2] = tempString[0];
		tempString[3] = tempString[1];
		tempString[0] = ' ';
		tempString[1] = ' ';
	
	}
	else if(num<1000)
	{   tempString[3] = tempString[2];
		tempString[2] = tempString[1];
		tempString[1] = tempString[0];
		tempString[0] = ' ';
	    
	}

}

void main(void)
{	
	uchar ii;
	SDI = 1; //spi数据引脚空闲时高电平
	SCK = 0; //时钟引脚空闲时低电平
	SS = 1;  //片选引脚空闲时高电平
	dat = 0xa1;
	init_sys();
	setup();
	Timer0_Init();
	Timer1_Init();
    while(1)
    {	
	    
		for(ii=0;ii<100;ii++)
		{
			seg_I = 0;
			printf("%d",counter);
			displayConvert(counter);
			s7sSendStringSPI(tempString);
			
			delay(40000);
		}

	}
}
void setup()
{
	SS = 1;
	cleanDisplaySPI();
	s7sSendStringSPI("-HI-");
	setDecimalsSPI(0X3F);
	setBrightnessSPI(0);
	delay(500000);
	setBrightnessSPI(255);
	delay(500000);
	cleanDisplaySPI();
}

void s7sSendStringSPI(uchar *p)
{
	uchar i;
	SS = 0;
	for(i=0;i<4;i++)
	{
		SPI_WriteByte(*(p+i));
	}
	SS = 1;

}
void cleanDisplaySPI()
{
	SS = 0;
	SPI_WriteByte(0X76);
    SS = 1;
}

void setBrightnessSPI(uchar value)
{
	SS = 0;
    SPI_WriteByte(0X7A);
	SPI_WriteByte(value);
	SS = 1;
}

void setDecimalsSPI(uchar decimal)
{
	SS = 0;
	SPI_WriteByte(0X77);
    SPI_WriteByte(decimal);
	SS = 1;
}


