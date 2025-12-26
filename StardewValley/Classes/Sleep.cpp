#include "GameScene.h"
#include "GameClock.h"
#include "CropSystem.h"
#include "ForageSystem.h"
#include "AnimalSystem.h"
#include "EnergySystem.h"
#include "HudLayer.h"

USING_NS_CC;

void BackgroundLayer::showSleepDialog()
{
    if (_sleepDialogActive || _isSleeping) return;
    _sleepDialogActive = true;
    _waitingForSleepInput = false;
    _waitingForEarningsInput = false;
    auto director = Director::getInstance();
    Size visibleSize = director->getVisibleSize();
    Vec2 origin = director->getVisibleOrigin();
    if (!_sleepOverlay)
    {
        _sleepOverlay = LayerColor::create(Color4B(0, 0, 0, 160));
        _sleepOverlay->setContentSize(visibleSize);
        _sleepOverlay->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
        Vec2 layerWorldPos = this->getPosition();
        _sleepOverlay->setPosition(origin - layerWorldPos);
        addChild(_sleepOverlay, 5000);
    }
    else
    {
        _sleepOverlay->setVisible(true);
        _sleepOverlay->setOpacity(160);
    }

    if (_sleepOverlay)
    {
        _sleepOverlay->removeAllChildren();
        _sleepLabel = nullptr;
        auto label = Label::createWithSystemFont("Do you want to sleep? press Y/N", "Arial", 32.0f);
        if (label)
        {
            label->setPosition(Vec2(visibleSize.width * 0.5f, visibleSize.height * 0.5f));
            _sleepOverlay->addChild(label);
        }
    }
}

void BackgroundLayer::beginSleep()
{
    if (_isSleeping) return;
    _isSleeping = true;

    if (_confirmationOverlay) {
        _confirmationOverlay->removeFromParent();
        _confirmationOverlay = nullptr;
    }

    if (!_sleepOverlay)
    {
        auto director = Director::getInstance();
        Size visibleSize = director->getVisibleSize();
        _sleepOverlay = LayerColor::create(Color4B(0, 0, 0, 0));
        _sleepOverlay->setContentSize(visibleSize);
        _sleepOverlay->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
        addChild(_sleepOverlay, 5000);
    }

    if (_sleepOverlay)
    {
        auto director = Director::getInstance();
        Vec2 origin = director->getVisibleOrigin();
        Vec2 layerWorldPos = this->getPosition();
        _sleepOverlay->setPosition(origin - layerWorldPos);

        _sleepOverlay->setVisible(true);
        _sleepOverlay->stopAllActions();
        _sleepOverlay->setOpacity(0);
        
        auto fadeIn = FadeTo::create(3.0f, 255);
        
        std::function<void()> sleepCallback = [this]() {
             this->_waitingForSleepInput = true;
             
             if (!this->_sleepLabel)
             {
                 this->_sleepLabel = Label::createWithSystemFont("Click to continue", "Arial", 48);
                 if (this->_sleepLabel)
                 {
                     Size overlaySize = this->_sleepOverlay->getContentSize();
                     this->_sleepLabel->setPosition(Vec2(overlaySize.width * 0.45f, overlaySize.height * 0.5f));
                     this->_sleepLabel->setAnchorPoint(Vec2::ANCHOR_MIDDLE);
                     this->_sleepOverlay->addChild(this->_sleepLabel);
                 }
             }
             
             if (this->_sleepLabel)
             {
                 this->_sleepLabel->setVisible(true);
                 this->_sleepLabel->stopAllActions();
                 this->_sleepLabel->runAction(RepeatForever::create(Sequence::create(FadeTo::create(0.5f, 150), FadeTo::create(0.5f, 255), nullptr)));
             }
        };
        
        auto waitForInput = CallFunc::create(sleepCallback);
        
        _sleepOverlay->runAction(Sequence::create(fadeIn, waitForInput, nullptr));
    }
    else
    {
        if (GameScene::sClock)
        {
            GameScene::sClock->setHour(6);
            GameScene::sClock->setMinute(0);
            GameScene::sClock->addDay(1);
        }
        CropSystem::getInstance()->updateDailyGrowth();
        ForageSystem::getInstance()->newDay(GameScene::sClock->getSeason());
        int earned = 0;
        if (GameScene::sHud)
        {
            earned = GameScene::sHud->settleBucketAndGetTotal();
        }
        GameScene::sTodayEarnings = earned;
        if (GameScene::sWasFainted)
        {
            EnergySystem::getInstance()->resetEnergy(0.75f);
        }
        else
        {
            EnergySystem::getInstance()->resetEnergy();
        }
        GameScene::sWasFainted = false;
        GameScene::sMidnightWarned = false;
        
        if (_type != BackgroundType::Home)
        {
             GameScene::sStartAtHomeBed = true;
             auto next = GameScene::createScene(BackgroundType::Home);
             Director::getInstance()->replaceScene(next);
             return;
        }

        if (_player && _hasBedRect)
        {
            Vec2 pos(_bedRect.getMidX(), _bedRect.getMidY());
            _player->setPosition(pos);
        }
        _isSleeping = false;
    }
}

