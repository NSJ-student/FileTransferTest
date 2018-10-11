#include "tcpclient.h"

TcpClient::TcpClient(QObject *parent) : QThread(parent)
{
    connect(&tcpClient, SIGNAL(connected()), this, SLOT(startTransfer()));
}

bool TcpClient::startClient(QString ip, int port)
{
    if(tcpClient.state() != QAbstractSocket::ConnectedState)
    {
        tcpServerIP = ip;
        tcpServerPort = port;

        QHostAddress address(tcpServerIP);
        tcpClient.connectToHost(address, tcpServerPort);

        return true;
    }
    else
    {
        return false;
    }
}

bool TcpClient::stopClient()
{
    tcpClient.close();
    return true;
}

bool TcpClient::writeData(const char *data, int length)
{
    if(tcpClient.state() == QAbstractSocket::ConnectedState)
    {
        tcpClient.write(data, length);
        return true;
    }
    else
    {
        return false;
    }
}

void TcpClient::startTransfer()
{
    emit tcpConnected();
}

void TcpClient::displayError(QAbstractSocket::SocketError err)
{
    QDateTime time = QDateTime::currentDateTime();
    QString fileName =
            QString::number(time.date().year()) + "-"
            + QString::number(time.date().month()) + "-"
            + QString::number(time.date().day()) + "_"
            + QString::number(time.time().hour()) + "_"
            + QString::number(time.time().minute()) + "_"
            + QString::number(time.time().second()) + "_"
            + QString::number(time.time().msec());
    qDebug() << fileName
             << ": "
             << tcpClient.errorString();
}

void TcpClient::tcpStateChanged(QAbstractSocket::SocketState state)
{
    switch(state)
    {
    case QAbstractSocket::UnconnectedState:
        qDebug("UnconnectedState");
    break;
    case QAbstractSocket::HostLookupState:
        qDebug("HostLookupState");
    break;
    case QAbstractSocket::ConnectingState:
        qDebug("ConnectingState");
    break;
    case QAbstractSocket::ConnectedState:
        qDebug("ConnectedState");
    break;
    case QAbstractSocket::BoundState:
        qDebug("BoundState");
    break;
    case QAbstractSocket::ClosingState:
        qDebug("ClosingState");
    break;
    case QAbstractSocket::ListeningState:
        qDebug("ListeningState");
    break;
    }
}

void TcpClient::tcpDisconnected()
{
    if(tcpClient.isOpen())
    {
        tcpClient.close();
    }
}

void TcpClient::run()
{
    protState = PROT_START;

    while(1)
    {
        if(tcpClient.state() != QAbstractSocket::ConnectedState)
        {
            qDebug("Client Disconnected");
            break;
        }

        qint64 readByte;
        qint64 bytes = tcpClient.bytesAvailable();
        while(bytes > 0)
        {
            switch(protState)
            {
            case PROT_START:
                readByte = tcpClient.read(rxStart, 1);
                if(static_cast<char>(rxStart[0]) == static_cast<char>(0x02))
                {
                    protState = PROT_LEN;
                    rxBytes = 0;
                }
                else
                {
                    qDebug("start: %d", rxStart[0]);
                }
            break;

            case PROT_LEN:
                if(bytes >= 4)
                {
                    readByte = tcpClient.read(rxLen, 4);

                    rxBytes = qFromBigEndian<qint32>(rxLen);
                    qDebug("len: %d", rxBytes);
                    protState = PROT_DATA;
                }
            break;

            case PROT_DATA:
                if(bytes >= rxBytes)
                {
                    char * data = new char[rxBytes];
                    readByte = tcpClient.read(data, rxBytes);
                    emit rxDone(data, readByte);
                    protState = PROT_START;
                }
            break;
            }

            bytes = tcpClient.bytesAvailable();
        }
        msleep(30);
    }

    tcpClient.close();
    qDebug("Thread End");
}
