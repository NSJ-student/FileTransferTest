#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QThread>
#include <QList>
#include <QTcpServer>
#include <QTcpSocket>

#include "tcpserverthread.h"

class TcpServer : public QObject
{
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = NULL);
    bool startServer(QString ip, int port);
    bool stopServer();
    bool writeData(int idx, const char* data, int len);

signals:
    void rxDone(int idx, const char*,int);

public slots:
    void acceptConnection();
    void endThread();
    void rxComplete(int idx, const char* data,int len);

private:
    QList<TcpServerThread *> tcpList;

    QString tcpIP;
    int tcpPort;

    QTcpServer tcpServer;
};

#endif // TCPSERVER_H
