#include "GameScene.h"
#include "CropSystem.h"
#include "ForageSystem.h"
#include "AnimalSystem.h"
#include "EnergySystem.h"
#include "HudLayer.h"
#include "ShopLayer.h"

USING_NS_CC;

void BackgroundLayer::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (_isSleeping || GameScene::sWasFainted)
    {
        return;
    }

    if (_isFishing)
    {
        if (_fishingGame)
        {
            _fishingGame->onKeyPressed(keyCode);
        }
        if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
        {
            endFishing(false);
        }
        return;
    }

    if (PauseLayer::isGamePaused())
    {
        if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
        {
            if (_pauseMenu)
            {
                _pauseMenu->hide();
                _pauseMenu = nullptr;
            }
        }
        return;
    }
    
    if (_confirmationOverlay)
    {
        if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE || keyCode == EventKeyboard::KeyCode::KEY_N)
        {
            _confirmationOverlay->removeFromParent();
            _confirmationOverlay = nullptr;
        }
        else if (keyCode == EventKeyboard::KeyCode::KEY_Y)
        {
        }
        return; 
    }

    if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
    {
        if (!_sleepDialogActive && !_isSleeping)
        {
            _pauseMenu = PauseLayer::create();
            _pauseMenu->show(this->getParent() ? this->getParent() : this);
            return;
        }
    }

    if (_sleepDialogActive)
    {
        if (keyCode == EventKeyboard::KeyCode::KEY_Y)
        {
            _sleepDialogActive = false;
            if (_sleepOverlay)
            {
                _sleepOverlay->removeAllChildren();
                _sleepLabel = nullptr;
            }
            beginSleep();
        }
        else if (keyCode == EventKeyboard::KeyCode::KEY_N || keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
        {
            cancelSleepDialog();
        }
        return;
    }
    if (_isSleeping)
    {
        return;
    }

    if (keyCode == EventKeyboard::KeyCode::KEY_GRAVE)
    {
        _isDebugMode = !_isDebugMode;
        GameScene::sDebugMode = _isDebugMode;
        if (GameScene::sHud) { GameScene::sHud->updateInventoryUI(); }
        Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("DEBUG_MODE_CHANGED");
        return;
    }

    if (_type == BackgroundType::Farm)
    {
        Vec2 tile = getFacingTile();
        switch (keyCode)
        {
        case EventKeyboard::KeyCode::KEY_1:
            CropSystem::getInstance()->setSelectedCrop(CropType::Parsnip);
            break;
        case EventKeyboard::KeyCode::KEY_2:
            CropSystem::getInstance()->setSelectedCrop(CropType::Cauliflower);
            break;
        case EventKeyboard::KeyCode::KEY_3:
            CropSystem::getInstance()->setSelectedCrop(CropType::Potato);
            break;
        case EventKeyboard::KeyCode::KEY_T:
            CropSystem::getInstance()->tillTile(tile);
            EnergySystem::getInstance()->consumeEnergy(EnergySystem::COST_HOE);
            break;
        case EventKeyboard::KeyCode::KEY_G:
        {
            if (GameScene::sInventory)
            {
                int slot = GameScene::sInventory->getSelectedSlot();
                if (GameScene::sInventory->hasItem(slot))
                {
                    Item& item = GameScene::sInventory->getItem(slot);
                    if (item.type == ItemType::Tool && item.toolType == ToolType::WateringCan)
                    {
                        Vec2 facingTile = getFacingTile();
                        
                        bool inPool = false;
                        if (_map && _groundLayer)
                        {
                            Size tileSize = _map->getTileSize();
                            Vec2 tilePos = _groundLayer->getPositionAt(facingTile);
                            Vec2 centerPos(tilePos.x + tileSize.width * 0.5f, tilePos.y + tileSize.height * 0.5f);
                            inPool = isWater(centerPos);
                        }
                        
                        if (inPool)
                        {
                            item.currentWater = item.maxWater;
                            EnergySystem::getInstance()->consumeEnergy(4.0f);
                            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
                            std::string msg = "Watering Can Refilled";
                            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
                        }
                        else
                        {
                            if (item.currentWater > 0)
                            {
                                CropSystem::getInstance()->waterTile(tile);
                                EnergySystem::getInstance()->consumeEnergy(EnergySystem::COST_WATERING_CAN);
                                item.currentWater -= 1.0f;
                                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
                            }
                            else
                            {
                                std::string msg = "Watering Can is empty!";
                                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
                            }
                        }
                    }
                }
            }
        }
            break;
        case EventKeyboard::KeyCode::KEY_H:
            CropSystem::getInstance()->harvestTile(tile);
            break;
        case EventKeyboard::KeyCode::KEY_UP_ARROW:
            if (_isDebugMode && GameScene::sClock)
            {
                GameScene::sClock->addDay(1);
                CropSystem::getInstance()->updateDailyGrowth();
                ForageSystem::getInstance()->newDay(GameScene::sClock->getSeason());
                AnimalSystem::getInstance()->updateDailyGrowth();
                if (GameScene::sHud)
                {
                    int earned = GameScene::sHud->settleBucketAndGetTotal();
                    GameScene::sTodayEarnings = earned;
                }
            }
            break;
        case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
            if (_isDebugMode && GameScene::sClock)
            {
                GameScene::sClock->addDay(-1);
                CropSystem::getInstance()->updateDailyGrowth();
                ForageSystem::getInstance()->newDay(GameScene::sClock->getSeason());
                AnimalSystem::getInstance()->updateDailyGrowth();
            }
            break;
        case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
            if (_isDebugMode && GameScene::sClock)
            {
                GameScene::sClock->addHour(-1);
                CropSystem::getInstance()->updateDailyGrowth();
            }
            break;
        case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
            if (_isDebugMode && GameScene::sClock)
            {
                GameScene::sClock->addHour(1);
                CropSystem::getInstance()->updateDailyGrowth();
            }
            break;
        default:
            break;
        }
    }

    if (_player)
    {
        bool blockInput = false;
        if (_isDebugMode)
        {
            if (keyCode == EventKeyboard::KeyCode::KEY_UP_ARROW ||
                keyCode == EventKeyboard::KeyCode::KEY_DOWN_ARROW ||
                keyCode == EventKeyboard::KeyCode::KEY_LEFT_ARROW ||
                keyCode == EventKeyboard::KeyCode::KEY_RIGHT_ARROW)
            {
                blockInput = true;
            }
        }

        if (!blockInput)
        {
            _player->onKeyPressed(keyCode);
        }
    }
}

