#include "threeaxissensor.h"
#include <linux/input.h>
#include <QDebug>
#include <QDir>

ThreeAxisSensor::ThreeAxisSensor(QObject *parent) : QObject(parent)
{
    m_x = m_y = m_z = 0;
    m_isAvailable = false;
    m_isActive = false;
    m_isDeviceSet = false;

    m_eventFile = new QFile();
    m_enableFile = new QFile();
    m_pollIntervalFile = new QFile();
    m_positionFile = new QFile();
}

/**
 * @brief ThreeAxisSensor::ThreeAxisSensor
 * @param sensorName - name of sensor driver, for example "mag3110"
 * @param parent
 */
ThreeAxisSensor::ThreeAxisSensor(QString sensorName, QObject *parent) : ThreeAxisSensor(parent)
{
    setDevice(sensorName);
}

ThreeAxisSensor::~ThreeAxisSensor()
{
    setActive(false);
    delete m_eventFile;
    delete m_enableFile;
    delete m_pollIntervalFile;
    delete m_positionFile;
}

/**
 * @brief ThreeAxisSensor::setActive
 *
 * Write new value to 'enable' sysfs file. If active == true,
 * open event file and create QSocketNotifier with it.
 * If active == false - close event file and delete notifier.
 *
 * @param active
 */
void ThreeAxisSensor::setActive(bool active)
{
    if (active == m_isActive)
        return;

    if (!m_isDeviceSet)
    {
        qWarning()<<"setActive Failed! Device is not set!";
        return;
    }

    int count;

    m_enableFile->open(QIODevice::WriteOnly);
    count = m_enableFile->write(QByteArray::number(active));
    m_enableFile->close();

    if (count < 1)
    {
        qWarning()<<"Failed to write to file" << m_enableFile->fileName();
        return;
    }

    if (active == true)
    {
       m_eventFile->open(QIODevice::ReadOnly|QIODevice::Unbuffered);
       if (!m_eventFile->isOpen())
       {
           qWarning() << "Failed no open file " << m_eventFile->fileName();
           return;
       }
       m_eventNotifier = new QSocketNotifier(m_eventFile->handle(),
                                              QSocketNotifier::Read, this);
       connect(m_eventNotifier, &QSocketNotifier::activated,
                this, &ThreeAxisSensor::m_readData);
    } else
    {
        m_eventFile->close();
        delete m_eventNotifier;
    }

    m_isActive = active;
    emit isActiveChanged(m_isActive);
}

void ThreeAxisSensor::setPollInterval(int interval)
{
    if (m_pollInterval == interval)
        return;

    if (!m_isDeviceSet)
    {
        qWarning()<<"setPollInterval failed! Device is not set!";
        return;
    }

    if (interval < m_pollMin || interval > m_pollMax)
    {
        qWarning()<<"Poll interval" << interval << "is out of range";
        return;
    }

    int count;
    m_pollIntervalFile->open(QIODevice::WriteOnly);
    count = m_pollIntervalFile->write(QByteArray::number(interval));
    m_pollIntervalFile->close();

    if (count <= 0)
    {
        qWarning()<<"Failed to write to file" << m_pollIntervalFile->fileName();
        return;
    }

    m_pollInterval = interval;
    emit pollIntervalChanged(m_pollInterval);
}


/**
 * @brief ThreeAxisSensor::setPosition
 * @param position - in range from 0 to 7
 *
 * code of sensor orientation, for more details see sensor's appnote
 */
void ThreeAxisSensor::setPosition(int position)
{
    if (position == m_position)
        return;

    if (!m_isDeviceSet)
    {
        qWarning()<<"setPollInterval failed! Device is not set!";
        return;
    }

    int count;

    m_positionFile->open(QIODevice::WriteOnly);
    count = m_positionFile->write(QByteArray::number(position));
    m_positionFile->close();

    if (count <= 0)
    {
        qWarning()<<"Failed to write to file"<<m_positionFile->fileName();
        return;
    }

    m_position = position;
    emit positionChanged(m_position);
}

