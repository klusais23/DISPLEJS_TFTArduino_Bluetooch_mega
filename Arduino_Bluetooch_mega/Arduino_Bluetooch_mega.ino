
// ARDUINO MEGA   pins   MOSI 51    MISO 50    SCK 52    int 47    cs (chip select )49
////////////////////////////////////////////////////////
//


//Axilometer   A15 = vcc   A14=x   A13 = y  A12= z   A11=GND


#include <mcp_can.h>
#include <SPI.h>
#include <Adafruit_TFTLCD.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>



#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

#define TS_MINX 122
#define TS_MINY 73
#define TS_MAXX 914
#define TS_MAXY 914

#define YP A3
#define XM A2
#define YM 9
#define XP 8

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);


TouchScreen ts = TouchScreen(XP, YP, XM, YM, 364);

boolean buttonEnabled = true;
int screen_nr = 1;
boolean ExtraButtonScreen0 = false;
boolean ExtraButtonScreen1 = false;
boolean ExtraButtonScreen2 = false;

bool antilag = false;
bool relay1 = false;
bool relay2 = false;
bool relay3 = false;
bool relay4 = false;
bool chekEngineLight = true;


struct CanInMesages
{
	unsigned int rpm;
	int map;
	int iat;
	int ect;
	unsigned char  afr;	
	int tps;
};

CanInMesages parameters;

byte dataToSend[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
unsigned long CanSendTimer = 0;

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
char msgString[128];                        // Array to store serial string

#define CAN0_INT 47                              // Set INT to pin 2
MCP_CAN CAN0(49);                               // Set CS to pin 10

void FirstSceen() {

	tft.fillScreen(BLACK);
	tft.drawRect(0, 0, 319, 240, YELLOW);

	tft.setCursor(40, 50);
	tft.setTextColor(GREEN);
	tft.setTextSize(8);
	tft.print("P D K");

	tft.setCursor(65, 130);
	tft.setTextColor(WHITE);
	tft.setTextSize(3);
	tft.print("ChipTuning");

	tft.setCursor(80, 180);
	tft.setTextColor(YELLOW);
	tft.setTextSize(3);
	tft.print("CAN BUS ");

	//PrintButtonsTop();
}


void setup()
{
	Serial.begin(115200);

	tft.reset();
	uint16_t identifier = tft.readID();
	tft.begin(identifier);
	tft.setRotation(1);

	// Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
	if (CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK)
		Serial.println("MCP2515 Initialized Successfully!");
	else
		Serial.println("Error Initializing MCP2515...");

	CAN0.setMode(MCP_NORMAL);                     // Set operation mode to normal so the MCP2515 sends acks to received data.

	pinMode(CAN0_INT, INPUT_PULLUP);                            // Configuring pin for /INT input
	pinMode(45, OUTPUT);
	digitalWrite(45, HIGH);                          //  POWER +5 Volt for can module

	pinMode(A15, OUTPUT);
	digitalWrite(A15, HIGH);
	pinMode(A11, OUTPUT);
	digitalWrite(A11, LOW);

	FirstSceen();
	delay(3000);
	GetScreen();
}

void loop()
{

	getMeasge();
	TachpadLocation();

	if ((millis() - CanSendTimer) > 250)
	{
		byte sndStat = CAN0.sendMsgBuf(0x100, 0, 8, dataToSend);
		//Serial.println(dataToSend[0]);

		if (parameters.ect>105)
		{
			chekEngineLight = true;
			
		}

		if (relay2)
		{
			if (parameters.tps>95)
			{
				relay2 = false;
				dataToSend[2] = 0;
				if (ExtraButtonScreen0 )
				{
					Screen0();
				}
			}
		}

		CanSendTimer = millis();
	}

}

void PrintButtonsTop() {
	tft.fillRect(5, 5, 35, 35, GREEN);
	tft.drawRect(5, 5, 35, 35, BLACK);
	tft.setCursor(10, 15);
	tft.setTextColor(WHITE);
	tft.setTextSize(2);
	tft.print("<<");

	tft.fillRect(280, 5, 35, 35, GREEN);
	tft.drawRect(280, 5, 35, 35, BLACK);
	tft.setCursor(285, 15);
	tft.setTextColor(WHITE);
	tft.setTextSize(2);
	tft.print(">>");
}

void getMeasge() {

	if (!digitalRead(CAN0_INT))                         // If CAN0_INT pin is low, read receive buffer
	{
		CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)			

		sprintf(msgString, "Standard ID: 0x%.3lX       DLC: %1d  Data:", rxId, len);   // Determine if ID is standard (11 bits) or extended (29 bits)

		Serial.print(msgString);


		for (byte i = 0; i < len; i++) {
			sprintf(msgString, " 0x%.2X", rxBuf[i]);
			Serial.print(msgString);

		}
		Serial.print(msgString);
		Serial.println();
		
		unsigned char a = 0;
		unsigned char b = 0;

		switch (rxId)
		{
		case 0x280:
			parameters.rpm = 0;
			a = rxBuf[2];
			b = rxBuf[3];
			
			((unsigned char*)(&parameters.rpm))[0] = a;
			((unsigned char*)(&parameters.rpm))[1] = b;

			//Serial.println("debug 640/////////////////");
			if (ExtraButtonScreen1)
			{
				Screen1(true);
			}
			break;
		case 0x5f2:
			parameters.map = 0;
			a = rxBuf[2];
			b = rxBuf[3];
			((unsigned char*)(&parameters.map))[0] = b;
			((unsigned char*)(&parameters.map))[1] = a;
			parameters.map = parameters.map - 1000;

			parameters.iat = 0;
			a = rxBuf[4];
			b = rxBuf[5];
			((unsigned char*)(&parameters.iat))[0] = b;
			((unsigned char*)(&parameters.iat))[1] = a;
			parameters.iat = ((parameters.iat  / 10) -32) * 5 / 9;

			parameters.ect = 0;
			a = rxBuf[6];
			b = rxBuf[7];
			((unsigned char*)(&parameters.ect))[0] = b;
			((unsigned char*)(&parameters.ect))[1] = a;
			parameters.ect = ((parameters.ect / 10) - 32) * 5 / 9;

			break;
		case 0x5f3:
			parameters.tps = 0;
			a = rxBuf[0];
			b = rxBuf[1];
			((unsigned char*)(&parameters.tps))[0] = b;
			((unsigned char*)(&parameters.tps))[1] = a;
			parameters.tps = map(parameters.tps, 0, 1023, 0, 100);	

			break;

		case 0x60F:
			//parameters.afr = 0;
			a = rxBuf[0];
			parameters.afr = a;
						

			break;
		default:
			break;
		}
	}
}

