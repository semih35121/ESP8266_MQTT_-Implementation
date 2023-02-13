/*
 * wifi_connect.c
 *
 *  Created on: Jun 7, 2022
 *      Author: ibrhm
 */
#include "wifi_connect.h"
char pingreqpacket[2] = { 0xC0, 0X00 };
char pingresppacket[2] = { 0xD0, 0X00 };
extern char tx_buffer[150];
extern char rx_buffer[150];
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
mqtt_buffer_t mqttbuffer;
extern char Uart_data[150];
extern char return_data[125];
extern uint8_t rx_flag;
extern uint8_t rx_count;
received_data_info_t received_mqttdata;
typedef void (*mymqttcallback)(const char*, const char*);
mymqttcallback receivedfirstcallbck;
void (*subscribecallback)(void);
uint16_t ProtocolNameLength;
uint16_t ClientIDLength;
uint16_t packetID = 0x01;
uint8_t level = 0x04;
uint8_t connect = 0x10;
uint8_t publishCon = 0x30;
uint8_t subscribeCon = 0x82;
char tx_infodata[150];
char uart_mqttcopy_data[100];
int cnt;
static bool send_esp8266(const char *cmd) {
if (HAL_UART_Transmit(&huart1, (uint8_t*) cmd, strlen(cmd), 100) == HAL_OK)
	return true;
return false;
}
// Gönderdiğimiz komuta göre IP BİLGİSİ VS . ONU ALMAK İÇİN ESP8266 PARSER KODU YAZABİLİRİZ.
static void flush_response() {
	uint8_t garbage[1];
	while (HAL_UART_Receive(&huart1, garbage, 1, 10) == HAL_OK)
		;
}
void mqtthandler(const char *topic, const char *payload) {
	if ((!strcmp(topic, "SEMİH54")) && (!strcmp(payload, "ON"))) {
		mqtt_publish("ledinfoiot1", "SEMledacildi");
		HAL_Delay(200);
		HAL_UART_Transmit(&huart2, (uint8_t) *tx_infodata,
				sprintf(tx_infodata, "led1acildi\r\n"), 100);
	}
	if ((!strcmp(topic, "IOT")) && (!strcmp(payload, "ON"))) {
		HAL_UART_Transmit(&huart2, (uint8_t) *tx_infodata,
				sprintf(tx_infodata, "led1acildi\r\n"), 100);

		mqtt_publish("ledinfo", "led1acildi");

	}

}

 void esp8266_rxcallback(void) {
	if (!received_mqttdata.is_data_received_start)
		received_mqttdata.is_data_received_start = true;
	received_mqttdata.received_uart_data[received_mqttdata.current_data_idx++] =
			received_mqttdata.received_uart_char_data;
	if (received_mqttdata.current_data_idx == 2
			&& (!strcmp(received_mqttdata.received_uart_data, "\r\n")
					|| (!strcmp(received_mqttdata.received_uart_data, "\n\r")))) {
		received_mqttdata.current_data_idx = 0;
	}

	else if (received_mqttdata.current_data_idx > 2
			&& ((strstr(received_mqttdata.received_uart_data, "\r")))
			|| (strstr(received_mqttdata.received_uart_data, "\n"))) {
		if(strstr(received_mqttdata.received_uart_data,"CONNECT"))
			received_mqttdata.tcp_conn_handled=true;
		received_mqttdata.is_data_first_part_recived_completely = true;
	}
	else if (strstr(received_mqttdata.received_uart_data, ">"))
		received_mqttdata.is_data_first_part_recived_completely = true;


	else {
		received_mqttdata.is_data_first_part_recived_completely = false;
	}
	if (received_mqttdata.current_data_idx
			>= sizeof(received_mqttdata.received_uart_data) - 1) {
		received_mqttdata.current_data_idx = 0;
	}
	HAL_UART_Receive_IT(&huart1,
			(uint8_t*) &received_mqttdata.received_uart_char_data, 1);
}
static void fill_with_garbage_data(char garbage_val) {
	for (int i = 0; i < sizeof(received_mqttdata.received_uart_data); i++){
		if (i == (sizeof(received_mqttdata.received_uart_data) - 1)) {
			received_mqttdata.received_uart_data[i] = '\0';
			continue;
		}
		received_mqttdata.received_uart_data[i] = garbage_val;

}
}
static uint16_t return_last_garbage_idx(char garbage_val) {
for (int i = 0; i < received_mqttdata.current_data_idx; i++)
	if (received_mqttdata.received_uart_data[i] == garbage_val) {
		return i;
	}
return -1;
}
uint8_t send_cmd_receive_total_response(const char *send_atcmd,
	uint32_t timeout) {
static is_at_cmd_sended = false;
int received_last_idx = 0;
 //Burada tüm data uzunluğu $ karakteri ile dolduruluyor.
if (is_at_cmd_sended == false) {
	fill_with_garbage_data('$');
	flush_response();
	if (send_esp8266(send_atcmd)){
		is_at_cmd_sended = true;
	}
	else
		return false;
}
uint32_t start_execution_time = HAL_GetTick();
while (!received_mqttdata.is_data_received_start)
	; //ilk data gelene kadar bekle.
while (((HAL_GetTick() - start_execution_time) < timeout)
		&& received_mqttdata.is_data_received_start)
	; //Burada received_mqtt buffer'ına datalar gelmeye başladı.
if (is_at_cmd_sended)
	received_mqttdata.is_data_received_start = true; //Data Alma işleminin bittiği düşünülüyor
if (received_mqttdata.is_data_first_part_recived_completely) {
	if (received_mqttdata.current_data_idx > 0) {
		for (int i = 0; i < 2; i++)
			received_last_idx += return_last_garbage_idx('$');
		if ((2 * return_last_garbage_idx('$')) == received_last_idx) {
			is_at_cmd_sended = false;
			received_mqttdata.is_data_first_part_recived_completely = false;
			return true;

		}

	}
}
return false;
}

