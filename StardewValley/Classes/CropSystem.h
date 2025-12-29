#ifndef __CROP_SYSTEM_H__
#define __CROP_SYSTEM_H__

#include "cocos2d.h"
#include "GameClock.h"
#include "Wallet.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

enum class CropType
{
    Parsnip,
    Cauliflower,
    Potato,
    Blueberry,
    Melon,
    Starfruit,
    Pumpkin,
    Eggplant,
    Yam,
    Powdermelon,
    Fish,
    Anchovy,
    Bream,
    LargemouthBass,
    // Animal Products
    Egg,
    LargeEgg,
    Wool,
    RabbitsFoot,
    // Spring Forage
    Daffodil,
    Leek,
    WildHorseradish,
    // Summer Forage
    Grape,
    SpiceBerry,
    SweetPea,
    // Fall Forage
    Blackberry,
    CommonMushroom,
    WildPlum,
    // Winter Forage
    CrystalFruit,
    SnowYam,
    WinterRoot
};

struct CropData
{
    int growthDays;
    int sellPrice;
    int xp; // Added XP field
    int regrowDays; // 0 if no regrowth
    int baseYield; // default 1
    float extraYieldChance; // chance for +1 yield
    std::string seedlingSprite;
    std::vector<std::string> sproutSprites;
    std::string matureSprite;
    std::vector<int> sproutThresholdDays;
    std::vector<GameClock::Season> allowedSeasons;
    
    // For Inventory
    std::string itemName;
    std::string itemIcon;
    int energyRestore; // Energy restored when consumed
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
class Basket;

class CropSystem
{
public:
    static CropSystem* getInstance();

    void init(cocos2d::TMXTiledMap* map, GameClock* clock, Wallet* wallet, Inventory* inventory, bool isFarmMap = true);
    void setMap(cocos2d::TMXTiledMap* map, bool isFarmMap = true); // New method to handle map switching
    void setBasket(Basket* basket);
    void setSelectedCrop(CropType type);
    void onTileClicked(const cocos2d::Vec2& tileIndex);
    void tillTile(const cocos2d::Vec2& tileIndex);
    bool plantSelected(const cocos2d::Vec2& tileIndex);
    void waterTile(const cocos2d::Vec2& tileIndex);
    bool harvestTile(const cocos2d::Vec2& tileIndex);
    bool removeWithered(const cocos2d::Vec2& tileIndex);
    bool destroyTile(const cocos2d::Vec2& tileIndex);
    
    // Check methods for UI feedback
    bool canTill(const cocos2d::Vec2& tileIndex) const;
    bool isOccupied(const cocos2d::Vec2& tileIndex) const;
    bool canPlant(const cocos2d::Vec2& tileIndex, CropType type) const;
    bool canWater(const cocos2d::Vec2& tileIndex) const;
    bool canHarvest(const cocos2d::Vec2& tileIndex) const;
    bool canClearWithered(const cocos2d::Vec2& tileIndex) const;
    bool canDestroy(const cocos2d::Vec2& tileIndex) const;

    void updateDailyGrowth();
    int getSellPrice(CropType type) const;
    const CropData* getCropData(CropType type) const;
    const CropInstance* getCropAt(const cocos2d::Vec2& tileIndex) const;
    int sellBasket();

private:
    static CropSystem* sInstance;
    CropSystem();

    cocos2d::TMXTiledMap* _map;
    cocos2d::TMXLayer* _groundLayer;
    GameClock* _clock;
    Wallet* _wallet;
    Inventory* _inventory;
    Basket* _basket;
    CropType _selected;
    int _lastProcessedDay;
    int _lastProcessedSeason;
    std::vector<std::vector<TileSlot>> _tiles;
    std::unordered_map<CropType, CropData> _data;
    std::unordered_set<int> _activeTileIndices;
    bool _isFarmMap = true;

    void loadCropData();
    void ensureGridSize();
    void markActive(const cocos2d::Vec2& tileIndex);
    void unmarkActive(const cocos2d::Vec2& tileIndex);
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
