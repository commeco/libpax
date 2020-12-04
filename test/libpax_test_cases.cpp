#include <limits.h>
#include <string.h>
#include "unity.h"
#include <libpax.h>
#include <libpax_api.h>

/* test the function  libpax_counter_add_mac
1. add 100 diffrent mac addresses, the count should increase
2. add 100 same mac addresses, the count should not increase
 */

void test_collision_add() {
  libpax_counter_reset();
  uint8_t test_mac_addr[6];

  uint16_t *test_mac_addr_p = (uint16_t *)(test_mac_addr + 4);
  *test_mac_addr_p = 1;
  for (int i = 0; i < 100; i++) {
    int count_start = libpax_wifi_counter_count();
    mac_add(test_mac_addr, MAC_SNIFF_WIFI);
    TEST_ASSERT_EQUAL(1, libpax_wifi_counter_count() - count_start);
    *test_mac_addr_p += 1;
  }

  ESP_LOGI("testing", "Collision tests starts ###");
  *test_mac_addr_p = 1;
  for (int i = 0; i < 100; i++) {
    int count_start = libpax_wifi_counter_count();
    mac_add(test_mac_addr, MAC_SNIFF_WIFI);
    TEST_ASSERT_EQUAL(libpax_wifi_counter_count(), count_start);
    *test_mac_addr_p += 1;
  }
}

/* test the function  libpax_counter_reset()
1. when count >= 0, reset should worked
2. when count >0 (add 100 diffrent mac addresses),reset should worked
 */
void test_counter_reset() {
  libpax_counter_reset();
  TEST_ASSERT_EQUAL(0, libpax_wifi_counter_count());

  uint8_t test_mac_addr[6] = {1, 1, 1, 1, 1, 1};
  mac_add(test_mac_addr, MAC_SNIFF_WIFI);
  TEST_ASSERT_EQUAL(1, libpax_wifi_counter_count());

  libpax_counter_reset();
  TEST_ASSERT_EQUAL(0, libpax_wifi_counter_count());
}



/*
configuration test
*/
void test_config_store() {
  struct libpax_config_t configuration;
  struct libpax_config_t current_config;  
  libpax_default_config(&configuration);
  libpax_update_config(&configuration);
  libpax_get_current_config(&current_config);
  TEST_ASSERT_EQUAL(0, memcmp(&configuration, &current_config, sizeof(struct libpax_config_t)));
  current_config.wifi_channel_map = 0b101;
  TEST_ASSERT_NOT_EQUAL(0, memcmp(&configuration, &current_config, sizeof(struct libpax_config_t)));
  struct libpax_config_t read_configuration;
  char* configuration_memory = (char*)malloc(sizeof(struct libpax_config_storage_t));
  libpax_store_config(configuration_memory, &current_config);
  TEST_ASSERT_EQUAL(0, libpax_load_config(configuration_memory, &read_configuration));
  TEST_ASSERT_EQUAL(0, memcmp(&current_config, &read_configuration, sizeof(struct libpax_config_t)));
}

struct count_payload_t count_from_libpax;
int time_called_back = 0;

void process_count(void) {
  time_called_back++;
  printf("pax: %d; %d; %d;\n", count_from_libpax.pax, count_from_libpax.wifi_count, count_from_libpax.ble_count);
}


/*
integration test
*/
void test_integration() {
  time_called_back = 0;
  struct libpax_config_t configuration; 
  libpax_default_config(&configuration);
  
  // only wificounter is active
  configuration.blecounter = 1;
  configuration.blescantime = 0; //infinit
  configuration.wificounter = 1; 
  configuration.wifi_channel_map = WIFI_CHANNEL_ALL;
  configuration.wifi_channel_switch_interval = 50;
  configuration.wifi_rssi_threshold = -80;
  libpax_update_config(&configuration);

  // internal processing initialization
  int err_code = libpax_counter_init(process_count, &count_from_libpax, 10*1000, 1); 
  TEST_ASSERT_EQUAL(0, err_code);

  err_code = libpax_counter_start();
  TEST_ASSERT_EQUAL(0, err_code);
  
  printf("libpax should be running\n");
  vTaskDelay(pdMS_TO_TICKS(1000*60 + 100));

  TEST_ASSERT_EQUAL(6, time_called_back);
  err_code = libpax_counter_stop();
  TEST_ASSERT_EQUAL(0, err_code);
  
  time_called_back = 0;
  vTaskDelay(pdMS_TO_TICKS(1000*30)); 
  printf("libpax should be stopped\n");

  TEST_ASSERT_EQUAL(0, time_called_back);
}

void run_tests() {
   UNITY_BEGIN();

    RUN_TEST(test_collision_add);
    RUN_TEST(test_counter_reset);
    RUN_TEST(test_config_store);
    RUN_TEST(test_integration);

    UNITY_END();
}

#ifdef LIBPAX_ARDUINO
void setup() {
   run_tests();
}
void loop() {}
#endif
#ifdef LIBPAX_ESPIDF
extern "C" void app_main();
void app_main()
{
   run_tests();
}
#endif