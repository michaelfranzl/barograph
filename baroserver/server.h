#ifndef SERVER_H
#define SERVER_H

#include <QtWebSockets/QtWebSockets>

#include "barometer.h"
#include "qfouriertransformer.h"

//#define ADD_TESTSIGNAL

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QCoreApplication *parent = 0);
    ~Server();

Q_SIGNALS:
    void closed();

private Q_SLOTS:
    void onNewConnection();
    void socketDisconnected();
    void writeAll(QString str);
    void pressureAvailable(float p);
    void temperatureAvailable(float t);
    void onTextMessageReceived(QString str);

private:
    QWebSocketServer *m_server;
    QList<QWebSocket *> m_clients;
    Barometer *m_barometer;
    float m_samples[17000];
    float m_fft[17000];
    int m_count;
    int m_fftsize;
    int m_samplerate;
    QFourierTransformer m_transformer;
    qint64 m_last_fft;

    void doFFT();
    void setFftSize(int size);

};

#endif // SERVER_H
