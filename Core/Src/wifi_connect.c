/*
 * wifi_connect.c
 *
 *  Created on: Jun 7, 2022
 *      Author: ibrhm
 */#include "wifi_connect.h"
 char pingreqpacket[2]={0xC0,0X00};
 char pingresppacket[2]={0xD0,0X00};
extern char tx_buffer[150];
extern char rx_buffer[150];
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
mqtt_buffer_t mqttbuffer;
extern char Uart_data[150];
extern char return_data[125];
extern uint8_t rx_flag;
extern uint8_t rx_count;
void(*mymqttcallback)(const char * topic,const char * payload);
void (*subscribecallback)(void);
uint16_t ProtocolNameLength;
uint16_t ClientIDLength;
uint16_t packetID=0x01;
uint8_t level=0x04;
uint8_t connect = 0x10;
uint8_t publishCon = 0x30;
uint8_t subscribeCon = 0x82;
char tx_infodata[150];
void mqtthandler(const char * topic,const char * payload){
	if((!strcmp(topic,"SEMİH54"))&&(!strcmp(payload,"ON"))){
		mqtt_publish("ledinfoiot1", "SEMledacildi");

		HAL_UART_Transmit(&huart2, (uint8_t)*tx_infodata, sprintf(tx_infodata,"led1acildi\r\n") ,100);
	}
	if((!strcmp(topic,"IOT"))&&(!strcmp(payload,"ON"))){
			HAL_UART_Transmit(&huart2, (uint8_t)*tx_infodata, sprintf(tx_infodata,"led1acildi\r\n") ,100);

			mqtt_publish("ledinfo", "led1acildi");


		}

}
 void subscribe_handler(){
	mqtt_subscribe("SEMİH54", 0x00);
	HAL_Delay(2000);
	mqtt_subscribe("IOT", 0x00);
	HAL_Delay(2000);
	mqtt_subscribe("iot2", 0x00);
	HAL_Delay(2000);


}
void read_message(){
	int remain_length=0,message_length=0,topic_length=0;
		char message[100];
		char topic[150];
		HAL_UART_AbortReceive_IT(&huart1);
		for(int i=0;i<sizeof(rx_buffer);i++)
		{
			 if(rx_buffer[i] == 0x30)
			 {
					remain_length = rx_buffer[i+1];
					topic_length  = rx_buffer[i+2]+rx_buffer[i+3];
					message_length = remain_length -(topic_length + 2);
					for(int k=0;k<topic_length;k++){
									topic[k]=rx_buffer[i+4+k];
								}
					for(int j=0;j<message_length;j++)
					{
						message[j] = rx_buffer[i+4+topic_length+j];
					}
					mymqttcallback(topic,message);
					break;
			 }
		}
		memset(rx_buffer,0,sizeof(rx_buffer)); 											// clear buffer
		memset(message,0,sizeof(message));
		memset(topic,0,sizeof(topic));
		HAL_UART_Receive_IT(&huart1,(uint8_t *)rx_buffer,100);
	}



