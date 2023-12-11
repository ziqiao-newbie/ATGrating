#include <QDataStream>
#include <QtMath>
#include <QDateTime>
#include <QElapsedTimer>
#include "agswacontrol.h"
#include <chrono>
#include <ctime>

//QStringLiteral
AgswaControl::AgswaControl(QObject* parent)
{
    m_tcpReceiver = new TcpReceiver();

    connect(this,&AgswaControl::sendData,m_tcpReceiver,&TcpReceiver::sendData);
    connect(this, &AgswaControl::connectTcp, m_tcpReceiver, &TcpReceiver::connectTCP);
    connect(this, &AgswaControl::closeConnect, m_tcpReceiver, &TcpReceiver::closeConnect);
    connect(m_tcpReceiver, &TcpReceiver::readyCMDReceived, this, &AgswaControl::readyPacketReceived);
    connect(m_tcpReceiver, &TcpReceiver::socketError,this, &AgswaControl::socketError);

    this->moveToThread(&m_ImonThread);
    m_ImonThread.start();

    connectTimer = new QTimer();
    connectTimer->setSingleShot(true);
    connect(connectTimer, &QTimer::timeout, this, &AgswaControl::connectTimeout);
}

void AgswaControl::connectIMON(QString ip){
    connectSucceed = false;
    isCoefficientInit = false;
     m_tcpReceiver->setServerIp(ip);
     emit connectTcp();

//     connectTimer->start(1000*3);
}
void AgswaControl::connectTimeout(){
    if(!connectSucceed){
        emit closeConnect();
    }
}
/**
 * 5.1 Get basic information
 * @brief ImonControl::getDeviceBasicInfo
 */
void AgswaControl::getDeviceBasicInfo(){
    auto data = m_tcpReceiver->buildInitPacket(GET_BASIC_INFO);
    emit sendData(data,false);
}

/**
 * 5.2 Start continuous wavelength calculate
 * @brief ImonControl::startContiunousCalculateWl
 * @param hz
 */
void AgswaControl::startContiunousCalculateWl(quint32 hz){
    if(isContinuousStart || !isCoefficientInit){
        return ;
    }
    auto data = m_tcpReceiver->buildInitPacket(AgswaControl::START_CONTINUOUS_WL);
    data.append((char *)&hz, 4);
    emit sendData(data,false);
    isContinuousStart = true;
}

/**
 * 5.3 Stop continuous wavelength calculate
 * @brief ImonControl::sendStopCommand
 */
void AgswaControl::sendStopCommand(){
    auto data = m_tcpReceiver->buildInitPacket(STOP);
    emit sendData(data,false);
}

/**
 * 5.5 Get parameter
 * @brief ImonControl::getConfigFromImonBoard
 */
void AgswaControl::getConfigFromImonBoard(){
    if(!isCoefficientInit){
        return ;
    }
    auto data = m_tcpReceiver->buildInitPacket(GET_IMON_PARAM);
    emit sendData(data,false);
}

/**
 * 5.6 Save parameter by channel
 * @brief ImonControl::saveImonConfigToBoardByChannel
 * @param channelIndex
 * @param enabel
 * @param highCE
 * @param gain
 * @param isHdrEnable
 * @param hdrHighCE
 * @param hdrGain
 */
void AgswaControl::saveImonConfigToBoardByChannel(quint8 channelIndex, bool enabel,quint16 threshold,
                                                 bool highCE, quint16 gain,bool isHdrEnable, bool hdrHighCE, quint16 hdrGain, RangeLine *rangeArray){
    auto data = m_tcpReceiver->buildInitPacket(SAVE_ONE_CHANNEL_PARAM);
    data.append((char *)&channelIndex, 1);
    data.append((char *)&enabel, 1);
    data.append((char *)&highCE, 1);
    data.append((char *)&gain, 2);
    data.append((char *)&threshold, 2);

    data.append((char *)&isHdrEnable, 1);
    data.append((char *)&hdrHighCE, 1);
    data.append((char *)&hdrGain, 2);

    for(int i = 0; i < 2;i++){
        data.append((char *)&rangeArray[i].isEnable, 1);
        quint16 downWl = rangeArray[i].m_downWlValue*10;
        quint16 upWl = rangeArray[i].m_upWlValue*10;

        data.append((char *)&downWl, 2);
        data.append((char *)&upWl, 2);
    }
    emit sendData(data,false);
}

/**
 * 5.7 Obtain device details and configuration information
 * @brief data
 */
void AgswaControl::getDeviceDetailAndConfiguration(){
    auto data = m_tcpReceiver->buildInitPacket(AgswaControl::REQ_COEFF);
    emit sendData(data,false);
}

