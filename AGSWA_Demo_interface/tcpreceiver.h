#ifndef TCPRECEIVER_H
#define TCPRECEIVER_H
#include <QTcpSocket>
#include <QString>
#include <QSemaphore>
#include <QTimer>
#include <QThread>

class TcpReceiver :public QObject
{
    Q_OBJECT

public:
    TcpReceiver();
    ~TcpReceiver();
    void setServerIp(QString sip);
    QByteArray buildInitPacket(quint16 type);
public slots:
    void connectTCP();
    void closeConnect();
    void readTcpData();
    void connected();
    void sendData(QByteArray data, bool isRaw);
    void onError(QAbstractSocket::SocketError);
    void heartBeatTimeout();
signals:
    void readyCMDReceived(QByteArray data);
    void socketError(QString msg);

private:
    bool isConnected = false;
    QTimer connectTimer;
    QThread m_workerThread;
    QTcpSocket  *m_socket = nullptr;
    QString ip;
    quint16 hdr_len = 0;

    quint16 heartBeatNum = 0;
    static const quint8  HDR_SIZE = 2;
    QByteArray heartBeatPacket;
};

#endif // TCPRECEIVER_H