unsigned char CalcCHK() {

	int ch = 0;
	ch += rxId += len;
	for (size_t i = 0; i < sizeof(rxBuf); i++)
	{
		ch += rxBuf[i];
	}

	int u = ch / 11;
	unsigned char chk = 11 - u;
	Serial.println(ch);
	Serial.println(u);
	Serial.println(chk);

	return chk;
}


void TachpadLocation() {

	TSPoint p = ts.getPoint();

	if (p.z > ts.pressureThreshhold) {

		p.x = map(p.x, TS_MAXX, TS_MINX, 0, 240);
		p.y = map(p.y, TS_MAXY, TS_MINY, 0, 320);

		Serial.print(p.x);
		Serial.print("    ");
		Serial.print(p.y);
		Serial.print("    ");
		Serial.println(p.z);

		if (p.x > 0 && p.x < 40 && p.y > 0 && p.y < 40 && buttonEnabled) {

			buttonEnabled = false;
			screen_nr--;

			pinMode(XM, OUTPUT);
			pinMode(YP, OUTPUT);
			Serial.println("minussss");

			GetScreen();
			delay(200);

		}
		if (p.x > 0 && p.x < 40 && p.y > 280 && p.y < 320 && buttonEnabled) {

			buttonEnabled = false;
			screen_nr++;

			pinMode(XM, OUTPUT);
			pinMode(YP, OUTPUT);
			Serial.println("Pluss");

			GetScreen();
			delay(200);

		}

		if (p.x > 44 && p.x < 67 && p.y > 47 - 40 && p.y < 218 - 40 && buttonEnabled && ExtraButtonScreen0) {

			buttonEnabled = false;

			pinMode(XM, OUTPUT);
			pinMode(YP, OUTPUT);

			antilag = !antilag;
			Serial.print("ALS ");
			Serial.println(antilag);
			Screen0();
			delay(200);

		}
		if (p.x > 95 && p.x < 120 && p.y > 47 - 40 && p.y < 218 - 40 && buttonEnabled && ExtraButtonScreen0) {

			buttonEnabled = false;

			pinMode(XM, OUTPUT);
			pinMode(YP, OUTPUT);

			relay1 = !relay1;
			Serial.print("rel1 ");
			Serial.println(relay1);
			Screen0();
			delay(200);

		}
		if (p.x > 144 && p.x < 167 && p.y > 47 - 40 && p.y < 218 - 40 && buttonEnabled && ExtraButtonScreen0) {

			buttonEnabled = false;

			pinMode(XM, OUTPUT);
			pinMode(YP, OUTPUT);

			relay2 = !relay2;
			Serial.print("relay2 ");
			Serial.println(relay2);
			Screen0();
			delay(200);

		}
		if (p.x > 194 && p.x < 218 && p.y > 47 - 40 && p.y < 218 - 40 && buttonEnabled && ExtraButtonScreen0) {

			buttonEnabled = false;

			pinMode(XM, OUTPUT);
			pinMode(YP, OUTPUT);

			relay3 = !relay3;
			Serial.print("relay3 ");
			Serial.println(relay3);
			chekEngineLight = !chekEngineLight;
			Screen0();
			delay(200);

		}
		if (buttonEnabled && ExtraButtonScreen2) {

			buttonEnabled = false;

			Screen2(p.x, p.y);

		}
		if (p.x >230 && p.x < 260 && p.y > 0 && p.y < 20 && buttonEnabled && ExtraButtonScreen2) {

			buttonEnabled = false;

			GetScreen();
			delay(100);

		}
	}

}

