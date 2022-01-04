/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#include <EEPROM.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define COMMAND_WRITE           0x01
#define COMMAND_READ            0x02
#define COMMAND_CLEAR           0x03
#define COMMAND_SET_REAL_TIME   0x04
#define COMMAND_DISABLE_CELL    0x05

#define DELAY_TIME              10       // thời gian delay snooze

// CELL 1/2/3/4
unsigned int cell_1_old = 0, cell_2_old =0, cell_3_old = 0, cell_4_old = 0;

unsigned int gate_result = 0;
unsigned int gate_test = 0;

/*
shiftOut ra 1 Module 74HC595 LED 7 đoạn đơn
*/
int latchPin = 21;    //chân ST_CP của 74HC595
int clockPin = 18;    //chân SH_CP của 74HC595
int dataPin = 19;     //Chân DS của 74HC595

unsigned int  interval_program = 2000;   // mỗi 2s kiểm tra
unsigned long currentMillis_program = 0;
unsigned long previousMillis_program = 0;

byte flag_count = 0;
// delay for speaker
int setup_time_loa = 0;
int flag_cell_1_settime1 = 0, flag_cell_1_settime2 = 0, flag_cell_1_settime3 = 0;
int flag_cell_2_settime1 = 0, flag_cell_2_settime2 = 0, flag_cell_2_settime3 = 0;
int flag_cell_3_settime1 = 0, flag_cell_3_settime2 = 0, flag_cell_3_settime3 = 0;
int flag_cell_4_settime1 = 0, flag_cell_4_settime2 = 0, flag_cell_4_settime3 = 0;

unsigned int  interval_loa_cell = 2;                      // 2 phut

unsigned long currentMillis_loa_cell_1_settime1 = 0;
unsigned long previousMillis_loa_cell_1_settime1 = 0;
unsigned long currentMillis_loa_cell_1_settime2 = 0;
unsigned long previousMillis_loa_cell_1_settime2 = 0;
unsigned long currentMillis_loa_cell_1_settime3 = 0;
unsigned long previousMillis_loa_cell_1_settime3 = 0;
    
unsigned long currentMillis_loa_cell_2_settime1 = 0;
unsigned long previousMillis_loa_cell_2_settime1 = 0;
unsigned long currentMillis_loa_cell_2_settime2 = 0;
unsigned long previousMillis_loa_cell_2_settime2 = 0;
unsigned long currentMillis_loa_cell_2_settime3 = 0;
unsigned long previousMillis_loa_cell_2_settime3 = 0;


unsigned long currentMillis_loa_cell_3_settime1 = 0;
unsigned long previousMillis_loa_cell_3_settime1 = 0;
unsigned long currentMillis_loa_cell_3_settime2 = 0;
unsigned long previousMillis_loa_cell_3_settime2 = 0;
unsigned long currentMillis_loa_cell_3_settime3 = 0;
unsigned long previousMillis_loa_cell_3_settime3 = 0;

unsigned long currentMillis_loa_cell_4_settime1 = 0;
unsigned long previousMillis_loa_cell_4_settime1 = 0;
unsigned long currentMillis_loa_cell_4_settime2 = 0;
unsigned long previousMillis_loa_cell_4_settime2 = 0;
unsigned long currentMillis_loa_cell_4_settime3 = 0;
unsigned long previousMillis_loa_cell_4_settime3 = 0;


volatile int flag_wait_count_gate_open = 0;
volatile int wait_count_gate_open = 0;
volatile int flag_count_gate_open = 0;
volatile int count_gate_open = 0;


int enable_open_cell_speaker = 0;

// BUTTON
int interval_button = 100;
unsigned long currentMillis_button = 0;
unsigned long previousMillis_button = 0;

unsigned long currentMinutes_btn_stop = 0;
unsigned long previousMinutes_btn_stop = 0;

// định nghĩa chân kết nối INPUT - button
const int btn_snooze = 36;      // Snooze
const int btn_stop = 39;      // Stop

byte btn_snooze_value = 0;
byte btn_stop_value = 0;

int flag_btn_snooze = 0, flag_btn_stop = 0;
int flag_snooze_cell_1 = 0, flag_snooze_cell_2 = 0, flag_snooze_cell_3 = 0, flag_snooze_cell_4 = 0;

// định nghĩa chân kết nối INPUT - cảm biến khoảng cách tại cell 1/2/3/4
//const int position_pin[4] = {27, 14, 12, 13};
const int position_pin[4] = {13, 12, 14, 27};
int position_value[4] = {0};

// định nghĩa pin kết nối ADC - đọc ADC pin
const int ADC_pin = 34;
float ADC_pin_value = 0;
float voltage_in = 8.0; // 8.0V 
int R1 = 10000, R2 = 4700;

// We assigned a name LED pin to pin number 5
//const int LEDPIN = 5;
const int SPEAKER_PIN = 23; 
byte loa = 1;


// pin for led battery
const int led_battery[5] = {15, 2, 0, 4, 5}; 



// Ta sẽ xây dựng mảng hằng số với các giá trị cho trước
// Các bit được đánh số thứ tự (0-7) từ phải qua trái (tương ứng với A-F,DP)
// Vì ta dùng LED 7 đoạn chung cực dương nên với các bit 0
// thì các đoạn của LED 7 đoạn sẽ sáng
// với các bit 1 thì đoạn ấy sẽ tắt
const byte Seg[11] = {
  0b11000000,   // 0 - các thanh từ a-f sáng
  0b11111001,   // 1 - chỉ có 2 thanh b,c sáng
  0b10100100,   // 2
  0b10110000,   // 3
  0b10011001,   // 4
  0b10010010,   // 5
  0b10000010,   // 6
  0b11111000,   // 7
  0b10000000,   // 8
  0b10010000,   // 9
  0b11111111,   // off
};


byte *display_led = new byte[4];

/********************* FLASH **********************/
#define FLASH_MEMORY_SIZE 60

byte flag_write = 0, flag_read = 0, flag_clear = 0, flag_set_real_time = 0, flag_disable_cell = 0;

enum flash {
  store_cell_1 = 0,             // lưu số thuốc cần uống cell_1
  store_cell_1_settime1_h,      // lưu setup time lần 1 của cell_1 (hour)
  store_cell_1_settime1_m,      // lưu setup time lần 1 của cell_1 (minute)
  store_cell_1_settime2_h,      // lưu setup time lần 2 của cell_1 (hour)
  store_cell_1_settime2_m,      // lưu setup time lần 2 của cell_1 (minute)
  store_cell_1_settime3_h,      // lưu setup time lần 3 của cell_1 (hour)
  store_cell_1_settime3_m,      // lưu setup time lần 3 của cell_1 (minute)
  store_cell_1_ignore,          // lưu số lần bỏ uống thuốc cell_1

  store_cell_2 = 10,             // lưu số thuốc cần uống cell_2
  store_cell_2_settime1_h,      // lưu setup time lần 1 của cell_2 (hour)
  store_cell_2_settime1_m,      // lưu setup time lần 1 của cell_2 (minute)
  store_cell_2_settime2_h,      // lưu setup time lần 2 của cell_2 (hour)
  store_cell_2_settime2_m,      // lưu setup time lần 2 của cell_2 (minute)
  store_cell_2_settime3_h,      // lưu setup time lần 3 của cell_2 (hour)
  store_cell_2_settime3_m,      // lưu setup time lần 3 của cell_2 (minute)
  store_cell_2_ignore,          // lưu số lần bỏ uống thuốc cell_2

  store_cell_3 = 30,             // lưu số thuốc cần uống cell_3
  store_cell_3_settime1_h,      // lưu setup time lần 1 của cell_3 (hour)
  store_cell_3_settime1_m,      // lưu setup time lần 1 của cell_3 (minute)
  store_cell_3_settime2_h,      // lưu setup time lần 2 của cell_3 (hour)
  store_cell_3_settime2_m,      // lưu setup time lần 2 của cell_3 (minute)
  store_cell_3_settime3_h,      // lưu setup time lần 3 của cell_3 (hour)
  store_cell_3_settime3_m,      // lưu setup time lần 3 của cell_3 (minute)
  store_cell_3_ignore,          // lưu số lần bỏ uống thuốc cell_3

  store_cell_4 = 40,             // lưu số thuốc cần uống cell_4
  store_cell_4_settime1_h,      // lưu setup time lần 1 của cell_4 (hour)
  store_cell_4_settime1_m,      // lưu setup time lần 1 của cell_4 (minute)
  store_cell_4_settime2_h,      // lưu setup time lần 2 của cell_4 (hour)
  store_cell_4_settime2_m,      // lưu setup time lần 2 của cell_4 (minute)
  store_cell_4_settime3_h,      // lưu setup time lần 3 của cell_4 (hour)
  store_cell_4_settime3_m,      // lưu setup time lần 3 của cell_4 (minute)
  store_cell_4_ignore,          // lưu số lần bỏ uống thuốc cell_4

