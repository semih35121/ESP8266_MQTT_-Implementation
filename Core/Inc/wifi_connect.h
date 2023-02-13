/*
 * wifi_connect.h
 *
 *  Created on: Jun 7, 2022
 *      Author: ibrhm
 */

#ifndef INC_WIFI_CONNECT_H_
#define INC_WIFI_CONNECT_H_
#include <stm32f4xx.h>
#include "stm32f4xx_hal_def.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#define MQTT_TOPIC_SIZE 100
#define MQTT_PAYLOAD_SIZE 100
char Uart_data[150];
char return_data[125];
uint8_t rx_flag;
//uint8_t rx_count=0;
typedef enum{
	MQTT_NEW_MESSAGE,
	MQTT_NO_MESSAGE
}mqtt_event_e;

typedef struct mqtt_buffer{
	char mqtt_topic[MQTT_TOPIC_SIZE];
	char mqtt_payload[MQTT_PAYLOAD_SIZE];
	uint8_t is_new;
}mqtt_buffer_t;

typedef enum mqtt_event{
	MQTT_CONNECTED,
	MQTT_IS_NOT_CONNECTED
}mqtt_connection_e;
typedef struct received_data_info{
	//bool is_data_sended;
	char received_uart_data[100];
	uint16_t current_data_idx;
	bool is_data_received_start;
	char received_uart_char_data;
	bool is_data_first_part_recived_completely;
	bool tcp_conn_handled;
}received_data_info_t;
void read_message(void);
void mqtthandler(const char * topic,const char * payload);
 mqtt_buffer_t mqttbuffer;
char tx_buffer[150];
char rx_buffer[150];
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart1;
void Wifi_connect(char *SSID ,char *Password);
void connect_broker(char *ip,char *port,char *protocoltype,char * client_id,uint16_t keepalive,uint8_t flag);
void setmqttcallback(void (*mqttcallback)(const char *topic,const char* payload));
void mqtt_loop();
void mqtt_publish(const char *topic, const char *message);
void mqtt_subscribe(const char *topic,uint8_t Qos);
mqtt_connection_e mqtt_is_connected();
void send_command_info(const char *cmd);
void send_string();
void subscribe_handler(void);
uint8_t send_cmd_receive_total_response(const char * send_atcmd,uint32_t timeout);
 void esp8266_rxcallback(void);
#endif /* INC_WIFI_CONNECT_H_ */
