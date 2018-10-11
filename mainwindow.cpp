#include "mainwindow.h"
#include "ui_mainwindow.h"

const char rxName = 0xC1;
const char rxLen = 0xC2;
const char rxData = 0xC3;
const char txData[6] = {0x02, 0x00, 0x00, 0x00, 0x01, 0xC3};
static const int PayloadSize = 1024 * 1024; // 64 KB


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    bytesBuffer(0)
{
    ui->setupUi(this);

    InitializeUi();

    connect(m_ServerStartButton, SIGNAL(clicked()),
            this, SLOT(start()));
    connect(&m_tcpServer, SIGNAL(rxDone(int, const char*,int)),
            this, SLOT(rxDone(int, const char*,int)));

    connect(m_ClientStartButton, SIGNAL(clicked()),
            this, SLOT(startClient()));
    connect(&m_tcpClient, SIGNAL(tcpConnected()),
            this, SLOT(startTransfer()));
    connect(&m_tcpClient, SIGNAL(rxDone(const char *, int)),
            this, SLOT(rxcomplete(const char *, int)));
    connect(&(m_tcpClient.tcpClient), SIGNAL(bytesWritten(qint64)),
            this, SLOT(clientWriteEnd(qint64)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::saveDir()
{
    const QString dirName =
             QFileDialog::getExistingDirectory(
                this,
                ("Select Save Folder"),
                "D:\\");

    m_DirPath->setText(dirName);
}

void MainWindow::openFile()
{
    const QStringList fileNames =
            QFileDialog::getOpenFileNames(
                this,
                tr("Open file"),
                "D:/",
                "*.*;;");

    if (fileNames.count()) {
        m_FilePath->setText(fileNames.at(0));
    }
}

void MainWindow::startTransfer()
{
    m_tcpClient.start();
    // called when the TCP client connected to the loopback server
    m_LoadFile = new QFile(m_FilePath->text());
    m_LoadFile->open(QIODevice::ReadOnly);
    m_LoadStream = new QDataStream(m_LoadFile);

    QFileInfo info(m_FilePath->text());
    QString filename = info.fileName();

    bytesToWrite = m_LoadFile->size();

    QByteArray array;
    array.append((char)0x02);
    array.append(((filename.length()+1) >> 24)&0xFF);
    array.append(((filename.length()+1) >> 16)&0xFF);
    array.append(((filename.length()+1) >> 8)&0xFF);
    array.append(((filename.length()+1) >> 0)&0xFF);
    array.append((char)0xAB);
    array.append(filename);

    array.append((char)0x02);
    array.append((char)0x00);
    array.append((char)0x00);
    array.append((char)0x00);
    array.append((char)0x05);
    array.append((char)0xAC);
    array.append((bytesToWrite >> 24)&0xFF);
    array.append((bytesToWrite >> 16)&0xFF);
    array.append((bytesToWrite >> 8)&0xFF);
    array.append((bytesToWrite >> 0)&0xFF);

    m_tcpClient.writeData(array.data(), array.length());

    headerWritten = false;
    bytesWritten = 0;

    qDebug("%s: %d/%d", qUtf8Printable(filename), filename.length(), bytesToWrite);
    m_StatusLabel->setText(tr("Connected"));
}

void MainWindow::rxcomplete(const char *data, int length)
{
    if(bytesToWrite <= bytesWritten)
    {
        m_NetworkProgressBar->setMaximum(bytesToWrite);
        m_NetworkProgressBar->setValue(bytesWritten);
        m_StatusLabel->setText(tr("Sent %1MB")
                                  .arg(bytesWritten / (1024 * 1024)));

        QDateTime time = QDateTime::currentDateTime();
        qDebug("<%s:%d> tx: %d/%d",
               qUtf8Printable(time.toString()),
               time.time().msec(),
               bytesWritten, bytesToWrite);
        m_LoadFile->close();
        delete m_LoadFile;
        delete m_LoadStream;
        m_tcpClient.stopClient();
        m_ClientStartButton->setEnabled(true);
        return;
    }

    bytesBuffer = new char[PayloadSize + 6];
    int len = m_LoadStream->readRawData(bytesBuffer+6, PayloadSize);
    bytesBuffer[0] = static_cast<char>(0x02);
    bytesBuffer[1] = ((len+1) >> 24)&0xFF;
    bytesBuffer[2] = ((len+1) >> 16)&0xFF;
    bytesBuffer[3] = ((len+1) >> 8)&0xFF;
    bytesBuffer[4] = ((len+1) >> 0)&0xFF;
    bytesBuffer[5] = static_cast<char>(0xAD);

    // only write more if not finished and when the Qt write buffer is below a certain size.

    m_tcpClient.writeData(bytesBuffer, len + 6);

    bytesWritten += len;
    qDebug("tx: %d %d %d/%d", bytesBuffer[0], len, bytesWritten, bytesToWrite);
    m_NetworkProgressBar->setMaximum(bytesToWrite);
    m_NetworkProgressBar->setValue(bytesWritten);
    m_StatusLabel->setText(tr("Sent %1MB %2")
                           .arg(bytesWritten / (1024 * 1024))
                           .arg(QString::number(bytesBuffer[0],16)));
}

void MainWindow::clientWriteEnd(qint64 bytes)
{
    if(bytesBuffer != 0)
    {
        delete bytesBuffer;
        bytesBuffer = 0;
    }
}

void MainWindow::rxDone(int idx, const char *data, int len)
{
    if(static_cast<char>(data[0]) == static_cast<char>(0xAB))
    {
        QString fileName(QByteArray::fromRawData(data+1, len-1));

        m_SaveFile = new QFile(m_DirPath->text() + fileName);
        bool ret = m_SaveFile->open(QIODevice::WriteOnly);
        if(!ret)
        {
            qDebug("open fail: %s", qUtf8Printable(fileName));
            return;
        }
        m_SaveStream = new QDataStream(m_SaveFile);

    }
    else if(static_cast<char>(data[0]) == static_cast<char>(0xAC))
    {
        TotalBytes = qFromBigEndian<qint32>(data+1);
        bytesReceived = 0;
        m_NetworkProgressBar->setMaximum(TotalBytes);

        qDebug("rx: 0/%d", TotalBytes);
        m_tcpServer.writeData(idx, txData, 6);
    }
    else if(static_cast<char>(data[0]) == static_cast<char>(0xAD))
    {
        m_SaveStream->writeRawData(data+1, len-1);

        bytesReceived += len-1;

        m_NetworkProgressBar->setValue(bytesReceived);
        m_StatusLabel->setText(tr("Received %1MB %2")
                                   .arg(bytesReceived / (1024 * 1024))
                                   .arg(QString::number(data[len-1],16)));

        qDebug("rx: %d/%d", bytesReceived, TotalBytes);
        m_tcpServer.writeData(idx, txData, 6);

        if (bytesReceived >= TotalBytes) {
            m_SaveFile->close();
            delete m_SaveStream;
            delete m_SaveFile;

            qDebug("End");
            m_ServerStartButton->setEnabled(true);
        }
    }
    else
    {

    }

    delete data;
}

void MainWindow::startClient()
{
    bool result = m_tcpClient.startClient(m_ServerIpEdit->text(),
                            m_ServerPortEdit->text().toInt());

    if(result)
    {
        bytesWritten = 0;
        m_StatusLabel->setText(tr("Connecting"));
        m_ClientStartButton->setEnabled(false);
    }
    else
    {
        m_StatusLabel->setText(tr("Connect Fail"));
    }
}

void MainWindow::start()
{
    int port = m_ServerPortEdit->text().toInt();
    bool result = m_tcpServer.startServer(m_ServerIpEdit->text(), port);
    if(result)
    {
        m_ServerStartButton->setEnabled(false);
        m_StatusLabel->setText(tr("Listening"));
    }
    else
    {
        m_StatusLabel->setText(tr("Fail to start server"));
    }
}

void MainWindow::InitializeUi()
{
    m_MainWidget = new QWidget();
    m_SettingLayout = new QFormLayout();
    m_MainWidget->setLayout(m_SettingLayout);
    this->setCentralWidget(m_MainWidget);

    m_NetworkProgressLabel = new QLabel("Progress");
    m_NetworkProgressBar = new QProgressBar();
    m_SettingLayout->addRow(m_NetworkProgressLabel, m_NetworkProgressBar);

    m_ServerIpLabel = new QLabel("IP");
    m_ServerIpEdit = new QLineEdit("127.0.0.1");
    m_SettingLayout->addRow(m_ServerIpLabel, m_ServerIpEdit);

    m_ServerPortLabel = new QLabel("Port");
    m_ServerPortEdit = new QLineEdit("7777");
    m_SettingLayout->addRow(m_ServerPortLabel, m_ServerPortEdit);

    m_DirPathButton = new QPushButton("Save");
    connect(m_DirPathButton, SIGNAL(clicked()),
            this, SLOT(saveDir()));
    m_DirPath = new QLineEdit("D:/");
    m_DirPath->setReadOnly(true);
    m_SettingLayout->addRow(m_DirPathButton, m_DirPath);

    m_FilePathButton = new QPushButton("Open");
    connect(m_FilePathButton, SIGNAL(clicked()),
            this, SLOT(openFile()));
    m_FilePath = new QLineEdit();
    m_FilePath->setReadOnly(true);
    m_SettingLayout->addRow(m_FilePathButton, m_FilePath);

    m_ServerStartButton = new QPushButton("Server Start");
    m_SettingLayout->addRow(m_ServerStartButton);

    m_ClientStartButton = new QPushButton("Client Start");
    m_SettingLayout->addRow(m_ClientStartButton);

    m_StatusLabel = new QLabel("None");
    m_SettingLayout->addRow(m_StatusLabel);
}
