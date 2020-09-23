#include <ESP8266WiFi.h>
#include <espnow.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define MODEL 'S'

#if MODEL == 'M'
uint8_t broadcastAddress[] = { 0xF4, 0xCF, 0xA2, 0xD1, 0x82, 0x04 };
uint32_t HCSR04Timer(600000), sendTimer(0);
const uint8_t trigPin(12), echoPin(13);
uint32_t duration(0), distance(0);
uint8_t HCSR04Iter(0), sendTry(0);
bool send(false);
#endif

#if MODEL == 'S'
#include "pitches.h"
/*************************************************
 Melody credits: https://www.youtube.com/redirect?q=https%3A%2F%2Fwww.drive.google.com%2Fdrive%2Ffolders%2F1yH2TnQntyjxTACFiJCyzE9jY4F3DJgM0%3Fusp%3Dsharing&redir_token=sOm1FFj-ZK5Nn2zLavHViFT8Ewd8MTU4OTI3OTkyOEAxNTg5MTkzNTI4&v=nVVzBPgpCsM&event=video_description
 *************************************************/
int melody[] = {
  NOTE_AS4, NOTE_AS4, NOTE_AS4, NOTE_AS4,
  NOTE_AS4, NOTE_AS4, NOTE_AS4, NOTE_AS4,
  NOTE_AS4, NOTE_AS4, NOTE_AS4, NOTE_AS4,
  NOTE_AS4, NOTE_AS4, NOTE_AS4, NOTE_AS4,
  NOTE_AS4, NOTE_AS4, NOTE_AS4, NOTE_AS4,
  NOTE_D5, NOTE_D5, NOTE_D5, NOTE_D5,
  NOTE_C5, NOTE_C5, NOTE_C5, NOTE_C5,
  NOTE_F5, NOTE_F5, NOTE_F5, NOTE_F5,
  NOTE_G5, NOTE_G5, NOTE_G5, NOTE_G5,
  NOTE_G5, NOTE_G5, NOTE_G5, NOTE_G5,
  NOTE_G5, NOTE_G5, NOTE_G5, NOTE_G5,
  NOTE_C5, NOTE_AS4, NOTE_A4, NOTE_F4,
  NOTE_G4, 0, NOTE_G4, NOTE_D5,
  NOTE_C5, 0, NOTE_AS4, 0,
  NOTE_A4, 0, NOTE_A4, NOTE_A4,
  NOTE_C5, 0, NOTE_AS4, NOTE_A4,
  NOTE_G4,0, NOTE_G4, NOTE_AS5,
  NOTE_A5, NOTE_AS5, NOTE_A5, NOTE_AS5,
  NOTE_G4,0, NOTE_G4, NOTE_AS5,
  NOTE_A5, NOTE_AS5, NOTE_A5, NOTE_AS5,
  NOTE_G4, 0, NOTE_G4, NOTE_D5,
  NOTE_C5, 0, NOTE_AS4, 0,
  NOTE_A4, 0, NOTE_A4, NOTE_A4,
  NOTE_C5, 0, NOTE_AS4, NOTE_A4,
  NOTE_G4,0, NOTE_G4, NOTE_AS5,
  NOTE_A5, NOTE_AS5, NOTE_A5, NOTE_AS5,
  NOTE_G4,0, NOTE_G4, NOTE_AS5,
  NOTE_A5, NOTE_AS5, NOTE_A5, NOTE_AS5
};

int noteDurations[] = {
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
  4,4,4,4,
};
uint8_t thisNote(0), oledLevel(50), buttonState(0);
uint16_t levelMin(0), levelMax(100);
uint32_t oledTimer(0), piezoTimer(0), buttonTimer(0);
bool oledFan(true), buttonHandler(false), buttonLongPress(false), alertState(false), preventAlertState(false), alertLED(true), buzzerStatus(true);

Adafruit_SSD1306 display(128, 64, &Wire, -1);
#endif

typedef struct struct_message {
	uint8_t level;
	bool fan;
} struct_message;
struct_message data;
uint32_t dieTimer(0);
uint8_t dieCounter(0);

void preinit() {
	ESP8266WiFiClass::preinitWiFiOff();
}

void setup()
{
	Serial.begin(115200);
	yield();

	Serial.print(F("Heap: "));
	Serial.println(ESP.getFreeHeap(), DEC);
	Serial.print(F("MAC Address:  "));
	Serial.println(WiFi.macAddress());

	data.fan = true;
	data.level = 50;

#if MODEL == '0'
	EEPROM.begin(512);
	EEPROM.put(0, (uint16_t)0);
	EEPROM.put(2, (uint16_t)500);
	EEPROM[4] = 0;
	if (EEPROM.commit())
		Serial.println(F("Init success! Please change mode to M or S"));
	while (true)
		yield();
#endif

	WiFi.mode(WIFI_STA);
	yield();

	if (esp_now_init() != 0) {
		Serial.println(F("Error initializing ESP-NOW"));
		delay(10000);
		return;
	}

#if MODEL == 'M'
	esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
	esp_now_register_send_cb(OnDataSent);

	esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 14, NULL, 0);
	data.level = 50;
	data.fan = false;

	pinMode(12, OUTPUT);
	pinMode(13, INPUT);
	pinMode(5, INPUT);
