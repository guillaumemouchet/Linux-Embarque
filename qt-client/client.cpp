#include "client.h"

Client::Client(QWidget *parent) : QDialog(parent)
{
    // Debug brut values
    this->length = 0;
    this->maxValue = 1;

    this->values_R = nullptr;
    this->values_G = nullptr;
    this->values_B = nullptr;
    this->values_IR = nullptr;

    // Define offset
    this->xOffset = 20;
    this->yOffset = 20;

    // Define sizes
    this->w = 800;
    this->h = 400;

    // Define step
    int xLength = w - (2 * xOffset);
    int yLength = h - yOffset - 80;
    this->xStep = xLength / 10;
    this->yStep = yLength / 10;

    clientGeometry();
    clientControl();
    clientAppearance();
}

void Client::clientGeometry()
{
    this->portLineEdit = new QLineEdit(this);
    this->ipLineEdit = new QLineEdit(this);
    this->btnConnect = new QPushButton("connect", this);
    this->btnGetData = new QPushButton("get data", this);
    this->tcpSocket = new QTcpSocket(this);
    this->lblIP = new QLabel("Server IP : ");
    this->lblPort = new QLabel("Server port : ");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *configLayout = new QHBoxLayout();
    QHBoxLayout *controlLayout = new QHBoxLayout();

    configLayout->addWidget(this->lblIP);
    configLayout->addWidget(this->ipLineEdit);
    configLayout->addWidget(this->lblPort);
    configLayout->addWidget(this->portLineEdit);

    controlLayout->addWidget(this->btnConnect);
    controlLayout->addWidget(this->btnGetData);

    mainLayout->addLayout(configLayout);
    mainLayout->addLayout(controlLayout);
    mainLayout->addStretch();
}

void Client::clientControl()
{
    in.setDevice(this->tcpSocket);
    in.setVersion(QDataStream::Qt_4_0);

    connect(btnConnect, &QAbstractButton::clicked, this, &Client::connection);
    connect(btnGetData, &QAbstractButton::clicked, this, &Client::getData);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &Client::readData);
}

void Client::clientAppearance()
{
    this->btnGetData->setEnabled(false);
    setMinimumSize(this->w, this->h);
    setMaximumSize(this->w, this->h);
}

void Client::drawValues(int *values, QPainter &painter, QColor color)
{
    QPen pen;
    pen.setColor(color);
    pen.setWidth(0);
    painter.setPen(pen);

    int yLength = h - yOffset - 80;
    double step = yLength / this->maxValue;

    // First point
    QPointF org, dest;
    org = QPointF(xOffset, h - yOffset);

    // Draw the lines
    for (int i = 1; i < this->length; i++)
    {
        dest = QPointF(xOffset + (i * (xStep / 10)), h - (values[i] * step) - yOffset);
        painter.drawLine(org, dest);
        org = dest;
    }
}

void Client::connection()
{
    tcpSocket->connectToHost(ipLineEdit->text(), portLineEdit->text().toInt());
    this->btnConnect->setEnabled(false);
    this->btnGetData->setEnabled(true);
}

void Client::getData()
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);

    out << QString("BONJOUR");

    tcpSocket->write(block);
}

void Client::readData()
{
    in.startTransaction();

    qint64 lengthInput;
    qint64 tempData;

    in >> lengthInput;

    if (lengthInput > 0)
    {
        this->length = lengthInput;

        if (this->values_R != nullptr)
        {
            delete[] this->values_R;
            delete[] this->values_G;
            delete[] this->values_B;
            delete[] this->values_IR;
        }

        this->values_R = new int[this->length];
        this->values_G = new int[this->length];
        this->values_B = new int[this->length];
        this->values_IR = new int[this->length];
    }

    for (int i = 0; i < this->length; i++)
    {
        in >> tempData;
        this->values_R[i] = tempData;

        in >> tempData;
        this->values_G[i] = tempData;

        in >> tempData;
        this->values_B[i] = tempData;

        in >> tempData;
        this->values_IR[i] = tempData;
    }

    qDebug() << "R : " << this->values_R[this->length - 1] << " G : " << this->values_G[this->length - 1] << " B : " << this->values_B[this->length - 1] << " IR : " << this->values_IR[this->length - 1];

    if (!in.commitTransaction())
    {
        return;
    }

    update();
}

void Client::paintEvent(QPaintEvent *)
{
    QPainter paint(this);

    drawAxis(paint);

    this->maxValue = getMaxValue();

    drawValues(this->values_R, paint, Qt::red);
    drawValues(this->values_G, paint, Qt::green);
    drawValues(this->values_B, paint, Qt::blue);
    drawValues(this->values_IR, paint, Qt::yellow);
}

void Client::drawAxis(QPainter &painter)
{
    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(2);

    painter.setPen(pen);
    painter.drawLine(xOffset, h - yOffset, w - xOffset, h - yOffset);
    painter.drawLine(xOffset, h - yOffset, xOffset, 80);

    painter.translate(xOffset, h - yOffset);

    for (int i = 1; i <= 10; i++)
    {
        painter.drawLine(i * xStep, 5, i * xStep, -5);
    }

    for (int i = 1; i <= 10; i++)
    {
        painter.drawLine(-5, -i * yStep, 5, -i * yStep);
    }

    painter.translate(-xOffset, -(h - yOffset));
}

int Client::getMaxValue()
{
    double max = 1;

    for (int i = 0; i < this->length; i++)
    {
        int r = this->values_R[i];
        if (r > max)
        {
            max = r;
        }
        int g = this->values_G[i];
        if (g > max)
        {
            max = g;
        }
        int b = this->values_B[i];
        if (b > max)
        {
            max = b;
        }
        int ir = this->values_IR[i];
        if (ir > max)
        {
            max = ir;
        }
    }

    return max;
}
