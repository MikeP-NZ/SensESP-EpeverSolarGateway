// Example code to show the usage of the eModbus library to retrieve data from an Epever Tracer 2206AN Solar charge controller and publish it to a signalK server
// Please refer to root/Readme.md for a (more)full description.


// Includes: <Arduino.h> for Serial etc.
#include <Arduino.h>
//#include "sensesp_app.h"
#include "sensesp/sensors/sensor.h"
#include "sensesp/signalk/signalk_output.h"
#include "sensesp_app_builder.h"
#include "ModbusClientRTU.h"
//#define SERIAL_DEBUG_DISABLED = true

#define Modbus_RX_pin GPIO_NUM_16
#define Modbus_TX_pin GPIO_NUM_17

using namespace sensesp;

reactesp::ReactESP app;

const String CHARGER_NAME = "epever1";

ModbusClientRTU* MB;
FloatSensor* panel_voltage;
FloatSensor* panel_current;
FloatSensor* charger_voltage;
FloatSensor* charger_current;
FloatSensor* charger_temperature;
FloatSensor* load_current;
FloatSensor* battery_temperature;
FloatSensor* output_today;
FloatSensor* output_this_month;
FloatSensor* output_this_year;
FloatSensor* output_total;
StringSensor* charging_mode;


float panel_power;
float battery_nominal_voltage;
float battery_voltage;
float battery_current;
float battery_power;
float battery_chg_power;
float load_voltage;
float load_power;
float battery_SOC;
uint16_t battery_volt_state;
uint16_t battery_temperature_state;
uint16_t controller_input_state;
uint16_t controller_charging_state;
uint16_t RTC_second;
uint16_t RTC_minute;
uint16_t RTC_hour;
uint16_t RTC_day;
uint16_t RTC_month;
uint16_t RTC_year;


  inline static const char *getBatteryVoltStatusText(uint16_t value) {
    switch (value) {
    case 0x01:
      return "Overvolt";
      break;
    case 0x00:
      return "Normal";
      break;
    case 0x02:
      return "Under Volt";
      break;  
    case 0x03:
      return "Low Volt Disconnect";
      break;  
    case 0x04:
      return "Fault";
    default:
      return "Unexpected Value";
    }
  }

  inline static const char *getBatteryTempStatusText(uint16_t value) {
    switch (value) {
    case 0x01:
      return "Over Temp";
      break;
    case 0x00:
      return "Normal";
      break;
    case 0x02:
      return "Low Temp";
      break;  
    default:
      return "Unexpected Value";
    }
  }

  inline static const char *getControllerInputVoltStatusText(uint16_t value) {
    switch (value) {
    case 0x01:
      return "No Power Connected";
      break;
    case 0x00:
      return "Normal";
      break;
    case 0x02:
      return "Higher Volt Input";
      break;  
    case 0x03:
      return "Input Volt Error";
      break;  
    default:
      return "Unexpected Value";
    } 
  }

  inline static const char *getControllerChargingStatusText(uint16_t value) {
    switch (value) {
    case 0x01:
      return "Float";
      break;
    case 0x00:
      return "No Charging";
      break;
    case 0x02:
      return "Boost";
      break;  
    case 0x03:
      return "Equalization";
      break;  
    default:
      return "Unexpected Value";
    } 
  }


