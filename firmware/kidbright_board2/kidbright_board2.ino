/*
 * KidBright 32 - micro-ROS Board 2 (Node: kidbright_board2)
 * 
 * Hardware Mapping (KidBright 32 / ESP32-WROOM):
 * - Switch 1 (S1): GPIO 16 (Active LOW with internal pull-up)
 * - Switch 2 (S2): GPIO 14 (Active LOW with internal pull-up)
 * - Buzzer:        GPIO 13 (Tone / PWM)
 * - WIFI LED:      GPIO 2  (Connection status indicator)
 * - IOT LED:       GPIO 12 (Activity indicator)
 * - LED Matrix:    HT16K33 16x8 (I2C: SDA=21, SCL=22, Addr=0x70)
 * 
 * ROS 2 Communication:
 * - Subscriber:  /kidbright/board1/button (std_msgs/msg/Int32)
 *                -> Reacts to button presses from Board 1!
 *                -> data=1: Plays cheerful melody, shows Heart on Matrix, blinks LED
 *                -> data=2: Plays alert beep, shows Smile on Matrix, blinks LED
 * - Publisher:   /kidbright/board2/button (std_msgs/msg/Int32)
 *                -> Sends button events back to Board 1 or Host!
 */

#include <micro_ros_arduino.h>
#include <stdio.h>
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include "ht16k33_matrix.h"

// ============================================================================
// Transport Configuration (Wi-Fi or Serial)
// ============================================================================
#define USE_WIFI 1   // Set to 1 for Wi-Fi UDP, 0 for USB Serial

#if USE_WIFI
  #include <WiFi.h>
  char ssid[] = "YOUR_WIFI_SSID";             // <--- ใส่ชื่อ Wi-Fi ของท่าน
  char password[] = "YOUR_WIFI_PASSWORD";     // <--- ใส่รหัสผ่าน Wi-Fi
  char agent_ip[] = "192.168.1.100";          // <--- ใส่ IP ของ Host (WSL/Windows) เช่น "192.168.1.100"
  const size_t agent_port = 8888;
#endif

// ============================================================================
// Pin Definitions for KidBright 32
// ============================================================================
#define PIN_SW1       16  // Switch 1 (ปุ่ม S1)
#define PIN_SW2       14  // Switch 2 (ปุ่ม S2)
#define PIN_BUZZER    13  // ลำโพง Buzzer
#define PIN_LED_WIFI   2  // LED WIFI
#define PIN_LED_IOT   12  // LED IOT

// ============================================================================
// Global Objects
// ============================================================================
KidbrightMatrix matrix;

rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rclc_executor_t executor;
rcl_publisher_t publisher;
rcl_subscription_t subscriber;

std_msgs__msg__Int32 msg_pub;
std_msgs__msg__Int32 msg_sub;

enum AgentStatus {
  WAITING_AGENT,
  AGENT_CONNECTED
} agent_state = WAITING_AGENT;

// Sound generator helper for Buzzer
void buzzerBeep(int freq, int duration_ms) {
#if defined(ESP_ARDUINO_VERSION) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  tone(PIN_BUZZER, freq, duration_ms);
  delay(duration_ms);
#else
  ledcSetup(0, freq, 8);
  ledcAttachPin(PIN_BUZZER, 0);
  ledcWriteTone(0, freq);
  delay(duration_ms);
  ledcWrite(0, 0);
#endif
}

void soundHappy() {
  buzzerBeep(523, 70); // C5
  buzzerBeep(659, 70); // E5
  buzzerBeep(784, 70); // G5
  buzzerBeep(1047, 120); // C6
}

void soundAlert() {
  buzzerBeep(880, 80);
  delay(40);
  buzzerBeep(880, 80);
}

void soundChirp() {
  buzzerBeep(1200, 40);
}

// Callback when Board 1 presses a button
void subscription_callback(const void * msinf) {
  const std_msgs__msg__Int32 * msg = (const std_msgs__msg__Int32 *)msinf;
  digitalWrite(PIN_LED_IOT, HIGH);

  if (msg->data == 1) {
    // Board 1 pressed Switch 1: Heart ❤️ + Happy Sound + LED
    matrix.showIcon(ICON_HEART);
    soundHappy();
    delay(500);
    matrix.showIcon(ICON_NUM2); // กลับมาแสดงเลขบอร์ด 2
  } 
  else if (msg->data == 2) {
    // Board 1 pressed Switch 2: Smile 😊 + Alert Sound + LED
    matrix.showIcon(ICON_SMILE);
    soundAlert();
    delay(500);
    matrix.showIcon(ICON_NUM2);
  }

  digitalWrite(PIN_LED_IOT, LOW);
}

