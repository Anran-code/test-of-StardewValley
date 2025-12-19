#include "ForageSystem.h"
#include "GameScene.h"
#include "Inventory.h"
#include "Item.h"

USING_NS_CC;

ForageSystem* ForageSystem::sInstance = nullptr;

ForageSystem* ForageSystem::getInstance()
{
    if (!sInstance)
    {
        sInstance = new ForageSystem();
    }
    return sInstance;
}

ForageSystem::ForageSystem() : _layer(nullptr), _pendingSpawn(false), _pendingSeason(GameClock::Season::Spring), _lastDay(-1)
{
}

void ForageSystem::update(GameClock* clock)
{
    if (!clock) return;
    int currentDay = clock->getDay();
    
    // On first update, if day is different from -1, it triggers newDay.
    // This ensures items spawn on game start.
    if (_lastDay != currentDay)
    {
        // Only trigger if we have moved to a new day (or first day)
        newDay(clock->getSeason());
        _lastDay = currentDay;
    }
}

void ForageSystem::init(BackgroundLayer* layer)
{
    _layer = layer;
    
    if (_pendingSpawn)
    {
        _items.clear();
        newDay(_pendingSeason);
        _pendingSpawn = false;
        return;
    }
    
    // Restore visuals for existing items
    for (auto& item : _items)
    {
        // If sprite exists (shouldn't if cleared), remove it
        if (item.sprite)
        {
            item.sprite->removeFromParent();
            item.sprite = nullptr;
        }
        
        auto data = CropSystem::getInstance()->getCropData(item.type);
        item.sprite = Sprite::create(data->itemIcon);
        if (item.sprite)
        {
            // Position logic (same as spawn)
            if (_layer && _layer->getMap())
            {
                Size tileSize = _layer->getMap()->getTileSize();
                float mapHeight = _layer->getMap()->getMapSize().height * tileSize.height;
                float cx = (item.tilePosition.x + 0.5f) * tileSize.width;
                float cy = mapHeight - (item.tilePosition.y + 0.5f) * tileSize.height;
                item.sprite->setPosition(Vec2(cx, cy));
                item.sprite->setScale(2.0f); // Make them visible
                _layer->addChild(item.sprite, (int)(mapHeight - cy)); // Z-order
            }
        }
    }
}

void ForageSystem::clearVisuals()
{
    for (auto& item : _items)
    {
        if (item.sprite)
        {
            item.sprite->removeFromParent();
            item.sprite = nullptr;
        }
    }
}

void ForageSystem::detachLayer()
{
    clearVisuals();
    _layer = nullptr;
}

void ForageSystem::newDay(GameClock::Season season)
{
    // Clear previous day's items completely
    clearVisuals();
    
    if (!_layer)
    {
        _pendingSpawn = true;
        _pendingSeason = season;
        return;
    }
    
    _items.clear();
    
    auto types = getForageTypesForSeason(season);
    if (types.empty()) return;
    
    auto map = _layer->getMap();
    if (!map) return;
    
    Size mapSize = map->getMapSize();
    Size tileSize = map->getTileSize();
    float mapHeight = mapSize.height * tileSize.height;
    
    // Spawn 6-10 items
    int count = RandomHelper::random_int(6, 10);
    
    for (int i = 0; i < count; ++i)
    {
        // Try to find a valid position (limit attempts)
        for (int attempt = 0; attempt < 20; ++attempt)
        {
            int x = RandomHelper::random_int(0, (int)mapSize.width - 1);
            int y = RandomHelper::random_int(0, (int)mapSize.height - 1);
            Vec2 tilePos((float)x, (float)y);
            
            if (isValidTile(tilePos))
            {
                // Check if we already have an item here
                bool alreadyHasItem = false;
                for (const auto& existing : _items)
                {
                    if (existing.tilePosition == tilePos)
                    {
                        alreadyHasItem = true;
                        break;
                    }
                }
                if (alreadyHasItem) continue;
                
                // Spawn!
                CropType type = types[RandomHelper::random_int(0, (int)types.size() - 1)];
                auto data = CropSystem::getInstance()->getCropData(type);
                
                cocos2d::Sprite* sprite = cocos2d::Sprite::create(data->itemIcon);
                if (sprite)
                {
                    float cxPos = (x + 0.5f) * tileSize.width;
                    float cyPos = mapHeight - (y + 0.5f) * tileSize.height;
                    
                    sprite->setPosition(Vec2(cxPos, cyPos));
                     
                    if (sprite->getContentSize().width > tileSize.width)
                    {
                        sprite->setScale(tileSize.width / sprite->getContentSize().width);
                    }
                     
                    int zOrder = static_cast<int>(mapHeight - cyPos);
                    _layer->addChild(sprite, zOrder);
                    
                    ForageItem newItem;
                    newItem.type = type;
                    newItem.tilePosition = tilePos;
                    newItem.sprite = sprite;
                    _items.push_back(newItem);
                    
                    break; // Success for this item
                }
            }
        }
    }
}

bool ForageSystem::tryHarvest(const cocos2d::Vec2& tilePos)
{
    for (auto it = _items.begin(); it != _items.end(); ++it)
    {
        if (it->tilePosition == tilePos)
        {
            // Found item
            auto data = CropSystem::getInstance()->getCropData(it->type);
            
            // Add to inventory
            if (GameScene::sInventory)
            {
                Item item = Item::createCrop(it->type, data->itemName, data->itemIcon, 1);
                GameScene::sInventory->addItem(item);
                
                // Optional: Play sound
                // CocosDenshion::SimpleAudioEngine::getInstance()->playEffect("pickup.wav");
            }
            
            // Remove visual
            if (it->sprite)
            {
                it->sprite->removeFromParent();
            }
            
            // Remove from list
            _items.erase(it);
            return true;
        }
    }
    return false;
}

bool ForageSystem::hasItem(const cocos2d::Vec2& tilePos) const
{
    for (const auto& item : _items)
    {
        if (item.tilePosition == tilePos)
        {
            return true;
        }
    }
    return false;
}

std::vector<CropType> ForageSystem::getForageTypesForSeason(GameClock::Season season)
{
    std::vector<CropType> list;
    switch (season)
    {
    case GameClock::Season::Spring:
        list = { CropType::Daffodil, CropType::Leek, CropType::WildHorseradish, CropType::Daffodil }; // Weighted
        break;
    case GameClock::Season::Summer:
        list = { CropType::Grape, CropType::SpiceBerry, CropType::SweetPea };
        break;
    case GameClock::Season::Fall:
        list = { CropType::Blackberry, CropType::CommonMushroom, CropType::WildPlum };
        break;
    case GameClock::Season::Winter:
        list = { CropType::CrystalFruit, CropType::SnowYam, CropType::WinterRoot };
        break;
    }
    return list;
}

bool ForageSystem::isValidTile(const cocos2d::Vec2& tilePos)
{
    if (!_layer) return false;
    return _layer->isValidSpawnPosition((int)tilePos.x, (int)tilePos.y);
}