static bool send_cmd_receive_expected_resp(const char *send_at_cmd,
	uint32_t timeout, char *expected_at_resp, uint16_t wait_time) {
uint32_t start_time = HAL_GetTick();
bool is_time_expired = false;
bool  is_command_received_totaly=false;
while (!(is_command_received_totaly=(send_cmd_receive_total_response(send_at_cmd, timeout)))
		&& ((HAL_GetTick() - start_time) < wait_time))
	;
if (is_command_received_totaly==false)
	is_time_expired = true;
if (!is_time_expired) {
	received_mqttdata.is_data_first_part_recived_completely = false;
	char copy_data[112] = { 0 };
	uint8_t last_received_idx;
	if ((last_received_idx = return_last_garbage_idx('$')) != -1)
		last_received_idx--; //Burada son data $ ile doldurulmuştu.1 eksiği son gelen data indeksini tutuyor.
	else
		return false;
	received_mqttdata.received_uart_data[last_received_idx + 1] = '\0';
	memcpy(copy_data, received_mqttdata.received_uart_data,
			strlen(received_mqttdata.received_uart_data));
	//received_mqttdata.received_uart_data
	if (strstr(copy_data, expected_at_resp)) {
		memset(received_mqttdata.received_uart_data, '\0',
				sizeof(received_mqttdata.received_uart_data));
		received_mqttdata.current_data_idx = 0;
		return true;
	} else
		return false;
}
return false;

}

void subscribe_handler() {//Burada Herhangi bir  Topic'e üye olma işlemleri otomatikleştirilimiş
mqtt_subscribe("SEMİH54", 0x00);
HAL_Delay(2000);
mqtt_subscribe("IOT", 0x00);
HAL_Delay(2000);
mqtt_subscribe("iot2", 0x00);
HAL_Delay(2000);

}
void read_message() {
int remain_length = 0, message_length = 0, topic_length = 0;
char message[100];
char topic[150];
HAL_UART_AbortReceive_IT(&huart1);
for (int i = 0; i < sizeof(rx_buffer); i++) {
	if (rx_buffer[i] == 0x30) {
		remain_length = rx_buffer[i + 1];
		topic_length = rx_buffer[i + 2] + rx_buffer[i + 3];
		message_length = remain_length - (topic_length + 2);
		for (int k = 0; k < topic_length; k++) {
			topic[k] = rx_buffer[i + 4 + k];
		}
		for (int j = 0; j < message_length; j++) {
			message[j] = rx_buffer[i + 4 + topic_length + j];
		}
		receivedfirstcallbck(topic, message);
		break;
	}
}
memset(rx_buffer, 0, sizeof(rx_buffer)); 						// clear buffer
memset(message, 0, sizeof(message));
memset(topic, 0, sizeof(topic));
HAL_UART_Receive_IT(&huart1, (uint8_t*) rx_buffer, 100);
}

void Wifi_connect(char *SSID, char *Password) {
bool err = false;
err += (uint8_t) send_cmd_receive_expected_resp("AT+CWMODE=1\r\n", 1500, "OK",
		600); /*HAL_UART_Transmit(&huart1,(uint8_t *)"AT+CWMODE=1\r\n",strlen("AT+CWMODE=1\r\n"),1000);*/
err += (uint8_t) send_cmd_receive_expected_resp("AT+CWQAP\r\n", 1500, "OK",
		600); /*HAL_UART_Transmit(&huart1,(uint8_t *)"AT+CWQAP\r\n",strlen("AT+CWQAP\r\n"),1000);*/
//	HAL_UART_Transmit(&huart1,(uint8_t *)"AT+RST\r\n",strlen("AT+RST\r\n"),100);
//	HAL_Delay(5000);
char wifi_data[45] = { 0 };
sprintf(wifi_data, "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, Password); /*HAL_UART_Transmit(&huart1,(uint8_t *)tx_buffer,sprintf(tx_buffer,"AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,Password),1000);*/
err += (uint8_t) send_cmd_receive_expected_resp(wifi_data, 1500, "OK", 600);
if (err == 3)
	return true;
