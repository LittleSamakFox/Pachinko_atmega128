/********************************
 * 마이크로프로세서설계및실습	*
 * 2020-2학기 기말 팀프로젝트	*
 *								*
 * 사이버펑크2020				*
 * 2015920035 이정석			*
 * 2016920050 정진용			*
 *								*
 * 랜덤숫자 미니 인형뽑기		*
 ********************************/

#define F_CPU 16000000UL
#define STOP 0
#define GO 1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

unsigned char fnd_sel[4] = { 0x01, 0x02, 0x04, 0x08 };
unsigned char digit[10] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7c, 0x07, 0x7f, 0x67 };

volatile int status = STOP;
volatile int game_select = 0;

volatile int count = 0;
volatile int x;
volatile int y;
volatile int z;
volatile int time;
volatile int firstNumber;
volatile int secondNumber;
volatile int firstRandomNumber;
volatile int secondRandomNumber;
volatile int throwtime;
volatile int handler1;
volatile int handler2;

volatile int bounced = 0;		// bouncing으로 입력된 횟수
volatile int pressed = 0;		// 버튼이 눌렸는지 판별하기 위한 변수
volatile int bounce_value_R = 5;	// 일정 수 bouncing이 일어났을 때 버튼이 눌렸다고 판단하기 위한 기준
volatile int bounce_value_B = 40;	// R, B 버튼의 내장 LED에 따라 각각 기준 숫자가 다르게 설정되어야 함

void init();									// 인자 초기화
void timer3_pwm_init();							// PWM 타이머 초기화
void serveMotorControl();						// 서브모터 동작
void fnd(int number, int section, int delay);	// FND 동작
void getMainScreen();							// 메인화면 게임 선택
void buzzer(int hz, int hzcount);				// buzzer 소리
void printAction(int flag);						// 승리, 실패 화면
void levelOne();								// LV 1 게임
void levelTwo();								// LV 2 게임
ISR(INT4_vect);		// R 버튼 제어 인터럽트 함수(select, slot stop)
ISR(INT5_vect);		// B 버튼 제어 인터럽트 함수(start, exit)

int main() {
	DDRA = 0xFF;	// LED 출력 설정
	DDRB = 0x10;	// buzzer 설정 (PORTB 4번 핀 출력)
	DDRC = 0xFF;	// FND 출력 데이터 설정
	DDRG = 0x0F;	// FND 출력 위치 설정
	DDRE = 0xCF;	// 버튼 입력 설정 (PORTE 4, 5번 핀 입력)

	EICRB = 0x0F;	// 4, 5번 핀에 Upper-edge Interrupt 설정 (0x0F == 0b00001111)
	EIMSK = 0x30;	// 4, 5번 핀에 Interrupt 마스크 레지스터 설정 (0x30 = 0b00110000)
	sei();			// Interrupt 허용 설정

	timer3_pwm_init();
	while (1) {
		getMainScreen();
		if (game_select == 0)
			levelOne();
		else
			levelTwo();
	}
}

/* 빨간색 버튼 : select, slot stop
 * 이 버튼을 누르면 PORTE, 4번 핀으로 Interrupt 신호를 보냄
 * count 값을 변화시켜 게임 선택, 슬롯 번호 결정 기능을 수행
 */
ISR(INT4_vect)
{
	if (pressed == 0)
	{
		// 만약 bouncing이 일정 수 이상 일어나면 pressed로 판단
		if (bounced > bounce_value_R)
		{
			// 버튼을 누르고 수행할 연산 : count 증가
			count++;

			bounced = 0;
			pressed = 1;
			_delay_ms(10);
		}
	}
	// 만약 pressed 되었는데 계속 bouncing이 일어나면 모두 0으로 초기화
	else if (pressed == 1 && bounced > 0)
	{
		pressed = 0;
		bounced = 0;
	}
	bounced++;
}

/* 파란색 버튼 : start, exit
 * 이 버튼을 누르면 PORTE, 5번 핀으로 Interrupt 신호를 보냄
 * status 값을 변화시켜 게임 시작, 메뉴 화면으로 돌아가는 기능을 수행
 */
