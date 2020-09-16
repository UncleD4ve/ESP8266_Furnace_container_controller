#include <ESP8266WiFi.h>
#include <espnow.h>

#define MODEL 'M'

#if MODEL == 'M'
	uint8_t broadcastAddress[] = { 0xF4, 0xCF, 0xA2, 0xD1, 0x82, 0x04 };
	uint32_t HCSR04Timer(600000), sendTimer(0);
	const uint8_t trigPin(12), echoPin(13);
	uint32_t duration(0), distance(0);
	uint8_t HCSR04Iter(0), sendTry(0);
	bool send(false);
#endif

typedef struct struct_message {
	uint8_t level;
	bool fan;
} struct_message;
struct_message data;





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

#if MODEL == '0'
	while (true)
		yield()
#endif

	WiFi.mode(WIFI_STA);
	yield();

	if (esp_now_init() != 0) {
		Serial.println(F("Error initializing ESP-NOW"));
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
	




		
	

}


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

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
	memcpy(&data, incomingData, sizeof(data));
	Serial.print(F("Bytes received: "));
	Serial.println(len);
	Serial.print(F("Level: "));
	Serial.println(data.level);
	Serial.print(F("Fan: "));
	Serial.println(data.fan);
}