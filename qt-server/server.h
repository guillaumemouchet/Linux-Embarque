#pragma once

#include <QtWidgets>
#include <QtNetwork>
#include <QtCore>

#include "driver.h"

class Server : public QDialog
{
    Q_OBJECT

public:
    explicit Server(QWidget *parent = nullptr);

private slots:
    void addSocket();
    void sendData();

private:
    void initServer();
    int length;

    QPushButton *quitButton = nullptr;
    QHBoxLayout *buttonLayout = nullptr;
    QVBoxLayout *mainLayout = nullptr;
    QLabel *statusLabel = nullptr;
    QTcpServer *tcpServer = nullptr;
    QList<QTcpSocket *> sockets;
};
