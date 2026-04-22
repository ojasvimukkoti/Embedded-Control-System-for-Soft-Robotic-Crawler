/* 
This code will work on taking commands from user and then having the ESP32 process the commands over WiFi.

TAKES IN COMMANDS LIKE: 
    - Crawl 3 steps once: ros2 topic pub -1 /ctrl_sys_cmd std_msgs/msg/Int32 "{data: 83}"
        * 01010011 = 83 (in decimal)
    - Crawl 5 steps once: ros2 topic pub -1 /ctrl_sys_cmd std_msgs/msg/Int32 "{data: 85}"
    - Deflate Kresling 1 once: ros2 topic pub -1 /ctrl_sys_cmd std_msgs/msg/Int32 "{data: 85}"
        - (3 << 4 = 48) [3 comes from case 3 and shifts 3 in binary to the left by 4 bits]
    - Inflate Kresling 1: ros2 topic pub -1 /ctrl_sys_cmd std_msgs/msg/Int32 "{data: 16}"

  * Has an 8-bit protocol so users can send commands to test the deflation/inflation of individual Kreslings
    - [ command (upper 4 bits) ][ steps (lower 4 bits) ]
      - Ex.) 5 -> *crawl command* -> 0101
            steps -> 0000-1111 (0-15 steps)
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

rcl_subscription_t subscriber;
std_msgs__msg__Int32 recv_msg;

rcl_publisher_t publisher;
std_msgs__msg__Int32 pub_msg;

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
  // int steps = steps; // number of steps for crawler to moves
  // int stepTime_ms = 200 // step waiting time
  Serial.print("Executing crawl for steps: ");
  Serial.println(steps);


  for (int i = 0; i<steps; i++){
    Serial.print("Step: ");
    Serial.println(i+1);

    //air leaves Kresling 1
    digitalWrite(valvePin1, LOW);
    digitalWrite(pumpPin1, HIGH);

    delay(3200);

    //kresling 1 put air back in
    digitalWrite(valvePin1, LOW);
    digitalWrite(pumpPin1, LOW);

    delay(1000); //small pause (optional)

    //air leaves Kresling 2
    digitalWrite(valvePin2, LOW);
    digitalWrite(pumpPin2, HIGH);

    delay(5200);

    //kresling 2 put air back in
    digitalWrite(valvePin2, LOW);
    digitalWrite(pumpPin2, LOW);

    delay(1000); //small pause (optional

  }
}

int last_command = -1;
bool crawl_requested = false;

void command_callback(const void * msgin){
  Serial.begin(115200);
  Serial.println("CALLBACK TRIGGERED");

  const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msgin;

  int data = msg->data;

  //extracting upper 4 bits (command)
  int command = (data >> 4) & 0x0F;

  //extracting lower 4 bits (value)
  int steps = data & 0x0F;

  Serial.println("=== NEW MESSAGE ===");
  Serial.print("Raw: ");
  Serial.println(data);
  Serial.print("Binary: ");
  Serial.println(data, BIN);
  Serial.print("Command: ");
  Serial.println(command);
  Serial.print("Steps: ");
  Serial.println(steps);

  //storing command for loop()
  current_command = command;
  current_steps = steps;
  new_command = true;

  pub_msg.data = msg->data;
  rcl_publish(&publisher, &pub_msg, NULL);
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

  //create subscriber (/ctrl_sys_cmd)
  rclc_subscription_init_best_effort(
    &subscriber,
    &ctrl_sys_node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "ctrl_sys_cmd"
  );

  //create publisher (/ctrl_sys_status)
  rclc_publisher_init_default(
    &publisher,
    &ctrl_sys_node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "ctrl_sys_status"); 

  //executor
  rclc_executor_init(&executor, &support.context, 1, &allocator);

  rclc_executor_add_subscription(
    &executor,
    &subscriber,
    &recv_msg,
    &command_callback,
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
