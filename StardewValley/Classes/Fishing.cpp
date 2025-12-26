#include "GameScene.h"
#include "CropSystem.h"
#include "ExperienceSystem.h"
#include "EnergySystem.h"

USING_NS_CC;

void BackgroundLayer::startFishing()
{
    if (_isFishing) return;
    if (!_map || !_player) return;

    EnergySystem::getInstance()->consumeEnergy(EnergySystem::COST_FISHING);

    _isFishing = true;
    _fishBite = false;
    _fishingElapsed = 0.0f;
    _biteTime = cocos2d::RandomHelper::random_real(1.0f, 3.0f);
    _biteWindow = 1.0f;

    auto director = Director::getInstance();
    Size visibleSize = director->getVisibleSize();
    Vec2 origin = director->getVisibleOrigin();

    if (!_fishingOverlay)
    {
        _fishingOverlay = LayerColor::create(Color4B(0, 0, 0, 0));
        _fishingOverlay->setContentSize(visibleSize);
        _fishingOverlay->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
        _fishingOverlay->setPosition(origin);

        auto label = Label::createWithSystemFont("Fishing... Wait for a bite", "Arial", 16.0f);
        if (label)
        {
            _fishingOverlay->addChild(label);
            _fishingLabel = label;
        }

        if (this->getParent())
        {
            this->getParent()->addChild(_fishingOverlay, 6000);
        }
        else
        {
            addChild(_fishingOverlay, 6000);
        }
        
        float gameWidth = 80.0f;
        float gameHeight = visibleSize.height * 0.5f; 
        cocos2d::Size gameSize(gameWidth, gameHeight);
        
        _fishingGame = FishingMiniGame::create(gameSize);
        
        float gameX = visibleSize.width * 0.15f;

        int r = cocos2d::RandomHelper::random_int(0, 2);
        if (r == 0) _currentFishType = CropType::Anchovy;
        else if (r == 1) _currentFishType = CropType::Bream;
        else _currentFishType = CropType::LargemouthBass;

        if (_fishingGame)
        {
            if (_currentFishType == CropType::Anchovy) {
                _fishingGame->setupDifficulty(80.0f, 0.8f, 1.5f);
            } else if (_currentFishType == CropType::Bream) {
                _fishingGame->setupDifficulty(120.0f, 0.5f, 1.0f);
            } else {
                _fishingGame->setupDifficulty(180.0f, 0.3f, 0.7f);
            }

            float centerY = visibleSize.height * 0.55f; 
            _fishingGame->setPosition(Vec2(gameX - gameSize.width * 0.5f, centerY - gameSize.height * 0.5f));
            _fishingOverlay->addChild(_fishingGame, 1);
        }

        if (label)
        {
            float centerY = visibleSize.height * 0.55f;
            float labelY = centerY + (gameHeight * 0.5f) + 20.0f;
            label->setPosition(Vec2(gameX, labelY));
        }
    }
    else
    {
        _fishingOverlay->setVisible(true);
        _fishingOverlay->setOpacity(0);
        _fishingOverlay->setPosition(origin);
        
        if (_fishingLabel)
        {
            _fishingLabel->setString("Press space to control the green block");
        }
        
        int r = cocos2d::RandomHelper::random_int(0, 2);
        if (r == 0) _currentFishType = CropType::Anchovy;
        else if (r == 1) _currentFishType = CropType::Bream;
        else _currentFishType = CropType::LargemouthBass;

        if (_fishingGame)
        {
            _fishingGame->restart();
            if (_currentFishType == CropType::Anchovy) {
                _fishingGame->setupDifficulty(80.0f, 0.8f, 1.5f);
            } else if (_currentFishType == CropType::Bream) {
                _fishingGame->setupDifficulty(120.0f, 0.5f, 1.0f);
            } else {
                _fishingGame->setupDifficulty(180.0f, 0.3f, 0.7f);
            }
        }
    }
}

void BackgroundLayer::endFishing(bool success)
{
    _isFishing = false;
    _fishBite = false;
    _fishingElapsed = 0.0f;

    if (_fishingOverlay)
    {
        _fishingOverlay->stopAllActions();
        _fishingOverlay->setOpacity(0);
    }

    if (success)
    {
        if (GameScene::sInventory)
        {
            const CropData* data = CropSystem::getInstance()->getCropData(_currentFishType);
            if (data)
            {
                Item fish;
                fish.type = ItemType::Crop;
                fish.cropType = _currentFishType;
                fish.name = data->itemName;
                fish.iconPath = data->itemIcon;
                fish.quantity = 1;
                fish.maxStack = 999;
                GameScene::sInventory->addItem(fish);
                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
                
                ExperienceSystem::getInstance()->addExperience(SkillType::Fishing, data->xp);
                
                if (_fishingLabel)
                {
                    _fishingLabel->setString("You caught a " + data->itemName + "!");
                }
                
                if (_player)
                {
                    _player->showToolFeedback(data->itemIcon);
                }
            }
        }
    }
    else
    {
        if (_fishingLabel)
        {
            _fishingLabel->setString("you failed");
        }
    }

    if (_fishingOverlay)
    {
        auto fadeOut = FadeTo::create(0.8f, 0);
        auto hide = CallFunc::create([this]() {
            if (_fishingOverlay)
            {
                _fishingOverlay->setVisible(false);
            }
        });
        _fishingOverlay->runAction(Sequence::create(fadeOut, hide, nullptr));
    }
}

