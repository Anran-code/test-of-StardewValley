#ifndef __FORAGE_SYSTEM_H__
#define __FORAGE_SYSTEM_H__

#include "cocos2d.h"
#include "CropSystem.h" // For CropType
#include "GameClock.h"
#include <vector>

// Forward declaration
class BackgroundLayer;

struct ForageItem {
    CropType type;
    cocos2d::Vec2 tilePosition;
    cocos2d::Sprite* sprite;
};

class ForageSystem {
public:
    static ForageSystem* getInstance();

    // Initialize with the current map layer (Farm)
    void init(BackgroundLayer* layer);
    
    // Call this when a new day starts
    void newDay(GameClock::Season season);
    
    // Call this when player clicks/interacts
    bool tryHarvest(const cocos2d::Vec2& tilePos);

    // Check if there is an item at the given position
    bool hasItem(const cocos2d::Vec2& tilePos) const;

    // Clear items (e.g. when leaving scene)
    void clearVisuals();
    
    // Clear the layer reference
    void detachLayer();

    // Update loop to check for day changes
    void update(GameClock* clock);

private:
    ForageSystem();
    static ForageSystem* sInstance;

    BackgroundLayer* _layer;
    std::vector<ForageItem> _items;
    bool _pendingSpawn;
    GameClock::Season _pendingSeason;
    int _lastDay;

    std::vector<CropType> getForageTypesForSeason(GameClock::Season season);
    bool isValidTile(const cocos2d::Vec2& tilePos);
};

#endif
