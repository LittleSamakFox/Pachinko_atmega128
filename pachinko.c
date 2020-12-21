#define F_CPU 16000000UL
#define STOP 0
#define GO 1

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
volatile int time=0;
volatile int game_select = 0;
volatile int status = STOP;


ISR(INT4_vect){
	count++;
	_delay_ms(20);
}

ISR(INT5_vect){
	if(status==STOP)
		status = GO;
	else
		status = STOP;
}

void init(){
	x = 10;
	y = 10;
	z = 10;
	time = 0;
	count =0;
}

void timer3_pwm_init(){
	DDRE |= (1<<3);
	TCCR3A |= (1<<COM3A1) | (1<<WGM31);
	TCCR3B |= (1<<WGM33) | (1<<WGM32) | (1<<CS32);
	ICR3 = 1250;
	OCR3A = 90;
}

void serveMotorControl(){
	OCR3A = 90;
	_delay_ms(1000);
	OCR3A = 150;
	_delay_ms(1000);
	OCR3A = 90;
	_delay_ms(1000);
}

void fnd(int number, int section, int delay){
	PORTC = number;
	PORTG = section;
	_delay_ms(delay);
}

void getMainScreen(){
	while(1){
		if(status == GO){
			init();
			break;
		}
		fnd(0x08,0x0e,2);
		if(count%2==0){
			game_select = 0;
			fnd(0x30,0x01,2);
		}
		else{
			game_select = 1;
			fnd(0x5B,0x01,2);
		}
	}
}

void buzzer(int hz, int hzcount){
	int i, ms;
	ms = 500/hz;
	for(i=0; i<hzcount; i++){
		PORTB = 0x10;
		_delay_ms(ms);
		PORTB = 0x00;
		_delay_ms(ms);
	}
}


void printAction(int flag){
	while(1){
		if(status == STOP){
			init();
			status = GO;
			break;
		}
		if(flag==1){
			PORTA = 0xff;
			buzzer(480,12);
			fnd(0x3C,0x08,2);
			fnd(0x1E,0x04,2);

			buzzer(320,8);
			fnd(0x30,0x02,2);
			fnd(0x37,0x01,2);

			time++;
			if(time>35)
				status = STOP;
		}
		else{
			PORTA = rand()%256;
			buzzer(480, 12);
			fnd(0x5E,0x08,2);
			fnd(0x79,0x04,2);

			buzzer(320, 8);
			fnd(0x77,0x02,2);
			fnd(0x5E,0x01,2);

			time++;
			if(time>35)
				status = STOP;
		}
	}
}

void getGambleNumber(){
	while(1){
		if(status == STOP){
			init();
			break;
		}
		for(int i =0; i<10; i++){
			PORTA = rand()%256;

			if(x==10){
				if(count==1)
					x = i;
				fnd(digit[i],0x08,8);
			}
			else{
				PORTA = 0xC0;
				fnd(digit[x],0x08,8);
			}

			if(y==10){
				if(count==2)
					y = i;
				fnd(digit[9-i],0x04,8);
			}
			else{
				PORTA = 0xF0;
				fnd(digit[y],0x04,8);
			}

			if(z==10){
				if(count==3)
					z = i;
				fnd(digit[i],0x02,8);
			}
			else{
				PORTA = 0xFC;
				fnd(digit[z],0x02,8);
			}

			if(count>3){
				if(x==y && y==z){
					printAction(1);
					serveMotorControl();
				}
				else
					printAction(2);
			}
		}
	}
}
/*
int selectDollNumber(){

}

int getButtonNumber(){

}

void getThrowNumber(){
	int dollNumber = selectDollNumber();
	int buttonNumber = getButtonNumber();
	while(1){
		fnd(digital[dollNumber/10],0x08,8);
	}
}
*/
int main(){
	DDRA = 0xff; //SWITCH
	DDRB = 0x10; //port B bit4 output
	DDRC = 0xff; //FND data
	DDRG = 0x0f; //FND select
	DDRE = 0xcf; //INT 4,5

	EICRB = 0x0a; //falling edge
	EIMSK = 0x30; //interupt en
	sei(); //interupt enable

	timer3_pwm_init();

	while(1){
		getMainScreen();
		if(game_select ==0)
			getGambleNumber();
		//else
			//getThrowNumber();
	}
}
