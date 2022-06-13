#include "server.h"

Server::Server(QWidget *parent) : QDialog(parent)
{
    this->length = Driver::dataLength;

    this->statusLabel = new QLabel();
    this->mainLayout = new QVBoxLayout(this);
    this->quitButton = new QPushButton(tr("Quit"));
    this->buttonLayout = new QHBoxLayout;

    buttonLayout->addStretch(1);
    buttonLayout->addWidget(quitButton);
    buttonLayout->addStretch(1);

    mainLayout->addWidget(statusLabel);
    mainLayout->addLayout(buttonLayout);

    initServer();

    connect(quitButton, &QAbstractButton::clicked, this, &QWidget::close);
    connect(tcpServer, &QTcpServer::newConnection, this, &Server::addSocket);
}

void Server::initServer()
{
    tcpServer = new QTcpServer(this);
    if (!tcpServer->listen())
    {
        QMessageBox::critical(this, tr("Fortune Server"),
                              tr("Unable to start the server: %1.")
                                  .arg(tcpServer->errorString()));
        close();
        return;
    }

    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i)
    {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost && ipAddressesList.at(i).toIPv4Address())
        {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }

    // if we did not find one, use IPv4 localhost
    if (ipAddress.isEmpty())
    {
        ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    }

    statusLabel->setText(tr("The server is running on\n\nIP: %1\nport: %2").arg(ipAddress).arg(tcpServer->serverPort()));
}

void Server::addSocket()
{
    QTcpSocket *clientConnection = tcpServer->nextPendingConnection();
    connect(clientConnection, &QTcpSocket::readyRead, this, &Server::sendData);
    this->sockets.append(clientConnection);
}

void Server::sendData()
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);

    struct RGB **orderedData = Driver::getOrderedData();

    qint64 arrayLength = this->length;

    out << arrayLength;

    for (int i = 0; i < this->length; i++)
    {
        qint64 temp;

        temp = orderedData[i]->r;
        out << temp;

        temp = orderedData[i]->g;
        out << temp;

        temp = orderedData[i]->b;
        out << temp;

        temp = orderedData[i]->ir;
        out << temp;
    }

    for (auto socket : this->sockets)
    {
        socket->write(block);
    }
}