void BackgroundLayer::onKeyReleased(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (PauseLayer::isGamePaused()) return;
    if (_confirmationOverlay) return;

    if (_isFishing)
    {
        if (_fishingGame)
        {
            _fishingGame->onKeyReleased(keyCode);
        }
        return;
    }
    if (_player)
    {
        bool blockInput = false;
        if (_isDebugMode)
        {
            if (keyCode == EventKeyboard::KeyCode::KEY_UP_ARROW ||
                keyCode == EventKeyboard::KeyCode::KEY_DOWN_ARROW ||
                keyCode == EventKeyboard::KeyCode::KEY_LEFT_ARROW ||
                keyCode == EventKeyboard::KeyCode::KEY_RIGHT_ARROW)
            {
                blockInput = true;
            }
        }

        if (!blockInput)
        {
            _player->onKeyReleased(keyCode);
        }
    }
}

void BackgroundLayer::onMouseDown(Event* event)
{
    EventMouse* e = (EventMouse*)event;

    if (handleSleepMouseDown(e))
    {
        return;
    }

    if (PauseLayer::isGamePaused()) return;
    if (_confirmationOverlay) return;
    
    if (e->getMouseButton() == EventMouse::MouseButton::BUTTON_RIGHT)
    {
        if (GameScene::sInventory)
        {
            int slot = GameScene::sInventory->getSelectedSlot();
            if (GameScene::sInventory->hasItem(slot))
            {
                const Item& item = GameScene::sInventory->getItem(slot);
                if (item.type == ItemType::Crop)
                {
                    const CropData* data = CropSystem::getInstance()->getCropData(item.cropType);
                    if (data && data->energyRestore > 0)
                    {
                        std::string confirmMsg = "Eat " + data->itemName + "?";
                        showConfirmationDialog(confirmMsg, [slot, data]() {
                            EnergySystem::getInstance()->restoreEnergy((float)data->energyRestore);
                            GameScene::sInventory->removeItem(slot, 1);
                            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");

                            std::string msg = "You ate " + data->itemName + ". Energy +" + std::to_string(data->energyRestore);
                            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
                        });
                    }
                }
            }
        }
        return;
    }

    if (e->getMouseButton() != EventMouse::MouseButton::BUTTON_LEFT) return;

    Vec2 clickPos = e->getLocation();
    if (GameScene::sHud && (GameScene::sHud->isPointInToolbarWorld(clickPos) || GameScene::sHud->isConsumingClick())) return;
    
    // Check for Shop Interaction in Town (Left Click)
    if (_type == BackgroundType::Town && _hasShopRect && _player)
    {
        Rect box = _player->getBoundingBox();
        if (box.intersectsRect(_shopRect))
        {
             if (!Director::getInstance()->getRunningScene()->getChildByName("ShopLayer"))
             {
                 auto shop = ShopLayer::create(GameScene::sWallet, GameScene::sBasket, CropSystem::getInstance(), GameScene::sInventory);
                 if (shop)
                 {
                     shop->setName("ShopLayer");
                     Director::getInstance()->getRunningScene()->addChild(shop, 2000);
                     _enteredShop = true;
                     return; // Consume click
                 }
             }
        }
    }

    Vec2 targetTile = getFacingTile();
    
    if (!_map) return;
    Size tileSize = _map->getTileSize();
    Size mapSize = _map->getMapSize();
    
    if (targetTile.x < 0 || targetTile.x >= mapSize.width || 
        targetTile.y < 0 || targetTile.y >= mapSize.height) return;

    if (!_facingDebug) {
        _facingDebug = DrawNode::create();
        _map->addChild(_facingDebug, 1000);
    }
    _facingDebug->clear();
    
    float mapHeight = mapSize.height * tileSize.height;
    float cxPos = (targetTile.x + 0.5f) * tileSize.width;
    float cyPos = mapHeight - (targetTile.y + 0.5f) * tileSize.height;
    _facingDebug->drawDot(Vec2(cxPos, cyPos), 5.0f, Color4F::GREEN);

    if (_type == BackgroundType::Home)
    {
        if (_hasBedRect && _map && _groundLayer && _player)
        {
            Size tileSize2 = _map->getTileSize();
            Vec2 tilePos = _groundLayer->getPositionAt(targetTile);
            Rect facingRect(tilePos.x, tilePos.y, tileSize2.width, tileSize2.height);
            if (facingRect.intersectsRect(_bedRect))
            {
                showSleepDialog();
                return;
            }
        }
        
        if (_canExitHomeDoor && !_exitedHomeDoor && _map && _groundLayer)
        {
            _exitedHomeDoor = true;
            GameScene::sSpawnAtFarmStart = true;
            auto next = GameScene::createScene(BackgroundType::Farm);
            if (next)
            {
                auto trans = TransitionFade::create(0.5f, next);
                Director::getInstance()->replaceScene(trans);
                return;
            }
        }
        
        return;
    }
    
    if (_type == BackgroundType::Farm)
    {
        if (_canEnterHomeDoor && !_enteredHome && _map && _groundLayer && _player)
        {
            _enteredHome = true;
            GameScene::sLastFarmPlayerPos = _player->getPosition();
            GameScene::sHasLastFarmPlayerPos = true;
            auto next = GameScene::createScene(BackgroundType::Home);
            if (next)
            {
                auto trans = TransitionFade::create(0.5f, next);
                Director::getInstance()->replaceScene(trans);
                return;
            }
        }

        if (_canEnterHenhouse && !_enteredHenhouse && _map && _groundLayer && _player)
        {
            _enteredHenhouse = true;
            GameScene::sLastFarmPlayerPos = _player->getPosition();
            GameScene::sHasLastFarmPlayerPos = true;
            auto next = GameScene::createScene(BackgroundType::Henhouse);
            if (next)
            {
                auto trans = TransitionFade::create(0.5f, next);
                Director::getInstance()->replaceScene(trans);
                return;
            }
        }

        if (_canEnterTownFromRight && !_exitedRight && _map && _groundLayer && _player)
        {
            _exitedRight = true;
            GameScene::sLastFarmPlayerPos = _player->getPosition();
            GameScene::sHasLastFarmPlayerPos = true;
            GameScene::switchViaRightExit(0.5f);
            return;
        }

        if (_hasBucket && _map && _groundLayer)
        {
            Size tileSize2 = _map->getTileSize();
            Vec2 tilePos = _groundLayer->getPositionAt(targetTile);
            Rect facingRect(tilePos.x, tilePos.y, tileSize2.width, tileSize2.height);
            if (facingRect.intersectsRect(_bucketRect))
            {
                if (GameScene::sHud)
                {
                    GameScene::sHud->openBucketWindow();
                }
                return;
            }
        }
    }

    const Item* item = GameScene::sInventory ? GameScene::sInventory->getSelectedItem() : nullptr;
    if (_type == BackgroundType::Town)
    {
        if (_canReturnFarmFromTown && _hasTownHomewayRect && _map && _groundLayer && _player)
        {
            if (GameScene::sHasFarmTownwayPos)
            {
                GameScene::sLastFarmPlayerPos = GameScene::sFarmTownwayPos;
                GameScene::sHasLastFarmPlayerPos = true;
                GameScene::sSpawnAtFarmStart = false;
            }
            auto next = GameScene::createScene(BackgroundType::Farm);
            if (next)
            {
                auto trans = TransitionFade::create(0.5f, next);
                Director::getInstance()->replaceScene(trans);
                return;
            }
        }
    }

    if (_type == BackgroundType::Henhouse)
    {
        if (_canEnterHenhouse && _hasHenhouseDoor && _map && _groundLayer && _player)
        {
            if (GameScene::sHasFarmHenhouseOutPos)
            {
                GameScene::sLastFarmPlayerPos = GameScene::sFarmHenhouseOutPos;
                GameScene::sHasLastFarmPlayerPos = true;
                GameScene::sSpawnAtFarmStart = false;
            }
            else if (GameScene::sHasFarmHenhouseDoorPos)
            {
                GameScene::sLastFarmPlayerPos = GameScene::sFarmHenhouseDoorPos;
                GameScene::sHasLastFarmPlayerPos = true;
                GameScene::sSpawnAtFarmStart = false;
            }
            auto next = GameScene::createScene(BackgroundType::Farm);
            if (next)
            {
                auto trans = TransitionFade::create(0.5f, next);
                Director::getInstance()->replaceScene(trans);
                return;
            }
        }
        
        if (item)
        {
            if (item->name.find("Egg") != std::string::npos)
            {
                 if (AnimalSystem::getInstance()->tryIncubateEgg(targetTile, item->name))
                 {
                     int slot = GameScene::sInventory->getSelectedSlot();
                     GameScene::sInventory->removeItem(slot, 1);
                     Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
                     return;
                 }
            }
        }
        
        if (item && item->name == "Hay")
        {
            if (AnimalSystem::getInstance()->tryDepositHay(targetTile))
            {
                 int slot = GameScene::sInventory->getSelectedSlot();
                 GameScene::sInventory->removeItem(slot, 1);
                 Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
                 return;
            }
            if (AnimalSystem::getInstance()->tryPlaceHay(targetTile))
            {
                 int slot = GameScene::sInventory->getSelectedSlot();
                 GameScene::sInventory->removeItem(slot, 1);
                 Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
                 return;
            }
        }
        else
        {
            if (AnimalSystem::getInstance()->tryWithdrawHay(targetTile))
            {
                return;
            }
        }
    }

    if (_type == BackgroundType::Farm)
    {
        std::string harvestedIcon;
        if (AnimalSystem::getInstance()->tryHarvestProduct(targetTile, &harvestedIcon))
        {
             if (_player && !harvestedIcon.empty()) 
             {
                 _player->showToolFeedback(harvestedIcon);
             }
             return;
        }

        std::string forageIcon = "";
        const ForageItem* fItem = ForageSystem::getInstance()->getItemAt(targetTile);
        if (fItem) {
            auto data = CropSystem::getInstance()->getCropData(fItem->type);
            if (data) forageIcon = data->itemIcon;
        }

        if (ForageSystem::getInstance()->tryHarvest(targetTile))
        {
            if (!forageIcon.empty() && _player) _player->showToolFeedback(forageIcon);
            return;
        }
    }

    if (!GameScene::sClock) return;
    item = GameScene::sInventory->getSelectedItem();
    
    if (_type == BackgroundType::Farm && _hasPoolRect && _map && _groundLayer && _player)
    {
        if (item && item->type == ItemType::Tool && item->toolType == ToolType::FishingRod)
        {
            Size tileSize2 = _map->getTileSize();
            Vec2 tilePos = _groundLayer->getPositionAt(targetTile);
            Rect facingRect(tilePos.x, tilePos.y, tileSize2.width, tileSize2.height);

            bool inPool = false;
            for (const auto& r : _poolRects)
            {
                if (facingRect.intersectsRect(r))
                {
                    inPool = true;
                    break;
                }
            }

            if (inPool)
            {
                startFishing();
                return;
            }
        }
    }

    std::string cropIcon = "";
    const CropInstance* cInst = CropSystem::getInstance()->getCropAt(targetTile);
    if (cInst) {
        auto data = CropSystem::getInstance()->getCropData(cInst->type);
        if (data) cropIcon = data->itemIcon;
    }

    if (CropSystem::getInstance()->harvestTile(targetTile))
    {
        if (!cropIcon.empty() && _player) _player->showToolFeedback(cropIcon);
        return;
    }
    
    if (!item) return;

    if (item->type == ItemType::Tool && _player)
    {
         _player->showToolFeedback(item->iconPath);
    }

    if (hasObstacle(targetTile))
    {
        int obsType = getObstacleType(targetTile);
        bool removed = false;
        if (item->type == ItemType::Tool)
        {
            if (obsType == 0 && item->toolType == ToolType::Axe) removed = true;
            else if (obsType == 1 && item->toolType == ToolType::Pickaxe) removed = true;
            else if (obsType == 2 && item->toolType == ToolType::Scythe) 
            {
                removed = true;
                if (GameScene::sInventory) {
                    Item hay;
                    hay.type = ItemType::Resource;
                    hay.name = "Hay";
                    hay.iconPath = "block/Hay.png";
                    hay.quantity = 1;
                    hay.maxStack = 99;
                    GameScene::sInventory->addItem(hay);
                    Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
                    
                    if (_player) {
                         _player->showToolFeedback("block/Hay.png");
                    }
                }
            }
        }
        
        if (removed)
        {
            removeObstacle(targetTile);
            if (item->toolType == ToolType::Axe)
            {
                float cost = EnergySystem::COST_AXE - (item->toolLevel * 0.5f);
                if (cost < 0.5f) cost = 0.5f;
                EnergySystem::getInstance()->consumeEnergy(cost);
            }
            else if (item->toolType == ToolType::Pickaxe)
            {
                float cost = EnergySystem::COST_PICKAXE - (item->toolLevel * 0.5f);
                if (cost < 0.5f) cost = 0.5f;
                EnergySystem::getInstance()->consumeEnergy(cost);
            }
            return;
        }
        else
        {
            return;
        }
    }

    if (item->type == ItemType::Tool)
    {
        switch (item->toolType)
        {
        case ToolType::Hoe:
            CropSystem::getInstance()->tillTile(targetTile);
            {
                 float cost = EnergySystem::COST_HOE - (item->toolLevel * 0.5f);
                 if (cost < 0.5f) cost = 0.5f;
                 EnergySystem::getInstance()->consumeEnergy(cost);
            }
            break;
        case ToolType::WateringCan:
        {
            Size tileSize2 = _map->getTileSize();
            Vec2 tilePos = _groundLayer->getPositionAt(targetTile);
            Rect facingRect(tilePos.x, tilePos.y, tileSize2.width, tileSize2.height);
            bool inPool = false;
            for (const auto& r : _poolRects)
            {
                if (facingRect.intersectsRect(r))
                {
                    inPool = true;
                    break;
                }
            }

            Item& mutableItem = GameScene::sInventory->getItem(GameScene::sInventory->getSelectedSlot());
            if (inPool)
            {
                mutableItem.currentWater = mutableItem.maxWater;
                EnergySystem::getInstance()->consumeEnergy(4.0f);
                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
                std::string msg = "Watering Can Refilled";
                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
            }
            else
            {
                if (mutableItem.currentWater > 0)
                {
                     if (CropSystem::getInstance()->canWater(targetTile))
                     {
                         CropSystem::getInstance()->waterTile(targetTile);
                         float cost = EnergySystem::COST_WATERING_CAN - (mutableItem.toolLevel * 0.5f);
                         if (cost < 0.5f) cost = 0.5f;
                         EnergySystem::getInstance()->consumeEnergy(cost);
                         mutableItem.currentWater -= 1.0f;
                         Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
                     }
                }
                else
                {
                    std::string msg = "Watering Can is empty!";
                    Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
                }
            }
        }
            break;
        case ToolType::Scythe:
            CropSystem::getInstance()->removeWithered(targetTile);
            break;
        case ToolType::Pickaxe:
            CropSystem::getInstance()->destroyTile(targetTile);
            {
                 float cost = EnergySystem::COST_PICKAXE - (item->toolLevel * 0.5f);
                 if (cost < 0.5f) cost = 0.5f;
                 EnergySystem::getInstance()->consumeEnergy(cost);
            }
            break;
        default:
            break;
        }
    }
    else if (item->type == ItemType::Seed)
    {
        CropSystem::getInstance()->setSelectedCrop(item->cropType);
        
        if (CropSystem::getInstance()->plantSelected(targetTile))
        {
            if (_player)
            {
                _player->showToolFeedback(item->iconPath);
            }

            int slot = GameScene::sInventory->getSelectedSlot();
            GameScene::sInventory->removeItem(slot, 1);
            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
        }
    }
}

