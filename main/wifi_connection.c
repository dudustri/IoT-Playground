#include "wifi_connection.h"

//connection retry state
int retry_conn_num = 0;

//handler for the wifi events loop
static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id,void *event_data){
    switch (event_id){
        case WIFI_EVENT_STA_START:
            ESP_LOGI("WIFI EVENT", "Starting the WiFi connection, wait...\n");
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI("WIFI EVENT", "Connected!\n");
            retry_conn_num = 0;
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI("WIFI EVENT", "WiFi connection lost\n");
            if(retry_conn_num<5){
                esp_wifi_connect();
                retry_conn_num++;
                ESP_LOGI("WIFI EVENT", "Retrying to connect... (%d)\n", retry_conn_num); 
                vTaskDelay(1000 *retry_conn_num / portTICK_PERIOD_MS);
            }
            break;
        case IP_EVENT_STA_GOT_IP:
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            uint32_t ip_addr = ip4_addr_get_u32(&event->ip_info.ip);
            char ip_str[16]; // Buffer to store IP address string (xxx.xxx.xxx.xxx\0)
            ip4addr_ntoa_r((const ip4_addr_t *)&ip_addr, ip_str, sizeof(ip_str));
            ESP_LOGI("WIFI EVENT", "IP address of the connected device: %s\n", ip_str);
            break;
    }
}

void wifi_connection_init(){
    esp_netif_init(); // initiates network interface
    esp_event_loop_create_default(); // dispatch events loop callback
    esp_netif_create_default_wifi_sta(); // create default wifi station
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_init_config); // initialize wifi
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL); // register event handler for events
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL); // register event handler for network
}

void wifi_connection_start(const char *ssid, const char *pass){
    wifi_config_t wifi_configuration = {
        .sta= {
            .ssid = "",
            .password= "", 
        },
    };
    strcpy((char*)wifi_configuration.sta.ssid, ssid);
    strcpy((char*)wifi_configuration.sta.password, pass);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_start(); // start wifi
    esp_wifi_set_mode(WIFI_MODE_STA); // set wifi mode to connect to an existing Wi-Fi network as a client (station)
    esp_wifi_connect();
    ESP_LOGI("WIFI MAIN CONNECTION", "wifi_init_softap finished. SSID:%s  password:%s \n", ssid, pass);
}

int wifi_connection_get_status() {
    wifi_ap_record_t ap_info;
    int wifi_status = esp_wifi_sta_get_ap_info(&ap_info);

    if (wifi_status == ESP_OK) {
        if (ap_info.rssi >= -80) {
            return ESP_OK;  // Connected and good signal strength -> bigger than -80 dBm
        } else {
            return ESP_ERR_WIFI_NOT_CONNECT;  // Signal strength below threshold
        }
    } else {
        return ESP_ERR_WIFI_NOT_CONNECT;  // Not connected to WiFi
    }
}