/**
 * 5.8 Start continuous raw spectral data
 * @brief ImonControl::startContunous
 * @param hz
 * @param isNeedSavePeak2File
 * @param dirToSave
 * @return
 */
bool AgswaControl::startContunousSpectralData(quint32 hz){
    if(isContinuousStart || !isCoefficientInit){
        return false;
    }
    auto data = m_tcpReceiver->buildInitPacket(START_CONTINUOUS_SPECT);
    data.append((char *)&hz, 4);

    emit sendData(data,false);
    isContinuousStart = true;
    return true;
}

void AgswaControl::stopContinuous(){
    sendStopCommand();
    isContinuousStart = false;
}

void AgswaControl::readyPacketReceived(QByteArray data){
    quint16 hdr_type = data.at(0) | data.at(1)<< 8;
    qDebug("type 0x%04x", hdr_type);
    switch (hdr_type) {
    case REQ_COEFF:{
        connectSucceed = true;
        processDeviceDetailAndConfiguration(data);
        break;
    }
    case GET_IMON_PARAM:{
        QDataStream data_stream(&data, QIODevice::ReadOnly);
        data_stream.skipRawData(2);//skip 2bytes type
        processImonParam(data_stream);
        break;
    }
    case CONTINUOUS_SPECT_TYPE:{
        if(!isContinuousStart){
            break;
        }
        spectrumPacketRecived(data);
        break;
    }
    case STOP:
        break;
    case KEEP_ALIVE:
        processKeepAliveData(data);
        break;
    case CONTINUOUS_WL_DATA:
        processContinuousWLDate(data);
        break;
    case GET_BASIC_INFO:
        processGetBasicInfo(data);
        break;
    default:
        qDebug("hdr_type 0x%04x no handle", hdr_type);
        break;
    }
    data.clear();
    return;
}
void AgswaControl::processContinuousWLDate(QByteArray data){
    QDataStream data_stream(&data, QIODevice::ReadOnly);
    data_stream.skipRawData(2);//skip 2bytes type

    int totalWl = 0;
    quint16 sequence;
    data_stream.readRawData((char*) &sequence, 2);
    quint32 channels;
    qint16 temp16;
    data_stream.readRawData((char*) &channels, sizeof (quint32));
    data_stream.readRawData((char*) &temp16, 2);
    float temp = temp16/128.0f;

    qInfo("seq:%d,temp:%.2f",sequence, temp);

    for(int i = 0; i < m_channelNum; i++){
        if(channels & 0x01){
            qInfo("CH<%d> centralWavelength:",i);
            quint8 wlNum = 0;
            data_stream.readRawData((char*) &wlNum, 1);
            for(int j =0; j < wlNum;j++){
                quint32 wl;
                data_stream.readRawData((char*) &wl, 4);
                double centralWavelength = wl/10000.0;
                qInfo("%.4f",centralWavelength);

                totalWl++;
            }
        }
        channels = channels >> 1;
    }
    qInfo("processContinuousWLDate end, totalWl num :%d",totalWl);
}

void AgswaControl::processGetBasicInfo(QByteArray data){
    QDataStream data_stream(&data, QIODevice::ReadOnly);
    data_stream.skipRawData(2);//skip 2bytes type
    //device sn
    char sn[7];
    data_stream.readRawData(sn, 6);
    sn[6] = '\0';

    //channel num
    quint8 num;
    data_stream.readRawData((char*)&num, 1);
    m_channelNum = num;
    //temp
    qint16 temp16;
    data_stream.readRawData((char*)&temp16, 2);
    float temp = temp16/128.0f;
    qInfo("processGetBasicInfo SN:%s ChannelNum:%d temp:%.2f",sn ,num,temp);
}
void AgswaControl::processDeviceDetailAndConfiguration(QByteArray coeff){
    QDataStream data_stream(&coeff, QIODevice::ReadOnly);
    data_stream.skipRawData(2);//skip 2bytes type

    //device sn
    char sn[7];
    data_stream.readRawData(sn, 6);
    sn[6] = '\0';
    m_sn = QString(sn);

    //channel num
    data_stream.readRawData((char*)&m_channelNum, 1);
    qDebug()<< "device SN:" << m_sn << "channelNum:" << m_channelNum;
    //coeff
    for(quint8 i = 0; i < 10; i++){
        data_stream.readRawData((char*) (m_IMONCoefficient + i),8);
    }

    //temp
    qint16 temp16;
    data_stream.readRawData((char*)&temp16, 2);
    qDebug()<< "temp:" << temp16/128.0;

    //network info
    data_stream.readRawData((char*)&m_ip, 4);
    data_stream.readRawData((char*)&m_subnetMask, 4);
    data_stream.readRawData((char*)&m_gateway, 4);

    //mac addr
    char macArray[6];
    data_stream.readRawData(macArray, 6);
    QByteArray array(macArray, 6);
    m_macAddr = QString(array.toHex(':')).toUpper();

    data_stream.readRawData((char*)&m_ccdPixelNum, sizeof (quint16));
    qDebug() << "m_ccdPixelNum" << m_ccdPixelNum;

    processImonParam(data_stream);

    for(quint16 index = 0; index < m_ccdPixelNum; index++){
        m_index2Wavelength[index] = m_IMONCoefficient[0] + index*m_IMONCoefficient[1] + qPow(index,2)*m_IMONCoefficient[2]
                + qPow(index, 3)*m_IMONCoefficient[3] + qPow(index, 4)*m_IMONCoefficient[4] + qPow(index,5)*m_IMONCoefficient[5];
    }
    isCoefficientInit = true;
}