#endif

#if MODEL == 'S'
		esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
		esp_now_register_recv_cb(OnDataRecv);

		pinMode(12, INPUT);
		pinMode(13, OUTPUT);

		if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
		{
			delay(10000);
			return;
		}

		display.clearDisplay();
		display.display();
		display.setTextWrap(true);
		display.setTextColor(WHITE);

		display.setTextSize(2);
		display.setCursor(10, 1);
		display.print(F("Furnance"));
		display.setCursor(10, 18);
		display.print(F("Fuel"));
		display.setCursor(10, 35);
		display.print(F("Level"));
		display.setTextSize(1);
		display.setCursor(10, 55);
		display.print(F("By David Friday"));
		display.display();

		EEPROM.begin(512);
		EEPROM.get(0, levelMin);
		EEPROM.get(2, levelMax);
		buzzerStatus = EEPROM[4];
		delay(3000);
#endif
}

void loop()
{
	yield();

#if MODEL == 'M'

	if ((millis() - HCSR04Timer) > 600000) {
		digitalWrite(trigPin, LOW);
		delayMicroseconds(2);
		digitalWrite(trigPin, HIGH);
		delayMicroseconds(10);
		digitalWrite(trigPin, LOW);
		duration = pulseIn(echoPin, HIGH);
		distance += duration / 58;
		HCSR04Iter++;

		if (HCSR04Iter > 99)
		{
			uint8_t level(distance / 100);
			if (data.level != level)
			{
				data.level = level;
				send = true;
			}
			HCSR04Iter = distance = 0;
			HCSR04Timer = millis();
		}
	}

	if (digitalRead(5) == HIGH)
	{
		if (data.fan == false)
		{
			data.fan = true;
			send = true;
		}
	}
	else if (data.fan == true)
	{
		data.fan = false;
		send = true;
	}

	if (send)
		if ((millis() - sendTimer) > 1000) {
			esp_now_send(broadcastAddress, (uint8_t *)&data, sizeof(data));
			sendTimer = millis();
			if (++sendTry > 19)
				sendTry = send = false;
		}
#endif
	
#if MODEL == 'S'
	if (buttonHandler) {
		if (digitalRead(12) == LOW && !buttonLongPress)
		{
			if (alertState)
				alertState = false;
			else if (buttonState == 1 || buttonState == 2)
			{
				if (++levelMin > 999)
					levelMin = 999;
				buttonState = 1;
			}
			else if (buttonState == 3 || buttonState == 4)
			{
				if (--levelMin > 999)
					levelMin = 0;
				buttonState = 3;
			}
			else if (buttonState == 5 || buttonState == 6)
			{
				if (++levelMax > 999)
					levelMax = 0;
				buttonState = 5;
			}
			else if (buttonState == 7 || buttonState == 8)
			{
				if (--levelMax > 999)
					levelMax = 0;
				buttonState = 7;
			}
			else if (buttonState == 9 || buttonState == 10)
			{
				buzzerStatus = !buzzerStatus;
				buttonState = 9;
			}
			else if (buttonState == 11 || buttonState == 12)
			{
				EEPROM.begin(512);
				EEPROM.put(0, (uint16_t)levelMin);
				EEPROM.put(2, (uint16_t)levelMax);
				EEPROM[4] = buzzerStatus;
				if (EEPROM.commit())
				{
					display.setCursor(22, 50);
					display.print(F("     Save? OK "));
					display.display();
					delay(500);
				}
				buttonState = 0;
			}

			oledTimer = millis() + 1001;
			buttonHandler = false;
		}
		else if ((millis() - buttonTimer) > 1000)
			if (buttonLongPress = digitalRead(12) == LOW ? false : true)
			{
				if (buttonState == 0)
					buttonState = 1;
				else if (buttonState == 1 || buttonState == 2)
					buttonState = 3;
				else if (buttonState == 3 || buttonState == 4)
					buttonState = 5;
				else if (buttonState == 5 || buttonState == 6)
					buttonState = 7;
				else if (buttonState == 7 || buttonState == 8)
					buttonState = 9;
				else if (buttonState == 9 || buttonState == 10)
					buttonState = 11;
				else 
					buttonState = 0;

				oledTimer = millis() + 1001;
				buttonTimer = millis();
			}
			else
				buttonHandler = false;
	}
	else if (digitalRead(12) == HIGH)
	{
		buttonHandler = true;
		buttonTimer = millis();
	}
	

	if ((millis() - oledTimer) > 1000) {

		display.clearDisplay();
		display.setTextSize(1);
		display.setCursor(35, 0);
		display.setTextColor(WHITE);
		display.print(F("FUEL LEVEL"));

		if (buttonState != 9 && buttonState != 10) {
			display.setCursor(122, 0);
			display.print((char)14);
			if (!buzzerStatus)
			{
				display.setCursor(122, 0);
				display.setTextColor(BLACK);
				display.print('/');
				display.setTextColor(WHITE);
			}
		}
		display.setCursor(0, 32);
		display.print(F("Min:"));
		display.setCursor(25, 32);
		display.print(levelMin);

		display.setCursor(85, 32);
		display.print(F("Max:"));
		display.setCursor(110, 32);
		display.print(levelMax);

		display.drawRect(0, 44, 128, 20, WHITE);
		display.setCursor(22, 50);
		if(buttonState == 0)
			if (data.fan)
				display.print(F("Furnance is ON"));
			else
			{
				if (oledFan) {
					display.fillRect(0 + 1, 44 + 1, 126, 20 - 2, WHITE);
					display.setTextColor(BLACK, WHITE);
				}
				display.print(F("Furnance is OFF"));
				oledFan = !oledFan;
			}
		
		drawProgressbar(0, 10, 128, 19, oledLevel);

		if (buttonState > 0)
		{
			display.setTextColor(WHITE);
			if (buttonState != 11 && buttonState != 12)
			{
				display.setCursor(22, 50);
				display.print(F(" Editing mode "));
			}
			if (buttonState == 1)
			{
				display.setCursor(45, 32);
				display.print('/');
				display.setCursor(49, 32);
				display.print('\\');
				buttonState = 2;
			}
			else if (buttonState == 2)
				buttonState = 1;

			if (buttonState == 3)
			{
				display.setCursor(45, 32);
				display.print('\\');
				display.setCursor(49, 32);
				display.print('/');
				buttonState = 4;
			}
			else if (buttonState == 4)
				buttonState = 3;

			if (buttonState == 5)
			{
				display.setCursor(72, 32);
				display.print('/');
				display.setCursor(76, 32);
				display.print('\\');
				buttonState = 6;
			}
			else if (buttonState == 6)
				buttonState = 5;

			if (buttonState == 7)
			{
				display.setCursor(72, 32);
				display.print('\\');
				display.setCursor(76, 32);
				display.print('/');
				buttonState = 8;
			}
			else if (buttonState == 8)
				buttonState = 7;

			if (buttonState == 9)
			{
				display.setCursor(122, 0);
				display.print((char)14);
				if (!buzzerStatus)
				{
					display.setCursor(122, 0);
					display.setTextColor(BLACK);
					display.print('/');
				}
				buttonState = 10;
			}
			else if (buttonState == 10)
				buttonState = 9;

			if (buttonState == 11)
			{
				display.setCursor(22, 50);
				display.print(F("     Save?    "));
				buttonState = 12;
			}
			else if (buttonState == 12)
				buttonState = 11;
		}
		display.display();

		if (oledLevel < 10)
			alertLED = true;
		else
			alertLED = false;

		if (alertLED)
			digitalWrite(13, HIGH);
		else
			digitalWrite(13, LOW);

		oledTimer = millis();
	}

	if (oledLevel == 0)
		if(!preventAlertState)
			alertState = preventAlertState = true;

	if (oledLevel > 10)
		preventAlertState = false;
	
	if (alertState)
	{
		if (thisNote < 112) {

			uint16_t noteDuration = 750 / noteDurations[thisNote];
			tone(14, melody[thisNote++], noteDuration);
			uint16_t pauseBetweenNotes = noteDuration * 1.30;
			delay(pauseBetweenNotes);
			noTone(14);
			piezoTimer = millis();
		}

		if ((millis() - piezoTimer) > 600000)
			if (thisNote > 111)
				thisNote = 0;
	}
#endif
	//Prevent esp stuck
	if (millis() - dieTimer >= 3000)
		Serial.printf_P(PSTR("Die counter: %d/3\n"), ++dieCounter);
	if (dieCounter > 2)
	{
		yield();
		WiFi.disconnect(true);
		delay(100);
		ESP.restart();
	}
	else
		dieCounter = 0;
	dieTimer = millis();
}

#if MODEL == 'M'
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
	Serial.print(F("Last Packet Send Status: "));
	if (sendStatus == 0)
	{
		Serial.println(F("Delivery success"));
		send = false; sendTry = 0;
	}
	else 
		Serial.println(F("Delivery fail"));
}
#endif

#if MODEL == 'S'
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
	memcpy(&data, incomingData, sizeof(data));
	oledLevel = map(data.level,levelMin,levelMax,0,100);
	yield();
}

void drawProgressbar(int x, int y, int width, int height, int progress)
{

	progress = progress > 100 ? 100 : progress;
	progress = progress < 0 ? 0 : progress;

	float bar = ((float)(width - 1) / 100) * progress;
	if (progress > 98)
		bar = width-4;
	display.drawRect(x, y, width, height, WHITE);
	display.fillRect(x + 2, y + 2, bar, height - 4, WHITE);

	if (height >= 15) {
		display.setCursor((width / 2) - 6, y + 6);
		display.setTextSize(1);
		display.setTextColor(WHITE);
		if (progress >= 50)
			display.setTextColor(BLACK, WHITE);

		display.print(progress);
		display.print('%');
	}
}
#endif