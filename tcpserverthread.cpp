#include "tcpserverthread.h"

TcpServerThread::TcpServerThread(QTcpSocket *conn)
{
    tcpServerConnection = conn;

    connect(tcpServerConnection, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
    connect(tcpServerConnection, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(tcpStateChanged(QAbstractSocket::SocketState)));
    connect(tcpServerConnection, SIGNAL(disconnected()),
            this, SLOT(tcpDisconnected()));

}

void TcpServerThread::closeSocket()
{
    tcpServerConnection->close();

    wait(100);
}

bool TcpServerThread::writeData(const char *data, int length)
{
    if(tcpServerConnection->state() == QAbstractSocket::ConnectedState)
    {
        tcpServerConnection->write(data, length);
        return true;
    }
    else
    {
        return false;
    }
}

void TcpServerThread::displayError(QAbstractSocket::SocketError err)
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
             << tcpServerConnection->errorString();
}

void TcpServerThread::tcpStateChanged(QAbstractSocket::SocketState state)
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

void TcpServerThread::tcpDisconnected()
{
    tcpServerConnection->close();
}

void TcpServerThread::run()
{
    protState = PROT_START;

    while(1)
    {
        if(tcpServerConnection->state() != QAbstractSocket::ConnectedState)
        {
            qDebug("Client Disconnected");
            break;
        }
        qint64 readByte;
        qint64 bytes = tcpServerConnection->bytesAvailable();
        while(bytes > 0)
        {
            switch(protState)
            {
            case PROT_START:
                readByte = tcpServerConnection->read(rxStart, 1);
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
                    readByte = tcpServerConnection->read(rxLen, 4);

                    rxBytes = qFromBigEndian<qint32>(rxLen);
                    qDebug("len: %d", rxBytes);
                    protState = PROT_DATA;
                }
            break;

            case PROT_DATA:
                if(bytes >= rxBytes)
                {
                    char * data = new char[rxBytes];
                    readByte = tcpServerConnection->read(data, rxBytes);
                    emit rxDone(listIndex, data, readByte);
                    protState = PROT_START;
                }
            break;
            }
            bytes = tcpServerConnection->bytesAvailable();
        }
        msleep(10);
    }

    tcpServerConnection->close();
    qDebug("Thread End");
}