else
	return false;
}
static bool learn_local_esp8266_ip(char *ip_addr) {
while (!(send_cmd_receive_total_response("AT+CIFSR\r\n", 1500)))
	;
	if (sscanf(received_mqttdata.received_uart_data, "+CIFSR:STAIP,\"%[^\"]",
			ip_addr) == 1) {
		memset(received_mqttdata.received_uart_data, '\0',
				sizeof(received_mqttdata.received_uart_data));
		received_mqttdata.current_data_idx = 0;
		received_mqttdata.is_data_first_part_recived_completely = false;
		return true;

	}
	return false;

}


void connect_broker(char *ip, char *port, char *protocoltype, char *client_id,
	uint16_t keepalive, uint8_t flag) {
uint8_t err=0;
char esp8266_ip[16]={0};
err=send_cmd_receive_expected_resp("AT+CIPCLOSE\r\n", 1200, "OK", 800);
HAL_Delay(100);
err=send_cmd_receive_expected_resp("AT+CIPMUX=0\r\n", 1500, "OK",600);
HAL_Delay(100);
err=learn_local_esp8266_ip(esp8266_ip);
HAL_Delay(100);
		sprintf(tx_buffer, "AT+CIPSTART=\"TCP\",\"%s\",%s\r\n", ip, port);
		send_cmd_receive_expected_resp(tx_buffer, 1500, "OK", 600);

HAL_Delay(2000);
if(received_mqttdata.tcp_conn_handled){
ProtocolNameLength = strlen(protocoltype);
ClientIDLength = strlen(client_id);
uint8_t remainlength;
remainlength = 2 + ProtocolNameLength + 6 + ClientIDLength;
uint16_t total_length = sprintf(tx_buffer, "%c%c%c%c%s%c%c%c%c%c%c%s",
		(char) connect, (char) remainlength, (char) (ProtocolNameLength << 8),
		(char) ProtocolNameLength, protocoltype, (char) level, (char) flag,
		(char) (keepalive << 8), (char) keepalive, (char) (ClientIDLength << 8),
		(char) ClientIDLength, client_id);
		sprintf(tx_buffer, "AT+CIPSEND=%d\r\n", total_length);
	if(send_esp8266(tx_buffer)){
		sprintf(tx_buffer, "%c%c%c%c%s%c%c%c%c%c%c%s", (char) connect,
				(char) remainlength, (char) (ProtocolNameLength << 8),
				(char) ProtocolNameLength, protocoltype, (char) level,
				(char) flag, (char) (keepalive << 8), (char) keepalive,
				(char) (ClientIDLength << 8), (char) ClientIDLength, client_id);
		send_esp8266(tx_buffer);
HAL_Delay(100);
	}
	}
}
/*static void mqtthandler(const char * topic,const char * payload){
 if((!strcmp(topic,"IOT"))&&(!strcmp(payload,"ON"))){
 mqtt_publish("ledinfo", "led1acildi");
 HAL_UART_Transmit(&huart2, (uint8_t)*transmit_infodata, sprintf(transmit_infodata,"led1acildi\r\n") ,10);



 }
 }*/
static mqtt_event_e wifi_loop() {
char *payload = mqttbuffer.mqtt_payload;
char *topic = mqttbuffer.mqtt_topic;
memset(topic, '\0', MQTT_TOPIC_SIZE);
memset(payload, '\0', MQTT_PAYLOAD_SIZE);
uint8_t mqttflag = 0;
int remain_length = 0;
int topic_length = 0;
int payload_length = 0;
HAL_UART_AbortReceive_IT(&huart1);
for (int i = 0; i < sizeof(rx_buffer); i++) {
	if (rx_buffer[i] == 0x30) {
		remain_length = rx_buffer[i + 1];
		topic_length = rx_buffer[i + 2] + rx_buffer[i + 3];
		payload_length = remain_length - (topic_length + 2);
		for (int k = 0; k < topic_length; k++) {
			topic[k] = rx_buffer[i + 4 + k];
		}
		//topic=mqttbuffer.mqtt_topic;
		for (int j = 0; j < payload_length; j++) {
			payload[j] = rx_buffer[i + 4 + topic_length + j];
		}
		//payload=mqttbuffer.mqtt_payload;
		if (mqttbuffer.is_new == 0)
			mqttbuffer.is_new = 1;
		mqttflag = 1;
		break;

	}
}
memset(rx_buffer, '\0', sizeof(rx_buffer));
memset(topic, '\0', strlen(topic));
memset(payload, '\0', strlen(payload));
memset(received_mqttdata.received_uart_data, '\0',
		sizeof(received_mqttdata.received_uart_data));
HAL_UART_Receive_IT(&huart1, (uint8_t*) rx_buffer, 100);
if (mqttflag == 1)
	return MQTT_NEW_MESSAGE;
else {
	return MQTT_NO_MESSAGE;
}

}
void mqtt_loop() {
if (wifi_loop() == MQTT_NEW_MESSAGE) {
	if (mqttbuffer.is_new == 1) {
		receivedfirstcallbck(mqttbuffer.mqtt_topic, mqttbuffer.mqtt_payload);
		mqttbuffer.is_new = 0;
	}
}
}