void GetScreen() {

	if (screen_nr > 2) screen_nr = 0;
	if (screen_nr < 0) screen_nr = 2;

	if (screen_nr != 0) ExtraButtonScreen0 = false;
	if (screen_nr != 1) ExtraButtonScreen1 = false;
	if (screen_nr != 2) ExtraButtonScreen2 = false;


	Serial.println(screen_nr);

	switch (screen_nr)
	{
	case 0:
		tft.fillScreen(BLACK);
		PrintButtonsTop();
		prinoOnTEST(screen_nr);
		Screen0();

		ExtraButtonScreen0 = true;

		delay(200);
		buttonEnabled = true;
		break;

	case 1:
		tft.fillScreen(BLACK);
		PrintButtonsTop();
		prinoOnTEST(screen_nr);

		Screen1(false);

		ExtraButtonScreen1 = true;

		delay(200);
		buttonEnabled = true;
		break;
	case 2:
		tft.fillScreen(BLACK);
		PrintButtonsTop();
		prinoOnTEST(screen_nr);

		Screen2(0, 0);

		ExtraButtonScreen2 = true;

		delay(200);
		buttonEnabled = true;
		break;

	default:
		break;
	}


}

void prinoOnTEST(int n) {

	tft.setCursor(45, 10);
	tft.setTextColor(WHITE);
	tft.setTextSize(1);
	tft.print("Screen NR. ");
	tft.print(n);

}

void Screen0() {
	pinMode(XM, OUTPUT);
	pinMode(YP, OUTPUT);

	if (antilag)
	{
		tft.fillRect(45 - 40, 45, 170, 23, GREEN);
		tft.setCursor(50 - 40, 50);
		tft.setTextColor(BLACK);
		tft.setTextSize(2);
		tft.print("ANTI LAG ON ");

		dataToSend[0] = 1;
	}
	else
	{
		tft.fillRect(45 - 40, 45, 170, 23, YELLOW);
		tft.setCursor(50 - 40, 50);
		tft.setTextColor(BLACK);
		tft.setTextSize(2);
		tft.print("ANTI LAG OFF ");
		dataToSend[0] = 0;
	}

	if (relay1)
	{
		tft.fillRect(45 - 40, 95, 170, 23, GREEN);
		tft.setCursor(50 - 40, 100);
		tft.setTextColor(BLACK);
		tft.setTextSize(2);
		tft.print("SPORT ON ");

		dataToSend[1] = 1;
	}
	else
	{
		tft.fillRect(45 - 40, 95, 170, 23, YELLOW);
		tft.setCursor(50 - 40, 100);
		tft.setTextColor(BLACK);
		tft.setTextSize(2);
		tft.print("SPORT OFF ");
		dataToSend[1] = 0;
	}

	if (relay2)
	{
		tft.fillRect(45 - 40, 145, 170, 23, GREEN);
		tft.setCursor(50 - 40, 150);
		tft.setTextColor(BLACK);
		tft.setTextSize(2);
		tft.print("Launch ON ");

		dataToSend[2] = 1;
	}
	else
	{
		tft.fillRect(45 - 40, 145, 170, 23, YELLOW);
		tft.setCursor(50 - 40, 150);
		tft.setTextColor(BLACK);
		tft.setTextSize(2);
		tft.print("Launch OFF ");
		dataToSend[2] = 0;
	}

	if (relay3)
	{
		tft.fillRect(45 - 40, 195, 170, 23, GREEN);
		tft.setCursor(50 - 40, 200);
		tft.setTextColor(BLACK);
		tft.setTextSize(2);
		tft.print("relay3 ON ");

		dataToSend[3] = 1;
	}
	else
	{
		tft.fillRect(45 - 40, 195, 170, 23, YELLOW);
		tft.setCursor(50 - 40, 200);
		tft.setTextColor(BLACK);
		tft.setTextSize(2);
		tft.print("relay3 OFF ");
		dataToSend[3] = 0;
	}

	if (chekEngineLight)
	{
		//tft.fillRect(195, 195, 80, 23, GREEN);
		tft.fillCircle(250, 190, 45, GREEN);
		tft.setCursor(215, 185);
		tft.setTextColor(BLACK);
		tft.setTextSize(2);
		tft.print("ALL OK ");
	}
	else
	{
		//tft.fillRect(195, 195, 80, 23, RED);
		tft.fillCircle(250, 190, 45, RED);
		tft.setCursor(225, 185);
		tft.setTextColor(BLACK);
		tft.setTextSize(2);
		tft.print("STOP");

	}

	buttonEnabled = true;
}