void Wifi_connect(char *SSID ,char *Password)
{
	HAL_UART_Transmit(&huart1,(uint8_t *)"AT+CWMODE=1\r\n",strlen("AT+CWMODE=1\r\n"),1000);
	HAL_Delay(1000);
	HAL_UART_Transmit(&huart1,(uint8_t *)"AT+CWQAP\r\n",strlen("AT+CWQAP\r\n"),1000);
	HAL_Delay(1000);
//	HAL_UART_Transmit(&huart1,(uint8_t *)"AT+RST\r\n",strlen("AT+RST\r\n"),100);
//	HAL_Delay(5000);
	HAL_UART_Transmit(&huart1,(uint8_t *)tx_buffer,sprintf(tx_buffer,"AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,Password),1000);

}
void connect_broker(char *ip,char *port,char *protocoltype,char * client_id,uint16_t keepalive,uint8_t flag){
	HAL_UART_Transmit(&huart1,(uint8_t *)"AT+CIPCLOSE\r\n", strlen("AT+CIPCLOSE\r\n"), 1000);
	HAL_Delay(100);
	HAL_UART_Transmit(&huart1, (uint8_t *)"AT+CIPMUX=0\r\n", strlen("AT+CIPMUX=0\r\n"), 1000);
	HAL_Delay(100);
	HAL_UART_Transmit(&huart1, (uint8_t *)"AT+CIFSR\r\n", strlen("AT+CIFSR\r\n"), 1000);
	HAL_Delay(100);
	HAL_UART_Transmit(&huart1, (uint8_t *)tx_buffer, sprintf(tx_buffer,"AT+CIPSTART=\"TCP\",\"%s\",%s\r\n",ip,port),5000);

	HAL_Delay(2000);
	ProtocolNameLength=strlen(protocoltype);
	ClientIDLength=strlen(client_id);
	uint8_t remainlength;
	remainlength=2+ProtocolNameLength+6+ClientIDLength;
	uint16_t total_length=sprintf(tx_buffer,"%c%c%c%c%s%c%c%c%c%c%c%s",(char)connect,(char)remainlength,(char)(ProtocolNameLength<<8),(char)ProtocolNameLength,protocoltype,(char)level,(char)flag,(char)(keepalive<<8),(char)keepalive,(char)(ClientIDLength<<8),(char)ClientIDLength,client_id);
	HAL_UART_Transmit(&huart1, (uint8_t*)tx_buffer, sprintf(tx_buffer,"AT+CIPSEND=%d\r\n",total_length), 1000);
	HAL_Delay(100);
	HAL_UART_Transmit(&huart1,(uint8_t *)tx_buffer,sprintf(tx_buffer,"%c%c%c%c%s%c%c%c%c%c%c%s",(char)connect,(char)remainlength,(char)(ProtocolNameLength << 8),(char)ProtocolNameLength,protocoltype,(char)level,(char)flag,(char)(keepalive << 8),(char)keepalive,(char)(ClientIDLength << 8),(char)ClientIDLength,client_id),5000);
	HAL_Delay(100);
}
/*static void mqtthandler(const char * topic,const char * payload){
	if((!strcmp(topic,"IOT"))&&(!strcmp(payload,"ON"))){
		mqtt_publish("ledinfo", "led1acildi");
		HAL_UART_Transmit(&huart2, (uint8_t)*transmit_infodata, sprintf(transmit_infodata,"led1acildi\r\n") ,10);



	}
}*/
static mqtt_event_e wifi_loop(){
	char* payload;
	char *topic;
	memset(topic,'\0',MQTT_TOPIC_SIZE);
	memset(payload,'\0',MQTT_PAYLOAD_SIZE);
	uint8_t mqttflag=0;
	int remain_length=0;
	int topic_length=0;
	int payload_length=0;
	HAL_UART_AbortReceive_IT(&huart1);
	for(int i=0;i<sizeof(rx_buffer);i++){
		if(rx_buffer[i]==0x30){
			remain_length=rx_buffer[i+1];
			topic_length=rx_buffer[i+2]+rx_buffer[i+3];
			payload_length=remain_length-(topic_length+2);
			for(int k=0;k<topic_length;k++){
				topic[k]=rx_buffer[i+4+k];
			}
			topic=mqttbuffer.mqtt_topic;
			for(int j=0;j<payload_length;j++){
				payload[j]=rx_buffer[i+4+topic_length+j];
			}
			payload=mqttbuffer.mqtt_payload;
			mqttbuffer.is_new=1;
			mqttflag=1;
			break;

		}
	}
	memset(rx_buffer,'\0',sizeof(rx_buffer));
	memset(topic,'\0',strlen(topic));
	memset(payload,'\0',strlen(payload));
	if(mqttflag==1)
		return MQTT_NEW_MESSAGE;
	else{
		return MQTT_NO_MESSAGE;
	}
	HAL_UART_Receive_IT(&huart1, (uint8_t*)rx_buffer, 100);
}
void mqtt_loop(){
	if(wifi_loop()==MQTT_NEW_MESSAGE){
		if(mqttbuffer.is_new==1){
			(*mymqttcallback)(mqttbuffer.mqtt_topic,mqttbuffer.mqtt_payload);
			mqttbuffer.is_new=0;
		}
	}
}

void mqtt_publish(const char *topic, const char *message)
{

	uint16_t topiclength = strlen(topic);
	uint8_t remainlength = 2+topiclength+strlen(message);
	int length = sprintf(tx_buffer,"%c%c%c%c%s%s",(char)publishCon,(char)remainlength,(char)(topiclength << 8),(char)topiclength,topic,message);
	HAL_UART_Transmit(&huart1,(uint8_t *)tx_buffer,sprintf(tx_buffer,"AT+CIPSEND=%d\r\n",length),100);
	HAL_Delay(100);
	HAL_UART_Transmit(&huart1,(uint8_t *)tx_buffer,sprintf(tx_buffer,"%c%c%c%c%s%s",(char)publishCon,(char)remainlength,(char)(topiclength << 8),(char)topiclength,topic,message),5000);
	HAL_Delay(100);

}
void mqtt_subscribe(const char *topic,uint8_t Qos){
	uint16_t TopicLength = strlen(topic);
		uint8_t RemainLength = 2+2+TopicLength+1; // packetIDlength(2) + topiclengthdata(2)+topiclength+Qos
		uint16_t length = sprintf(tx_buffer,"%c%c%c%c%c%c%s%c",(char)subscribeCon,(char)RemainLength,(char)(packetID << 8),(char)packetID,(char)(TopicLength << 8),(char)TopicLength,topic,(char)Qos);
		HAL_UART_Transmit(&huart1,(uint8_t *)tx_buffer,sprintf(tx_buffer,"AT+CIPSEND=%d\r\n",length),1000);
		HAL_Delay(100);
		HAL_UART_Transmit(&huart1,(uint8_t *)tx_buffer,sprintf(tx_buffer,"%c%c%c%c%c%c%s%c",(char)subscribeCon,(char)RemainLength,(char)(packetID << 8),(char)packetID,(char)(TopicLength << 8),(char)TopicLength,topic,(char)Qos),5000);
}
void setmqttcallback(void (*mqttcallback)(const char *topic,const char* payload)){
	mymqttcallback=mqttcallback;
}
static void send_command(const char *cmd) {
	HAL_UART_Transmit(&huart1, (uint8_t*) cmd, strlen(cmd), 5000);
}
 void send_command_info(const char *cmd) {
	HAL_UART_Transmit(&huart2, (uint8_t*) cmd, strlen(cmd), 100);
}

static void flush_response() {
	uint8_t garbage[1];
	while (HAL_UART_Receive(&huart1, garbage, 1, 10) == HAL_OK);
}
static uint8_t receive_response(char *resp, uint8_t num_of_bytes, uint32_t timeout_ms) {
	uint32_t start = HAL_GetTick();
	uint8_t resp_buf;
	while (num_of_bytes) {
		resp_buf = '\0';
		if(HAL_UART_Receive(&huart1, &resp_buf, 1, 250)==HAL_OK){
			if (resp_buf != '\0') {
					send_command_info("paket byte alindi\r\n");

					*resp++ = resp_buf;
					num_of_bytes--;

					}
		}

		else if (HAL_GetTick() - start > timeout_ms) {
						return 0;
						break;
					}
		else{
			send_command_info("paket byte alinamadi\r\n");
		}


	}
	return 1;
}

 mqtt_connection_e mqtt_is_connected() {
	 uint8_t length_data=sprintf(tx_buffer,"%c%c",(char)pingreqpacket[0],(char)pingreqpacket[1]);
	char resp[2] ={};
	flush_response();
	HAL_UART_Transmit(&huart1,(uint8_t *)tx_buffer, sprintf(tx_buffer,"AT+CIPSEND=%d\r\n",length_data), 1000);
	HAL_Delay(100);
	HAL_UART_Transmit(&huart1, (uint8_t *)tx_buffer, sprintf(tx_buffer,"%c%c",(char)pingreqpacket[0],(char)pingreqpacket[1]), 5000);
	HAL_Delay(100);

	if(receive_response(resp, length_data, 10000)){
		if (strstr(resp, pingresppacket)) {
				send_command_info("PAKETE CEVAP ALİNDİ\r\n");
				return MQTT_CONNECTED;
			}
	}
	else{
		send_command_info("pakete cevap alinamamistir\r\n");
		return MQTT_IS_NOT_CONNECTED;

	}

}
 void send_string(){
	 send_command_info("asdasd\r\n");

 }




