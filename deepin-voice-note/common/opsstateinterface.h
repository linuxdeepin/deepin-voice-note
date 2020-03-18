#ifndef OPSSTATEINTERFACE_H
#define OPSSTATEINTERFACE_H

#include <QtGlobal>
class OpsStateInterface;

OpsStateInterface* gVNoteOpsStates();

class OpsStateInterface
{
public:
    OpsStateInterface();

    enum {
        StateNone = 0,
        StateSearching,
        StateRecording,
        StatePlaying,
        StateVoice2Text,
        StateAppQuit,
        //Add other state at here
        StateMax,
    };

    void operState(int type, bool isSet);

    bool isSearching()  const;
    bool isRecording()  const;
    bool isPlaying()    const;
    bool isVoice2Text() const;
    bool isAppQuit()    const;
protected:
    quint64 m_states {StateNone};
};

#endif // OPSSTATEINTERFACE_H