void BackgroundLayer::cancelSleepDialog()
{
    _sleepDialogActive = false;
    if (_sleepOverlay)
    {
        _sleepOverlay->setVisible(false);
        _sleepOverlay->stopAllActions();
    }
}

bool BackgroundLayer::handleSleepMouseDown(EventMouse* e)
{
    if (_waitingForSleepInput)
    {
        if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT)
        {
            _waitingForSleepInput = false;
            _waitingForEarningsInput = true;
            if (_sleepLabel) _sleepLabel->setVisible(false);

            if (GameScene::sClock)
            {
                GameScene::sClock->setHour(6);
                GameScene::sClock->setMinute(0);
                GameScene::sClock->addDay(1);
            }
            CropSystem::getInstance()->updateDailyGrowth();
            ForageSystem::getInstance()->newDay(GameScene::sClock->getSeason());
            AnimalSystem::getInstance()->updateDailyGrowth();
            
            int earned = 0;
            if (GameScene::sHud)
            {
                earned = GameScene::sHud->settleBucketAndGetTotal();
            }
            GameScene::sTodayEarnings = earned;
            
            if (_sleepOverlay)
            {
                Size overlaySize = _sleepOverlay->getContentSize();
                auto earningsLabel = Label::createWithSystemFont("Today Earnings: " + std::to_string(earned) + " g", "Arial", 36.0f);
                if (earningsLabel)
                {
                    earningsLabel->setPosition(Vec2(overlaySize.width * 0.5f, overlaySize.height * 0.6f));
                    _sleepOverlay->addChild(earningsLabel);
                }
            }
            
            if (GameScene::sWasFainted)
            {
                EnergySystem::getInstance()->resetEnergy(0.75f);
            }
            else
            {
                EnergySystem::getInstance()->resetEnergy();
            }
            GameScene::sWasFainted = false;
            GameScene::sMidnightWarned = false;
            
            return true;
        }
    }

    if (_waitingForEarningsInput)
    {
        if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT)
        {
            _waitingForEarningsInput = false;

            if (_type != BackgroundType::Home)
            {
                GameScene::sStartAtHomeBed = true;
                auto next = GameScene::createScene(BackgroundType::Home);
                Director::getInstance()->replaceScene(next);
                return true;
            }

            if (_player && _hasBedRect)
            {
                Vec2 pos(_bedRect.getMidX(), _bedRect.getMidY());
                _player->setPosition(pos);
            }

            if (_sleepOverlay)
            {
                _sleepOverlay->stopAllActions();
                _sleepOverlay->runAction(Sequence::create(
                    FadeTo::create(1.0f, 0),
                    CallFunc::create([this](){
                        _sleepOverlay->setVisible(false);
                        _isSleeping = false;
                    }),
                    nullptr
                ));
            }
            else
            {
                _isSleeping = false;
            }
            return true;
        }
    }

    if (_isSleeping || GameScene::sWasFainted)
    {
        return true;
    }

    return false;
}

