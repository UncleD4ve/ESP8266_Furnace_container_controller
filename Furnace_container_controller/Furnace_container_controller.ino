#include <ESP8266WiFi.h>
#include <espnow.h>

#define MODEL 'S'

#if MODEL == 'M'
	uint8_t broadcastAddress[] = { 0xF4, 0xCF, 0xA2, 0xD1, 0x82, 0x04 };
#else
	uint8_t broadcastAddress[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
#endif

typedef struct struct_message {
	uint8_t level;
	bool fan;
} struct_message;
struct_message data;

uint32_t wiFiTimer(0), lastTime(0), timerDelay(2000);


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

	if (MODEL == '0')
		while (true)
			yield();

	WiFi.mode(WIFI_STA);
	yield();

	if (esp_now_init() != 0) {
		Serial.println(F("Error initializing ESP-NOW"));
		return;
	}

	if (MODEL == 'M') {
		esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
		esp_now_register_send_cb(OnDataSent);

		esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 14, NULL, 0);
	}

	if (MODEL == 'S') {
		esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
		esp_now_register_recv_cb(OnDataRecv);
	}

}
void loop()
{
	yield();

	if(MODEL == 'M')
		if ((millis() - lastTime) > timerDelay) {
			data.level = 100;
			data.fan = false;

			esp_now_send(broadcastAddress, (uint8_t *)&data, sizeof(data));

			lastTime = millis();
		}
}


void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
	Serial.print(F("Last Packet Send Status: "));
	if (sendStatus == 0) 
		Serial.println(F("Delivery success"));
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