/**
 * @brief ThreeAxisSensor::setDevice
 * @param sensorName
 * @return -1 if error occured, 0 otherwise
 *
 * Set new sensor device and init (or reinit) instances of sysfs files.
 * If sensor was activated - restarts it.
 * If failed to find device - set isAvailable flag to false (or to true otherwise )
 */
int ThreeAxisSensor::setDevice(QString sensorName)
{
    m_eventName = m_findDevice(sensorName);
    if (!m_eventName.length())
    {
        qWarning()<<"Failed to find sensor" << sensorName;
        if (m_isAvailable)
        {
            m_isAvailable = false;
            emit isAvailableChanged(m_isAvailable);
        }
        return -1;
    }

    bool oldActive = m_isActive;
    if (m_isActive)
        setActive(false);

    // reinit sysfs files
    m_eventFile->setFileName(DEV_INPUT_PATH+m_eventName);
    m_enableFile->setFileName(SYSFS_INPUT_PATH+m_eventName+SYSFS_POSTFIX_ENABLE);
    m_pollIntervalFile->setFileName(SYSFS_INPUT_PATH+m_eventName+SYSFS_POSTFIX_POLLRATE);
    m_positionFile->setFileName(SYSFS_INPUT_PATH+m_eventName+SYSFS_POSTFIX_POSITION);

    // get default poll parameters from sensor driver
    m_pollIntervalFile->open(QIODevice::ReadOnly);
    m_pollInterval = m_pollIntervalFile->readAll().simplified().toInt();
    m_pollIntervalFile->close();

    QFile pollMin(SYSFS_INPUT_PATH+m_eventName+SYSFS_POSTFIX_MIN);
    pollMin.open(QIODevice::ReadOnly);
    m_pollMin = pollMin.readAll().simplified().toInt();
    pollMin.close();

    QFile pollMax(SYSFS_INPUT_PATH+m_eventName+SYSFS_POSTFIX_MAX);
    pollMax.open(QIODevice::ReadOnly);
    m_pollMax = pollMax.readAll().simplified().toInt();
    pollMax.close();

    // get default position from driver
    m_positionFile->open(QIODevice::ReadOnly);
    m_position = m_positionFile->readAll().simplified().toInt();
    m_positionFile->close();

    // start again if was started before
    if (oldActive)
        setActive(true);

    m_isDeviceSet = true;
    if (!m_isAvailable)
    {
        m_isAvailable = true;
        emit isAvailableChanged(m_isAvailable);
    }
    return 0;
}

/**
 * @brief ThreeAxisSensor::m_findDevice
 * @param sensorName
 * @return name of event device
 *
 * Looking for event device for given sensor in DEV_INPUT_PATH directory
 */
QString ThreeAxisSensor::m_findDevice(QString sensorName)
{
    if (!sensorName.length())
        return "";
    QDir devInputDir(DEV_INPUT_PATH);
    QFile file;
    char evDevName[64];
    QStringList fileList = devInputDir.entryList(QDir::Files|QDir::System);
    QString eventName = "";

    foreach (QString fileName, fileList) {
        file.setFileName(devInputDir.absoluteFilePath(fileName));
        file.open(QIODevice::ReadOnly);
        if (ioctl(file.handle(), EVIOCGNAME(sizeof evDevName), evDevName) >= 0) {
            if (sensorName.compare(evDevName, Qt::CaseInsensitive) == 0)
            {
                qDebug()<<"Sensor"<<sensorName<<"found!";
                eventName = fileName;
                break;
            }
        }
        file.close();
    }

    return eventName;
}

/**
 * @brief ThreeAxisSensor::m_readData
 * Read data from event device. If event.type == 0 (sync event) then all
 * changed axis recived - signal can be emited.
 */
void ThreeAxisSensor::m_readData()
{
    struct input_event event;
    int count;
    int size = sizeof(struct input_event);

    count = m_eventFile->read((char*)&event, size);
    if (count < size)
    {
        qWarning()<<"Failed to read data from"<<m_eventFile->fileName();
        return;
    }

    if (event.type == 0)
    {
        emit readingsChanged(m_x, m_y, m_z);
        return;
    }

    switch (event.code) {
        case 0:
            m_x = event.value;
            break;
        case 1:
            m_y = event.value;
            break;
        case 2:
            m_z = event.value;
            break;
    }
}
