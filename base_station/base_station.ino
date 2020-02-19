//#include <Sodaq_R4X.h>
#include <Sodaq_N2X.h>
#include <Sodaq_wdt.h>
#include <Sodaq_LSM303AGR.h>
#include <TM1637.h>
#if defined(ARDUINO_SODAQ_AUTONOMO)
/* SODAQ AUTONOMO + SODAQ NB-IoT R41XM Bee */
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial1
#define powerPin BEE_VCC
#define enablePin BEEDTR
#elif defined(ARDUINO_SODAQ_SARA)
/* SODAQ SARA AFF*/
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial1
#define powerPin SARA_ENABLE
#define enablePin SARA_TX_ENABLE
#define MODEM_ON_OFF_PIN SARA_ENABLE
#elif defined(ARDUINO_SODAQ_SFF)
/* SODAQ SARA SFF*/
#define DEBUG_STREAM SerialUSB
#define MODEM_STREAM Serial
#define powerPin SARA_ENABLE
#define enablePin SARA_TX_ENABLE
#else
#error "Please use one of the listed boards or add your board."
#endif
#define DEBUG_STREAM SerialUSB
#define DEBUG_STREAM_BAUD 115200
#define STARTUP_DELAY 1000

// NL LMT NB
const char* apn = "nb-iot.lmt.lv";
const char* forceOperator = "24701"; // optional - depends on SIM / network (network Local) Country 
uint8_t urat = 20;

#define MQ3_PIN A7
#define MQ3_MAX_LIFETIME 20000
#define MQ3_UPDATE_RATE 1000
int mq3_reading = -1;
unsigned long mq3_current_lifetime = 0;

#define DATA_SEND_INTERVAL 2500
unsigned long next_send_time = 0;

#define LCD_CLK A4
#define LCD_DIO A5
TM1637 lcd(LCD_CLK, LCD_DIO);

Sodaq_N2X n2x;
Sodaq_LSM303AGR AccMeter;
static Sodaq_SARA_N211_OnOff saraR4xxOnOff;

void send_mq3_reading_tcp()
{
    if(millis() < next_send_time) return;
    next_send_time = millis() + DATA_SEND_INTERVAL;
    DEBUG_STREAM.println();
    DEBUG_STREAM.println("Sending message through UDP");
    int localPort = 16666;
    int socketID = n2x.socketCreate(localPort, TCP); //0 = TCP
    if (socketID >= 7 || socketID < 0) {
        DEBUG_STREAM.println("Socket:" + socketID);
        DEBUG_STREAM.println("Failed to create socket");
        return;
    }
    DEBUG_STREAM.println("Created UDP socket!");
    String deviceId = "5goY2L60l3RQHJgweUnXLYrA";
    String token = "maker:4MC9F0vopz86m0lqFyOE6RgK58yDtGoBsHff4Tr1";
    //String value = "{\"value\": \"" + String(analogRead(A0)) + "\"}";
    String value = "{\"co2\":{\"value\": " + String(mq3_reading) + "}}";
    String reading = deviceId + '\n' + token + '\n' + value;
    uint8_t size = reading.length();
    int lengthSent = n2x.socketSend(socketID, "40.68.172.187", 8891, (uint8_t*)reading.c_str(), size);
    DEBUG_STREAM.print("Length buffer vs sent:");
    DEBUG_STREAM.print(size);
    DEBUG_STREAM.print(",");
    DEBUG_STREAM.println(lengthSent);
    n2x.socketClose(socketID);
    DEBUG_STREAM.println();
}

void update_mq3_reading() {
    int r = analogRead(MQ3_PIN);
    if(r > mq3_reading) {
        mq3_reading = r;
        mq3_current_lifetime = millis() + MQ3_MAX_LIFETIME;
    }
    else {
        if(millis() > mq3_current_lifetime) {
            mq3_reading = r;
            mq3_current_lifetime = millis() + MQ3_MAX_LIFETIME;
            
        }
    }
    send_mq3_reading_tcp();
    DEBUG_STREAM.print("MQ3 current: ");
    DEBUG_STREAM.print(r);
    DEBUG_STREAM.print(",MQ3 max: ");
    DEBUG_STREAM.print(mq3_reading);
    DEBUG_STREAM.print(",until: ");
    DEBUG_STREAM.println(mq3_current_lifetime);
    lcd.displayNum(mq3_reading*6+random(-10,10));
}

void setup()
{
    lcd.init();
    lcd.set(BRIGHT_TYPICAL);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
    lcd.clearDisplay();
    lcd.displayStr("*   ");
    pinMode(MQ3_PIN, INPUT);
    sodaq_wdt_safe_delay(STARTUP_DELAY);
    #ifdef powerPin
    // Turn the nb-iot module on
    pinMode(powerPin, OUTPUT); 
    digitalWrite(powerPin, HIGH);
    DEBUG_STREAM.println("powerPin!");
  #endif
  #ifdef enablePin
    // Set state to active
    pinMode(enablePin, OUTPUT);
    digitalWrite(enablePin, HIGH);
    DEBUG_STREAM.println("enablePin!");
  #endif // enablePin
    Wire.begin();
    lcd.displayStr("*** ");
    delay(1000);
    DEBUG_STREAM.println("BASE STATION");
    DEBUG_STREAM.begin(DEBUG_STREAM_BAUD);
    MODEM_STREAM.begin(n2x.getDefaultBaudrate());
    DEBUG_STREAM.println("Initializing and connecting... ");
    n2x.setDiag(DEBUG_STREAM);
    n2x.init(&saraR4xxOnOff, MODEM_STREAM);
    lcd.displayStr("Conn");
    if (n2x.on()) {
       DEBUG_STREAM.println("turning modem on");
    }
    if (!n2x.connect(apn,"0",forceOperator, urat)) {
       DEBUG_STREAM.println("FAILED TO CONNECT TO MODEM");
       lcd.displayStr("Err0");
       while(true) {
           //Freeze
       }
    }
}

void loop()
{
    sodaq_wdt_safe_delay(MQ3_UPDATE_RATE);
    update_mq3_reading();
    digitalWrite(LED_BUILTIN, millis() % 500 > 250);
}