  store_real_time_h = 50,       // lưu thời gian thực khi mất điện
  store_real_time_m
};

unsigned int cell_1;             // số thuốc cần uống cell_1
unsigned int cell_2;             // số thuốc cần uống cell_2
unsigned int cell_3;             // số thuốc cần uống cell_3
unsigned int cell_4;             // số thuốc cần uống cell_4

unsigned int cell_1_ignore =0;
unsigned int cell_2_ignore =0;
unsigned int cell_3_ignore =0;
unsigned int cell_4_ignore =0;

unsigned int cell_1_settime1;       // lưu setup time lần 1 của cell_1 (minute = hour * 60 + minute)
unsigned int cell_1_settime2;       // lưu setup time lần 2 của cell_1 (minute = hour * 60 + minute)
unsigned int cell_1_settime3;       // lưu setup time lần 3 của cell_1 (minute = hour * 60 + minute)

unsigned int cell_2_settime1;       // lưu setup time lần 1 của cell_2 (minute = hour * 60 + minute)
unsigned int cell_2_settime2;       // lưu setup time lần 2 của cell_2 (minute = hour * 60 + minute )
unsigned int cell_2_settime3;       // lưu setup time lần 3 của cell_2 (minute = hour * 60 + minute)

unsigned int cell_3_settime1;       // lưu setup time lần 1 của cell_3 (minute = hour * 60 + minute)
unsigned int cell_3_settime2;       // lưu setup time lần 2 của cell_3 (minute = hour * 60 + minute)
unsigned int cell_3_settime3;       // lưu setup time lần 3 của cell_3 (minute = hour * 60 + minute)

unsigned int cell_4_settime1;       // lưu setup time lần 1 của cell_4 (minute = hour * 60 + minute)
unsigned int cell_4_settime2;       // lưu setup time lần 2 của cell_4 (minute = hour * 60 + minutenute)
unsigned int cell_4_settime3;       // lưu setup time lần 3 của cell_4 (minute = hour * 60 + minute)


unsigned int flag_cell_1;           // Cờ dùng để thông báo cell_1 đã thông báo uống thuốc, không kiểm tra thêm, sang phút mới sẽ được clear
unsigned int flag_cell_2;           // Cờ dùng để thông báo cell_2 đã thông báo uống thuốc, không kiểm tra thêm
unsigned int flag_cell_3;           // Cờ dùng để thông báo cell_3 đã thông báo uống thuốc, không kiểm tra thêm
unsigned int flag_cell_4;           // Cờ dùng để thông báo cell_4 đã thông báo uống thuốc, không kiểm tra thêm


byte disable_cell = 0;              // Lưu cell bị disable

/******************* END FLASH*********************/

/********************* TIMER **********************/
unsigned int count = 0, tmp_battery = 0;
unsigned int real_time = 0;
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

/*
 * Return 0b0000  - cell1/cell2/cell3/cell4
 * if 0 - cell close
 * if 1 - cell open
*/
unsigned int check_cell_open(void);

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  count++;
  flag_count = 0;
  if (count % 60 == 0) {         // biến count đếm 60s
    count = 0;
    real_time++;
    Serial.print("real_time: ");
    Serial.print(real_time / 60, DEC);
    Serial.print("h:");
    Serial.print(real_time % 60, DEC);
    Serial.println("m");
    Serial.print("gate_result: ");
    Serial.println(gate_result, BIN);

    Serial.print("check_cell_open: ");
    Serial.println(check_cell_open(), BIN);
    
    Serial.print("wait_count_gate_open: ");
    Serial.println(wait_count_gate_open);
    Serial.print("count_gate_open: ");
    Serial.println(count_gate_open);

    Serial.print("flag_wait_count_gate_open: ");
    Serial.println(flag_wait_count_gate_open);

    Serial.print("flag_count_gate_open: ");
    Serial.println(flag_count_gate_open);
    
    
    flag_cell_1 = 0;
    flag_cell_2 = 0;
    flag_cell_3 = 0;
    flag_cell_4 = 0;
    flag_btn_stop = 0;
    if (real_time == 1440)      // Đếm đủ 1 ngày = 1440 phút, reset đếm lại.
      real_time = 0;
  }
  if (flag_wait_count_gate_open == 1) {
    wait_count_gate_open++;
  }
  if (flag_count_gate_open == 1) {
    count_gate_open++;
  }
      
  portEXIT_CRITICAL_ISR(&timerMux);

}
/*******************END TIMER*********************/

/********************* BLE ***********************/
BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue[5] = {0};

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("deviceConnected = true");
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.println("deviceConnected = false");
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i], HEX);
        Serial.println();
        Serial.println("*********");
        switch (rxValue[0]) {
          case COMMAND_WRITE:
            flag_write = 1;
            if (rxValue[1] == 0x01) {        // set số thuốc cell_1
              cell_1 = rxValue[2];
              cell_1_settime1 = rxValue[3] * 60 + rxValue[4];
              cell_1_settime2 = rxValue[5] * 60 + rxValue[6];
              cell_1_settime3 = rxValue[7] * 60 + rxValue[8];
            }
            else if (rxValue[1] == 0x02) {   // set số thuốc cell_2
              cell_2 = rxValue[2];
              cell_2_settime1 = rxValue[3] * 60 + rxValue[4];
              cell_2_settime2 = rxValue[5] * 60 + rxValue[6];
              cell_2_settime3 = rxValue[7] * 60 + rxValue[8];
            }
            else if (rxValue[1] == 0x03) {   // set số thuốc cell_3
              cell_3 = rxValue[2];
              cell_3_settime1 = rxValue[3] * 60 + rxValue[4];
              cell_3_settime2 = rxValue[5] * 60 + rxValue[6];
              cell_3_settime3 = rxValue[7] * 60 + rxValue[8];
            }
            else if (rxValue[1] == 0x04) {   // set số thuốc cell_4
              cell_4 = rxValue[2];
              cell_4_settime1 = rxValue[3] * 60 + rxValue[4];
              cell_4_settime2 = rxValue[5] * 60 + rxValue[6];
              cell_4_settime3 = rxValue[7] * 60 + rxValue[8];
            }
            break;
          case COMMAND_READ:          // Hiển thị thông ra console, thông tin về số lần bỏ thuốc được gửi 5s/lần
            flag_read = 1;
            break;
          case COMMAND_CLEAR:         // clear số lần bỏ thuốc
            flag_clear = 1;
            break;
          case COMMAND_SET_REAL_TIME:
            flag_set_real_time = 1;
            real_time = rxValue[1] * 60 + rxValue[2];
            break;
          case COMMAND_DISABLE_CELL:
            flag_disable_cell = 1;
            disable_cell = rxValue[1];
            break;
          
          default:
            break;
        }
      }
    }
};
/************************************************/

void save_flash(void);
void clear_flash(void);
void clear_setup(void);

void print_data(void);

void setbit(unsigned int *num, unsigned int pos);
void clearbit(unsigned int *num, unsigned int pos);
/*
 * Hiển thị số thuốc cần uống tại lần lượt các ô 1-2-3-4
*/ 
void Display_led_7seg(void);


void output(int x);
float read_percent_volt_battery(void);
/*
 * Controll led battery
*/
void control_led_battery(void);

