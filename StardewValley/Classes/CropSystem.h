#ifndef __CROP_SYSTEM_H__
#define __CROP_SYSTEM_H__

#include "cocos2d.h"
#include "GameClock.h"
#include "Wallet.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

enum class CropType
{
    Parsnip,
    Cauliflower,
    Potato
};

struct CropData
{
    int growthDays;
    int sellPrice;
    std::string seedlingSprite;
    std::vector<std::string> sproutSprites;
    std::string matureSprite;
    std::vector<int> sproutThresholdDays;
    std::vector<GameClock::Season> allowedSeasons;
    
    // For Inventory
    std::string itemName;
    std::string itemIcon;
};

struct CropInstance
{
    CropType type;
    int daysWatered;
    int stageIndex;
    cocos2d::Sprite* sprite;
    bool withered;
};

struct TileSlot
{
    bool tilled;
    bool watered;
    std::unique_ptr<CropInstance> crop;
};

class GameClock;
class Wallet;
class Inventory;
// class Basket;

class CropSystem
{
public:
    static CropSystem* getInstance();

    void init(cocos2d::TMXTiledMap* map, GameClock* clock, Wallet* wallet, Inventory* inventory);
    // void setBasket(Basket* basket);
    void setSelectedCrop(CropType type);
    void tillTile(const cocos2d::Vec2& tileIndex);
    bool plantSelected(const cocos2d::Vec2& tileIndex);
    void waterTile(const cocos2d::Vec2& tileIndex);
    bool harvestTile(const cocos2d::Vec2& tileIndex);
    bool removeWithered(const cocos2d::Vec2& tileIndex);
    void updateDailyGrowth();
    int getSellPrice(CropType type) const;

private:
    static CropSystem* sInstance;
    CropSystem();

    cocos2d::TMXTiledMap* _map;
    cocos2d::TMXLayer* _groundLayer;
    GameClock* _clock;
    Wallet* _wallet;
    Inventory* _inventory;
    // Basket* _basket;
    CropType _selected;
    int _lastProcessedDay;
    int _lastProcessedSeason;
    std::vector<std::vector<TileSlot>> _tiles;
    std::unordered_map<CropType, CropData> _data;

    void loadCropData();
    void ensureGridSize();
    void placeOrUpdateSprite(const cocos2d::Vec2& tileIndex, CropInstance* inst);
    void fitSpriteToTile(cocos2d::Sprite* sprite);
    cocos2d::Vec2 tileBottomCenterWorld(const cocos2d::Vec2& tileIndex) const;
    bool inBounds(const cocos2d::Vec2& tileIndex) const;
    void darkenTile(const cocos2d::Vec2& tileIndex);
    void waterTintTile(const cocos2d::Vec2& tileIndex);
    void resetTileColor(const cocos2d::Vec2& tileIndex);
    bool isSeasonAllowed(CropType type, GameClock::Season s) const;
};

#endif
