#ifndef GLOBALDEF_H
#define GLOBALDEF_H

#define DEEPIN_VOICE_NOTE "deepin-voice-note"

//Default main window size
#define DEFAULT_WINDOWS_WIDTH 820
#define DEFAULT_WINDOWS_HEIGHT 634

//TitleBar height
#define VNOTE_TITLEBAR_HEIGHT 50

//LeftView width
#define VNOTE_LEFTVIEW_W 250

//SearchEdit Size
#define VNOTE_SEARCHBAR_H  36
#define VNOTE_SEARCHBAR_W  350

//Time format
#define VNOTE_TIME_FMT "yyyy-MM-dd HH:mm:ss.zzz"

// Uncomment follow line if need debug layout
//#define VNOTE_LAYOUT_DEBUG

#include <sys/time.h>

#define TM(s,e) (\
((e.tv_sec-s.tv_sec)*1000 + (e.tv_usec-s.tv_usec)/1000.0)\
)

#endif // GLOBALDEF_H
