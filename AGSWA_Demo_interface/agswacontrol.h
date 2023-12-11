#ifndef AGSWACONTROL_H
#define AGSWACONTROL_H

#include <QObject>
#include <QThread>
#include <QList>
#include <QPointF>
#include <QTimer>
#include "tcpreceiver.h"
#include "channelconfiguration.h"


class AgswaControl : public QObject
{
    Q_OBJECT

public:
    AgswaControl(QObject* parent=0);
    ~AgswaControl();
    void connectIMON(QString ip);

    bool startContunousSpectralData(quint32 hz);
    void stopContinuous();
    void getDeviceDetailAndConfiguration();

    void startContiunousCalculateWl(quint32 hz);
    void getDeviceBasicInfo();

    void getChannelSnapshotData(quint8 channelIndex,quint16 gain, quint16 snapThreshold,bool highCE, bool hdrEnable = false, quint16 gain2 = 0, bool highCE2 = false);
    void getChannelContinuousData(quint8 channelIndex,quint16 restTime, quint16 snapThreshold,bool highCE, bool hdrEnable = false, quint16 gain2 = 0, bool highCE2 = false);
    void getConfigFromImonBoard();

    void saveImonConfigToBoardByChannel(quint8 channelIndex, bool enabel,quint16 threshold , bool highCE,
                                        quint16 gain,bool isHdrEnable, bool hdrHighCE, quint16 hdrGain, RangeLine *rangeArray);
    void sendStopCommand();
public slots:
    void readyPacketReceived(QByteArray data);
    void socketError(QString);
    void connectTimeout();
signals:
    // send to tcpsocket
    void sendData(QByteArray data, bool isRaw);
    void connectTcp();
    void closeConnect();

private:
    void spectrumPacketRecived(QByteArray &data);
    void oneChannelspectrumPacketRecived(QByteArray &data, bool isContinus);

    void processKeepAliveData(QByteArray &data);
    void processDeviceDetailAndConfiguration(QByteArray coeff);
    void processImonParam(QDataStream &data_stream);
    void processGetBasicInfo(QByteArray data);
    void processContinuousWLDate(QByteArray data);
    double calibreateWavelength(float temp, quint16 index);


    // device info
    QString m_sn;
    quint8 m_channelNum;
    quint16 m_ccdPixelNum;
    quint32 m_ip;
    quint32 m_subnetMask;
    quint32 m_gateway;
    QString m_macAddr;

    double m_IMONCoefficient[10];
    double m_index2Wavelength[512];
    bool isCoefficientInit = false;


    QList<ChannelConfiguration* > channelConfigList;
    QTimer *connectTimer = nullptr;
    bool connectSucceed = false;

    TcpReceiver *m_tcpReceiver;
    QThread m_ImonThread;

    quint16 pixeArrayChache[512*sizeof (quint16)];
    quint16 pixeArrayChache2[512*sizeof (quint16)];

    volatile bool isContinuousStart = false;
public:
    static const quint16 REQ_COEFF = 0x0001;
    static const quint16 START_CONTINUOUS_SPECT = 0x0003;
    static const quint16 STOP = 0x0004;
    static const quint16 GET_BASIC_INFO = 0x0005;

    static const quint16 CONTINUOUS_SPECT_TYPE = 0x0008;
    static const quint16 KEEP_ALIVE = 0x0009;

    static const quint16 GET_IMON_PARAM = 0x000C;
    static const quint16 SAVE_ONE_CHANNEL_PARAM = 0x000D;
    static const quint16 CONTINUOUS_WL_DATA = 0x000E;
    static const quint16 START_CONTINUOUS_WL = 0x000F;

    static const quint8 ERROR_CODE = 1;
    static const quint8 CONNECTED = 2;
};

#endif // AGSWACONTROL_H