/*
 * Controll speaker
 * val = LOW off speaker
 * val = HIGH on speaker
*/
void speaker(unsigned char val);
void setup() {
  int i = 0;
  Serial.begin(115200);
  Serial.print("h:");
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  pinMode(btn_snooze, INPUT);
  pinMode(btn_stop, INPUT);
  
  pinMode(SPEAKER_PIN, OUTPUT);

  for (i = 0; i < 5; ++i) {
    pinMode(led_battery[i], OUTPUT);
  }

  for (i = 0; i < 4; ++i) {
    pinMode(position_pin[i], INPUT);;
  }
  /* Create the FLash */  
  Serial.print("h:");
  EEPROM.begin(FLASH_MEMORY_SIZE);
  cell_1 = EEPROM.read(store_cell_1);   // Đọc số thuốc cần uống cell_1 trong flash
  cell_2 = EEPROM.read(store_cell_2);   // Đọc số thuốc cần uống cell_1 trong flash
  cell_3 = EEPROM.read(store_cell_3);   // Đọc số thuốc cần uống cell_1 trong flash
  cell_4 = EEPROM.read(store_cell_4);   // Đọc số thuốc cần uống cell_1 trong flash

  cell_1_ignore = EEPROM.read(store_cell_1_ignore);   // Đọc số thuốc 
  cell_2_ignore = EEPROM.read(store_cell_2_ignore);   // Đọc số thuốc ignore
  cell_3_ignore = EEPROM.read(store_cell_3_ignore);   // Đọc số thuốc ignore
  cell_4_ignore = EEPROM.read(store_cell_4_ignore);   // Đọc số thuốc ignore

  cell_1_settime1 = EEPROM.read(store_cell_1_settime1_h)*60 + EEPROM.read(store_cell_1_settime1_m);     // Đọc thời gian uống thuốc 1, cell_1 (minute = hour * 60 + minute)
  cell_1_settime2 = EEPROM.read(store_cell_1_settime2_h)*60 + EEPROM.read(store_cell_1_settime2_m);     // Đọc thời gian uống thuốc 2, cell_1 (minute = hour * 60 + minute)
  cell_1_settime3 = EEPROM.read(store_cell_1_settime3_h)*60 + EEPROM.read(store_cell_1_settime3_m);     // Đọc thời gian uống thuốc 3, cell_1 (minute = hour * 60 + minute)

  cell_2_settime1 = EEPROM.read(store_cell_2_settime1_h)*60 + EEPROM.read(store_cell_2_settime1_m);     // Đọc thời gian uống thuốc 1, cell_2 (minute = hour * 60 + minute)
  cell_2_settime2 = EEPROM.read(store_cell_2_settime2_h)*60 + EEPROM.read(store_cell_2_settime2_m);     // Đọc thời gian uống thuốc 2, cell_2 (minute = hour * 60 + minute)
  cell_2_settime3 = EEPROM.read(store_cell_2_settime3_h)*60 + EEPROM.read(store_cell_2_settime3_m);     // Đọc thời gian uống thuốc 3, cell_2 (minute = hour * 60 + minute)

  cell_3_settime1 = EEPROM.read(store_cell_3_settime1_h)*60 + EEPROM.read(store_cell_3_settime1_m);     // Đọc thời gian uống thuốc 1, cell_3 (minute = hour * 60 + minute)
  cell_3_settime2 = EEPROM.read(store_cell_3_settime2_h)*60 + EEPROM.read(store_cell_3_settime2_m);     // Đọc thời gian uống thuốc 2, cell_3 (minute = hour * 60 + minute)
  cell_3_settime3 = EEPROM.read(store_cell_3_settime3_h)*60 + EEPROM.read(store_cell_3_settime3_m);     // Đọc thời gian uống thuốc 3, cell_3 (minute = hour * 60 + minute)

  cell_4_settime1 = EEPROM.read(store_cell_4_settime1_h)*60 + EEPROM.read(store_cell_4_settime1_m);     // Đọc thời gian uống thuốc 1, cell_4 (minute = hour * 60 + minute)
  cell_4_settime2 = EEPROM.read(store_cell_4_settime2_h)*60 + EEPROM.read(store_cell_4_settime2_m);     // Đọc thời gian uống thuốc 2, cell_4 (minute = hour * 60 + minute)
  cell_4_settime3 = EEPROM.read(store_cell_4_settime3_h)*60 + EEPROM.read(store_cell_4_settime3_m);     // Đọc thời gian uống thuốc 3, cell_4 (minute = hour * 60 + minute)
  print_data();

  
  
  /* Create the interrupt timner*/ 
  timer = timerBegin(0, 80, true);              // khỏi tạo timer với chu kì 1us vì thạch anh của ESP chạy 8MHz
  timerAttachInterrupt(timer, &onTimer, true);  //khởi tạo hàm xử lý ngắt ngắt cho Timer
  timerAlarmWrite(timer, 1000000, true);        //khởi tạo thời gian ngắt cho timer là 1s (1000000 us)
  timerAlarmEnable(timer);                      //bắt đầu chạy timer
  /* 
   * Create the BLE Device
   */
  BLEDevice::init("Smart medicine chest");
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

  display_led[0] = display_led[1] = display_led[2] = display_led[3] = 10;
  Display_led_7seg();         // off all led
}

