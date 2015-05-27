#include <SFE_BMP180.h>
#include <Wire.h>

SFE_BMP180 bmp180;

void setup() {
  Serial.begin(115200);
  Serial.println("REBOOT");

  // Initialize the sensor (it is important to get calibration values stored on the device).

  if (bmp180.begin())
    Serial.println("BMP180 init success v1");
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.

    Serial.println("BMP180 init fail\n\n");
    while(1); // Pause forever.
  }
}

unsigned long loopcount = 0;
double temp = 20.0;
char buf[20];

void loop()
{
  char status;
  double pressure;
  double p0,a;

  // Start a temperature measurement:
  // If request is successful, the number of ms to wait is returned.
  // If request is unsuccessful, 0 is returned.
  if (loopcount == 0) {
    status = bmp180.startTemperature();
    if (status != 0) {
      delay(status);
      status = bmp180.getTemperature(temp);
      if (status == 0) {
        Serial.print("error getting temperature measurement");
        temp = -10; // this is an error
      } else {
        sprintf(buf, "t%lu;", (unsigned long)(1000 * temp));
        Serial.write(buf);
      }
    } else {
      Serial.print("error starting temperature measurement");
    }
  }
  
  loopcount += 1;
  if (loopcount == 10000000) {
    // read temperature only each 10 Million samples.
    // That's accurate enough for lab conditions.
    loopcount = 0;
  }

  // read the pressure. Parameter is 0, 1, 2, or 3 for oversampling, see Bosch BMP180 datasheet.
  status = bmp180.startPressure(3);
  if (status != 0) {
    delay(status);
    status = bmp180.getPressure(pressure,temp);
    if (status != 0) {
      // We will submit intergers rather than floats because
      // sprintf doesn't support floats without using tricks.
      // It is easy to divide by 10000 on the receivers side
      // to get back floats.
      pressure = 10000 * pressure;
      sprintf(buf, "p%lu;", (unsigned long)pressure);
      Serial.write(buf);
    } else {
      Serial.println("error retrieving pressure measurement\n");
    }
  } else {
    Serial.println("error starting pressure measurement\n");
  }
}
