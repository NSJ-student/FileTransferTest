#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFileDialog>
#include <QMessageBox>
#include "tcpserver.h"
#include "tcpclient.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void saveDir();
    void start();
    void rxDone(int idx, const char * data,int len);

    void startClient();
    void openFile();
    void rxcomplete(const char *data, int length);
    void startTransfer();

private:
    void InitializeUi();

    TcpServer m_tcpServer;
    QFile * m_SaveFile;
    QDataStream * m_SaveStream;
    quint32 bytesReceived;
    quint32 TotalBytes;

    TcpClient m_tcpClient;
    QFile * m_LoadFile;
    QDataStream * m_LoadStream;
    int bytesToWrite;
    int bytesWritten;
    bool headerWritten;

    Ui::MainWindow *ui;

    QWidget * m_MainWidget;
    QFormLayout * m_SettingLayout;
        QLabel * m_NetworkProgressLabel;
        QProgressBar * m_NetworkProgressBar;

        QLabel * m_ServerIpLabel;
        QLineEdit * m_ServerIpEdit;

        QLabel * m_ServerPortLabel;
        QLineEdit * m_ServerPortEdit;

        QPushButton * m_DirPathButton;
        QLineEdit * m_DirPath;

        QPushButton * m_FilePathButton;
        QLineEdit * m_FilePath;

        QPushButton * m_ServerStartButton;
        QPushButton * m_ClientStartButton;
        QLabel * m_StatusLabel;
};

#endif // MAINWINDOW_H
