#include <QCoreApplication>
#include <QDebug>

#include "qfouriertransformer.h"
#include "server.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // All we do here is create a single subclassed QWebSocketServer

    qDebug() << "Creating Server ...";
    Server *server = new Server(&a);

    return a.exec();
}
