#ifndef TCPSERVERTHREAD_H
#define TCPSERVERTHREAD_H

#include <QObject>
#include <QtEndian>
#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDateTime>

#define START_BYTE      0x02
#define FILE_RX_NAME    0xAB
#define FILE_RX_LEN     0xAC
#define FILE_RX_DATA    0xAD

class TcpServerThread : public QThread
{
    Q_OBJECT
public:
    explicit TcpServerThread(QTcpSocket *conn);
    void closeSocket();
    bool writeData(const char * data, int length);
    void setListIndex(int idx)
    {
        listIndex = idx;
    }
    int getListIndex()
    {
        return listIndex;
    }
    QString getClientIp()
    {
        if(tcpServerConnection->state() == QAbstractSocket::ConnectedState)
            return tcpServerConnection->peerAddress().toString();
        else
            return "";
    }
    int getClientPort()
    {
        if(tcpServerConnection->state() == QAbstractSocket::ConnectedState)
            return tcpServerConnection->peerPort();
        else
            return 0;
    }
    QAbstractSocket::SocketState getState()
    {
        return tcpServerConnection->state();
    }
    enum ProtocolState{
        PROT_START,
        PROT_LEN,
        PROT_DATA
    };

signals:
    void rxDone(int idx, const char * data, int length);

public slots:
    void displayError(QAbstractSocket::SocketError err);
    void tcpStateChanged(QAbstractSocket::SocketState state);
    void tcpDisconnected();

private:
    QTcpSocket *tcpServerConnection;
    ProtocolState protState;
    char rxStart[2];
    char rxLen[4];
    quint32 rxBytes;

    int listIndex;

    void run();
};

#endif // TCPSERVERTHREAD_H