void Screen1(bool x) {

	pinMode(XM, OUTPUT);
	pinMode(YP, OUTPUT);

	if (x)
	{
		tft.fillRect(55, 45, 80, 23, BLACK);
		tft.setCursor(60, 50);

		if (parameters.rpm<400)
		{
			tft.setTextColor(BLUE);
			tft.setTextSize(2);
			tft.print(parameters.rpm);
		}
		else if(parameters.rpm>7000){
			tft.setTextColor(RED);
			tft.setTextSize(2);
			tft.print(parameters.rpm);
		}
		else
		{
			tft.setTextColor(YELLOW);
			tft.setTextSize(2);
			tft.print(parameters.rpm);
		}
		
		tft.drawFastHLine(150, 55, 200, BLACK);
		tft.drawFastHLine(150, 55, parameters.rpm / 50, RED);

		tft.fillRect(55, 65, 80, 23, BLACK);
		tft.setCursor(60, 70);
		tft.setTextColor(YELLOW);
		tft.setTextSize(2);
		tft.print(parameters.map);

		tft.fillRect(55, 85, 80, 23, BLACK);
		tft.setCursor(60, 90);
		tft.setTextColor(YELLOW);
		tft.setTextSize(2);
		tft.print(parameters.iat);

		tft.fillRect(55, 105, 80, 23, BLACK);
		tft.setCursor(60, 110);

		if (parameters.ect<50)
		{
			tft.setTextColor(BLUE);
			tft.setTextSize(2);
			tft.print(parameters.ect);
		}
		else if (parameters.ect>102) {
			tft.setTextColor(RED);
			tft.setTextSize(2);
			tft.print(parameters.ect);
		}
		else
		{
			tft.setTextColor(YELLOW);
			tft.setTextSize(2);
			tft.print(parameters.ect);
		}
		

		tft.fillRect(55, 125, 80, 23, BLACK);
		tft.setCursor(60, 130);
		tft.setTextColor(YELLOW);
		tft.setTextSize(2);
		tft.print(parameters.afr);

		tft.fillRect(55, 145, 80, 23, BLACK);
		tft.setCursor(60, 150);
		tft.setTextColor(YELLOW);
		tft.setTextSize(2);
		tft.print(parameters.tps);
	}
	else
	{
		tft.setCursor(10, 50);
		tft.setTextColor(YELLOW);
		tft.setTextSize(2);
		tft.print("RPM: ");

		tft.setCursor(10, 70);
		tft.setTextColor(YELLOW);
		tft.setTextSize(2);
		tft.print("MAP: ");

		tft.setCursor(10, 90);
		tft.setTextColor(YELLOW);
		tft.setTextSize(2);
		tft.print("IAT: ");

		tft.setCursor(10, 110);
		tft.setTextColor(YELLOW);
		tft.setTextSize(2);
		tft.print("ECT: ");

		tft.setCursor(10, 130);
		tft.setTextColor(YELLOW);
		tft.setTextSize(2);
		tft.print("AFR: ");

		tft.setCursor(10, 150);
		tft.setTextColor(YELLOW);
		tft.setTextSize(2);
		tft.print("TPS: ");
	}


}

void Screen2(int x, int y) {

	pinMode(XM, OUTPUT);
	pinMode(YP, OUTPUT);

	//Axilometer   A15 = vcc   A14=x   A13 = y  A12= z   A11=GND

	/*
	tft.fillRect(5, 145, 220, 23, BLACK);
	tft.setCursor(10, 150);
	tft.setTextColor(YELLOW);
	tft.setTextSize(2);
	tft.print("Z:");
	tft.print(analogRead(A12));
	tft.print(" Y:");
	tft.print(analogRead(A13));
	tft.print(" X:");
	tft.print(analogRead(A14));
	*/

	tft.drawPixel(y, x, WHITE);

	buttonEnabled = true;

}


