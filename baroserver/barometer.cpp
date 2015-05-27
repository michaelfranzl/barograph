#include "barometer.h"
#include <QDebug>

Barometer::Barometer(QObject *parent) :
    QObject(parent)
{
    // Qt makes it easy to get all available serial devices
    QList<QSerialPortInfo> sp_list = QSerialPortInfo::availablePorts();

    // We select the first connected device.
    // If you use an Arduino UNO, this will be /dev/ttyACM0
    QSerialPortInfo serport = sp_list.first();

    qDebug() << "Barometer: Connecting to" << serport.systemLocation();

    m_serial = new QSerialPort(serport);
    m_serial->setBaudRate(QSerialPort::Baud115200);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->open(QIODevice::ReadOnly);

    connect(m_serial, &QSerialPort::readyRead, this, &Barometer::onRead);

    // We will not always receive a full line from the Arduino in one go,
    // so we have to re-assemble lines with the help of this buffer.
    m_buf = QString();
}

void Barometer::onRead() {   
    // Read up to 100 bytes at once
    QByteArray data = m_serial->read(100);
    QString str = QString(data);
    str = str.trimmed();

    if (str.endsWith(";")) {
        // We have received a full line, or the rest of a line.
        // Concatenate with whatever is in m_buf.
        m_buf += str;

        if (m_buf.startsWith("p")) {
            // Arduino program has sent a pressure value in the format "p:\d+;"
            // Arduino program doesn't send floats, but multiplies a float pressure
            // by 10000 to send an integer.
            QString pressure = m_buf.mid(1, m_buf.size() - 2); // Strip letter and semicolon
            float p = pressure.toFloat() / 10000.0; // convert back into a float value
            emit pressureAvailable(p); // whoever is interested, handle this pressure

        } else {
            // Arduino program has sent a temperature value.
            // Arduino program doesn't send floats, but multiplies a float temperature
            // by 1000 to send an integer.
            QString temperature = m_buf.mid(1, m_buf.size() - 2); // Strip letter and semicolon
            float t = temperature.toFloat() / 1000.0; // convert back into a float value
            emit temperatureAvailable(t); // whoever is interested, handle this temperature
        }
        m_buf.clear();

    } else {
        // We have not received a full line. We have to concatenate later,
        // when the rest is sent.
        m_buf = str;
    }
}