void loop() {
  byte i = 0;
  /*
   * Read button every interval_button = 100ms
   */
  currentMillis_button = millis();
  if (currentMillis_button - previousMillis_button >= interval_button) {
        previousMillis_button = currentMillis_button;
        btn_snooze_value = digitalRead(btn_snooze);
        btn_stop_value = digitalRead(btn_stop);
        if (btn_snooze_value == 0)    // nhấn nút snooze
          flag_btn_snooze = 1;
        if (btn_stop_value == 0)    // nhấn nút snooze
          flag_btn_stop = 1;
    }
//    delay(100);
    /*
   * Read button every interval_button = 100ms
   */
  currentMillis_program = millis();
  if (currentMillis_program - previousMillis_program >= interval_program) {
    previousMillis_program = currentMillis_program;
    control_led_battery();
    if (flag_btn_stop == 1 )
     goto aa;
    // Check cell_1
    if (flag_cell_1 == 1 )
      goto cell2;
    if (real_time == cell_1_settime1) {
      Serial.print("cell_1_settime1: ");
      Serial.print(cell_1_settime1 / 60);
      Serial.print("h:");
      Serial.print(cell_1_settime1 % 60);
      Serial.println("m");
      display_led[0] = cell_1;
      Display_led_7seg();
      loa = 1;
      setup_time_loa = 1;
      flag_cell_1_settime1 = 1;
      flag_cell_1 = 1;
      if (flag_snooze_cell_1 == 1) {
        cell_1_settime1 = cell_1_settime1 - 2;      // return time when snooze button is pressed
        cell_1_settime2 = cell_1_settime2 - 2;
        cell_1_settime3 = cell_1_settime3 - 2;
        flag_snooze_cell_1 = 0;
      }
    }

    if (real_time == cell_1_settime2) {
      Serial.print("cell_1_settime2: ");
      Serial.print(cell_1_settime2 / 60);
      Serial.print("h:");
      Serial.print(cell_1_settime2 % 60);
      Serial.println("m");
      display_led[0] = cell_1;
      Display_led_7seg();
      loa = 1;
      setup_time_loa = 1;
      flag_cell_1_settime1 = 1;
      flag_cell_1 = 1;
      if (flag_snooze_cell_1 == 1) {
        cell_1_settime1 = cell_1_settime1 - 2;      // return time when snooze button is pressed
        cell_1_settime2 = cell_1_settime2 - 2;
        cell_1_settime3 = cell_1_settime3 - 2;
        flag_snooze_cell_1 = 0;
      }
    }

    if (real_time == cell_1_settime3) {
      Serial.print("cell_1_settime3: ");
      Serial.print(cell_1_settime3 / 60);
      Serial.print("h:");
      Serial.print(cell_1_settime3 % 60);
      Serial.println("m");
      display_led[0] = cell_1;
      Display_led_7seg();
      loa = 1;
      setup_time_loa = 1;
      flag_cell_1_settime1 = 1;
      flag_cell_1 = 1;
      if (flag_snooze_cell_1 == 1) {
        cell_1_settime1 = cell_1_settime1 - DELAY_TIME;      // return time when snooze button is pressed
        cell_1_settime2 = cell_1_settime2 - DELAY_TIME;
        cell_1_settime3 = cell_1_settime3 - DELAY_TIME;
        flag_snooze_cell_1 = 0;
      }
    }

    // Check cell_2
cell2:
    if (flag_cell_2 ==1 )
      goto cell3;
    if (real_time == cell_2_settime1) {
      Serial.print("cell_2_settime1: ");
      Serial.print(cell_2_settime1 / 60);
      Serial.print("h:");
      Serial.print(cell_2_settime1 % 60);
      Serial.println("m");
      display_led[1] = cell_2;
      Display_led_7seg();
      loa = 1;
      setup_time_loa = 1;
      flag_cell_2_settime1 = 1;
      flag_cell_2 = 1;
      if (flag_snooze_cell_2 == 1) {
        cell_2_settime1 = cell_2_settime1 - DELAY_TIME;      // return time when snooze button is pressed
        cell_2_settime2 = cell_2_settime2 - DELAY_TIME;
        cell_2_settime3 = cell_2_settime3 - DELAY_TIME;
        flag_snooze_cell_2 = 0;
      }
    }
    if (real_time == cell_2_settime2) {
      Serial.print("cell_2_settime2: ");
      Serial.print(cell_2_settime2 / 60);
      Serial.print("h:");
      Serial.print(cell_2_settime2 % 60);
      Serial.println("m");
      display_led[1] = cell_2;
      Display_led_7seg();
      loa = 1;
      setup_time_loa = 1;
      flag_cell_2_settime1 = 1;
      flag_cell_2 = 1;
      if (flag_snooze_cell_2 == 1) {
        cell_2_settime1 = cell_2_settime1 - DELAY_TIME;      // return time when snooze button is pressed
        cell_2_settime2 = cell_2_settime2 - DELAY_TIME;
        cell_2_settime3 = cell_2_settime3 - DELAY_TIME;
        flag_snooze_cell_2 = 0;
      }
    }

    if (real_time == cell_2_settime3) {
      Serial.print("cell_2_settime3: ");
      Serial.print(cell_2_settime3 / 60);
      Serial.print("h:");
      Serial.print(cell_2_settime3 % 60);
      Serial.println("m");
      display_led[1] = cell_2;
      Display_led_7seg();
      loa = 1;
      setup_time_loa = 1;
      flag_cell_2_settime1 = 1;
      flag_cell_2 = 1;
      if (flag_snooze_cell_2 == 1) {
        cell_2_settime1 = cell_2_settime1 - DELAY_TIME;      // return time when snooze button is pressed
        cell_2_settime2 = cell_2_settime2 - DELAY_TIME;
        cell_2_settime3 = cell_2_settime3 - DELAY_TIME;
        flag_snooze_cell_2 = 0;
      }
    }

    // Check cell_3
cell3:
    if (flag_cell_3 == 1)
      goto cell4;
    if (real_time == cell_3_settime1) {
      Serial.print("cell_3_settime1: ");
      Serial.print(cell_3_settime1 / 60);
      Serial.print("h:");
      Serial.print(cell_3_settime1 % 60);
      Serial.println("m");
      display_led[2] = cell_3;
      Display_led_7seg();
      loa = 1;
      setup_time_loa = 1;
      flag_cell_3_settime1 = 1;
      flag_cell_3 = 1;
      if (flag_snooze_cell_3 == 1) {
        cell_3_settime1 = cell_3_settime1 - DELAY_TIME;      // return time when snooze button is pressed
        cell_3_settime2 = cell_3_settime2 - DELAY_TIME;
        cell_3_settime3 = cell_3_settime3 - DELAY_TIME;
        flag_snooze_cell_3 = 0;
      }
    }
    if (real_time == cell_3_settime2) {
      Serial.print("cell_3_settime2: ");
      Serial.print(cell_3_settime2 / 60);
      Serial.print("h:");
      Serial.print(cell_3_settime2 % 60);
      Serial.println("m");
      display_led[2] = cell_3;
      Display_led_7seg();;
      loa = 1;
      setup_time_loa = 1;
      flag_cell_3_settime1 = 1;
      flag_cell_3 = 1;
      if (flag_snooze_cell_3 == 1) {
        cell_3_settime1 = cell_3_settime1 - DELAY_TIME;      // return time when snooze button is pressed
        cell_3_settime2 = cell_3_settime2 - DELAY_TIME;
        cell_3_settime3 = cell_3_settime3 - DELAY_TIME;
        flag_snooze_cell_3 = 0;
      }
    }

    if (real_time == cell_3_settime3) {
      Serial.print("cell_3_settime3: ");
      Serial.print(cell_3_settime3 / 60);
      Serial.print("h:");
      Serial.print(cell_3_settime3 % 60);
      Serial.println("m");
      display_led[2] = cell_3;
      Display_led_7seg();
      loa = 1;
      setup_time_loa = 1;
      flag_cell_3_settime1 = 1;
      flag_cell_3 = 1;
      if (flag_snooze_cell_3 == 1) {
        cell_3_settime1 = cell_3_settime1 - DELAY_TIME;      // return time when snooze button is pressed
        cell_3_settime2 = cell_3_settime2 - DELAY_TIME;
        cell_3_settime3 = cell_3_settime3 - DELAY_TIME;
        flag_snooze_cell_3 = 0;
      }
    }

    // Check cell_4
cell4:
    if (flag_cell_4 == 1)
      goto aa;
    if (real_time == cell_4_settime1) {
      Serial.print("cell_4_settime1: ");
      Serial.print(cell_4_settime1 / 60);
      Serial.print("h:");
      Serial.print(cell_4_settime1 % 60);
      Serial.println("m");
      display_led[3] = cell_4;
      Display_led_7seg();
      loa = 1;
      setup_time_loa = 1;
      flag_cell_4_settime1 = 1;
      flag_cell_4 =1;
      if (flag_snooze_cell_4 == 1) {
        cell_4_settime1 = cell_4_settime1 - DELAY_TIME;      // return time when snooze button is pressed
        cell_4_settime2 = cell_4_settime2 - DELAY_TIME;
        cell_4_settime3 = cell_4_settime3 - DELAY_TIME;
        flag_snooze_cell_4 = 0;
      }
    }
    if (real_time == cell_4_settime2) {
      Serial.print("cell_4_settime2: ");
      Serial.print(cell_4_settime2 / 60);
      Serial.print("h:");
      Serial.print(cell_4_settime2 % 60);
      Serial.println("m");
      display_led[3] = cell_4;
      Display_led_7seg();
      loa = 1;
      setup_time_loa = 1;
      flag_cell_4_settime1 = 1;
      flag_cell_4 =1;
      if (flag_snooze_cell_4 == 1) {
        cell_4_settime1 = cell_4_settime1 - DELAY_TIME;      // return time when snooze button is pressed
        cell_4_settime2 = cell_4_settime2 - DELAY_TIME;
        cell_4_settime3 = cell_4_settime3 - DELAY_TIME;
        flag_snooze_cell_4 = 0;
      }
    }

    if (real_time == cell_4_settime3) {
      Serial.print("cell_4_settime3: ");
      Serial.print(cell_4_settime3 / 60);
      Serial.print("h:");
      Serial.print(cell_4_settime3 % 60);
      Serial.println("m");
      display_led[3] = cell_4;
      Display_led_7seg();
      loa = 1;
      setup_time_loa = 1;
      flag_cell_4_settime1 = 1;
      flag_cell_4 =1;
      if (flag_snooze_cell_4 == 1) {
        cell_4_settime1 = cell_4_settime1 - DELAY_TIME;      // return time when snooze button is pressed
        cell_4_settime2 = cell_4_settime2 - DELAY_TIME;
        cell_4_settime3 = cell_4_settime3 - DELAY_TIME;
        flag_snooze_cell_4 = 0;
      }
    }
  }

aa:
  if (count % 10 == 0 && flag_count == 0) {
      if (deviceConnected) {
        txValue[0] = COMMAND_READ;
        txValue[1] = cell_1_ignore;
        txValue[2] = cell_2_ignore;
        txValue[3] = cell_3_ignore;
        txValue[4] = cell_4_ignore;
          
        pTxCharacteristic->setValue(txValue, 5);
        pTxCharacteristic->notify();
        delay(10); // bluetooth stack will go into congestion, if too many packets are 
        memset(txValue, 0, 5);        // clear buffer Tx
        flag_count = 1;
      if (flag_read) {                // COMMAND READ
        Serial.println("flag_read");
        flag_read = 0;                // clear flag_read
        print_data();
      }  
    }       // end deviceConnected

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(300); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
  }
  
  if (flag_write) {
    print_data();
    save_flash();
    flag_write = 0;
  }
  if (flag_clear) {
    cell_1_ignore = 0;
    cell_2_ignore = 0;
    cell_3_ignore = 0;
    cell_4_ignore = 0;
    
    EEPROM.write(store_cell_1_ignore, 0);   // reset default value
    EEPROM.write(store_cell_2_ignore, 0);   // 
    EEPROM.write(store_cell_3_ignore, 0);   // 
    EEPROM.write(store_cell_4_ignore, 0);   //
    EEPROM.commit();

    flag_clear = 0;
  }
  if (flag_set_real_time) {
    flag_set_real_time = 0;
  }

  if (flag_disable_cell == 1) {
    switch (disable_cell) {
      case 0x01:
        cell_1_settime1 = 10370;       // lưu setup time lần 1 của cell_1 (minute = hour * 60 + minute = AA * 60 + AA)
        cell_1_settime2 = 10370;       // lưu setup time lần 2 của cell_1 (minute = hour * 60 + minute)
        cell_1_settime3 = 10370;       // lưu setup time lần 3 của cell_1 (minute = hour * 60 + minute)
        break;
      case 0x02:
        cell_2_settime1 = 10370;       // lưu setup time lần 1 của cell_1 (minute = hour * 60 + minute = AA * 60 + AA)
        cell_2_settime2 = 10370;       // lưu setup time lần 2 của cell_1 (minute = hour * 60 + minute)
        cell_2_settime3 = 10370;       // lưu setup time lần 3 của cell_1 (minute = hour * 60 + minute)
        break;
      case 0x03:
        cell_3_settime1 = 10370;       // lưu setup time lần 1 của cell_1 (minute = hour * 60 + minute = AA * 60 + AA)
        cell_3_settime2 = 10370;       // lưu setup time lần 2 của cell_1 (minute = hour * 60 + minute)
        cell_3_settime3 = 10370;       // lưu setup time lần 3 của cell_1 (minute = hour * 60 + minute)
        break;
      case 0x04:
        cell_3_settime1 = 10370;       // lưu setup time lần 1 của cell_1 (minute = hour * 60 + minute = AA * 60 + AA)
        cell_3_settime2 = 10370;       // lưu setup time lần 2 của cell_1 (minute = hour * 60 + minute)
        cell_3_settime3 = 10370;       // lưu setup time lần 3 của cell_1 (minute = hour * 60 + minute)
        break;
      default:
        break;
    }
    flag_write = 1;             // write data flash
    flag_disable_cell = 0;
  }

  if (flag_btn_snooze == 1) {
    // cell_1
    if (flag_cell_1_settime1 == 1 || flag_cell_1_settime2 == 1 || flag_cell_1_settime3 == 1) {
      flag_cell_1_settime1 = 0;
      flag_cell_1_settime2 = 0;
      flag_cell_1_settime3 = 0;
      speaker(LOW);
      flag_cell_1 = 0;
      setup_time_loa = 1;
      cell_1_settime1 = cell_1_settime1 + DELAY_TIME;   // tăng DELAY_TIME phút
      cell_1_settime2 = cell_1_settime2 + DELAY_TIME;  
      cell_1_settime3 = cell_1_settime3 + DELAY_TIME; 
      flag_snooze_cell_1 = 1;                   // thông báo đã thêm thời gian hoãn
      display_led[0] = 10;
      Display_led_7seg();
    }

    // cell_2
    if (flag_cell_2_settime1 == 1 || flag_cell_2_settime2 == 1 || flag_cell_2_settime3 == 1) {
      flag_cell_2_settime1 = 0;
      flag_cell_2_settime2 = 0;
      flag_cell_2_settime3 = 0;
      speaker(LOW);
      flag_cell_2 = 0;
      setup_time_loa = 1;
      cell_2_settime1 = cell_2_settime1 + DELAY_TIME;   // tăng DELAY_TIME phút
      cell_2_settime2 = cell_2_settime2 + DELAY_TIME;  
      cell_2_settime3 = cell_2_settime3 + DELAY_TIME; 
      flag_snooze_cell_2 = 1;                   // thông báo đã thêm thời gian hoãn
      display_led[1] = 10;
      Display_led_7seg();
    }

    // cell_3
    if (flag_cell_3_settime1 == 1 || flag_cell_3_settime2 == 1 || flag_cell_3_settime3 == 1) {
      flag_cell_3_settime1 = 0;
      flag_cell_3_settime2 = 0;
      flag_cell_3_settime3 = 0;
      speaker(LOW);
      flag_cell_3 = 0;
      setup_time_loa = 1;
      cell_3_settime1 = cell_3_settime1 + DELAY_TIME;   // tăng DELAY_TIME phút
      cell_3_settime2 = cell_3_settime2 + DELAY_TIME;  
      cell_3_settime3 = cell_3_settime3 + DELAY_TIME; 
      flag_snooze_cell_3 = 1;                   // thông báo đã thêm thời gian hoãn
      display_led[2] = 10;
      Display_led_7seg();
    }

    // cell_4
    if (flag_cell_4_settime1 == 1 || flag_cell_4_settime2 == 1 || flag_cell_4_settime3 == 1) {
      flag_cell_4_settime1 = 0;
      flag_cell_4_settime2 = 0;
      flag_cell_4_settime3 = 0;
      speaker(LOW);
      flag_cell_4 = 0;
      setup_time_loa = 1;
      cell_4_settime1 = cell_4_settime1 + DELAY_TIME;   // tăng DELAY_TIME phút
      cell_4_settime2 = cell_4_settime2 + DELAY_TIME;  
      cell_4_settime3 = cell_4_settime3 + DELAY_TIME; 
      flag_snooze_cell_4 = 1;                   // thông báo đã thêm thời gian hoãn
      display_led[3] = 10;
      Display_led_7seg();
    }
    flag_btn_snooze = 0;
  }
  
  

  if (flag_btn_stop == 1) {     // nhấn stop thì kiểm tra xem có cell nào đang uống thuốc, nếu có tăng 1 lần bỏ thuốc

    if (flag_cell_1_settime1 == 1 || flag_cell_1_settime2 == 1 || flag_cell_1_settime3 == 1) {
      Serial.println("cell_1 ignore");
      cell_1_ignore++;
      EEPROM.write(store_cell_1_ignore, cell_1_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
      EEPROM.commit();
      flag_cell_1_settime1 = 0;
      flag_cell_1_settime2 = 0;
      flag_cell_1_settime3 = 0;
      
    }
      

    if (flag_cell_2_settime1 == 1 || flag_cell_2_settime2 == 1 || flag_cell_2_settime3 == 1) {
      Serial.println("cell_2 ignore");
      cell_2_ignore++;
      EEPROM.write(store_cell_2_ignore, cell_2_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
      EEPROM.commit();
      flag_cell_2_settime1 = 0;
      flag_cell_2_settime2 = 0;
      flag_cell_2_settime3 = 0;          
    }


    if (flag_cell_3_settime1 == 1 || flag_cell_3_settime2 == 1 || flag_cell_3_settime3 == 1) {
      Serial.println("cell_3 ignore");
      cell_3_ignore++;
      EEPROM.write(store_cell_3_ignore, cell_3_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
      EEPROM.commit();
      flag_cell_3_settime1 = 0;
      flag_cell_3_settime2 = 0;
      flag_cell_3_settime3 = 0;
    }


    if (flag_cell_4_settime1 == 1 || flag_cell_4_settime2 == 1 || flag_cell_4_settime3 == 1) {
      Serial.println("cell_4 ignore");
      cell_4_ignore++;
      EEPROM.write(store_cell_4_ignore, cell_4_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
      EEPROM.commit();
      flag_cell_4_settime1 = 0;
      flag_cell_4_settime2 = 0;
      flag_cell_4_settime3 = 0;
    }
    speaker(LOW);
    
    display_led[0] = display_led[1] = display_led[2] = display_led[3] = 10;
    Display_led_7seg();
  }


  if (loa == 1) { 
    if (setup_time_loa == 1) {
      // cell 1
      if (flag_cell_1_settime1 == 1) {
        currentMillis_loa_cell_1_settime1 = real_time;
        previousMillis_loa_cell_1_settime1 = currentMillis_loa_cell_1_settime1;
      }

      if (flag_cell_1_settime2 == 1) {
        currentMillis_loa_cell_1_settime2 = real_time;
        previousMillis_loa_cell_1_settime2 = currentMillis_loa_cell_1_settime2;
      }

      if (flag_cell_1_settime3 == 1) {
        currentMillis_loa_cell_1_settime3 = real_time;
        previousMillis_loa_cell_1_settime3 = currentMillis_loa_cell_1_settime3;
      }

      // cell_2
      if (flag_cell_2_settime1 == 1) {
        currentMillis_loa_cell_2_settime1 = real_time;
        previousMillis_loa_cell_2_settime1 = currentMillis_loa_cell_2_settime1;
      }

      if (flag_cell_2_settime2 == 1) {
        currentMillis_loa_cell_2_settime2 = real_time;
        previousMillis_loa_cell_2_settime2 = currentMillis_loa_cell_2_settime2;
      }

      if (flag_cell_2_settime3 == 1) {
        currentMillis_loa_cell_2_settime3 = real_time;
        previousMillis_loa_cell_2_settime3 = currentMillis_loa_cell_2_settime3;
      }

      // cell_3
      if (flag_cell_3_settime1 == 1) {
        currentMillis_loa_cell_3_settime1 = real_time;
        previousMillis_loa_cell_3_settime1 = currentMillis_loa_cell_3_settime1;
      }

      if (flag_cell_3_settime2 == 1) {
        currentMillis_loa_cell_3_settime2 = real_time;
        previousMillis_loa_cell_3_settime2 = currentMillis_loa_cell_3_settime2;
      }

      if (flag_cell_3_settime3 == 1) {
        currentMillis_loa_cell_3_settime3 = real_time;
        previousMillis_loa_cell_3_settime3 = currentMillis_loa_cell_3_settime3;
      }

      // cell_4
      if (flag_cell_4_settime1 == 1) {
        currentMillis_loa_cell_4_settime1 = real_time;
        previousMillis_loa_cell_4_settime1 = currentMillis_loa_cell_4_settime1;
      }

      if (flag_cell_4_settime2 == 1) {
        currentMillis_loa_cell_4_settime2 = real_time;
        previousMillis_loa_cell_4_settime2 = currentMillis_loa_cell_4_settime2;
      }

      if (flag_cell_4_settime3 == 1) {
        currentMillis_loa_cell_4_settime3 = real_time;
        previousMillis_loa_cell_4_settime3 = currentMillis_loa_cell_4_settime3;
      }
     
      setup_time_loa = 0;   // done setup
    }

    gate_result = check_cell_open();
    if ( flag_cell_1_settime1 || flag_cell_1_settime2 || flag_cell_1_settime3 ||  \
        flag_cell_2_settime1 || flag_cell_2_settime2 || flag_cell_2_settime3 ||  \
        flag_cell_4_settime1 || flag_cell_4_settime2 || flag_cell_4_settime3 ||  \
        flag_cell_3_settime1 || flag_cell_3_settime2 || flag_cell_3_settime3) {
    if (gate_result & 0b1000) {  // cell_1 mở 
      flag_cell_1_settime1 = 0;
      flag_cell_1_settime2 = 0;
      flag_cell_1_settime3 = 0;
      display_led[0] = 10;
      Display_led_7seg();
      speaker(LOW);
    }

    if (gate_result & 0b0100) {  //  cell_2 mở 
      flag_cell_2_settime1 = 0;
      flag_cell_2_settime2 = 0;
      flag_cell_2_settime3 = 0;
      display_led[1] = 10;
      Display_led_7seg();
      speaker(LOW);
    }

    if (gate_result & 0b0010) {  // cell_3 mở
      flag_cell_3_settime1 = 0;
      flag_cell_3_settime2 = 0;
      flag_cell_3_settime3 = 0;
      display_led[2] = 10;
      Display_led_7seg();
      speaker(LOW);
    }

    if (gate_result & 0b0001) {  // cell_4 mở 
      flag_cell_4_settime1 = 0;
      flag_cell_4_settime2 = 0;
      flag_cell_4_settime3 = 0;
      display_led[3] = 10;
      Display_led_7seg();
      speaker(LOW);
    }
        }
    if (gate_result != 0 && wait_count_gate_open == 0) { 
//      Serial.println("gate_result != 0 && wait_count_gate_open == 0");   
      flag_wait_count_gate_open = 1;                     // khi một trong 4 gate mở, biến này sẽ bắt đầu đếm từ 0, hàm ngắt 1s wait_count_gate_open++
    }
    if (gate_result !=0 && wait_count_gate_open == 90) {  // khi đủ 1.5 phút 1 trong 4 gate mở, biến wait_count_gate_open=180s, loa thông báo kêu trong 2 phút.
//      Serial.println("gate_result !=0 && wait_count_gate_open == 90");
      speaker(HIGH);
      flag_wait_count_gate_open = 0;
      
      flag_count_gate_open = 1;
    }
    if (gate_result !=0 && count_gate_open == 30) {
//      Serial.println("gate_result !=0 && count_gate_open == 30");
      wait_count_gate_open = 0;
      flag_count_gate_open = 0;
      count_gate_open = 0;
      if (display_led[0] == 10 && display_led[1] == 10 && display_led[2] == 10 && display_led[3] == 10) {   // Tất các các cell đều không trong trạng thái thông báo.
        speaker(LOW);                                   // off loa khi các cell không trong trạng thái thông báo uống thuốc.
      }
        
    }

    if (gate_result == 0) {
      if (display_led[0] == 10 && display_led[1] == 10 && display_led[2] == 10 && display_led[3] == 10) {   // Tất các các cell đều không trong trạng thái thông báo.
        speaker(LOW);                                   //  off loa khi các cell không trong trạng thái thông báo uống thuốc.
      }
      flag_wait_count_gate_open = 0;
      wait_count_gate_open = 0;
      flag_count_gate_open = 0;
      count_gate_open = 0;
    }
      
    // cell_1
    if (flag_cell_1_settime1 == 1) {
      speaker(HIGH);
      currentMillis_loa_cell_1_settime1 = real_time;
      if (currentMillis_loa_cell_1_settime1 - previousMillis_loa_cell_1_settime1 >=  interval_loa_cell) {
        speaker(LOW);
        cell_1_ignore++;               // Tăng số lần bỏ thuốc
        EEPROM.write(store_cell_1_ignore, cell_1_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
        EEPROM.commit();
        flag_cell_1_settime1 = 0;
        display_led[0] = 10;
        Display_led_7seg();
      }
    }

    if (flag_cell_1_settime2 == 1) {
      speaker(HIGH);
      currentMillis_loa_cell_1_settime2 = real_time;
      if (currentMillis_loa_cell_1_settime2 - previousMillis_loa_cell_1_settime2 >=  interval_loa_cell) {
        speaker(LOW);
        cell_1_ignore++;               // Tăng số lần bỏ thuốc
        EEPROM.write(store_cell_1_ignore, cell_1_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
        EEPROM.commit();
        flag_cell_1_settime2 = 0;
        display_led[0] = 10;
        Display_led_7seg();
      }
    }

    if (flag_cell_1_settime3 == 1) {
      speaker(HIGH);
      currentMillis_loa_cell_1_settime3 = real_time;
      if (currentMillis_loa_cell_1_settime3 - previousMillis_loa_cell_1_settime3 >=  interval_loa_cell) {
        speaker(LOW);
        cell_1_ignore++;               // Tăng số lần bỏ thuốc
        EEPROM.write(store_cell_1_ignore, cell_1_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
        EEPROM.commit();
        flag_cell_1_settime3 = 0;
        display_led[0] = 10;
        Display_led_7seg();
        
      }
    }

    // cell_2
    if (flag_cell_2_settime1 == 1) {
      speaker(HIGH);
      currentMillis_loa_cell_2_settime1 = real_time;
      if (currentMillis_loa_cell_2_settime1 - previousMillis_loa_cell_2_settime1 >=  interval_loa_cell) {
        speaker(LOW);
        cell_2_ignore++;               // Tăng số lần bỏ thuốc
        EEPROM.write(store_cell_2_ignore, cell_2_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
        EEPROM.commit();
        flag_cell_2_settime1 = 0;
        display_led[1] = 10;
        Display_led_7seg();
      }
    }

    if (flag_cell_2_settime2 == 1) {
      speaker(HIGH);
      currentMillis_loa_cell_2_settime2 = real_time;
      if (currentMillis_loa_cell_2_settime2 - previousMillis_loa_cell_2_settime2 >=  interval_loa_cell) {
        speaker(LOW);
        cell_2_ignore++;               // Tăng số lần bỏ thuốc
        EEPROM.write(store_cell_2_ignore, cell_2_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
        EEPROM.commit();
        flag_cell_2_settime2 = 0;
        display_led[1] = 10;
        Display_led_7seg();
        
      }
    }

    if (flag_cell_2_settime3 == 1) {
      speaker(HIGH);
      currentMillis_loa_cell_2_settime3 = real_time;
      if (currentMillis_loa_cell_2_settime3 - previousMillis_loa_cell_2_settime3 >=  interval_loa_cell) {
        speaker(LOW);
        cell_2_ignore++;               // Tăng số lần bỏ thuốc
        EEPROM.write(store_cell_2_ignore, cell_2_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
        EEPROM.commit();
        flag_cell_2_settime3 = 0;
        display_led[1] = 10;
        Display_led_7seg();
        
      }
    }

    // cell_3
    if (flag_cell_3_settime1 == 1) {
      speaker(HIGH);
      currentMillis_loa_cell_3_settime1 = real_time;
      if (currentMillis_loa_cell_3_settime1 - previousMillis_loa_cell_3_settime1 >=  interval_loa_cell) {
        speaker(LOW);
        cell_3_ignore++;               // Tăng số lần bỏ thuốc
        EEPROM.write(store_cell_3_ignore, cell_3_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
        EEPROM.commit();
        flag_cell_3_settime1 = 0;
        display_led[2] = 10;
        Display_led_7seg();
      }
    }

    if (flag_cell_3_settime2 == 1) {
      speaker(HIGH);
      currentMillis_loa_cell_3_settime2 = real_time;
      if (currentMillis_loa_cell_3_settime2 - previousMillis_loa_cell_3_settime2 >=  interval_loa_cell) {
        speaker(LOW);
        cell_3_ignore++;               // Tăng số lần bỏ thuốc
        EEPROM.write(store_cell_3_ignore, cell_3_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
        EEPROM.commit();
        flag_cell_3_settime2 = 0;
        display_led[2] = 10;
        Display_led_7seg();
        
      }
    }

    if (flag_cell_3_settime3 == 1) {
      speaker(HIGH);
      currentMillis_loa_cell_3_settime3 = real_time;
      if (currentMillis_loa_cell_3_settime3 - previousMillis_loa_cell_3_settime3 >=  interval_loa_cell) {
        speaker(LOW);
        cell_3_ignore++;               // Tăng số lần bỏ thuốc
        EEPROM.write(store_cell_3_ignore, cell_3_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
        EEPROM.commit();
        flag_cell_3_settime3 = 0;
        display_led[2] = 10;
        
        Display_led_7seg();
        
      }
    }


    // cell_4
    if (flag_cell_4_settime1 == 1) {
      speaker(HIGH);
      currentMillis_loa_cell_4_settime1 = real_time;
      if (currentMillis_loa_cell_4_settime1 - previousMillis_loa_cell_4_settime1 >=  interval_loa_cell) {
        speaker(LOW);
        cell_4_ignore++;               // Tăng số lần bỏ thuốc
        EEPROM.write(store_cell_4_ignore, cell_4_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
        EEPROM.commit();
        flag_cell_4_settime1 = 0;
        display_led[3] = 10;
        Display_led_7seg();
      }
    }

    if (flag_cell_4_settime2 == 1) {
      speaker(HIGH);
      currentMillis_loa_cell_4_settime2 = real_time;
      if (currentMillis_loa_cell_4_settime2 - previousMillis_loa_cell_4_settime2 >=  interval_loa_cell) {
        speaker(LOW);
        cell_4_ignore++;               // Tăng số lần bỏ thuốc
        EEPROM.write(store_cell_4_ignore, cell_4_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
        EEPROM.commit();
        flag_cell_4_settime2 = 0;
        display_led[3] = 10;
        Display_led_7seg();
        
      }
    }

    if (flag_cell_4_settime3 == 1) {
      speaker(HIGH);
      currentMillis_loa_cell_4_settime3 = real_time;
      if (currentMillis_loa_cell_4_settime3 - previousMillis_loa_cell_4_settime3 >=  interval_loa_cell) {
        speaker(LOW);
        cell_4_ignore++;               // Tăng số lần bỏ thuốc
        EEPROM.write(store_cell_4_ignore, cell_4_ignore);             // mỗi khi cell_x_ignore thay đổi sẽ ghi flash
        EEPROM.commit();
        flag_cell_4_settime3 = 0;
        display_led[3] = 10;
        Display_led_7seg();
        
     
      }
    }
    
  }
}


void save_flash(void)
{
  EEPROM.write(store_cell_1, cell_1);   // Lưu số thuốc cần uống cell_1 trong flash
  EEPROM.write(store_cell_2, cell_2);   // Lưu số thuốc cần uống cell_1 trong flash
  EEPROM.write(store_cell_3, cell_3);   // Lưu số thuốc cần uống cell_1 trong flash
  EEPROM.write(store_cell_4, cell_4);   // Lưu số thuốc cần uống cell_1 trong flash

  EEPROM.write(store_cell_1_settime1_h, cell_1_settime1 / 60); EEPROM.write(store_cell_1_settime1_m, cell_1_settime1 % 60);     // Lưu thời gian uống thuốc 1, cell_1 (minute = hour * 60 + minute)
  EEPROM.write(store_cell_1_settime2_h, cell_1_settime2 / 60); EEPROM.write(store_cell_1_settime2_m, cell_1_settime2 % 60);     // Lưu thời gian uống thuốc 2, cell_1 (minute = hour * 60 + minute)
  EEPROM.write(store_cell_1_settime3_h, cell_1_settime3 / 60); EEPROM.write(store_cell_1_settime3_m, cell_1_settime3 % 60);     // Lưu thời gian uống thuốc 3, cell_1 (minute = hour * 60 + minute)

  EEPROM.write(store_cell_2_settime1_h, cell_2_settime1 / 60); EEPROM.write(store_cell_2_settime1_m, cell_2_settime1 % 60);     // Lưu thời gian uống thuốc 1, cell_2 (minute = hour * 60 + minute)
  EEPROM.write(store_cell_2_settime2_h, cell_2_settime2 / 60); EEPROM.write(store_cell_2_settime2_m, cell_2_settime2 % 60);     // Lưu thời gian uống thuốc 2, cell_2 (minute = hour * 60 + minute)
  EEPROM.write(store_cell_2_settime3_h, cell_2_settime3 / 60); EEPROM.write(store_cell_2_settime3_m, cell_2_settime2 % 60);     // Lưu thời gian uống thuốc 3, cell_2 (minute = hour * 60 + minute)

  EEPROM.write(store_cell_3_settime1_h, cell_3_settime1 / 60); EEPROM.write(store_cell_3_settime1_m, cell_3_settime1 % 60);     // Lưu thời gian uống thuốc 1, cell_3 (minute = hour * 60 + minute)
  EEPROM.write(store_cell_3_settime2_h, cell_3_settime2 / 60); EEPROM.write(store_cell_3_settime2_m, cell_3_settime2 % 60);     // Lưu thời gian uống thuốc 2, cell_3 (minute = hour * 60 + minute)
  EEPROM.write(store_cell_3_settime3_h, cell_3_settime3 / 60); EEPROM.write(store_cell_3_settime3_m, cell_3_settime3 % 60);     // Lưu thời gian uống thuốc 3, cell_3 (minute = hour * 60 + minute)

  EEPROM.write(store_cell_4_settime1_h, cell_4_settime1 / 60); EEPROM.write(store_cell_4_settime1_m, cell_4_settime1 % 60);     // Lưu thời gian uống thuốc 1, cell_4 (minute = hour * 60 + minute)
  EEPROM.write(store_cell_4_settime2_h, cell_4_settime2 / 60); EEPROM.write(store_cell_4_settime2_m, cell_4_settime2 % 60);     // Lưu thời gian uống thuốc 2, cell_4 (minute = hour * 60 + minute)
  EEPROM.write(store_cell_4_settime3_h, cell_4_settime3 / 60); EEPROM.write(store_cell_4_settime3_m, cell_4_settime3 % 60);     // Lưu thời gian uống thuốc 3, cell_4 (minute = hour * 60 + minute)
  
  EEPROM.write(store_real_time_h, real_time / 60); EEPROM.write(store_real_time_m, real_time % 60);                             // Lưu real time

  EEPROM.write(store_cell_1_ignore, cell_1_ignore);   //
  EEPROM.write(store_cell_2_ignore, cell_2_ignore);   // 
  EEPROM.write(store_cell_3_ignore, cell_3_ignore);   // 
  EEPROM.write(store_cell_4_ignore, cell_4_ignore);   // 
  
  EEPROM.commit();
  Serial.println("Saving in flash memory. Done!!");
}


void clear_flash(void) 
{
  int j = 0;
  for (j = 0; j < FLASH_MEMORY_SIZE; j++)
    EEPROM.write(j, 0xAA);
    
  EEPROM.write(store_cell_1_ignore, 0);   // reset default value
  EEPROM.write(store_cell_2_ignore, 0);   // 
  EEPROM.write(store_cell_3_ignore, 0);   // 
  EEPROM.write(store_cell_4_ignore, 0);   //

  EEPROM.write(store_cell_1, 0);   // reset default value
  EEPROM.write(store_cell_2, 0);   // reset default value
  EEPROM.write(store_cell_3, 0);   // reset default value
  EEPROM.write(store_cell_4, 0);   // reset default value

  EEPROM.write(store_real_time_h,0); EEPROM.write(store_real_time_m, 0);                             // Lưu real time
  
  EEPROM.commit();
  Serial.println("Clearing in flash memory. Done!!");
}

void clear_setup(void)
{
  cell_1 = 0;             // số thuốc cần uống cell_1
  cell_2 = 0;             // số thuốc cần uống cell_2
  cell_3 = 0;             // số thuốc cần uống cell_3
  cell_4 = 0;             // số thuốc cần uống cell_4

  cell_1_settime1 = 10370;       // lưu setup time lần 1 của cell_1 (minute = hour * 60 + minute = AA * 60 + AA)
  cell_1_settime2 = 10370;       // lưu setup time lần 2 của cell_1 (minute = hour * 60 + minute)
  cell_1_settime3 = 10370;       // lưu setup time lần 3 của cell_1 (minute = hour * 60 + minute)

  cell_2_settime1 = 10370;       // lưu setup time lần 1 của cell_2 (minute = hour * 60 + minute)
  cell_2_settime2 = 10370;       // lưu setup time lần 2 của cell_2 (minute = hour * 60 + minute )
  cell_2_settime3 = 10370;       // lưu setup time lần 3 của cell_2 (minute = hour * 60 + minute)

  cell_3_settime1 = 10370;       // lưu setup time lần 1 của cell_3 (minute = hour * 60 + minute)
  cell_3_settime2 = 10370;       // lưu setup time lần 2 của cell_3 (minute = hour * 60 + minute)
  cell_3_settime3 = 10370;       // lưu setup time lần 3 của cell_3 (minute = hour * 60 + minute)

  cell_4_settime1 = 10370;       // lưu setup time lần 1 của cell_4 (minute = hour * 60 + minute)
  cell_4_settime2 = 10370;       // lưu setup time lần 2 của cell_4 (minute = hour * 60 + minutenute)
  cell_4_settime3 = 10370;       // lưu setup time lần 3 của cell_4 (minute = hour * 60 + minute)
}
void print_data(void) 
{
  Serial.print("cell_1_settime1:");
  Serial.print(cell_1_settime1 / 60 , DEC);
  Serial.print("h:");
  Serial.print(cell_1_settime1 % 60 , DEC);
  Serial.println("m");
  
  Serial.print("cell_1_settime2:");
  Serial.print(cell_1_settime2 / 60 , DEC);
  Serial.print("h:");
  Serial.print(cell_1_settime2 % 60 , DEC);
  Serial.println("m");

  Serial.print("cell_1_settime3:");
  Serial.print(cell_1_settime3 / 60 , DEC);
  Serial.print("h:");
  Serial.print(cell_1_settime3 % 60 , DEC);
  Serial.println("m");
  

  Serial.print("cell_2_settime1:");
  Serial.print(cell_2_settime1 / 60 , DEC);
  Serial.print("h:");
  Serial.print(cell_2_settime1 % 60 , DEC);
  Serial.println("m");
  
  Serial.print("cell_2_settime2:");
  Serial.print(cell_2_settime2 / 60 , DEC);
  Serial.print("h:");
  Serial.print(cell_2_settime2 % 60 , DEC);
  Serial.println("m");

  Serial.print("cell_2_settime3:");
  Serial.print(cell_2_settime3 / 60 , DEC);
  Serial.print("h:");
  Serial.print(cell_2_settime3 % 60 , DEC);
  Serial.println("m");


  Serial.print("cell_3_settime1:");
  Serial.print(cell_3_settime1 / 60 , DEC);
  Serial.print("h:");
  Serial.print(cell_3_settime1 % 60 , DEC);
  Serial.println("m");
  
  Serial.print("cell_3_settime2:");
  Serial.print(cell_3_settime2 / 60 , DEC);
  Serial.print("h:");
  Serial.print(cell_3_settime2 % 60 , DEC);
  Serial.println("m");

  Serial.print("cell_3_settime3:");
  Serial.print(cell_3_settime3 / 60 , DEC);
  Serial.print("h:");
  Serial.print(cell_3_settime3 % 60 , DEC);
  Serial.println("m");

  Serial.print("cell_4_settime1:");
  Serial.print(cell_4_settime1 / 60 , DEC);
  Serial.print("h:");
  Serial.print(cell_4_settime1 % 60 , DEC);
  Serial.println("m");
  
  Serial.print("cell_4_settime2:");
  Serial.print(cell_4_settime2 / 60 , DEC);
  Serial.print("h:");
  Serial.print(cell_4_settime2 % 60 , DEC);
  Serial.println("m");

  Serial.print("cell_4_settime3:");
  Serial.print(cell_4_settime3 / 60 , DEC);
  Serial.print("h:");
  Serial.print(cell_4_settime3 % 60 , DEC);
  Serial.println("m");

  Serial.print("cell_1_ignore: ");
  Serial.println(cell_1_ignore, DEC);
  Serial.print("cell_2_ignore: ");
  Serial.println(cell_2_ignore, DEC);
  Serial.print("cell_3_ignore: ");
  Serial.println(cell_3_ignore, DEC);
  Serial.print("cell_4_ignore: ");
  Serial.println(cell_4_ignore, DEC);
 

  Serial.print("real_time: ");
  Serial.print(real_time / 60, DEC);
  Serial.print("h:");
  Serial.print(real_time % 60, DEC);
  Serial.println("m");
  
  float adc_value = analogRead(ADC_pin);
  float vol = ( adc_value / 4095.0 * 3.3 - 1.0) / R2 * (R1 + R2);    // Vref 1100mV
  float result = (vol * 1.0 - 5.5) / (voltage_in - 5.5) * 100;               // dải áp từ 8V tới 5.5V
  Serial.print("vol:");
  Serial.print(vol, DEC);
  Serial.println(" V");


  Serial.print("gate: ");
  Serial.println(gate_result, BIN);

  Serial.print("gate_test: ");
  Serial.println(gate_test, BIN);
  
  Serial.print("check_cell_open: ");
  Serial.println(check_cell_open(), BIN);

  Serial.print("wait_count_gate_open: ");
    Serial.println(wait_count_gate_open);
    Serial.print("count_gate_open: ");
    Serial.println(count_gate_open);

    Serial.print("flag_wait_count_gate_open: ");
    Serial.println(flag_wait_count_gate_open);

    Serial.print("flag_count_gate_open: ");
    Serial.println(flag_count_gate_open);
}


void Display_led_7seg(void) 
{
  byte SoLed = 4;
  int j = 0;
  
  digitalWrite(latchPin, LOW);
  for (j = SoLed - 1; j >= 0; j--) {
    shiftOut(dataPin, clockPin, MSBFIRST, Seg[display_led[j]]);
  }
  digitalWrite(latchPin, HIGH);

}

void setbit(unsigned int *num, unsigned int pos)
{
  *num |= 1 << pos;
}

void clearbit(unsigned int *num, unsigned int pos)
{
  *num &= ~(1 << pos);
}
/*
 * Return 0b0000  - cell1/cell2/cell3/cell4
 * if 0 - cell close
 * if 1 - cell open
*/
unsigned int check_cell_open(void)
{
  unsigned int gate = 0;
  int i = 0;
  for (i = 0; i < 4; ++i) {
    position_value[i] = digitalRead(position_pin[i]);
    if (position_value[i] == LOW)
      clearbit(&gate, i);
    else
      setbit(&gate, i);
  }
//  Serial.print("gate: ");
//  Serial.println(gate, BIN);
  return gate;
}

void output(int x)
{
  switch (x) {
  case 1:
    digitalWrite(led_battery[0], LOW);
    digitalWrite(led_battery[1], HIGH);
    digitalWrite(led_battery[2], HIGH);
    digitalWrite(led_battery[3], HIGH);
    digitalWrite(led_battery[4], HIGH);
    break;
  case 2:
    digitalWrite(led_battery[0], LOW);
    digitalWrite(led_battery[1], LOW);
    digitalWrite(led_battery[2], HIGH);
    digitalWrite(led_battery[3], HIGH);
    digitalWrite(led_battery[4], HIGH);
    break;
  case 3:
    digitalWrite(led_battery[0], LOW);
    digitalWrite(led_battery[1], LOW);
    digitalWrite(led_battery[2], LOW);
    digitalWrite(led_battery[3], HIGH);
    digitalWrite(led_battery[4], HIGH);
    break;
  case 4:
    digitalWrite(led_battery[0], LOW);
    digitalWrite(led_battery[1], LOW);
    digitalWrite(led_battery[2], LOW);
    digitalWrite(led_battery[3], LOW);
    digitalWrite(led_battery[4], HIGH);
    break;
  case 5:
    digitalWrite(led_battery[0], LOW);
    digitalWrite(led_battery[1], LOW);
    digitalWrite(led_battery[2], LOW);
    digitalWrite(led_battery[3], LOW);
    digitalWrite(led_battery[4], LOW);
    break;
  default:
    digitalWrite(led_battery[0], HIGH);
    digitalWrite(led_battery[1], HIGH);
    digitalWrite(led_battery[2], LOW);
    digitalWrite(led_battery[3], LOW);
    digitalWrite(led_battery[4], LOW);
    break; 
  }
}
float read_percent_volt_battery(void)
{
  float adc_value = analogRead(ADC_pin);
  float vol = ( adc_value / 4095.0 * 3.3 - 1.0) / R2 * (R1 + R2);    // Vref 1100mV
  float result = (vol * 1.0 - 5.5) / (voltage_in - 5.5) * 100;               // dải áp từ 8V tới 5.5V
//  Serial.print("vol:");
//  Serial.println(vol, DEC);
  if (result >= 100)
    return 100;
  else if (result <= 0)
    return 0;
  else 
    return result;
}

/*
 * Controll led battery
*/
void control_led_battery(void)
{
  float per_battery = read_percent_volt_battery();
  if (per_battery < 20)
    output(1);
  else if (per_battery < 40)
    output(2);
  else if (per_battery < 60)
    output(3);
  else if (per_battery < 80)
    output(4);
  else if (per_battery <= 100)
    output(5);
}

void speaker(unsigned char val)
{
  if (val ==0)
    digitalWrite(SPEAKER_PIN, LOW);
  else 
    digitalWrite(SPEAKER_PIN, HIGH);
}
