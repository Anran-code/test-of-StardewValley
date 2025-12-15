#include "HudLayer.h"
#include "GameClock.h"
#include "Wallet.h"

USING_NS_CC;

HudLayer* HudLayer::create(GameClock* clock, Wallet* wallet)
{
    HudLayer* ret = new (std::nothrow) HudLayer();
    if (ret && ret->initWithSystems(clock, wallet))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool HudLayer::initWithSystems(GameClock* clock, Wallet* wallet)
{
    if (!Layer::init())
    {
        return false;
    }

    _clock = clock;
    _wallet = wallet;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    float margin = 10.0f;
    Vec2 topRight(origin.x + visibleSize.width - margin, origin.y + visibleSize.height - margin);

    _timeLabel = Label::createWithSystemFont("", "Arial", 24);
    _dateLabel = Label::createWithSystemFont("", "Arial", 24);
    _moneyLabel = Label::createWithSystemFont("", "Arial", 24);

    if (_timeLabel)
    {
        _timeLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        _timeLabel->setPosition(topRight);
        addChild(_timeLabel, 1000);
    }
    if (_dateLabel)
    {
        _dateLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        _dateLabel->setPosition(Vec2(topRight.x, topRight.y - 28.0f));
        addChild(_dateLabel, 1000);
    }
    if (_moneyLabel)
    {
        _moneyLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        _moneyLabel->setPosition(Vec2(topRight.x, topRight.y - 56.0f));
        addChild(_moneyLabel, 1000);
    }

    refresh();
    return true;
}

void HudLayer::refresh()
{
    if (_clock && _timeLabel)
    {
        _timeLabel->setString(_clock->getTimeString());
    }
    if (_clock && _dateLabel)
    {
        _dateLabel->setString(_clock->getDateString());
    }
    if (_wallet && _moneyLabel)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Gold: %d", _wallet->getMoney());
        _moneyLabel->setString(buf);
    }
}
