/* 
This code will work on taking commands from user and then having the ESP32 process the commands over WiFi.

Has two subscribers that subscriber to one node. One subscriber takes in the Crawler steps while the other 
takes in integers from 1-4 to inflate/deflate a specfic Kresling.
*/

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
#define ROS_PORT 8892          // 8892 port used for Crawler commands

//ROS variables
rcl_node_t ctrl_sys_node;
rclc_support_t support;
rcl_allocator_t allocator;
rclc_executor_t executor;

//declaring subscribers and messages
rcl_subscription_t subscriber_Crawler;
std_msgs__msg__Int32 recv_msg_Crawler;

rcl_publisher_t publisher_Crawler;
std_msgs__msg__Int32 pub_msg_Crawler;

rcl_subscription_t subscriber_Kresling;
std_msgs__msg__Int32 recv_msg_Kresling;

rcl_publisher_t publisher_Kresling;
std_msgs__msg__Int32 pub_msg_Kresling;

//pins for pump and valves
const int pumpPin1 = 26;
const int valvePin1 = 25;
const int pumpPin2 = 33;
const int valvePin2 = 32;

//control flags
volatile int current_command = -1;
volatile int current_steps = 0;
volatile bool new_command = false;

void crawl_sequence(int steps){
  Serial.print("Executing crawl for steps: ");
  Serial.println(steps);


  for (int i = 0; i<steps; i++){
    Serial.print("Step: ");
    Serial.println(i+1);

    //delay timings based on first iteration
    // int delay1 = max(1000, 3000 - i * 750);
    int delay1 = max(2250, 3000 - i * 750);
    int delay2 = max(2500, 4000 - i * 750);

    // int delay3 = max(1000, 400 + i * 200); //for inbetween putting air back in kresling1

    Serial.print("Timings Kresling 1/2: ");
    Serial.print(delay1);
    Serial.print(" ");
    Serial.println(delay2);

    //air leaves Kresling 1
    digitalWrite(valvePin1, LOW);
    digitalWrite(pumpPin1, HIGH);

    delay(delay1);

    //kresling 1 put air back in
    digitalWrite(valvePin1, LOW);
    digitalWrite(pumpPin1, LOW);

    // delay(delay3); //small pause (optional)

    //air leaves Kresling 2
    digitalWrite(valvePin2, LOW);
    digitalWrite(pumpPin2, HIGH);

    delay(delay2);

    //kresling 2 put air back in
    digitalWrite(valvePin2, LOW);
    digitalWrite(pumpPin2, LOW);

    delay(1100); //small pause (optional

  }
}

int last_command = -1;
bool crawl_requested = false;

void command_callback_crawler(const void * msgin){
  /* Function that is for Crawler callback, just takes in number of steps for Crawler. */
  // Serial.begin(115200);
  Serial.println("CRAWLER CALLBACK TRIGGERED");

  const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;

  int steps = msg->data;

  // //extracting upper 4 bits (command)
  // int command = (data >> 4) & 0x0F;

  // //extracting lower 4 bits (value)
  // int steps = data & 0x0F;

  Serial.println("=== NEW CRAWLER MESSAGE ===");
  Serial.print("Crawler Command Initiated...");
  Serial.print("Steps: ");
  Serial.println(steps);

  //storing command for loop()
  current_command = 5; //specfiying crawler command for Crawler subscriber
  current_steps = steps;
  new_command = true;

  pub_msg_Crawler.data = msg->data;
  rcl_publish(&publisher_Crawler, &pub_msg_Crawler, NULL);
}

void command_callback_kresling(const void * msgin){
  /* Function that is for individual Kresling callback, takes in numbers 1-4 
  for inflating/deflating a specifc Kresling. */

  // Serial.begin(115200);
  Serial.println("KRESLING CALLBACK TRIGGERED");

  const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;

  int command = msg->data;

  Serial.println("=== NEW KRESLING MESSAGE ===");
  Serial.print("Kresling Command Initiated...");
  Serial.print("Command: ");
  Serial.println(command);

  //storing command for loop()
  current_command = command;
  new_command = true;

  pub_msg_Kresling.data = msg->data;
  rcl_publish(&publisher_Kresling, &pub_msg_Kresling, NULL);
}

void setup() {
  Serial.begin(115200);

  pinMode(pumpPin1, OUTPUT);
  pinMode(pumpPin2, OUTPUT);
  pinMode(valvePin1, OUTPUT);
  pinMode(valvePin2, OUTPUT);

  digitalWrite(pumpPin1, LOW);
  digitalWrite(pumpPin2, LOW);
  digitalWrite(valvePin1, LOW);
  digitalWrite(valvePin2, LOW);

  //connecting microROS over WiFi
  set_microros_wifi_transports(WIFI_SSID, WIFI_PASS, ROS_IP, ROS_PORT);  
  delay(2000);

  allocator = rcl_get_default_allocator();

  //Init support
  rclc_support_init(&support, 0, NULL, &allocator);

  //creating node
  rclc_node_init_default(&ctrl_sys_node, "embedded_ctrl_system_node", "", &support);

  //subrsciber/publsiher for Crawler commands
  //create subscriber for crawler
  rclc_subscription_init_best_effort(
    &subscriber_Crawler,
    &ctrl_sys_node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "crawler_cmd"
  );

  //create publisher for crawler
  rclc_publisher_init_default(
    &publisher_Crawler,
    &ctrl_sys_node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "crawler_status"); 

    //subrsciber/publsiher for Kresling commands
    rclc_subscription_init_best_effort(
    &subscriber_Kresling,
    &ctrl_sys_node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "kresling_cmd"
  );

  //create publisher for kresling
  rclc_publisher_init_default(
    &publisher_Kresling,
    &ctrl_sys_node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "kresling_status"); 

  //executor
  rclc_executor_init(&executor, &support.context, 2, &allocator);

  rclc_executor_add_subscription(
    &executor,
    &subscriber_Crawler,
    &recv_msg_Crawler,
    &command_callback_crawler,
    ON_NEW_DATA
  );

  rclc_executor_add_subscription(
    &executor,
    &subscriber_Kresling,
    &recv_msg_Kresling,
    &command_callback_kresling,
    ON_NEW_DATA
  );

}

void loop() {
  // put your main code here, to run repeatedly:
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));

  if(new_command){
    new_command = false;

    switch (current_command){
      case 1: //inflate Kresling 1
        Serial.println("Inflating Kresling 1");
        digitalWrite(valvePin1, LOW);
        digitalWrite(pumpPin1, LOW);
        break;

      case 2: //inflating Kresling 2
        Serial.println("Inflating Kresling 2");
        digitalWrite(valvePin2, LOW);
        digitalWrite(pumpPin2, LOW);
        break;

      case 3: //deflating Kresling 1
        Serial.println("Deflating Kresling 1");
        digitalWrite(valvePin1, LOW);
        digitalWrite(pumpPin1, HIGH);
        break;

      case 4: //deflating Kresling 2
        Serial.println("Deflating Kresling 2");
        digitalWrite(valvePin2, LOW);
        digitalWrite(pumpPin2, HIGH);
        break;
      
      case 5: //crawling command
      if(current_steps <= 0) current_steps = 1;
      crawl_sequence(current_steps);
      break;

      default:
        Serial.println("Unknown Command");
        break;
    }
  }
}
