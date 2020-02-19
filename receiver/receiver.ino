//#include <Sodaq_R4X.h>
#include <Sodaq_N2X.h>
#include <Sodaq_wdt.h>
#include <Sodaq_LSM303AGR.h>
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
#define STARTUP_DELAY 5000

// NL LMT NB
const char* apn = "nb-iot.lmt.lv";
const char* forceOperator = "24701"; // optional - depends on SIM / network (network Local) Country 
uint8_t urat = 20;

const char* serv_ip = "45.77.137.6";
int serv_port = 31337;

uint8_t recv_buffer[32];
int recv_reading = 0;

Sodaq_N2X n2x;
Sodaq_LSM303AGR AccMeter;
static Sodaq_SARA_N211_OnOff saraR4xxOnOff;

#define close_button A6
#define READ_INTERVAL 5000
unsigned long next_read_time = 0;

void read_udp_data()
{
    if(millis() < next_read_time) return;
    next_read_time = millis() + READ_INTERVAL;
    DEBUG_STREAM.println();
    DEBUG_STREAM.println("Sending message through UDP");
    int localPort = 16666;
    int socketID = n2x.socketCreate(localPort, UDP); //0 = TCP
    if (socketID >= 7 || socketID < 0) {
        DEBUG_STREAM.println("Socket:" + socketID);
        DEBUG_STREAM.println("Failed to create socket");
        return;
    }
    DEBUG_STREAM.println("Created UDP socket!");
    String ping_msg = String("sup.");
    int lengthSent = n2x.socketSend(socketID, serv_ip, serv_port, (uint8_t*)ping_msg.c_str(), 4);
    DEBUG_STREAM.print("Length sent:");
    DEBUG_STREAM.println(lengthSent);
    if(n2x.socketWaitForReceive(socketID, 3000)) {
        size_t pending = n2x.socketGetPendingBytes(socketID);
        size_t read_size = n2x.socketReceive(socketID, recv_buffer, pending);
        DEBUG_STREAM.print("Len Pending buffer:");
        DEBUG_STREAM.print(pending);
        DEBUG_STREAM.print(", read buffer len:");
        DEBUG_STREAM.print(read_size);
        DEBUG_STREAM.print(", buffer:");
        DEBUG_STREAM.print(recv_buffer[0], BIN);
        DEBUG_STREAM.print(recv_buffer[1], BIN);
        recv_reading = recv_buffer[0] | (recv_buffer[1] << 8);
        DEBUG_STREAM.print(", decoded to int:");
        DEBUG_STREAM.print(recv_reading, BIN);
        DEBUG_STREAM.print(" ");
        DEBUG_STREAM.print(recv_reading);
    }
    else {
        DEBUG_STREAM.println("Remote server did not respond");
    }

    n2x.socketClose(socketID);
    DEBUG_STREAM.println();
}

void setup()
{
    pinMode(close_button, INPUT_PULLUP); 
    pinMode(A4, OUTPUT);
    pinMode(A0, OUTPUT);

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
    delay(1000);
    DEBUG_STREAM.begin(DEBUG_STREAM_BAUD);
    MODEM_STREAM.begin(n2x.getDefaultBaudrate());
    DEBUG_STREAM.println("Initializing and connecting... ");
    n2x.setDiag(DEBUG_STREAM);
    n2x.init(&saraR4xxOnOff, MODEM_STREAM);
    if (n2x.on()) {
       DEBUG_STREAM.println("turning modem on");
    }
    if (!n2x.connect(apn,"0",forceOperator, urat)) {
       DEBUG_STREAM.println("FAILED TO CONNECT TO MODEM");
    }
}

void loop()
{
    digitalWrite(LED_BUILTIN, millis() % 500 > 250);
    bool state = recv_reading < 200;
    //state = digitalRead(A7);
    //Serial.println(state);
    if (state == 0 && digitalRead(close_button) == 0) {
        //Open
        DEBUG_STREAM.println("Opening window");
        digitalWrite(A0, HIGH);
        digitalWrite(A4, LOW);
        sodaq_wdt_safe_delay(6000);
        DEBUG_STREAM.println("Window opened");
    }
    else if(state == 1 && digitalRead(close_button) != 0) {
        DEBUG_STREAM.println("Bruh moment");
        while(digitalRead(close_button) != 0){ 
            //DEBUG_STREAM.print("Closing window ");
            //DEBUG_STREAM.println(millis());
            //close
            digitalWrite(A0, LOW);
            digitalWrite(A4, HIGH);
        }
    }
    else {
        read_udp_data();
    }
    digitalWrite(A0, LOW);
    digitalWrite(A4, LOW);
}