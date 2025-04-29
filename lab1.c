#include <c8051f020.h>
#include<intrins.h>
#include<stdio.h>

typedef unsigned char uchar;
sbit SS = P1^0;
sbit SDI = P1^2;
sbit SCK = P1^3;


uchar tempString[5];

void init_Clock(void){
	OSCICN = 0x86; 		//-- 1000 0110b
	while ( (OSCICN & 0x10) == 0);	//-- poll for IFRDY -> 1
}

void init_Port(void){
	XBR2 |= 0x40;  //ʹ��P1~P3���������
	P1 = 0XFF;
	P1MDOUT = 0X0F;  
	
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
		else SDI = 1;    _nop_();_nop_();_nop_();
			_nop_();_nop_();_nop_();
		SCK = 1;  _nop_();_nop_();_nop_();_nop_();_nop_();_nop_();
			_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();
		SCK = 0;
	}
	

}

void setup(void);
void cleanDisplaySPI(void);
void s7sSendStringSPI(uchar *p);
void setDecimalsSPI(uchar decimal);
void setBrightnessSPI(uchar value);

//������������printf�ض���SEG

uchar seg_I = 0;

void sendByte(unsigned char dat){
	
	
//   SBUF = dat;//д�뷢�ͻ���Ĵ���
//   while(!TI);//�ȴ�������ɣ�TI���������־λ ��1
//   TI = 0;      //�������־λ����	
     tempString[seg_I] = dat;
	 seg_I++;
}

char putchar(char c){
	//����ض��򵽴���
	sendByte(c);
	return c;  //���ظ������ĵ�����printf
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

void init_ADC(){
	
	REF0CN=0x03;
	AMX0SL=0x02; //  ģ������
	ADC0CN=0x80;
}

void main(void)
{	
	uchar i;
	unsigned long val_adc ;
	unsigned int sum, Voltage;
	
	SDI = 1; //spi�������ſ���ʱ�ߵ�ƽ
	SCK = 0; //ʱ�����ſ���ʱ�͵�ƽ
	SS = 1;  //Ƭѡ���ſ���ʱ�ߵ�ƽ
	init_sys();
	init_ADC();		//initializing ADC
	setup();
    while(1)
    {	
		sum = 0;
		for(i=0;i<10;i++)
		{
			AD0BUSY=1;
			while(!AD0INT);		//check busy
			sum += ADC0H*256+ADC0L;	//read ADC value once
			
		}
		val_adc = sum/10;
		Voltage = val_adc * 5000 / 4095;//5V��ѹת��
		
		seg_I = 0;
		printf("%d",Voltage);
		displayConvert( Voltage);
		s7sSendStringSPI(tempString);
			
		delay(100000);
	

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


