#include <QCoreApplication>
#include <QDebug>

#include "threeaxissensor.h"

#define ACCEL_NAME      "mma845x"
#define MAGNET_NAME     "mag3110"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ThreeAxisSensor accel(ACCEL_NAME);
    QObject::connect(&accel, &ThreeAxisSensor::readingsChanged,
                     [&](int x, int y, int z){
        qDebug()<<"accel:"<<"\t"<<
                  "x:" << x <<"\t"<<
                  "y:" << y <<"\t"<<
                  "z:" << z;
    });

    ThreeAxisSensor mag(MAGNET_NAME);
    QObject::connect(&mag, &ThreeAxisSensor::readingsChanged,
                     [&](int x, int y, int z){
        qDebug()<<"magnet:"<<"\t"<<
                  "x:" << x <<"\t"<<
                  "y:" << y <<"\t"<<
                  "z:" << z;
    });

    accel.setPollInterval(500);
    mag.setPollInterval(500);

    accel.setActive(true);
    mag.setActive(true);

    return a.exec();
}