// Define an onData handler function to receive the regular responses
// Arguments are Modbus server ID, the function code requested, the message data and length of it, 
// plus a user-supplied token to identify the causing request
void handleData(ModbusMessage response, uint32_t token) 
{
  uint16_t temp;
  int32_t tempLong;

  switch(token){
    case 0x3100 :
      response.get(3,temp);
      panel_voltage->emit(temp/100.0f);
      Serial.printf("PV Voltage = %5.2f V\n",panel_voltage->get());
      response.get(5,temp);
      panel_current->emit(temp/100.0f);
      Serial.printf("PV Current = %5.2f A\n",panel_current->get());
    //Get Hi-word
      response.get(9,temp);
      tempLong=temp<<16;
    //get low word
      response.get(7,temp);
      tempLong+=temp;
      panel_power=tempLong/100.0f;
      Serial.printf("PV Power   = %5.2f W\n",panel_power);
      break;

    case 0x331A :
      response.get(3,temp);
      charger_voltage->emit(temp/100.0f);
      Serial.printf("Battery Voltage = %5.2f V\n", charger_voltage->get());
    //Get Hi-word
      response.get(7,temp);
      tempLong=temp<<16;
    //get low word
      response.get(5,temp);
      tempLong+=temp;
      charger_current->emit(tempLong / 100.0f);
      Serial.printf("Battery Current = %+5.2f A\n", charger_current->get());
      battery_power = charger_voltage->get() * charger_current->get();
      Serial.printf("Battery Power   = %+5.2f W\n", battery_power);
      break;

 
    case 0x3106 :
    //Get Hi-word
      response.get(5,temp);
      tempLong=temp<<16;
    //get low word
      response.get(3,temp);
      tempLong+= temp;
      battery_chg_power = tempLong / 100.0f;//Charge power is power to battery + power to load
      Serial.printf("Battery Charging Power = %5.2f W\n",battery_chg_power);
      break;
    case 0x310C :
      response.get(3,temp);
      load_voltage = temp / 100.0f;
      Serial.printf("Load Voltage = %5.2f V\n",load_voltage);
      response.get(5,temp);
      load_current->emit(temp / 100.0f);
      Serial.printf("Load Current = %5.2f A\n",load_current->get());
    //Get Hi-word
      response.get(9,temp);
      tempLong=temp<<16;
    //get low word
      response.get(7,temp);
      tempLong+=temp;
      load_power=tempLong / 100.0;
      Serial.printf("Load Power   = %5.2f W\n",load_power);
      break;

    case 0x311A :
      response.get(3,temp);
      battery_SOC=temp;
      Serial.printf("Battery SOC = %5.0f %%\n",battery_SOC);
      response.get(5,temp);
      battery_temperature->emit(273.15f + temp / 100.0f);//convert degC to K
      Serial.printf("Remote Battery Temperature = %5.2f °C\n", battery_temperature->get() - 273.15f);
      break;

    case 0x3110 :
      // This temperature seems fixed at 25°C
      //response.get(3,temp);
      //battery_temperature=temp/100.0;
      //Serial.printf("Battery Temperature = %5.2f °C\n", battery_temperature);
      response.get(5,temp);
      charger_temperature->emit(273.15f + temp / 100.0f);
      Serial.printf("Internal Temperature = %5.2f °C\n", charger_temperature->get() - 273.15f);
      break;
 
    case 0x311D :
      response.get(3,temp);
      battery_nominal_voltage=temp / 100.0f;
      Serial.printf("Battery Nominal Voltage = %5.2f V\n", battery_nominal_voltage);
      break;
   
    case 0x3200 :
      response.get(3,temp);
      battery_volt_state = temp & 0x000F;
      Serial.printf("Battery Volt State = (%02u)%s\n",battery_volt_state, getBatteryVoltStatusText(battery_volt_state));
      battery_temperature_state = (temp>>4) & 0x000F;
      Serial.printf("Battery Temp State = (%02u)%s\n",battery_temperature_state, getBatteryTempStatusText(battery_temperature_state));
      response.get(5,temp);
      controller_input_state = temp>>14;
      Serial.printf("Controller Input Volt State = (%02u)%s\n",controller_input_state, getControllerInputVoltStatusText(controller_input_state));
      controller_charging_state = temp>>2 & 0x0003;
      charging_mode->emit(getControllerChargingStatusText(controller_charging_state));
      Serial.printf("Controller Charging State = (%02u)%s\n",controller_charging_state, getControllerChargingStatusText(controller_charging_state));
      break;

    case 0x330C :
    //Get Hi-word
      response.get(5,temp);
      tempLong = temp << 16;
    //get low word
      response.get(3,temp);
      tempLong += temp;
      output_today->emit(tempLong * 36000.0f);//convert to Joules
      Serial.printf("Generated Today = %5.2f kWh\n",output_today->get()/3600000.0f);
    //Get Hi-word
      response.get(9,temp);
      tempLong = temp << 16;
    //get low word
      response.get(7,temp);
      tempLong += temp;
      output_this_month->emit(tempLong * 36000.0f);//convert to Joules
      Serial.printf("Generated This Month = %5.2f kWh\n",output_this_month->get() / 3600000.0f);
     //Get Hi-word
      response.get(13,temp);
      tempLong=temp<<16;
    //get low word
      response.get(11,temp);
      tempLong+=temp;
      output_this_year->emit(tempLong * 36000.0f);//convert to Joules
      Serial.printf("Generated This Year = %5.2f kWh\n",output_this_year->get() / 3600000.0f);
     //Get Hi-word
      response.get(17,temp);
      tempLong=temp<<16;
    //get low word
      response.get(15,temp);
      tempLong+=temp;
      output_total->emit(tempLong * 36000.0f);//convert to Joules
      Serial.printf("Generated Total = %5.2f kWh\n",output_total->get() / 3600000.0f);
      break;

 case 0x9013 :// real time clock
      response.get(4,RTC_second);
      response.get(3,RTC_minute);
      response.get(6,RTC_hour);
      response.get(5,RTC_day);
      response.get(8,RTC_month);
      response.get(7,RTC_year);
      Serial.printf("Clock = %02u/%02u/%02u %02u:%02u:%02u\n",RTC_day,RTC_month,RTC_year,RTC_hour,RTC_minute,RTC_second);
      break;
  }//switch
}

