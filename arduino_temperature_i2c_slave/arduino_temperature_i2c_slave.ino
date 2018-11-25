// Include the libraries we need
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

#define RESET_PIN 11

int soilPower = 13;
int soilPin = A0;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

float last_temperature = 0;
int16_t raw_temperature = 0;
uint16_t soil_moisture = 0;
int loop_counter = 0;

/*
   Setup function. Here we do the basics
*/
void setup(void)
{
  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");

  int default_address = 0x08;
  pinMode(3, INPUT);
  if (digitalRead(3) == 1)
  {
    default_address = 0x10;
  }

  // Setup I2C Slave
  Serial.print("I2C Address: 0x");
  Serial.println(default_address, HEX);
  Wire.begin(default_address);                // join i2c bus with address #8
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent);

  pinMode(soilPower, OUTPUT);//Set D7 as an OUTPUT
  digitalWrite(soilPower, LOW);//Set to LOW so no power is flowing through the sensor

  digitalWrite(RESET_PIN, HIGH);
  pinMode(RESET_PIN, OUTPUT);

  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  // Assign address manually. The addresses below will beed to be changed
  // to valid device addresses on your bus. Device address can be retrieved
  // by using either oneWire.search(deviceAddress) or individually via
  // sensors.getAddress(deviceAddress, index)
  // Note that you will need to use your specific address here
  //insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };

  // Method 1:
  // Search for devices on the bus and assign based on an index. Ideally,
  // you would do this to initially discover addresses on the bus and then
  // use those addresses and manually assign them (see above) once you know
  // the devices on your bus (and assuming they don't change).
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");

  // method 2: search()
  // search() looks for the next device. Returns 1 if a new address has been
  // returned. A zero might mean that the bus is shorted, there are no devices,
  // or you have already retrieved all of them. It might be a good idea to
  // check the CRC to make sure you didn't get garbage. The order is
  // deterministic. You will always get the same devices in the same order
  //
  // Must be called before search()
  //oneWire.reset_search();
  // assigns the first address found to insideThermometer
  //if (!oneWire.search(insideThermometer)) Serial.println("Unable to find address for insideThermometer");

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 9);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC);
  Serial.println();
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  // method 1 - slower
  //Serial.print("Temp C: ");
  //Serial.print(sensors.getTempC(deviceAddress));
  //Serial.print(" Temp F: ");
  //Serial.print(sensors.getTempF(deviceAddress)); // Makes a second call to getTempC and then converts to Fahrenheit

  // method 2 - faster
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
}
bool reset_now = false;
/*
   Main function. It will request the tempC from the sensors and display on Serial.
*/
void loop(void)
{
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");

  // It responds almost immediately. Let's print out the data
  last_temperature = sensors.getTempC(insideThermometer);
  Serial.print("Temp C: ");
  Serial.println(last_temperature);
  // Convert last temperature into an integer
  raw_temperature = last_temperature * 10;
  if (++loop_counter % 15 == 0)
  {
    soil_moisture = readSoil();
    Serial.print("Soil moisture: ");
    Serial.println(soil_moisture);
    loop_counter = 0;
  }
  else
  {
    Serial.print("Not reading soil moisture, loop counter == ");
    Serial.println(loop_counter);
  }

  delay(1000);
  if (reset_now)
  {
    delay(2000);
    digitalWrite(RESET_PIN, LOW);
  }
  Serial.println("a");
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

//This is a function used to get the soil moisture content
uint16_t readSoil()
{
  digitalWrite(soilPower, HIGH);//turn D7 "On"
  delay(10);//wait 10 milliseconds
  uint16_t val = analogRead(soilPin);//Read the SIG value form sensor
  digitalWrite(soilPower, LOW);//turn D7 "Off"
  return val;//send current moisture value
}

int cmd_id = 0;
bool response_valid = false;

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
  cmd_id = Wire.read();
  Serial.println("Request Event: " + String(cmd_id));
  //Wire.write((byte *)&raw_temperature, sizeof(raw_temperature));

  //if (response_valid)
  {
    switch (cmd_id)
    {
      case 0x55:
        // Test case
        Wire.write(cmd_id);
        break;
      case 0x01:
        // Read One Wire
        Wire.write((byte *)&raw_temperature, sizeof(raw_temperature));
        //reset_now = true;
        break;
      case 0x02:
        // Read Soil Moisture Sensor
        Wire.write((byte *)&soil_moisture, sizeof(soil_moisture));
        //reset_now = true;
        break;
      default:
        // Error
        break;
    }
  }
  //response_valid = false;

}

void receiveEvent(int num_bytes)
{
  Serial.println("Receive Event - " + String(num_bytes));
  /*
    // We really only expect 1
    if (Wire.available() >= 1)
    {
    cmd_id = Wire.read();
    response_valid = true;
    Serial.println("Receieved Command id: " + cmd_id);
    }
    else
    {
    Serial.println("No Command ID Received");
    }
  */
}