// ============================================================================
// micro-ROS Entity Management
// ============================================================================
bool create_entities() {
  allocator = rcl_get_default_allocator();

  rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
  if (rcl_init_options_init(&init_options, allocator) != RCL_RET_OK) return false;

  if (rclc_support_init_with_options(&support, 0, NULL, &init_options, &allocator) != RCL_RET_OK) {
    return false;
  }

  // สร้าง Node ชื่อ "kidbright_board2"
  if (rclc_node_init_default(&node, "kidbright_board2", "", &support) != RCL_RET_OK) {
    return false;
  }

  // สร้าง Subscriber รับค่าจาก Topic: /kidbright/board1/button
  if (rclc_subscription_init_default(
        &subscriber,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "/kidbright/board1/button") != RCL_RET_OK) {
    return false;
  }

  // สร้าง Publisher ส่งค่าไปยัง Topic: /kidbright/board2/button
  if (rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "/kidbright/board2/button") != RCL_RET_OK) {
    return false;
  }

  // สร้าง Executor สำหรับรับ Callback
  if (rclc_executor_init(&executor, &support.context, 1, &allocator) != RCL_RET_OK) {
    return false;
  }

  if (rclc_executor_add_subscription(
        &executor,
        &subscriber,
        &msg_sub,
        &subscription_callback,
        ON_NEW_DATA) != RCL_RET_OK) {
    return false;
  }

  return true;
}

void destroy_entities() {
  rcl_publisher_fini(&publisher, &node);
  rcl_subscription_fini(&subscriber, &node);
  rclc_executor_fini(&executor);
  rcl_node_fini(&node);
  rclc_support_fini(&support);
}

// ============================================================================
// Setup
// ============================================================================
void setup() {
  pinMode(PIN_SW1, INPUT_PULLUP);
  pinMode(PIN_SW2, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_WIFI, OUTPUT);
  pinMode(PIN_LED_IOT, OUTPUT);

  digitalWrite(PIN_LED_WIFI, LOW);
  digitalWrite(PIN_LED_IOT, LOW);

  // เริ่มต้นหน้าจอ Matrix 16x8
  matrix.begin();
  matrix.showIcon(ICON_NUM2); // แสดงเลข "2" ประจำบอร์ด
  soundChirp();

#if USE_WIFI
  set_microros_wifi_transports(ssid, password, agent_ip, agent_port);
#else
  Serial.begin(115200);
  set_microros_serial_transports(Serial);
#endif
}

// ============================================================================
// Main Loop
// ============================================================================
unsigned long last_ping_time = 0;
unsigned long last_sw1_time = 0;
unsigned long last_sw2_time = 0;
const unsigned long debounce_delay = 200;

void loop() {
  unsigned long now = millis();

  // State Machine สำหรับเชื่อมต่อ Agent
  switch (agent_state) {
    case WAITING_AGENT:
      // กระพริบไฟ LED_WIFI ขณะรอเชื่อมต่อ
      digitalWrite(PIN_LED_WIFI, (now / 250) % 2);

      if (now - last_ping_time > 1000) {
        last_ping_time = now;
        if (rmw_uros_ping_agent(100, 1) == RMW_RET_OK) {
          if (create_entities()) {
            agent_state = AGENT_CONNECTED;
            digitalWrite(PIN_LED_WIFI, HIGH); // ติดค้างเมื่อเชื่อมต่อสำเร็จ
            matrix.showIcon(ICON_CHECK);
            soundHappy();
            delay(500);
            matrix.showIcon(ICON_NUM2);
          } else {
            destroy_entities();
          }
        }
      }
      break;

    case AGENT_CONNECTED:
      // หมุน Executor รับข้อความจาก Board 1
      rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));

      // ตรวจสอบสถานะการเชื่อมต่อ Agent ทุก 3 วินาที
      if (now - last_ping_time > 3000) {
        last_ping_time = now;
        if (rmw_uros_ping_agent(100, 1) != RMW_RET_OK) {
          agent_state = WAITING_AGENT;
          digitalWrite(PIN_LED_WIFI, LOW);
          destroy_entities();
          break;
        }
      }

      // ตรวจสอบการกดปุ่ม Switch 1 บน Board 2 (ส่งกลับไปหา Board 1 หรือ ROS Host)
      if (digitalRead(PIN_SW1) == LOW) {
        if (now - last_sw1_time > debounce_delay) {
          last_sw1_time = now;
          msg_pub.data = 1;
          digitalWrite(PIN_LED_IOT, HIGH);
          rcl_publish(&publisher, &msg_pub, NULL);
          soundChirp();
          matrix.showIcon(ICON_CHECK);
          delay(100);
          digitalWrite(PIN_LED_IOT, LOW);
          matrix.showIcon(ICON_NUM2);
        }
      }

      // ตรวจสอบการกดปุ่ม Switch 2 บน Board 2
      if (digitalRead(PIN_SW2) == LOW) {
        if (now - last_sw2_time > debounce_delay) {
          last_sw2_time = now;
          msg_pub.data = 2;
          digitalWrite(PIN_LED_IOT, HIGH);
          rcl_publish(&publisher, &msg_pub, NULL);
          soundChirp();
          matrix.showIcon(ICON_EXCLAMATION);
          delay(100);
          digitalWrite(PIN_LED_IOT, LOW);
          matrix.showIcon(ICON_NUM2);
        }
      }
      break;
  }
}
