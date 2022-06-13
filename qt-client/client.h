#pragma once

#include <QtWidgets>
#include <QtNetwork>
#include <QtCore>

class Client : public QDialog
{
    Q_OBJECT

public:
    explicit Client(QWidget *parent = nullptr);

private:
    void clientGeometry();
    void clientControl();
    void clientAppearance();

    void drawValues(int *, QPainter &painter, QColor color);
    void drawAxis(QPainter &painter);
    int getMaxValue();

private slots:
    void connection();
    void getData();
    void readData();

private:
    int *values_R = nullptr;
    int *values_G = nullptr;
    int *values_B = nullptr;
    int *values_IR = nullptr;
    int length;

    int yOffset;
    int xOffset;
    int w;
    int h;
    double xStep;
    double yStep;
    double maxValue;

    QLineEdit *portLineEdit = nullptr;
    QLineEdit *ipLineEdit = nullptr;
    QPushButton *btnGetData = nullptr;
    QPushButton *btnConnect = nullptr;
    QLabel *lblIP = nullptr;
    QLabel *lblPort = nullptr;

    QTcpSocket *tcpSocket = nullptr;
    QDataStream in;

protected:
    void paintEvent(QPaintEvent *);
};
