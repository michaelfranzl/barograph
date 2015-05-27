#include "server.h"
#include "qmath.h"

Server::Server(QCoreApplication *parent) :
    QObject(parent),
    m_server(new QWebSocketServer(QStringLiteral("baroserver"), QWebSocketServer::NonSecureMode, this)),
    m_clients()
{
    if (m_server->listen(QHostAddress::Any, 7600)) {
        qDebug() << "Server::Server(): WebSocket listening on port 7600. You can connect to http://localhost:7600";
        connect(m_server, &QWebSocketServer::newConnection, this, &Server::onNewConnection);
        connect(m_server, &QWebSocketServer::closed, this, &Server::closed);
    } else {
        qDebug() << "Server::Server(): Cannot listen on port 7600. Is this port already taken?";
    }

    // We only use one Barometer in this example
    m_barometer = new Barometer();

    // The number of samples taken in one FFT round
    m_count = 0;

    // the current sample rate (samples per second)
    m_samplerate = 0;

    // 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192 and 16384
    m_fftsize = 128;

    // Configure FFTReal
    m_transformer.setSize(m_fftsize);
    m_transformer.setWindowFunction("Hann");

    // A timestamp
    m_last_fft = QDateTime::currentMSecsSinceEpoch();

    connect(m_barometer, &Barometer::pressureAvailable, this, &Server::pressureAvailable);
    connect(m_barometer, &Barometer::temperatureAvailable, this, &Server::temperatureAvailable);
}

Server::~Server() {
    qDebug() << "Server::~Server(): Cleaning up.";
    m_server->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}

void Server::onTextMessageReceived(QString str) {
    QWebSocket *client = qobject_cast<QWebSocket *>(sender());
    QStringList parts = str.split(":");
    QString cmd = parts.first();
    QString data = parts.last();
    qDebug() << "Server::onTextMessageReceived():" << str;
    if (cmd == "fftsize") {
        this->setFftSize(data.toInt());
    }
}

void Server::setFftSize(int size) {
    qDebug() << "Server::setFftSize():" << size;
    m_fftsize = size;
    m_transformer.setSize(m_fftsize);
    m_count = 0;
    m_last_fft = QDateTime::currentMSecsSinceEpoch();
}

void Server::onNewConnection() {
    QWebSocket *socket = m_server->nextPendingConnection();
    qDebug() << "Server::onNewConnection(): New client connected.";
    connect(socket, &QWebSocket::disconnected, this, &Server::socketDisconnected);
    connect(socket, &QWebSocket::textMessageReceived, this, &Server::onTextMessageReceived);
    m_clients << socket;
}

void Server::socketDisconnected() {
    QWebSocket *client = qobject_cast<QWebSocket *>(sender());
    qDebug() << "Server::socketDisconnected(): Client disconnected.";
    if (client) {
        m_clients.removeAll(client);
        client->deleteLater();
    }
}

void Server::temperatureAvailable(float t) {
    this->writeAll("t:" + QString::number(t));
}

void Server::pressureAvailable(float p) {
    //qDebug() << m_count << QString::number(p, 'g', 8);

    if (p == 0.0) {
        // When driving the BOSCH BMP 180 sensor at high speeds, I sometimes get single zero pressure values.
        // I haven't investigated the reason, and will simply skip such values.
        return;
    }

    // current timestamp
    qint64 msecs = QDateTime::currentMSecsSinceEpoch();

#ifdef ADD_TESTSIGNAL
    /* Before we do DFT, we add a small test signal of a defined frequency and amplitude,
     * so that we can calibrate our spectrum plot at a later point, and see that the DFT
     * indeed works as expected.
     */
    if (m_samplerate > 0) {
        // Do not do this the first time. We have to wait until we have calculated the first
        // accurate sample rate.

        // Specify test signal attributes here
        float frequency = 75.0; // Hz
        float amplitude = 0.02; // mbar

        float cycles = frequency * m_fftsize / m_samplerate;
        float radians = cycles * 2.0 * M_PI * (float)m_count / (float)m_fftsize;

        // add the test signal to the real signal
        p = p + amplitude * qSin(radians);
    }
#endif

    // Add the signal into the m_samples buffer, to be consumed later for DFT.
    m_samples[m_count] = p;
    if (m_count % 10 == 0) {
        this->writeAll("p:" + QString::number(p, 'g', 8));
    }
    m_count += 1;

    if (m_count == m_fftsize) {
        // We have enough data in the m_samples buffer. It is time to do a DFT.

        qint64 diff = msecs - m_last_fft; // Time in ms it took to read m_fftsize samples
        m_samplerate = 1000.0 * (float)m_fftsize / (float)diff; // Calculate samples per second
        qDebug() << "Read" << m_fftsize << "samples in" << diff << "ms. Samples/sec=" << m_samplerate;

        this->doFFT();

        m_last_fft = msecs;
        m_count = 0;
    }
}

void Server::doFFT() {
    // do the actual DFT
    m_transformer.forwardTransform(m_samples, m_fft);

    // compose CSV string to be sent to connected Clients
    QString csv = "fft:";
    for(int i = 0; i < m_fftsize; i++) {
        csv += QString::number(m_fft[i]);
        csv += ";";
    }
    // send this CSV string to all connected WebSocket clients.
    this->writeAll(csv);
}

void Server::writeAll(QString str) {
    // send a test message str to all connected clients
    for(int i = 0; i < m_clients.length(); i++) {
        m_clients.at(i)->sendTextMessage(str);
    }
}
