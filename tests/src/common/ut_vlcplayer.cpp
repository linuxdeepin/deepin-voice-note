#include "ut_vlcplayer.h"
#include "common/vlcpalyer.h"

ut_vlcplayer_test::ut_vlcplayer_test()
{
}

TEST_F(ut_vlcplayer_test, setPosition)
{
    VlcPalyer player;
    player.setPosition(1);
}

TEST_F(ut_vlcplayer_test, play)
{
    VlcPalyer player;
    player.play();
}

TEST_F(ut_vlcplayer_test, pause)
{
    VlcPalyer player;
    player.pause();
}

TEST_F(ut_vlcplayer_test, stop)
{
    VlcPalyer player;
    player.stop();
}

TEST_F(ut_vlcplayer_test, getState)
{
    VlcPalyer player;
    player.getState();
}