ISR(INT5_vect)
{
	if (pressed == 0)
	{
		// 만약 bouncing이 일정 수 이상 일어나면 pressed로 판단
		if (bounced > bounce_value_B)
		{
			// 버튼을 누르고 수행할 연산 : status 상태 변환
			if (status == STOP)
			{
				status = GO;
			}
			else
			{
				status = STOP;
			}

			bounced = 0;
			pressed = 1;
			_delay_ms(10);
		}
	}
	// 만약 pressed 되었는데 계속 bouncing이 일어나면 모두 0으로 초기화
	else if (pressed == 1 && bounced > 0)
	{
		pressed = 0;
		bounced = 0;
	}
	bounced++;
}

/* 게임 시작 전 변수 초기화 함수 (Lv.1, Lv.2) */
void init() {
	count = 0;
	x = 10;
	y = 10;
	z = 10;
	time = 0;
	firstNumber = 10;
	secondNumber = 10;
	firstRandomNumber = 10;
	secondRandomNumber = 10;
	throwtime = 0;
	handler1 = rand() % 10 + 100;
	handler2 = rand() % 20 + 144;
}

/* PWM 타이머 초기화 함수
 * PORTE 3번 핀을 통해 타이머 인터럽트 구동, 모터 제어
 */
void timer3_pwm_init() {
	DDRE |= (1 << 3);										// 타이머 bit 설정
	TCCR3A |= (1 << COM3A1) | (1 << WGM31);
	TCCR3B |= (1 << WGM33) | (1 << WGM32) | (1 << CS32);
	ICR3 = 1250;
	OCR3A = 90;												// 서보모터 초기 설정은 가로 방향으로 놓이도록 함
}

/* 서보모터 제어 함수 */
void serveMotorControl() {
	OCR3A = 90;			// 가로 방향
	_delay_ms(1000);
	OCR3A = 150;		// 세로 방향
	_delay_ms(1000);
	OCR3A = 90;			// 다시 가로 방향
	_delay_ms(1000);
}

/* FND 출력 함수 */
void fnd(int number, int section, int delay) {
	PORTC = number;		// FND 출력 내용
	PORTG = section;	// FND 출력 위치
	_delay_ms(delay);	// FND 출력 시간
}

/* 구동 초기 레벨 선택 화면 출력 함수 */
void getMainScreen() {
	while (1) {
		if (status == GO) {
			init();
			break;
		}
		fnd(0x38, 0x08, 2);		// L
		fnd(0x9C, 0x04, 2);		// v.

		// count가 짝수면 Lv.1 게임
		if (count % 2 == 0) {
			game_select = 0;
			fnd(0x30, 0x01, 2);	// 1
		}
		// count가 홀수면 Lv.2 게임
		else {
			game_select = 1;
			fnd(0x5B, 0x01, 2);	// 2
		}
	}
}

/* 부저 작동 함수 */
void buzzer(int hz, int hzcount) {
	int i, ms;
	ms = 500 / hz;
	for (i = 0; i < hzcount; i++) {
		PORTB = 0x10;
		_delay_ms(ms);
		PORTB = 0x00;
		_delay_ms(ms);
	}
}

/* 게임 결과(승리, 실패) 출력 함수 */
void printAction(int flag) {
	while (1) {
		// status : STOP -> 화면 출력 완료, GO로 바꾼 후 함수 종료
		// status : GO   -> 화면 출력 개시
		if (status == STOP) {
			init();
			status = GO;
			break;
		}
		// "Win" 출력 (W는 두 개의 FND를 이용해 출력)
		if (flag == 1) {
			PORTA = 0xff;

			buzzer(480, 12);
			fnd(0x3C, 0x08, 2);	// W 왼쪽 부분
			fnd(0x1E, 0x04, 2);	// W 오른쪽 부분

			buzzer(320, 8);
			fnd(0x30, 0x02, 2);	// I
			fnd(0x37, 0x01, 2);	// n

			time++;
			if (time > 35)
				status = STOP;
		}
		// "dead" 출력
		else {
			PORTA = rand() % 256;

			buzzer(480, 12);
			fnd(0x5E, 0x08, 2);	// d
			fnd(0x79, 0x04, 2);	// E

			buzzer(320, 8);
			fnd(0x77, 0x02, 2);	// A
			fnd(0x5E, 0x01, 2);	// d

			time++;
			if (time > 35)
				status = STOP;
		}
	}
}

