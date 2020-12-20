#define F_CPU 16000000UL

#include<avr/io.h>
#include<util/delay.h>
#include<stdlib.h>
#include<avr/interrupt.h>

unsigned char fnd_sel[4] = {0x01, 0x02, 0x04, 0x08};
unsigned char digit[10]= {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7c, 0x07, 0x7f, 0x67};

volatile int count = 0;
volatile int x=10;
volatile int y=10;
volatile int z=10;


ISR(INT4_vect){
	count++;
	_delay_ms(100);
}

void init(){
	x = 10;
	y = 10;
	z = 10;
	count =0;
}

void buzzer(int hz, int hzcount){
	int i, j, ms;
	ms = 500/hz;
	for(i=0; i<hzcount; i++){
		PORTB = 0x10;
		_delay_ms(ms);
		PORTB = 0x00;
		_delay_ms(ms);
	}
}


void printAction(int flag){
	if(flag==1){
		DDRB = 0x10; // 포트 B의 bit4 를 출력 상태로 세팅
		while(1) // 500 Hz로 동작
		{
			PORTA = 0xff;
			PORTC = 0x3C;
			PORTG =	0x08;
			_delay_ms(2);
			PORTC = 0x1E;
			PORTG =	0x04;
			_delay_ms(2);
			PORTC = 0x30;
			PORTG =	0x02;
			_delay_ms(2);
			PORTC = 0x37;
			PORTG =	0x01;
			_delay_ms(2);
			PORTB = 0x10; // 1ms 동안 ‘On’ 상태 유지
			_delay_ms(1);
			PORTB = 0x00; // 1ms 동안 ‘Off’ 상태 유지
			_delay_ms(1);
		}
	}
	else{
		DDRB = 0x10; // 포트 B의 bit4 를 출력 상태로 세팅
		while(1) // 500 Hz로 동작
		{
			buzzer(480, 12);
			PORTA = rand()%256;
			PORTC = 0x5E;
			PORTG =	0x08;
			_delay_ms(2);
			PORTC = 0x79;
			PORTG =	0x04;
			_delay_ms(2);
			buzzer(320, 8);
			PORTC = 0x77;
			PORTG =	0x02;
			_delay_ms(2);
			PORTC = 0x5E;
			PORTG =	0x01;
			_delay_ms(2);

			if(count>4){
				init();
				break;
			}
		}
	}
}

void getGambleNumber(){
	while(1){
		for(int i =0; i<10; i++){
			PORTA = rand()%256;
			if(x==10){
				if(count==1)
					x = i;
				PORTC = digit[i];
			}
			else{
				PORTC = digit[x];
				PORTA = 0xC0;
			}
			PORTG =	0x08;
			_delay_ms(8);

			if(y==10){
				if(count==2)
					y = i;
				PORTC = digit[9-i];
			}
			else{
				PORTC = digit[y];
				PORTA = 0xF0;
			}
			PORTG = 0x04;
			_delay_ms(8);

			if(z==10){
				if(count==3)
					z = i;
				PORTC = digit[i];
			}
			else{
				PORTC = digit[z];
				PORTA = 0xFC;
			}
			PORTG = 0x02;
			_delay_ms(8);
			if(count>3){
				if(x==y && y==z)
					printAction(1);
				else
					printAction(2);
			}
		}
	}
}

int main(){
	DDRA = 0xff;
	DDRC = 0xff;
	DDRG = 0x0f;

	DDRE = 0xef;
	EICRB = 0x02;
	EIMSK = 0x10;
	SREG |= 1<<7;
	getGambleNumber();


}
