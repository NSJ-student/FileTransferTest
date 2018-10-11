#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QtEndian>
#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDateTime>

class TcpClient : public QThread
{
    Q_OBJECT
public:
    explicit TcpClient(QObject *parent = nullptr);
    bool startClient(QString ip, int port);
    bool stopClient();
    bool writeData(const char * data, int length);
    QString getClientIp()
    {
        if(tcpClient.state() == QAbstractSocket::ConnectedState)
            return tcpClient.localAddress().toString();
        else
            return "";
    }
    int getClientPort()
    {
        if(tcpClient.state() == QAbstractSocket::ConnectedState)
            return tcpClient.localPort();
        else
            return 0;
    }
    QAbstractSocket::SocketState getState()
    {
        return tcpClient.state();
    }

    enum ProtocolState{
        PROT_START,
        PROT_LEN,
        PROT_DATA
    };
    QTcpSocket tcpClient;

signals:
    void tcpConnected();
    void rxDone(const char * data, int length);

public slots:
    void startTransfer();
    void displayError(QAbstractSocket::SocketError err);
    void tcpStateChanged(QAbstractSocket::SocketState state);
    void tcpDisconnected();

private:
    QString tcpServerIP;
    int tcpServerPort;

    ProtocolState protState;
    char rxStart[2];
    char rxLen[4];
    quint32 rxBytes;

    void run();
};

#endif // TCPCLIENT_H
