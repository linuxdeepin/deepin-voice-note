#include "ut_vlcplayer.h"
#include "common/vlcpalyer.h"
#include "stub.h"
#include <QDebug>

static void stub_vlcplayer_common()
{
    qInfo() << "I am vlcplayer common stub";
}

ut_vlcplayer_test::ut_vlcplayer_test()
{
}

TEST_F(ut_vlcplayer_test, setFilePath)
{
    Stub stub;
    stub.set(ADDR(VlcPalyer, setFilePath), stub_vlcplayer_common);
    VlcPalyer player;
    player.setFilePath("");
}

TEST_F(ut_vlcplayer_test, setPosition)
{
    Stub stub;
    stub.set(ADDR(VlcPalyer, setPosition), stub_vlcplayer_common);
    VlcPalyer player;
    player.setPosition(1);
}

TEST_F(ut_vlcplayer_test, play)
{
    Stub stub;
    stub.set(ADDR(VlcPalyer, play), stub_vlcplayer_common);
    VlcPalyer player;
    player.play();
}

TEST_F(ut_vlcplayer_test, pause)
{
    Stub stub;
    stub.set(ADDR(VlcPalyer, pause), stub_vlcplayer_common);
    VlcPalyer player;
    player.pause();
}

TEST_F(ut_vlcplayer_test, stop)
{
    Stub stub;
    stub.set(ADDR(VlcPalyer, stop), stub_vlcplayer_common);
    VlcPalyer player;
    player.stop();
}

TEST_F(ut_vlcplayer_test, getState)
{
    Stub stub;
    stub.set(ADDR(VlcPalyer, getState), stub_vlcplayer_common);
    VlcPalyer player;
    player.getState();
}

TEST_F(ut_vlcplayer_test, init)
{
    Stub stub;
    stub.set(ADDR(VlcPalyer, init), stub_vlcplayer_common);
    VlcPalyer player;
    player.init();
}

TEST_F(ut_vlcplayer_test, handleEvent)
{
    Stub stub;
    stub.set(ADDR(VlcPalyer, handleEvent), stub_vlcplayer_common);
    VlcPalyer player;
    player.handleEvent(nullptr, nullptr);
}
