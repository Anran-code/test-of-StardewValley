#include "ForageSystem.h"
#include "GameScene.h"
#include "Inventory.h"
#include "Item.h"
#include "ExperienceSystem.h"

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

ForageSystem::ForageSystem() : _layer(nullptr), _lastDay(-1)
{
}

void ForageSystem::update(GameClock* clock)
{
    if (!clock) return;
    int currentDay = clock->getDay();
    
    // Check if new day
    if (_lastDay != currentDay)
    {
        newDay(clock->getSeason());
        _lastDay = currentDay;
    }

    // Check if we need to spawn for the CURRENT layer (e.g. walked into Farm)
    if (_layer)
    {
        BackgroundType type = _layer->getType();
        if (isOutdoors(type))
        {
            if (_spawnedDays.find(type) == _spawnedDays.end() || _spawnedDays[type] != currentDay)
            {
                spawnForMap(_layer);
                _spawnedDays[type] = currentDay;
            }
        }
    }
}

void ForageSystem::init(BackgroundLayer* layer)
{
    _layer = layer;
    
    // Restore visuals for existing items matching THIS map
    BackgroundType currentType = layer->getType();
    
    for (auto& item : _items)
    {
        // Remove old sprite ref if it exists (it might be from previous scene)
        if (item.sprite)
        {
            item.sprite->removeFromParent();
            item.sprite = nullptr;
        }
        
        // Only render if it belongs to this map
        if (item.mapType == currentType)
        {
            auto data = CropSystem::getInstance()->getCropData(item.type);
            item.sprite = Sprite::create(data->itemIcon);
            if (item.sprite)
            {
                if (_layer && _layer->getMap())
                {
                    Size tileSize = _layer->getMap()->getTileSize();
                    float mapHeight = _layer->getMap()->getMapSize().height * tileSize.height;
                    float cx = (item.tilePosition.x + 0.5f) * tileSize.width;
                    float cy = mapHeight - (item.tilePosition.y + 0.5f) * tileSize.height;
                    item.sprite->setPosition(Vec2(cx, cy));
                    
                    // Fixed Scale Logic: Fit to tile
                    if (item.sprite->getContentSize().width > tileSize.width)
                    {
                        item.sprite->setScale(tileSize.width / item.sprite->getContentSize().width);
                    }
                    else
                    {
                         // Optional: scale up slightly if too small? 
                         // But for consistency with newDay, let's keep it simple.
                         // Stardew items are usually 16x16, tiles are 16x16 (scaled).
                         // If sprite is huge (high res), we scale down.
                    }

                    _layer->addChild(item.sprite, (int)(mapHeight - cy)); // Z-order
                }
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
    // Clear ALL items from previous day (Daily Reset)
    clearVisuals();
    _items.clear();
    // _spawnedDays will be updated as we visit maps
    // But we need to reset it?
    // Actually, if we clear items, we should clear _spawnedDays too, 
    // so that when we visit a map, it spawns NEW items.
    _spawnedDays.clear();
}

void ForageSystem::spawnForMap(BackgroundLayer* layer)
{
    if (!layer) return;
    
    auto map = layer->getMap();
    if (!map) return;
    
    BackgroundType mapType = layer->getType();
    
    // Get season from clock via GameScene (global access)
    // We assume season matches current clock
    GameClock::Season season = GameScene::sClock ? GameScene::sClock->getSeason() : GameClock::Season::Spring;

    auto types = getForageTypesForSeason(season);
    if (types.empty()) return;

    Size mapSize = map->getMapSize();
    Size tileSize = map->getTileSize();
    float mapHeight = mapSize.height * tileSize.height;
    
    // Spawn 6-10 items per map
    int count = RandomHelper::random_int(6, 10);
    
    for (int i = 0; i < count; ++i)
    {
        // Try to find a valid position (limit attempts)
        for (int attempt = 0; attempt < 20; ++attempt)
        {
            int x = RandomHelper::random_int(0, (int)mapSize.width - 1);
            int y = RandomHelper::random_int(0, (int)mapSize.height - 1);
            Vec2 tilePos((float)x, (float)y);
            
            // Check if valid spawn position on THIS layer
            if (layer->isValidSpawnPosition(x, y))
            {
                // Check if we already have an item here (on this map)
                bool alreadyHasItem = false;
                for (const auto& existing : _items)
                {
                    if (existing.mapType == mapType && existing.tilePosition == tilePos)
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
                    layer->addChild(sprite, zOrder);
                    
                    ForageItem newItem;
                    newItem.type = type;
                    newItem.tilePosition = tilePos;
                    newItem.sprite = sprite;
                    newItem.mapType = mapType; // Set map type
                    _items.push_back(newItem);
                    
                    break; // Success for this item
                }
            }
        }
    }
}

bool ForageSystem::tryHarvest(const cocos2d::Vec2& tilePos)
{
    if (!_layer) return false;
    BackgroundType currentMapType = _layer->getType();

    for (auto it = _items.begin(); it != _items.end(); ++it)
    {
        // Must match current map AND position
        if (it->mapType == currentMapType && it->tilePosition == tilePos)
        {
            // Found item
            auto data = CropSystem::getInstance()->getCropData(it->type);
            
            // Add to inventory
            if (GameScene::sInventory)
            {
                Item item = Item::createCrop(it->type, data->itemName, data->itemIcon, 1);
                GameScene::sInventory->addItem(item);
                
                // Gain Experience
                ExperienceSystem::getInstance()->addExperience(SkillType::Foraging, data->xp);
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
    if (!_layer) return false;
    BackgroundType currentMapType = _layer->getType();

    for (const auto& item : _items)
    {
        if (item.mapType == currentMapType && item.tilePosition == tilePos)
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

bool ForageSystem::isOutdoors(BackgroundType type)
{
    return (type == BackgroundType::Farm || 
            type == BackgroundType::Town || 
            type == BackgroundType::Path);
}
