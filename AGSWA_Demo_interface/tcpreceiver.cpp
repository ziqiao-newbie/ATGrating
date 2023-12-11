#include "tcpreceiver.h"
#include <QDataStream>
#include <QThread>
#include <QHostAddress>

#include "agswacontrol.h"
TcpReceiver::TcpReceiver()
{
    heartBeatPacket = buildInitPacket(AgswaControl::KEEP_ALIVE);
    connect(&connectTimer, &QTimer::timeout, this, &TcpReceiver::heartBeatTimeout);
    connectTimer.start(1000*20);
    connectTimer.moveToThread(&m_workerThread);
    moveToThread(&m_workerThread);
    m_workerThread.start();
}
void TcpReceiver::setServerIp(QString sip){
    this->ip = sip;
}

QByteArray TcpReceiver::buildInitPacket(quint16 type){
    QByteArray data;
    // len(2bytes)| type(2bytes)
    data.resize(4);
    // set type field
    data.replace(2,2, (char *)&type,2);
    return data;
}

void TcpReceiver::connectTCP(){
    if(m_socket == nullptr){
        isConnected = false;
        m_socket = new QTcpSocket(this);
        m_socket->connectToHost(QHostAddress(this->ip),5001);

        connect(m_socket, &QTcpSocket::readyRead, this, &TcpReceiver::readTcpData);
        connect(m_socket, &QTcpSocket::connected, this, &TcpReceiver::connected);
        connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(onError(QAbstractSocket::SocketError)));
    }
}

void TcpReceiver::onError(QAbstractSocket::SocketError){
    QTcpSocket* sock = (QTcpSocket*)sender();
    qInfo()<<"onError" << sock->errorString();
    emit socketError(sock->errorString());
    closeConnect();
}

/**
 * 5.4 Heartbeat
 * @brief TcpReceiver::heartBeatTimeout
 */
void TcpReceiver::heartBeatTimeout(){
    if(isConnected){
        heartBeatNum++;
        if(heartBeatNum >= 2){
            sendData(heartBeatPacket,false);
        }
    }
}

void TcpReceiver::sendData(QByteArray data, bool isRaw){
    if(m_socket != nullptr){
        if(!isRaw){
            quint16 size = data.size();
            data.replace(0,2, (char *)&size,2);//set the len
        }
        m_socket->write(data);
        m_socket->flush();
    }
}

void TcpReceiver::connected(){
    qInfo()<<"connected";
}
void TcpReceiver::readTcpData()
{
    heartBeatNum = 0;
    while(m_socket->bytesAvailable() >= HDR_SIZE){
        if(hdr_len <= 0){
            m_socket->read((char*) &hdr_len, 2);
        }
        if(m_socket->bytesAvailable() >= hdr_len - HDR_SIZE ){
            isConnected = true;
            QByteArray data = m_socket->read(hdr_len - HDR_SIZE);
            quint16 hdr_type = data.at(0) | data.at(1)<< 8;
            qDebug("received the packet len:%d, type:0x%04x",hdr_len, hdr_type) ;
            emit readyCMDReceived(data);
            hdr_len = 0;

        } else {
            break;
        }
    }
}

void TcpReceiver::closeConnect(){
    if(m_socket != nullptr){
        m_socket->close();
        isConnected = false;
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}
TcpReceiver::~TcpReceiver(){
    closeConnect();
    m_workerThread.terminate();
    m_workerThread.wait();
}
