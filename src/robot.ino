#include <SPI.h>
#include <Wire.h>
#include <string.h>
#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include <Adafruit_MotorShield.h>

#include "bluefruitConfig.h"

Adafruit_SSD1306 display = Adafruit_SSD1306();

Adafruit_MotorShield AFMS = Adafruit_MotorShield();

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

String BROADCAST_NAME = "bendo's robot";
String BROADCAST_CMD = String("AT+GAPDEVNAME=" + BROADCAST_NAME);

Adafruit_DCMotor *L_MOTOR = AFMS.getMotor(1);
Adafruit_DCMotor *R_MOTOR = AFMS.getMotor(4);

void error(const __FlashStringHelper*err)
{
	Serial.println(err);
	while (1);
}

uint8_t readPacket(Adafruit_BLE *ble, uint16_t timeout);
float parsefloat(uint8_t *buffer);
void printHex(const uint8_t *data, const uint32_t numBytes);
extern uint8_t packetbuffer[];

char buf[60];

void setup(void)
{
	Serial.begin(9600);

	AFMS.begin();

	L_MOTOR->setSpeed(0);
	L_MOTOR->run(RELEASE);

	R_MOTOR->setSpeed(0);
	R_MOTOR->run(RELEASE);

	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

	display.display();
	delay(1000);

	display.clearDisplay();
	display.display();

	Serial.begin(115200);

	BLEsetup();
}

float x, y;

int velocity = 0;

int L_restrict = 0;
int R_restrict = 0;

unsigned long lastAccelPacket = 0;

void loop(void)
{
	readPacket(&ble, BLE_READPACKET_TIMEOUT);

	control();

	printBatteryLife();
}

void printBatteryLife()
{
	float measuredvbat = analogRead(A7);
	measuredvbat *= 2;		// we divided by 2, so multiply back
	measuredvbat *= 3.3;		// Multiply by 3.3V, our reference voltage
	measuredvbat /= 1024;		// convert to voltage

	int procent = (measuredvbat-3.2) * 100;

	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0,0);
	display.print("Battery: ");
	display.print(measuredvbat);
	display.print("V - ");
	display.print(procent);
	display.println("%");
	display.display();
}

bool isMoving = false;

bool control()
{
	if (packetbuffer[1] == 'B') {
		uint8_t buttnum = packetbuffer[2] - '0';
		boolean pressed = packetbuffer[3] - '0';
		// Serial.println(buttnum);
		Serial.println(isMoving);
		if (pressed) {
			isMoving = true;
			if (buttnum == 5) {
				L_MOTOR -> run(FORWARD);
				R_MOTOR -> run(FORWARD);

				L_MOTOR -> setSpeed(200);
				R_MOTOR -> setSpeed(200);
			}
			if (buttnum == 6) {
				L_MOTOR -> run(BACKWARD);
				R_MOTOR -> run(BACKWARD);

				L_MOTOR -> setSpeed(200);
				R_MOTOR -> setSpeed(200);
			}
			if (buttnum == 7) {
				L_MOTOR -> run(BACKWARD);
				R_MOTOR -> run(FORWARD);

				L_MOTOR -> setSpeed(64);
				R_MOTOR -> setSpeed(64);
			}
			if (buttnum == 8) {
				L_MOTOR -> run(FORWARD);
				R_MOTOR -> run(BACKWARD);

				L_MOTOR -> setSpeed(64);
				R_MOTOR -> setSpeed(64);
			}
		}
		else {
			isMoving = false;
			L_MOTOR -> run(RELEASE);
			R_MOTOR -> run(RELEASE);
		}
		return true;
	}
	return false;
}

void BLEsetup()
{
	Serial.print(F("Initializing Bluefruit LE module"));
	message("Initializing...");

	if (!ble.begin(VERBOSE_MODE)) {
		error(F("Couldn't find Bluefruit, make sure it's in command mode!"));
		message("ERROR");
	}

	Serial.println(F("Initialization ok"));
	message("OK");

	Serial.println(F("Performing a factory reset"));
	message("Factory reset");

	if (!ble.factoryReset())
		error(F("Couldn't factory reset"));

	message("OK");

	BROADCAST_CMD.toCharArray(buf, 60);

	if (ble.sendCommandCheckOK(buf))
		Serial.println("name changed");

	delay(250);

	if (ble.sendCommandCheckOK("ATZ"))
		Serial.println("resetting");

	delay(250);

	ble.sendCommandCheckOK("AT+GAPDEVNAME");

	ble.echo(false);

	ble.info();

	ble.verbose(false);

	while (!ble.isConnected()) {
		printBatteryLife();
		delay(500);
	}

	Serial.println(F("Connected"));
	message("Connected");

	ble.setMode(BLUEFRUIT_MODE_DATA);
}

void message(const char* msg)
{
	display.clearDisplay();
	display.display();
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0,0);
	display.print(msg);
	display.display();
}