void mqtt_publish(const char *topic, const char *message) {

uint16_t topiclength = strlen(topic);
uint8_t remainlength = 2 + topiclength + strlen(message);
int length = sprintf(tx_buffer, "%c%c%c%c%s%s", (char) publishCon,
		(char) remainlength, (char) (topiclength << 8), (char) topiclength,
		topic, message);
HAL_UART_Transmit(&huart1, (uint8_t*) tx_buffer,
		sprintf(tx_buffer, "AT+CIPSEND=%d\r\n", length), 100);
HAL_Delay(100);
HAL_UART_Transmit(&huart1, (uint8_t*) tx_buffer,
		sprintf(tx_buffer, "%c%c%c%c%s%s", (char) publishCon,
				(char) remainlength, (char) (topiclength << 8),
				(char) topiclength, topic, message), 5000);
HAL_Delay(100);

}
void mqtt_subscribe(const char *topic, uint8_t Qos) {
uint16_t TopicLength = strlen(topic);
uint8_t RemainLength = 2 + 2 + TopicLength + 1; // packetIDlength(2) + topiclengthdata(2)+topiclength+Qos
uint16_t length = sprintf(tx_buffer, "%c%c%c%c%c%c%s%c", (char) subscribeCon,
		(char) RemainLength, (char) (packetID << 8), (char) packetID,
		(char) (TopicLength << 8), (char) TopicLength, topic, (char) Qos);
HAL_UART_Transmit(&huart1, (uint8_t*) tx_buffer,
		sprintf(tx_buffer, "AT+CIPSEND=%d\r\n", length), 1000);
HAL_Delay(100);
HAL_UART_Transmit(&huart1, (uint8_t*) tx_buffer,
		sprintf(tx_buffer, "%c%c%c%c%c%c%s%c", (char) subscribeCon,
				(char) RemainLength, (char) (packetID << 8), (char) packetID,
				(char) (TopicLength << 8), (char) TopicLength, topic,
				(char) Qos), 5000);
}
void setmqttcallback(mymqttcallback mqttcallback) {
receivedfirstcallbck = mqttcallback;
}

static uint8_t receive_response(char *resp, uint8_t num_of_bytes,
	uint32_t timeout_ms) {
uint32_t start = HAL_GetTick();
uint8_t resp_buf;
while (num_of_bytes) {
	resp_buf = '\0';
	if (HAL_UART_Receive(&huart1, &resp_buf, 1, 250) == HAL_OK) {
		if (resp_buf != '\0') {
			send_esp8266("paket byte alindi\r\n");

			*resp++ = resp_buf;
			num_of_bytes--;

		}
	}

	else if (HAL_GetTick() - start > timeout_ms) {
		return 0;
		//break;
	} else {
		send_esp8266("paket byte alinamadi\r\n");
	}

}
return 1;
}

mqtt_connection_e mqtt_is_connected() {
uint8_t length_data = sprintf(tx_buffer, "%c%c", (char) pingreqpacket[0],
		(char) pingreqpacket[1]);
char resp[2] = { };
flush_response();
HAL_UART_Transmit(&huart1, (uint8_t*) tx_buffer,
		sprintf(tx_buffer, "AT+CIPSEND=%d\r\n", length_data), 1000);
HAL_Delay(100);
flush_response();
HAL_UART_Transmit(&huart1, (uint8_t*) tx_buffer,
		sprintf(tx_buffer, "%c%c", (char) pingreqpacket[0],
				(char) pingreqpacket[1]), 5000);
HAL_Delay(100);

if (receive_response(resp, length_data, 10000)) {
	if (strstr(resp, pingresppacket)) {
		send_esp8266("PAKETE CEVAP ALİNDİ\r\n");
		return MQTT_CONNECTED;
	}
} else {
	send_esp8266("pakete cevap alinamamistir\r\n");
	return MQTT_IS_NOT_CONNECTED;

}

}


