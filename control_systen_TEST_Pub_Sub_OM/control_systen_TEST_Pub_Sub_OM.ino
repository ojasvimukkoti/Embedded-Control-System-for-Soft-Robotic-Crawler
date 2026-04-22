//Code to test/learn about how publishing./subscribing works in microROS
//Will allow user to press 1 and have a light turn on in EPS32

#include <WiFi.h>

#include <micro_ros_arduino.h>

#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <std_msgs/msg/int32.h>

//define WiFi information here 
#define WIFI_SSID "examp"
#define WIFI_PASS "examp_pw"
#define ROS_IP "ip_address"  // Change to match your ROS2 computer's IP
#define ROS_PORT 8891          // 8891 port used for testing this code

//LED PIN on ESP32
#define LED_PIN 13

//ROS variables
rcl_node_t node;
rclc_support_t support;
rcl_allocator_t allocator;
rclc_executor_t executor;

rcl_subscription_t subscriber;
std_msgs__msg__Int32 recv_msg;

rcl_publisher_t publisher;
std_msgs__msg__Int32 pub_msg;

//Callback function
void subscription_callback(const void * msgin){
  const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;

  if (msg-> data == 1){
    digitalWrite(LED_PIN, HIGH);
  }else{
    digitalWrite(LED_PIN, LOW);
  }

  //publish status back
  pub_msg.data = msg->data;
  rcl_publish(&publisher, &pub_msg, NULL);

}

void setup() {
  // //debug code to test WiFi connection
  // Serial.begin(115200);
  // delay(2000);

  // Serial.println("Connecting to WiFi....");
  // Serial.println(WIFI_SSID);
  // Serial.println(WIFI_PASS);

  // WiFi.begin(WIFI_SSID, WIFI_PASS);

  // while(WiFi.status() != WL_CONNECTED){
  //   delay(500);
  //   Serial.print(".");
  //   Serial.print(WiFi.status());
  // }

  // Serial.println("\nWiFi Connected! (yuh)");
  // Serial.print('ESP32: ');
  // Serial.println(WiFi.localIP());


  pinMode(LED_PIN, OUTPUT);

  //connecting microROS over WiFi
  set_microros_wifi_transports(WIFI_SSID, WIFI_PASS, ROS_IP, ROS_PORT);  
  delay(2000);

  allocator = rcl_get_default_allocator();

  //Init support
  rclc_support_init(&support, 0, NULL, &allocator);

  //creating node
  rclc_node_init_default(&node, "esp32_led_node", "", &support);

  //create subscriber (/led_cmd)
  rclc_subscription_init_best_effort(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "led_cmd"
  );

  //create publisher (/led_status)
  rclc_publisher_init_best_effort(
    &publisher,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "led_status"); 

  //executor
  rclc_executor_init(&executor, &support.context, 1, &allocator);

  rclc_executor_add_subscription(
    &executor,
    &subscriber,
    &recv_msg,
    &subscription_callback,
    ON_NEW_DATA
  );

}

void loop() {
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
  // setup();

}