double AgswaControl::calibreateWavelength(float temp, quint16 index){
    return (m_index2Wavelength[index] - m_IMONCoefficient[8]*temp - m_IMONCoefficient[9])
            /(1 + m_IMONCoefficient[6]*temp + m_IMONCoefficient[7]);
}

void AgswaControl::processImonParam(QDataStream &data_stream){
    quint32 channelEnableBits;
    data_stream.readRawData((char*)&channelEnableBits, sizeof (quint32));

    quint32 BitsCE;
    data_stream.readRawData((char*)&BitsCE, sizeof(quint32));

    quint32 hdrEnableBits;
    data_stream.readRawData((char*)&hdrEnableBits, sizeof (quint32));

    quint32 hdrHighCEBits;
    data_stream.readRawData((char*)&hdrHighCEBits, sizeof (quint32));

    quint16 *gain = new quint16[m_channelNum];
    data_stream.readRawData((char*)gain, sizeof (quint16)*m_channelNum);

    quint16 *hdrGain = new quint16[m_channelNum];
    data_stream.readRawData((char*)hdrGain, sizeof(quint16)*m_channelNum);

    quint16 *threshold  = new quint16[m_channelNum];
    data_stream.readRawData((char*)threshold, sizeof (quint16)*m_channelNum);

    qDeleteAll(channelConfigList);
    channelConfigList.clear();
    for (int chIndex = 0; chIndex < m_channelNum; ++chIndex){
        ChannelConfiguration* channelConfig = new ChannelConfiguration();
        channelConfig->m_gain = gain[chIndex];
        channelConfig->hdrGain = hdrGain[chIndex];
        channelConfig->m_autoScanThreshold = threshold[chIndex];

        channelConfig->m_isEnable = channelEnableBits & 0x01;
        channelConfig->isHighCE = BitsCE & 0x01;

        channelConfig->isHdrEnable = hdrEnableBits & 0x01;
        channelConfig->hdrIsHighCE = hdrHighCEBits & 0x01;

         qDebug("channel:%d, isEnable:%s,"
                " gain:%d, hdrGain:%d, threshold:%d",
                chIndex, channelConfig->m_isEnable ? "true" : "false",
                gain[chIndex] , hdrGain[chIndex], threshold[chIndex]);
         qDebug("channel:%d, isHighCE:%s,"
                " isHdrEnable:%s,"
                " hdrIsHighCE:%s",
                chIndex, channelConfig->isHighCE ? "true" : "false",
                channelConfig->isHdrEnable ? "true" : "false",
                channelConfig->hdrIsHighCE ? "true" : "false");
        for(int j = 0; j < 2; j++){
            bool hdrRangeEnable;
            data_stream.readRawData((char*)&hdrRangeEnable, 1);
            quint16 lowWL, upWL;
            data_stream.readRawData((char*)&lowWL, sizeof(quint16));
            data_stream.readRawData((char*)&upWL, sizeof(quint16));

            RangeLine *range = &channelConfig->hdrRange[j];
            range->isEnable = hdrRangeEnable;
            range->m_downWlValue = lowWL/10.0;
            range->m_upWlValue = upWL/10.0;
            qDebug("channel:%d,hdrRange%d: hdrRangeEnable:%d, downWlValue:%.1f, upWlValue:%.1f", chIndex, j,hdrRangeEnable, range->m_downWlValue , range->m_upWlValue);
        }

        BitsCE = BitsCE >> 1;
        channelEnableBits = channelEnableBits >> 1;
        hdrEnableBits = hdrEnableBits >> 1;
        hdrHighCEBits = hdrHighCEBits >> 1;
    }
    delete [] gain;
    delete [] hdrGain;
    delete [] threshold;
}