/* Lv.1 게임
 *
 * x, y, z 변수는 10으로 초기화
 * 버튼 누를 때마다 count 증가
 *
 * count : 1 -> 첫 번째 슬롯(x) 숫자 결정
 * count : 2 -> 두 번째 슬롯(y) 숫자 결정
 * count : 3 -> 세 번째 슬롯(z) 숫자 결정
 *
 * count가 3이 넘어가면 x, y, z 비교
 * 모든 값이 같으면 win, 그 외의 경우 dead
 */
void levelOne() {
	while (1) {
		// 게임 시작 전 초기화 세팅
		if (status == STOP) {
			init();
			break;
		}

		// 슬롯 숫자 출력
		for (int i = 0; i < 10; i++) {
			// LED 효과 추가
			PORTA = rand() % 256;

			// 첫 번째 슬롯 출력
			if (x == 10) {
				if (count == 1)
					x = i;
				fnd(digit[i], 0x08, 8);
			}
			else {
				PORTA = 0xC0;
				fnd(digit[x], 0x08, 8);
			}

			// 두 번째 슬롯 출력
			if (y == 10) {
				if (count == 2)
					y = i;
				fnd(digit[9 - i], 0x04, 8);
			}
			else {
				PORTA = 0xF0;
				fnd(digit[y], 0x04, 8);
			}

			// 세 번째 슬롯 출력
			if (z == 10) {
				if (count == 3)
					z = i;
				fnd(digit[i], 0x02, 8);
			}
			else {
				PORTA = 0xFC;
				fnd(digit[z], 0x02, 8);
			}

			// 결과 출력, 성공 시 서보모터 구동
			if (count > 3) {
				if (x == y && y == z) {
					printAction(1);
					serveMotorControl();
				}
				else
					printAction(2);
			}
		}
	}
}

/* Lv.2 게임
 *
 * firstNumber, secondNumber 변수는 10으로 초기화
 * 버튼 누를 때마다 count 증가
 *
 * count : 1 -> 첫 번째 숫자(firstNumber) 숫자 결정
 * count : 2 -> 두 번째 슬롯(secondNumber) 숫자 결정
 *
 * handler1, handler2는 init()을 통해 미리 랜덤하게 숫자가 부여되어 있음
 * 두 숫자가 결정된 후 handler1, handler2에 따라 throwtime 값이 계속 증가
 * throwtime이 handler1, handler2보다 클 때 firstRandomNumber, secondRandomNumber 결정
 * 모든 값이 같으면 win, 그 외의 경우 dead
 */
void levelTwo() {
	while (1) {
		if (status == STOP) {
			init();
			break;
		}
		for (int i = 0; i < 10; i++) {
			// 버튼을 눌러 첫 번째 숫자 결정
			if (firstNumber == 10) {
				if (count == 1)
					firstNumber = i;
				fnd(digit[i], 0x08, 8);
			}
			else {
				PORTA = 0xC0;
				fnd(digit[firstNumber], 0x08, 8);
			}

			// 버튼을 눌러 두 번째 숫자 결정
			if (secondNumber == 10) {
				if (count == 2)
					secondNumber = i;
				else if (count == 0)
					fnd(digit[0], 0x04, 8);
				else
					fnd(digit[i], 0x04, 8);
			}
			else {
				PORTA = 0xF0;
				fnd(digit[secondNumber], 0x04, 8);
				throwtime++;
			}

			// firstRandomNumber, secondRandomNumber 값 결정
			if (throwtime > 0) {
				if (firstRandomNumber == 10) {
					if (throwtime > handler1)
						firstRandomNumber = i;
					fnd(digit[i], 0x02, 8);
				}
				else {
					PORTA = 0x0C;
					fnd(digit[firstRandomNumber], 0x02, 8);
				}

				if (secondRandomNumber == 10) {
					if (throwtime > handler2)
						secondRandomNumber = i;
					fnd(digit[i], 0x01, 8);
				}
				else {
					PORTA = 0x0F;
					fnd(digit[secondRandomNumber], 0x01, 8);
				}
			}
			// 숫자 일치 여부 확인
			if (throwtime > handler2 + 30) {
				if (firstNumber == firstRandomNumber && secondNumber == secondRandomNumber) {
					printAction(1);
					serveMotorControl();
				}
				else
					printAction(2);
			}
		}
	}
}