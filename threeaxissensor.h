/**
 * Front-end class for Linux three-axis sensor drivers (community).
 * Tested with mma845x and mag3110 in Linux 4.1
 **/

#ifndef THREEAXISSENSOR_H
#define THREEAXISSENSOR_H

#include <QObject>
#include <QFile>
#include <QSocketNotifier>

// path must end with '/' - it will be appended with event filename
#define DEV_INPUT_PATH          "/dev/input/"
#define SYSFS_INPUT_PATH        "/sys/class/input/"

// postfix must begin with '/' - it will be added to sysfs path
#define SYSFS_POSTFIX_ENABLE    "/device/enable"
#define SYSFS_POSTFIX_POLLRATE  "/device/poll"
#define SYSFS_POSTFIX_MIN       "/device/min"
#define SYSFS_POSTFIX_MAX       "/device/max"
#define SYSFS_POSTFIX_POSITION  "/device/position"

class ThreeAxisSensor : public QObject
{
    Q_OBJECT
public:
    explicit ThreeAxisSensor(QObject *parent = 0);
    explicit ThreeAxisSensor(QString sensorName, QObject *parent = 0);
    ~ThreeAxisSensor();

    int x() const {return m_x;}
    int y() const {return m_y;}
    int z() const {return m_z;}
    bool isActive() const {return m_isActive;}
    bool isAvailable() const {return m_isAvailable;}
    int pollInterval() const {return m_pollInterval;}
    int position() const {return m_position;}

signals:
    void isActiveChanged(bool active);
    void isAvailableChanged(bool isAvailable);
    void pollIntervalChanged(int interval);
    void positionChanged(int position);
    void readingsChanged(int x, int y, int z);

public slots:
    void setActive(bool active);
    void setPollInterval(int interval);
    void setPosition(int position);
    int setDevice(QString sensorName);

protected:
    QString m_eventName;
    QFile *m_eventFile;
    QFile *m_enableFile;
    QFile *m_pollIntervalFile;
    QFile *m_positionFile;
    QSocketNotifier *m_eventNotifier;
    bool m_isAvailable;
    bool m_isActive;
    bool m_isDeviceSet;
    int m_position;
    int m_pollInterval, m_pollMax, m_pollMin;
    int m_x, m_y, m_z;

protected slots:
    QString m_findDevice(QString sensorName);
    void m_readData();
};

#endif // THREEAXISSENSOR_H
