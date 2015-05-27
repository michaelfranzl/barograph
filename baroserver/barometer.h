#ifndef BAROMETER_H
#define BAROMETER_H

#include <QObject>

#include <QtSerialPort/QtSerialPort>

class Barometer : public QObject
{
    Q_OBJECT
public:
    explicit Barometer(QObject *parent = 0);

Q_SIGNALS:
    void pressureAvailable(float);
    void temperatureAvailable(float);

private Q_SLOTS:
    void onRead();

private:
    QSerialPort *m_serial;
    QString m_buf;

};

#endif // BAROMETER_H
