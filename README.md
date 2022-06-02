# SensESP-EpeverSolarGateway
A SenseESP sensor that uses ModbusRTU client to read data from an Epever Solar Charge Controller and passes it on to a SignalK Server.

This code is working for me but is not a finished and well documented project.  Feel free to help me improve it.

It is run on an ESP32 DEVK1tc_V4 with an RS485 to TTL converter on GPIO pins 16 and 17.
The Epever solar charge controller has an RJ-45 connector for the RS485 signal.  Signals A and B are on pins 3,4 and 5,6.  Refer to the PDF or the image. There is also a power supply and ground. On my charger the power supply is 5VDC (Model Tracer 2206AN), however the reference document I have says it is 7.5VDC. So you should check the voltage before connecting anything.

The code reads the controllers real time clock but doesn't post it to signalK. It is only displayed on the serial output.  The RTC is used to roll over the daily, monthly and annual production statistics.

This code has been updated to run on the Heltec ESP32 Wifi Kit with an R485 to TTL serial converter connected to the RX/TX pins #defined in the code.

There is a problem that the Epever 5V supply does not seem capable of supplying enough current to run the ESP32.  This needs about 200mA when the wifi transmits.
