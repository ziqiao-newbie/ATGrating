#ifndef CHANNELCONFIGURATION_H
#define CHANNELCONFIGURATION_H

#include <QMap>

class RangeLine
{

public:
    RangeLine(){

    }

public:
    bool isEnable = false;
    quint16 m_upIndex = -1;
    quint16 m_downIndex = -1;

    float m_upWlValue;
    float m_downWlValue;
};

class ChannelConfiguration{

public:
    bool isEnable(){
        return m_isEnable;
    }

    bool m_isEnable;
    bool isHighCE ;
    quint16 m_gain;
    quint16 m_autoScanThreshold;

    bool isHdrEnable = false;
    bool hdrIsHighCE;
    quint16 hdrGain;
    RangeLine hdrRange[2];
};

#endif // CHANNELCONFIGURATION_H