// Define an onError handler function to receive error responses
// Arguments are the error code returned and a user-supplied token to identify the causing request
void handleError(Error error, uint32_t token) 
{
  // ModbusError wraps the error code and provides a readable error message for it
  ModbusError me(error);
  Serial.printf("Error response: %02X - %s\n", (int)me, (const char *)me);
}
/**
 * @brief Adds a read request to the queue
 * 
 * @param address Start address to read
 * @param length Number of bytes to be read
 */
void queueRequest(uint32_t address, uint32_t length)
{
  Error err = MB->addRequest(address, 1, READ_INPUT_REGISTER, address, length);
  if (err!=SUCCESS) {
    ModbusError e(err);
    Serial.printf("Error creating request: %02X - %s\n", (int)e, (const char *)e);
  }  
}

/**
 * @brief Create ReactESP App
 * 
 */

void setup(){
  String skPath;
  #ifndef SERIAL_DEBUG_DISABLED
    SetupSerialDebug(115200);
  #endif
  // Set up Serial2 connected to Modbus RTU
  Serial2.begin(115200, SERIAL_8N1, Modbus_RX_pin, Modbus_TX_pin);
  
  // Set up ModbusRTU client.
  // Create a ModbusRTU client instance
  // In my case the RS485 module had auto halfduplex, so no second parameter with the DE/RE pin is required!
  MB = new ModbusClientRTU(Serial2);
  // - provide onData handler function
  MB->onDataHandler(&handleData);
  // - provide onError handler function
  MB->onErrorHandler(&handleError);
  // Set message timeout to 2000ms
  MB->setTimeout(2000);
  // Start ModbusRTU background task
  MB->begin();
  
  delay(2000);//give wifi time to initialise

  // Create a builder object
  SensESPAppBuilder builder;
  // Create the global SensESPApp() object.
  sensesp_app = (&builder)
                    ->set_hostname("sksolar")
  //                ->set_wifi("SSID", "password") // if wifi credentials are not set here then connect to the ESP32 and set it using a browser.
                    ->get_app();
    skPath = "electrical.solar." + CHARGER_NAME + ".panelVoltage";
    panel_voltage = new FloatSensor;
    //panel_voltage->connect_to(new SKOutputFloat ("electrical.solar.epever1.panelVoltage", "", new SKMetadata("V")));
    panel_voltage->connect_to(new SKOutputFloat (skPath, "", new SKMetadata("V")));

    skPath = "electrical.solar." + CHARGER_NAME + ".panelCurrent";
    panel_current = new FloatSensor;
    panel_current->connect_to(new SKOutputFloat (skPath, "", new SKMetadata("A")));
    
    skPath = "electrical.solar." + CHARGER_NAME + ".chargingMode";
    charging_mode = new StringSensor;
    charging_mode->connect_to(new SKOutputString(skPath, ""));
    
    skPath = "electrical.solar." + CHARGER_NAME + ".Voltage";
    charger_voltage = new FloatSensor;
    charger_voltage->connect_to(new SKOutputFloat (skPath, "", new SKMetadata("V")));
    
    skPath = "electrical.solar." + CHARGER_NAME + ".Current";
    charger_current = new FloatSensor;
    charger_current->connect_to(new SKOutputFloat (skPath, "", new SKMetadata("A")));
    
    skPath = "electrical.solar." + CHARGER_NAME + ".loadCurrent";
    load_current = new FloatSensor;
    load_current->connect_to(new SKOutputFloat (skPath, "", new SKMetadata("A")));
    
    skPath = "electrical.solar." + CHARGER_NAME + ".temperature";
    charger_temperature = new FloatSensor;
    charger_temperature->connect_to(new SKOutputFloat (skPath, "", new SKMetadata("K")));
    
    skPath = "electrical.batteries.house.temperature";
    battery_temperature = new FloatSensor;
    battery_temperature->connect_to(new SKOutputFloat (skPath, "", new SKMetadata("K")));
    
    skPath = "electrical.solar." + CHARGER_NAME + ".output.thisDay";
    output_today = new FloatSensor;
    output_today->connect_to(new SKOutputFloat (skPath, "",
        new SKMetadata("J","Solar Output Today", "Solar charger output since midnight today", "Output Day")));
    
    skPath = "electrical.solar." + CHARGER_NAME + ".output.thisMonth";
    output_this_month = new FloatSensor;
    output_this_month->connect_to(new SKOutputFloat (skPath, "",
        new SKMetadata("J","Solar Output Month", "Solar charger output since 1st of this month", "Output Month")));
    
    skPath = "electrical.solar." + CHARGER_NAME + ".output.thisYear";
    output_this_year = new FloatSensor;
    output_this_year->connect_to(new SKOutputFloat (skPath, "",
        new SKMetadata("J","Solar Output Year", "Solar charger output since 1st January", "Output Year")));
    
    skPath = "electrical.solar." + CHARGER_NAME + ".output.total";
    output_total = new FloatSensor;
    output_total->connect_to(new SKOutputFloat (skPath, "",
        new SKMetadata("J","Solar Output Total", "Solar charger output since last reset", "Output Total")));
        
    /**
     * @brief queues a different request every second.  Repeats the sequence of requests every 15 seconds
     * 
     */
    app.onRepeat(1000, [] () {
      static uint16_t sequence = 0;
      if (sequence >=15){sequence = 0;}
      switch(sequence){
        case 0 :
          queueRequest(0x3100,4);//B1-B4
          break;
        case 1 :
          queueRequest(0x331A,3);//D26-D28
          break;
        case 2 :
          queueRequest(0x3106,2);//B7-B8
          break;
        case 3 :
          queueRequest(0x310C, 4);//B13-B16
          break;
        case 4 :
          queueRequest(0x311A, 2);//B27-B28
          break;
        case 5 :
          queueRequest(0x3110, 2);//B17-B18
          break;
        case 6 :
          queueRequest(0x311D, 1);//B30
          break;
        case 7 :
          queueRequest(0x3200, 3);//C1,C2,C7
          break;
        case 8 :
          queueRequest(0x3300, 4);//D0-D3
          break;
        case 9 :
          queueRequest(0x9013, 3);//real time clock
          break;
      }
      sequence++;
    });

     /**
     * @brief queues a request every 15 minutes
     * 
     */
    app.onRepeat(900000, [] () {
      queueRequest(0x330C, 8);//D12-D19
    });
   // Start networking, SK server connections and other SensESP internals
   sensesp_app->start();
}
/**
 * @brief Loop Function
 * 
 */
void loop() {app.tick();}