# ThreeAxisSensor
Qt class for reading 3-axis sensor data in (embedded) Linux

There are QObject-derived class and test app.

Tested on Linux 4.1 with standard drivers: 
- accelerometer mma8451 
- magnetometer mag3110 

Example:

ThreeAxisSensor accel("mma845x");
QObject::connect(&accel, &ThreeAxisSensor::readingsChanged,
                 [](int x, int y, int z){
    qDebug()<<"x:" << x << "y:" << y << "z:" << z;
});

accel.setPollInterval(500);
accel.setActive(true);


Class operates with driver's sysfs interface. Proper fiels finding
automatically by given sensor name.

Note: class reads raw sensor data. To interpret data see to
sensors' datasheet/appnote.
