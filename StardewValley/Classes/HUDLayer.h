#ifndef __HUD_LAYER_H__
#define __HUD_LAYER_H__

#include "cocos2d.h"
#include <string>

class GameClock;
class Wallet;

class HudLayer : public cocos2d::Layer
{
public:
    static HudLayer* create(GameClock* clock, Wallet* wallet);

    bool initWithSystems(GameClock* clock, Wallet* wallet);

    void refresh();

    CREATE_FUNC(HudLayer);

private:
    GameClock* _clock;
    Wallet* _wallet;

    cocos2d::Label* _timeLabel;
    cocos2d::Label* _dateLabel;
    cocos2d::Label* _moneyLabel;
};

#endif