void AgswaControl::processKeepAliveData(QByteArray &data){
    if(data.size() < 4){
        return;
    }
    QDataStream data_stream(&data, QIODevice::ReadOnly);
    data_stream.skipRawData(2);//skip 2bytes type

    //temperature
    qint16 temp16;
    data_stream.readRawData((char*)&temp16, 2);
    float temp = temp16/128.0f;
    qInfo()<<"heartBeat temp:" << temp;
}

void AgswaControl::oneChannelspectrumPacketRecived(QByteArray &data, bool isContinus){
    QDataStream data_stream(&data, QIODevice::ReadOnly);
    data_stream.skipRawData(2);//skip 2bytes type
    quint16 sequence;
    quint8 snapshotNum;
    data_stream.readRawData((char*) &sequence, 2);
    data_stream.readRawData((char*) &snapshotNum, 1);

    quint32 channels;
    qint16 temp16;
    quint32 hdrEnable;
    data_stream.readRawData((char*) &channels, sizeof (quint32));
    data_stream.readRawData((char*) &temp16, 2);
    data_stream.readRawData((char*) &hdrEnable, sizeof (quint32));
//    qDebug("oneChannelspectrumPacketRecived sequence %d frSize %d %u", sequence,snapshotNum, channels);
    quint16 piexNum = m_ccdPixelNum;

    float tempc = temp16/128.0;
    quint32 channelTemp = channels;
    quint8 channelIndex;
    for(quint8 ch_i = 0; ch_i < m_channelNum; ch_i++){
        if(channelTemp & 0x01){
            channelIndex = ch_i;
            break;
        }
        channelTemp = channelTemp >>1;
    }

    data_stream.readRawData((char*)pixeArrayChache, piexNum*sizeof (quint16));
    if(!data_stream.atEnd() && hdrEnable){
        data_stream.readRawData((char*)pixeArrayChache2, piexNum*sizeof (quint16));
//        fuseCCDForHdr(pixeArrayChache, pixeArrayChache2);
    }

    for(int i = 0; i < piexNum;i++){
        double wl = calibreateWavelength(tempc, i);
        double optialPower = pixeArrayChache[i];
    }
}

void AgswaControl::spectrumPacketRecived(QByteArray &data){
    QDataStream data_stream(&data, QIODevice::ReadOnly);
    data_stream.skipRawData(2);//skip 2bytes type
    quint16 sequence;
    quint8 frameTotalNum;
    data_stream.readRawData((char*) &sequence, 2);
    data_stream.readRawData((char*) &frameTotalNum, 1);
    qDebug("sequence:%d frame num:%d", sequence, frameTotalNum);
    quint32 channels;
    qint16 temp16;
    quint32 hdrEnable;
    data_stream.readRawData((char*) &channels, sizeof (quint32));
    data_stream.readRawData((char*) &temp16, 2);
    data_stream.readRawData((char*) &hdrEnable, sizeof (quint32));

    float temp = temp16/128.0;

    for(quint16 j = 0; j< frameTotalNum ;j++){
        quint32 channelTemp = channels;
        quint32 hdrEnableTemp = hdrEnable;
        for(quint8 ch_i = 0; ch_i < m_channelNum; ch_i++){
            if(channelTemp & 0x01){
                qDebug("read channel %d spectrum", ch_i);
                int frameSize = m_ccdPixelNum*sizeof (quint16);
                data_stream.readRawData((char*)pixeArrayChache, frameSize);
                if(hdrEnableTemp & 0x01){
                    data_stream.readRawData((char*)pixeArrayChache2, frameSize);
//                    fuseCCDForHdr(pixeArrayChache, pixeArrayChache2);
                }

                for(int i = 0; i < m_ccdPixelNum;i++){
                    double wl = calibreateWavelength(temp, i);
                    double opticalPower = pixeArrayChache[i];
                    qInfo("wavelength:%.4f, ad value:%f", wl, opticalPower);
                }

            }
            channelTemp = channelTemp >>1;
            hdrEnableTemp = hdrEnableTemp >> 1;
        }//end of channel for one snapshot
    }
}

void AgswaControl::socketError(QString er){
    qInfo() << "socketError" << er;
}

AgswaControl::~AgswaControl(){
    emit closeConnect();
    m_tcpReceiver->deleteLater();

    m_ImonThread.terminate();
    m_ImonThread.wait();

}
