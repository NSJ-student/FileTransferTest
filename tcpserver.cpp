#include "tcpserver.h"

TcpServer::TcpServer(QObject *parent) : QObject(parent)
{
    tcpIP   = "127.0.0.1";
    tcpPort = 7777;

    connect(&tcpServer, SIGNAL(newConnection()),
            this, SLOT(acceptConnection()));;
}

bool TcpServer::startServer(QString ip, int port)
{
    if(tcpServer.isListening())
        return false;

    tcpIP = ip;
    tcpPort = port;
    if(!tcpServer.listen(QHostAddress(tcpIP), tcpPort))
        return false;

    return true;
}

bool TcpServer::stopServer()
{
    if(!tcpServer.isListening())
        return false;

    for(int cnt=0; cnt < tcpList.count(); cnt++)
    {
        tcpList.at(cnt)->closeSocket();
    }
    tcpServer.close();
    return true;
}

bool TcpServer::writeData(int idx, const char *data, int len)
{
    if(tcpList.count()-1 < idx)
    {
        return false;
    }
    TcpServerThread * conn = tcpList.at(idx);

    return conn->writeData(data, len);
}

void TcpServer::acceptConnection()
{
    QTcpSocket * socket = tcpServer.nextPendingConnection();
    TcpServerThread * conn = new TcpServerThread(socket);
    tcpList.append(conn);
    conn->setListIndex(tcpList.count() - 1);

    connect(conn, SIGNAL(finished()),
            this, SLOT(endThread()));
    connect(conn, SIGNAL(rxDone(int, const char*,int)),
            this, SLOT(rxComplete(int, const char*,int)));
    conn->start();
}

void TcpServer::endThread()
{
    TcpServerThread * conn = qobject_cast<TcpServerThread *>(QObject::sender());
    conn->wait(100);
    int idx = conn->getListIndex();
    if(conn->isRunning())
    {
        qDebug("Still Running: %d", idx);
    }

    tcpList.removeAt(idx);
    delete conn;
}

void TcpServer::rxComplete(int idx, const char *data, int len)
{
    emit rxDone(idx, data, len);
}

