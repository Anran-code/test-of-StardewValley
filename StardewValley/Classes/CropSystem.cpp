#include "CropSystem.h"
#include "cocos2d.h"
#include "Inventory.h"
#include "ExperienceSystem.h"
// #include "Basket.h"

using namespace cocos2d;

CropSystem* CropSystem::sInstance = nullptr;

CropSystem* CropSystem::getInstance()
{
    if (!sInstance) sInstance = new CropSystem();
    return sInstance;
}

CropSystem::CropSystem()
    : _map(nullptr)
    , _groundLayer(nullptr)
    , _clock(nullptr)
    , _wallet(nullptr)
    , _inventory(nullptr)

    , _selected(CropType::Parsnip)
    , _lastProcessedDay(-1)
    , _lastProcessedSeason(-1)
{
}

void CropSystem::init(TMXTiledMap* map, GameClock* clock, Wallet* wallet, Inventory* inventory)
{
    if (map) setMap(map);
    
    if (clock) _clock = clock;
    if (wallet) _wallet = wallet;
    if (inventory) _inventory = inventory;
    loadCropData();
    
    // Only initialize day/season tracking if not yet set
    if (_clock && _lastProcessedDay == -1)
    {
        _lastProcessedDay = _clock->getDay();
        _lastProcessedSeason = (int)_clock->getSeason();
    }
}

void CropSystem::setMap(TMXTiledMap* map)
{
    if (_map == map) return;

    // Clear sprite pointers as they belong to the old map
    if (!_tiles.empty())
    {
        for (auto& row : _tiles)
        {
            for (auto& slot : row)
            {
                if (slot.crop)
                {
                    slot.crop->sprite = nullptr;
                }
            }
        }
    }

    _map = map;
    _groundLayer = nullptr;

    if (_map)
    {
        _groundLayer = _map->getLayer("ground");
        if (!_groundLayer && _map->getChildrenCount() > 0)
        {
            _groundLayer = dynamic_cast<TMXLayer*>(_map->getChildren().at(0));
        }
        ensureGridSize();
    }
}



void CropSystem::setSelectedCrop(CropType type)
{
    _selected = type;
}

bool CropSystem::isOccupied(const Vec2& tileIndex) const
{
    if (!inBounds(tileIndex)) return true; // Out of bounds is "occupied"
    const auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    return slot.tilled || slot.crop;
}

bool CropSystem::canTill(const Vec2& tileIndex) const
{
    if (!inBounds(tileIndex)) return false;

    // Check ground tile ID for valid soil (yellow plots)
    if (_groundLayer)
    {
        uint32_t gid = _groundLayer->getTileGIDAt(tileIndex);
        // Remove flags (flipped/rotated)
        gid &= 0x1FFFFFFF;
        
        // 228 and 227 are the yellow dirt tiles
        if (gid != 228 && gid != 227)
        {
            return false;
        }
    }

    const auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    // Can till if not already tilled and no crop (though crop usually implies tilled)
    // Also usually need to check for obstacles, but obstacles are handled in HomeScene.
    // CropSystem only cares about soil state.
    return !slot.tilled && !slot.crop;
}

void CropSystem::tillTile(const Vec2& tileIndex)
{
    if (!canTill(tileIndex)) return;
    
    auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    slot.tilled = true;
    darkenTile(tileIndex);
    markActive(tileIndex);
}

bool CropSystem::canPlant(const Vec2& tileIndex, CropType type) const
{
    if (!inBounds(tileIndex)) return false;
    const auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    if (!slot.tilled) return false;
    if (slot.crop) return false;

    // Check season
    if (!isSeasonAllowed(type, _clock->getSeason()))
    {
        return false;
    }
    return true;
}

bool CropSystem::plantSelected(const Vec2& tileIndex)
{
    if (!canPlant(tileIndex, _selected)) return false;

    auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    const auto& d = _data[_selected]; // Use internal map, assume exists if enum valid

    auto inst = std::make_unique<CropInstance>();
    inst->type = _selected;
    inst->daysWatered = 0;
    inst->stageIndex = 0;
    inst->withered = false;
    inst->sprite = Sprite::create(d.seedlingSprite);
    if (!inst->sprite) return false;
    fitSpriteToTile(inst->sprite);
    placeOrUpdateSprite(tileIndex, inst.get());
    _map->addChild(inst->sprite, 50);
    slot.crop = std::move(inst);
    markActive(tileIndex);
    return true;
}

bool CropSystem::canWater(const Vec2& tileIndex) const
{
    if (!inBounds(tileIndex)) return false;
    const auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    return slot.tilled && !slot.watered;
}

void CropSystem::waterTile(const Vec2& tileIndex)
{
    if (!canWater(tileIndex)) return;
    auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    slot.watered = true;
    waterTintTile(tileIndex);
    markActive(tileIndex);
}

bool CropSystem::canHarvest(const Vec2& tileIndex) const
{
    if (!inBounds(tileIndex)) return false;
    const auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    if (!slot.crop) return false;
    
    auto* inst = slot.crop.get();
    if (inst->withered) return false;
    
    // Check if fully grown
    // Need to look up data. _data is member.
    auto it = _data.find(inst->type);
    if (it == _data.end()) return false;
    const auto& d = it->second;
    
    return inst->daysWatered >= d.growthDays;
}

bool CropSystem::harvestTile(const Vec2& tileIndex)
{
    if (!canHarvest(tileIndex)) return false;
    
    auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    auto* inst = slot.crop.get();
    const auto& d = _data[inst->type]; // Safe because canHarvest checked it

    int yieldCount = d.baseYield;
    if (d.extraYieldChance > 0.0f)
    {
        float r = RandomHelper::random_real<float>(0.0f, 1.0f);
        if (r < d.extraYieldChance) yieldCount += 1;
    }
    
    if (_inventory)
    {
        Item item = Item::createCrop(inst->type, d.itemName, d.itemIcon, yieldCount);
        _inventory->addItem(item);
        Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");

        // Add Farming XP
        ExperienceSystem::getInstance()->addExperience(SkillType::Farming, d.xp);
    }
    
    // Regrowth logic
    if (d.regrowDays > 0)
    {
        inst->daysWatered = d.growthDays - d.regrowDays;
        inst->stageIndex = -1; // Force sprite update
        inst->withered = false;
        
        // Update sprite immediately to reflect harvested state
        // We can call updateDailyGrowth logic partially or just leave it for next update?
        // Better to update sprite now so it doesn't look like mature crop until next day
        // We reuse the logic from updateDailyGrowth or just simplified version
        
        int newStage = 0;
        for (int i = 0; i < (int)d.sproutThresholdDays.size(); ++i)
        {
            if (inst->daysWatered >= d.sproutThresholdDays[i]) newStage = i + 1;
        }
        // It won't be mature (>= growthDays)
        
        inst->stageIndex = newStage;
        if (inst->sprite)
        {
            std::string path;
            if (newStage == 0) path = d.seedlingSprite;
            else if (newStage <= (int)d.sproutSprites.size()) path = d.sproutSprites[newStage - 1];
            else path = d.matureSprite; // Should not happen if regrowDays > 0 properly
            
            inst->sprite->setTexture(path);
            fitSpriteToTile(inst->sprite);
            placeOrUpdateSprite(tileIndex, inst);
        }
        
        // Crop stays in slot
        slot.watered = false; // Reset watered state? usually yes.
        unmarkActive(tileIndex); // Unmark? No, it needs to continue growing.
        markActive(tileIndex); // Ensure it's active
    }
    else
    {
        if (inst->sprite)
        {
            inst->sprite->removeFromParent();
        }
        slot.crop.reset();
        unmarkActive(tileIndex);
    }
    
    return true;
}

bool CropSystem::canClearWithered(const Vec2& tileIndex) const
{
    if (!inBounds(tileIndex)) return false;
    const auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    if (!slot.crop) return false;
    return slot.crop->withered;
}

bool CropSystem::removeWithered(const Vec2& tileIndex)
{
    if (!canClearWithered(tileIndex)) return false;
    
    auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    if (slot.crop->sprite)
    {
        slot.crop->sprite->removeFromParent();
    }
    slot.crop.reset();
    
    if (slot.tilled)
    {
        if (slot.watered)
        {
            waterTintTile(tileIndex);
        }
        else
        {
            darkenTile(tileIndex);
        }
    }
    else
    {
        resetTileColor(tileIndex);
    }
    
    return true;
}

bool CropSystem::canDestroy(const Vec2& tileIndex) const
{
    if (!inBounds(tileIndex)) return false;
    const auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    // Can destroy if tilled (regardless of crop or water state)
    return slot.tilled;
}

bool CropSystem::destroyTile(const Vec2& tileIndex)
{
    if (!canDestroy(tileIndex)) return false;
    
    auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    
    // Remove crop if exists
    if (slot.crop)
    {
        if (slot.crop->sprite)
        {
            slot.crop->sprite->removeFromParent();
        }
        slot.crop.reset();
    }
    
    // Untill
    slot.tilled = false;
    slot.watered = false;
    
    // Reset visual
    resetTileColor(tileIndex);
    
    unmarkActive(tileIndex);
    return true;
}

void CropSystem::updateDailyGrowth()
{
    if (!_clock) return;
    int day = _clock->getDay();
    if (_lastProcessedDay == day) return;
    auto season = _clock->getSeason();
    bool seasonChanged = (_lastProcessedSeason != (int)season);

    if (_tiles.empty()) return;
    int width = (int)_tiles.size();

    if (_activeTileIndices.empty())
    {
        _lastProcessedDay = day;
        _lastProcessedSeason = (int)season;
        return;
    }

    std::vector<int> toRemove;
    toRemove.reserve(_activeTileIndices.size());

    for (int key : _activeTileIndices)
    {
        int x = key % width;
        int y = key / width;
        
        if (x < 0 || x >= width || y < 0 || y >= (int)_tiles[0].size()) continue;

        auto& slot = _tiles[x][y];
        bool stillActive = false;
        if (slot.crop)
        {
            auto* inst = slot.crop.get();
            const auto& d = _data[inst->type];
            if (seasonChanged && !isSeasonAllowed(inst->type, season))
            {
                inst->withered = true;
                if (_map && inst->sprite)
                {
                    inst->sprite->setTexture("Crop/Wilted_crop.png");
                    fitSpriteToTile(inst->sprite);
                    placeOrUpdateSprite(Vec2(x, y), inst);
                }
            }
            if (slot.watered)
            {
                if (!inst->withered)
                {
                    inst->daysWatered += 1;
                }
                slot.watered = false;
                darkenTile(Vec2(x, y)); // Safe if _groundLayer is null
            }

            if (!inst->withered)
            {
                int newStage = 0;
                for (int i = 0; i < (int)d.sproutThresholdDays.size(); ++i)
                {
                    if (inst->daysWatered >= d.sproutThresholdDays[i]) newStage = i + 1;
                }
                if (inst->daysWatered >= d.growthDays) newStage = (int)d.sproutSprites.size() + 1;

                if (newStage != inst->stageIndex)
                {
                    inst->stageIndex = newStage;
                    if (_map)
                    {
                        std::string path;
                        if (newStage == 0)
                        {
                            path = d.seedlingSprite;
                        }
                        else if (newStage <= (int)d.sproutSprites.size())
                        {
                            path = d.sproutSprites[newStage - 1];
                        }
                        else
                        {
                            path = d.matureSprite;
                        }
                        
                        if (inst->sprite)
                        {
                            inst->sprite->setTexture(path);
                            fitSpriteToTile(inst->sprite);
                            placeOrUpdateSprite(Vec2(x, y), inst);
                        }
                        else
                        {
                            // Create if missing
                            inst->sprite = Sprite::create(path);
                            if (inst->sprite)
                            {
                                fitSpriteToTile(inst->sprite);
                                placeOrUpdateSprite(Vec2(x, y), inst);
                                _map->addChild(inst->sprite, 50);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (slot.watered)
            {
                slot.watered = false;
                darkenTile(Vec2(x, y));
            }
        }

        if (slot.crop || slot.watered)
        {
            stillActive = true;
        }

        if (!stillActive)
        {
            toRemove.push_back(key);
        }
    }

    for (int key : toRemove)
    {
        _activeTileIndices.erase(key);
    }

    _lastProcessedDay = day;
    _lastProcessedSeason = (int)season;
}

void CropSystem::loadCropData()
{
    _data.clear();
    
    // --- Spring ---
    CropData parsnip;
    parsnip.growthDays = 4;
    parsnip.sellPrice = 35;
    parsnip.xp = 8;
    parsnip.regrowDays = 0;
    parsnip.baseYield = 1;
    parsnip.extraYieldChance = 0.0f;
    parsnip.seedlingSprite = "Crop/Parsnip_seedling.png";
    parsnip.sproutSprites = { "Crop/Parsnip_sprout_1.png", "Crop/Parsnip_sprout_2.png", "Crop/Parsnip_sprout_3.png" };
    parsnip.matureSprite = "Crop/Parsnip.png";
    parsnip.sproutThresholdDays = { 1, 2, 3 };
    parsnip.itemName = "Parsnip";
    parsnip.itemIcon = "Crop/Parsnip.png";
    parsnip.energyRestore = 25;
    parsnip.allowedSeasons = { GameClock::Season::Spring };

    CropData cauliflower;
    cauliflower.growthDays = 12;
    cauliflower.sellPrice = 175;
    cauliflower.xp = 23;
    cauliflower.regrowDays = 0;
    cauliflower.baseYield = 1;
    cauliflower.extraYieldChance = 0.0f;
    cauliflower.seedlingSprite = "Crop/Cauliflower_seedling.png";
    cauliflower.sproutSprites = { "Crop/Cauliflower_sprout_1.png", "Crop/Cauliflower_sprout_2.png", "Crop/Cauliflower_sprout_3.png", "Crop/Cauliflower_sprout_4.png" };
    cauliflower.matureSprite = "Crop/Cauliflower.png";
    cauliflower.sproutThresholdDays = { 3, 6, 9, 11 };
    cauliflower.itemName = "Cauliflower";
    cauliflower.itemIcon = "Crop/Cauliflower.png";
    cauliflower.energyRestore = 75;
    cauliflower.allowedSeasons = { GameClock::Season::Spring };

    CropData potato;
    potato.growthDays = 6;
    potato.sellPrice = 80;
    potato.xp = 14;
    potato.regrowDays = 0;
    potato.baseYield = 1;
    potato.extraYieldChance = 0.25f;
    potato.seedlingSprite = "Crop/Potato_seedling.png";
    potato.sproutSprites = { "Crop/Potato_sprout_1.png", "Crop/Potato_sprout_2.png", "Crop/Potato_sprout_3.png", "Crop/Potato_sprout_4.png", "Crop/Potato_sprout_5.png" };
    potato.matureSprite = "Crop/Potato.png";
    potato.sproutThresholdDays = { 1, 2, 3, 4, 5 };
    potato.itemName = "Potato";
    potato.itemIcon = "Crop/Potato.png";
    potato.energyRestore = 25;
    potato.allowedSeasons = { GameClock::Season::Spring };

    // --- Summer ---
    CropData blueberry;
    blueberry.growthDays = 13;
    blueberry.sellPrice = 50;
    blueberry.xp = 14;
    blueberry.regrowDays = 4;
    blueberry.baseYield = 3;
    blueberry.extraYieldChance = 0.0f;
    blueberry.seedlingSprite = "Crop/Blueberry_seedling.png";
    blueberry.sproutSprites = { "Crop/Blueberry_sprout_1.png", "Crop/Blueberry_sprout_2.png", "Crop/Blueberry_sprout_3.png", "Crop/Blueberry_sprout_4.png" };
    blueberry.matureSprite = "Crop/Blueberry.png";
    blueberry.sproutThresholdDays = { 2, 5, 8, 11 };
    blueberry.itemName = "Blueberry";
    blueberry.itemIcon = "Crop/Blueberry.png";
    blueberry.energyRestore = 25;
    blueberry.allowedSeasons = { GameClock::Season::Summer };

    CropData melon;
    melon.growthDays = 12;
    melon.sellPrice = 250;
    melon.xp = 27;
    melon.regrowDays = 0;
    melon.baseYield = 1;
    melon.extraYieldChance = 0.0f;
    melon.seedlingSprite = "Crop/Melon_seedling.png";
    melon.sproutSprites = { "Crop/Melon_sprout_1.png", "Crop/Melon_sprout_2.png", "Crop/Melon_sprout_3.png", "Crop/Melon_sprout_4.png" };
    melon.matureSprite = "Crop/Melon.png";
    melon.sproutThresholdDays = { 2, 4, 7, 10 };
    melon.itemName = "Melon";
    melon.itemIcon = "Crop/Melon.png";
    melon.energyRestore = 113;
    melon.allowedSeasons = { GameClock::Season::Summer };

    CropData starfruit;
    starfruit.growthDays = 13;
    starfruit.sellPrice = 750;
    starfruit.xp = 43;
    starfruit.regrowDays = 0;
    starfruit.baseYield = 1;
    starfruit.extraYieldChance = 0.0f;
    starfruit.seedlingSprite = "Crop/Starfruit_seedling.png";
    starfruit.sproutSprites = { "Crop/Starfruit_sprout_1.png", "Crop/Starfruit_sprout_2.png", "Crop/Starfruit_sprout_3.png", "Crop/Starfruit_sprout_4.png" };
    starfruit.matureSprite = "Crop/Starfruit.png";
    starfruit.sproutThresholdDays = { 2, 5, 8, 11 };
    starfruit.itemName = "Starfruit";
    starfruit.itemIcon = "Crop/Starfruit.png";
    starfruit.energyRestore = 125;
    starfruit.allowedSeasons = { GameClock::Season::Summer };

    // --- Fall ---
    CropData pumpkin;
    pumpkin.growthDays = 13;
    pumpkin.sellPrice = 320;
    pumpkin.xp = 31;
    pumpkin.regrowDays = 0;
    pumpkin.baseYield = 1;
    pumpkin.extraYieldChance = 0.0f;
    pumpkin.seedlingSprite = "Crop/Pumpkin_seedling.png";
    pumpkin.sproutSprites = { "Crop/Pumpkin_sprout_1.png", "Crop/Pumpkin_sprout_2.png", "Crop/Pumpkin_sprout_3.png", "Crop/Pumpkin_sprout_4.png" };
    pumpkin.matureSprite = "Crop/Pumpkin.png";
    pumpkin.sproutThresholdDays = { 2, 5, 8, 11 };
    pumpkin.itemName = "Pumpkin";
    pumpkin.itemIcon = "Crop/Pumpkin.png";
    pumpkin.energyRestore = 0;
    pumpkin.allowedSeasons = { GameClock::Season::Fall };

    CropData eggplant;
    eggplant.growthDays = 5;
    eggplant.sellPrice = 60;
    eggplant.xp = 10;
    eggplant.regrowDays = 5;
    eggplant.baseYield = 1;
    eggplant.extraYieldChance = 0.2f;
    eggplant.seedlingSprite = "Crop/Eggplant_seedling.png";
    eggplant.sproutSprites = { "Crop/Eggplant_sprout_1.png", "Crop/Eggplant_sprout_2.png", "Crop/Eggplant_sprout_3.png", "Crop/Eggplant_sprout_4.png" };
    eggplant.matureSprite = "Crop/Eggplant.png";
    eggplant.sproutThresholdDays = { 1, 2, 3, 4 };
    eggplant.itemName = "Eggplant";
    eggplant.itemIcon = "Crop/Eggplant.png";
    eggplant.energyRestore = 20;
    eggplant.allowedSeasons = { GameClock::Season::Fall };

    CropData yam;
    yam.growthDays = 10;
    yam.sellPrice = 160;
    yam.xp = 16;
    yam.regrowDays = 0;
    yam.baseYield = 1;
    yam.extraYieldChance = 0.0f;
    yam.seedlingSprite = "Crop/Yam_seedling.png";
    yam.sproutSprites = { "Crop/Yam_sprout_1.png", "Crop/Yam_sprout_2.png", "Crop/Yam_sprout_3.png" };
    yam.matureSprite = "Crop/Yam.png";
    yam.sproutThresholdDays = { 2, 5, 8 };
    yam.itemName = "Yam";
    yam.itemIcon = "Crop/Yam.png";
    yam.energyRestore = 45;
    yam.allowedSeasons = { GameClock::Season::Fall };

    // --- Winter ---
    CropData powdermelon;
    powdermelon.growthDays = 7;
    powdermelon.sellPrice = 60;
    powdermelon.xp = 15;
    powdermelon.regrowDays = 0;
    powdermelon.baseYield = 1;
    powdermelon.extraYieldChance = 0.0f;
    powdermelon.seedlingSprite = "Crop/Powdermelon_seedling.png";
    powdermelon.sproutSprites = { "Crop/Powdermelon_sprout_1.png", "Crop/Powdermelon_sprout_2.png", "Crop/Powdermelon_sprout_3.png", "Crop/Powdermelon_sprout_4.png" };
    powdermelon.matureSprite = "Crop/Powdermelon.png";
    powdermelon.sproutThresholdDays = { 1, 3, 5, 6 };
    powdermelon.itemName = "Powdermelon";
    powdermelon.itemIcon = "Crop/Powdermelon.png";
    powdermelon.energyRestore = 63;
    powdermelon.allowedSeasons = { GameClock::Season::Winter };

    // --- Fish ---
    CropData fish;
    fish.growthDays = 0;
    fish.sellPrice = 50;
    fish.xp = 10;
    fish.regrowDays = 0;
    fish.baseYield = 1;
    fish.extraYieldChance = 0.0f;
    fish.itemName = "Fish";
    fish.itemIcon = "fish.png";
    fish.energyRestore = 13;
    fish.allowedSeasons = { GameClock::Season::Spring, GameClock::Season::Summer, GameClock::Season::Fall, GameClock::Season::Winter };

    CropData anchovy;
    anchovy.growthDays = 0;
    anchovy.sellPrice = 30;
    anchovy.xp = 13;
    anchovy.regrowDays = 0;
    anchovy.baseYield = 1;
    anchovy.extraYieldChance = 0.0f;
    anchovy.itemName = "Anchovy";
    anchovy.itemIcon = "fish/Anchovy.png";
    anchovy.energyRestore = 13;
    anchovy.allowedSeasons = { GameClock::Season::Spring, GameClock::Season::Fall };

    CropData bream;
    bream.growthDays = 0;
    bream.sellPrice = 45;
    bream.xp = 14;
    bream.regrowDays = 0;
    bream.baseYield = 1;
    bream.extraYieldChance = 0.0f;
    bream.itemName = "Bream";
    bream.itemIcon = "fish/Bream.png";
    bream.energyRestore = 13;
    bream.allowedSeasons = { GameClock::Season::Spring, GameClock::Season::Summer, GameClock::Season::Fall, GameClock::Season::Winter };

    CropData largemouthBass;
    largemouthBass.growthDays = 0;
    largemouthBass.sellPrice = 100;
    largemouthBass.xp = 19;
    largemouthBass.regrowDays = 0;
    largemouthBass.baseYield = 1;
    largemouthBass.extraYieldChance = 0.0f;
    largemouthBass.itemName = "Largemouth Bass";
    largemouthBass.itemIcon = "fish/Largemouth_Bass.png";
    largemouthBass.energyRestore = 38;
    largemouthBass.allowedSeasons = { GameClock::Season::Spring, GameClock::Season::Summer, GameClock::Season::Fall, GameClock::Season::Winter };

    // --- Forage ---
    // Spring
    CropData daffodil;
    daffodil.itemName = "Daffodil";
    daffodil.itemIcon = "foraging/spring_foraging/Daffodil.png";
    daffodil.energyRestore = 0;
    daffodil.sellPrice = 30;
    daffodil.xp = 7;
    daffodil.growthDays = 0;
    daffodil.regrowDays = 0;
    daffodil.baseYield = 1;
    daffodil.allowedSeasons = { GameClock::Season::Spring };

    CropData leek;
    leek.itemName = "Leek";
    leek.itemIcon = "foraging/spring_foraging/Leek.png";
    leek.energyRestore = 40;
    leek.sellPrice = 60;
    leek.xp = 7;
    leek.growthDays = 0;
    leek.regrowDays = 0;
    leek.baseYield = 1;
    leek.allowedSeasons = { GameClock::Season::Spring };

    CropData wildHorseradish;
    wildHorseradish.itemName = "Wild Horseradish";
    wildHorseradish.itemIcon = "foraging/spring_foraging/Wild_Horseradish.png";
    wildHorseradish.energyRestore = 13;
    wildHorseradish.sellPrice = 50;
    wildHorseradish.xp = 7;
    wildHorseradish.growthDays = 0;
    wildHorseradish.regrowDays = 0;
    wildHorseradish.baseYield = 1;
    wildHorseradish.allowedSeasons = { GameClock::Season::Spring };

    // Summer
    CropData grape;
    grape.itemName = "Grape";
    grape.itemIcon = "foraging/summer_foraging/Grape.png";
    grape.energyRestore = 38;
    grape.sellPrice = 80;
    grape.xp = 7;
    grape.growthDays = 0;
    grape.regrowDays = 0;
    grape.baseYield = 1;
    grape.allowedSeasons = { GameClock::Season::Summer };

    CropData spiceBerry;
    spiceBerry.itemName = "Spice Berry";
    spiceBerry.itemIcon = "foraging/summer_foraging/Spice_Berry.png";
    spiceBerry.energyRestore = 25;
    spiceBerry.sellPrice = 80;
    spiceBerry.xp = 7;
    spiceBerry.growthDays = 0;
    spiceBerry.regrowDays = 0;
    spiceBerry.baseYield = 1;
    spiceBerry.allowedSeasons = { GameClock::Season::Summer };

    CropData sweetPea;
    sweetPea.itemName = "Sweet Pea";
    sweetPea.itemIcon = "foraging/summer_foraging/Sweet_Pea.png";
    sweetPea.energyRestore = 0;
    sweetPea.sellPrice = 50;
    sweetPea.xp = 7;
    sweetPea.growthDays = 0;
    sweetPea.regrowDays = 0;
    sweetPea.baseYield = 1;
    sweetPea.allowedSeasons = { GameClock::Season::Summer };

    // Fall
    CropData blackberry;
    blackberry.itemName = "Blackberry";
    blackberry.itemIcon = "foraging/fall_foraging/Blackberry.png";
    blackberry.energyRestore = 25;
    blackberry.sellPrice = 20;
    blackberry.xp = 7;
    blackberry.growthDays = 0;
    blackberry.regrowDays = 0;
    blackberry.baseYield = 1;
    blackberry.allowedSeasons = { GameClock::Season::Fall };

    CropData commonMushroom;
    commonMushroom.itemName = "Common Mushroom";
    commonMushroom.itemIcon = "foraging/fall_foraging/Common_Mushroom.png";
    commonMushroom.energyRestore = 38;
    commonMushroom.sellPrice = 40;
    commonMushroom.xp = 7;
    commonMushroom.growthDays = 0;
    commonMushroom.regrowDays = 0;
    commonMushroom.baseYield = 1;
    commonMushroom.allowedSeasons = { GameClock::Season::Fall };

    CropData wildPlum;
    wildPlum.itemName = "Wild Plum";
    wildPlum.itemIcon = "foraging/fall_foraging/Wild_Plum.png";
    wildPlum.energyRestore = 25;
    wildPlum.sellPrice = 80;
    wildPlum.xp = 7;
    wildPlum.growthDays = 0;
    wildPlum.regrowDays = 0;
    wildPlum.baseYield = 1;
    wildPlum.allowedSeasons = { GameClock::Season::Fall };

    // Winter
    CropData crystalFruit;
    crystalFruit.itemName = "Crystal Fruit";
    crystalFruit.itemIcon = "foraging/winter_foraging/Crystal_Fruit.png";
    crystalFruit.energyRestore = 63;
    crystalFruit.sellPrice = 150;
    crystalFruit.xp = 7;
    crystalFruit.growthDays = 0;
    crystalFruit.regrowDays = 0;
    crystalFruit.baseYield = 1;
    crystalFruit.allowedSeasons = { GameClock::Season::Winter };

    CropData snowYam;
    snowYam.itemName = "Snow Yam";
    snowYam.itemIcon = "foraging/winter_foraging/Snow_Yam.png";
    snowYam.energyRestore = 30;
    snowYam.sellPrice = 60;
    snowYam.xp = 7;
    snowYam.growthDays = 0;
    snowYam.regrowDays = 0;
    snowYam.baseYield = 1;
    snowYam.allowedSeasons = { GameClock::Season::Winter };

    CropData winterRoot;
    winterRoot.itemName = "Winter Root";
    winterRoot.itemIcon = "foraging/winter_foraging/Winter_Root.png";
    winterRoot.energyRestore = 25;
    winterRoot.sellPrice = 70;
    winterRoot.xp = 7;
    winterRoot.growthDays = 0;
    winterRoot.regrowDays = 0;
    winterRoot.baseYield = 1;
    winterRoot.allowedSeasons = { GameClock::Season::Winter };

    // Register
    _data[CropType::Parsnip] = parsnip;
    _data[CropType::Cauliflower] = cauliflower;
    _data[CropType::Potato] = potato;
    _data[CropType::Blueberry] = blueberry;
    _data[CropType::Melon] = melon;
    _data[CropType::Starfruit] = starfruit;
    _data[CropType::Pumpkin] = pumpkin;
    _data[CropType::Eggplant] = eggplant;
    _data[CropType::Yam] = yam;
    _data[CropType::Powdermelon] = powdermelon;
    _data[CropType::Fish] = fish;
    _data[CropType::Anchovy] = anchovy;
    _data[CropType::Bream] = bream;
    _data[CropType::LargemouthBass] = largemouthBass;
    
    // Forage Register
    _data[CropType::Daffodil] = daffodil;
    _data[CropType::Leek] = leek;
    _data[CropType::WildHorseradish] = wildHorseradish;
    _data[CropType::Grape] = grape;
    _data[CropType::SpiceBerry] = spiceBerry;
    _data[CropType::SweetPea] = sweetPea;
    _data[CropType::Blackberry] = blackberry;
    _data[CropType::CommonMushroom] = commonMushroom;
    _data[CropType::WildPlum] = wildPlum;
    _data[CropType::CrystalFruit] = crystalFruit;
    _data[CropType::SnowYam] = snowYam;
    _data[CropType::WinterRoot] = winterRoot;
}

void CropSystem::ensureGridSize()
{
    if (!_map) return;
    Size mapTiles = _map->getMapSize();
    
    // Check if we can restore state
    if (!_tiles.empty() && 
        _tiles.size() == (int)mapTiles.width && 
        !_tiles[0].empty() && 
        _tiles[0].size() == (int)mapTiles.height)
    {
        // Restore visual state
        for (int x = 0; x < _tiles.size(); ++x)
        {
            for (int y = 0; y < _tiles[x].size(); ++y)
            {
                auto& slot = _tiles[x][y];
                
                // Restore soil visual
                if (slot.tilled)
                {
                    if (slot.watered) waterTintTile(Vec2(x, y));
                    else darkenTile(Vec2(x, y));
                }
                
                // Restore crop sprite
                if (slot.crop)
                {
                    auto* inst = slot.crop.get();
                    // Previous sprite is invalid
                    inst->sprite = nullptr;
                    
                    const auto& d = _data[inst->type];
                    std::string path;
                    
                    if (inst->withered)
                    {
                        path = "Crop/Wilted_crop.png";
                    }
                    else if (inst->stageIndex == 0)
                    {
                        path = d.seedlingSprite;
                    }
                    else if (inst->stageIndex <= (int)d.sproutSprites.size())
                    {
                        path = d.sproutSprites[inst->stageIndex - 1];
                    }
                    else
                    {
                        path = d.matureSprite;
                    }
                    
                    inst->sprite = Sprite::create(path);
                    if (inst->sprite)
                    {
                        fitSpriteToTile(inst->sprite);
                        placeOrUpdateSprite(Vec2(x, y), inst);
                        _map->addChild(inst->sprite, 50);
                    }
                }
            }
        }
        return;
    }

    _tiles.clear();
    _activeTileIndices.clear();
    _tiles.resize((int)mapTiles.width);
    for (int x = 0; x < (int)mapTiles.width; ++x)
    {
        _tiles[x].resize((int)mapTiles.height);
        for (int y = 0; y < (int)mapTiles.height; ++y)
        {
            _tiles[x][y].tilled = false;
            _tiles[x][y].watered = false;
            _tiles[x][y].crop.reset();
        }
    }
}

void CropSystem::markActive(const Vec2& tileIndex)
{
    if (!_map) return;
    Size mapTiles = _map->getMapSize();
    int x = (int)tileIndex.x;
    int y = (int)tileIndex.y;
    if (x < 0 || y < 0 || x >= (int)mapTiles.width || y >= (int)mapTiles.height) return;
    int key = y * (int)mapTiles.width + x;
    _activeTileIndices.insert(key);
}

void CropSystem::unmarkActive(const Vec2& tileIndex)
{
    if (!_map) return;
    Size mapTiles = _map->getMapSize();
    int x = (int)tileIndex.x;
    int y = (int)tileIndex.y;
    if (x < 0 || y < 0 || x >= (int)mapTiles.width || y >= (int)mapTiles.height) return;
    int key = y * (int)mapTiles.width + x;
    _activeTileIndices.erase(key);
}

bool CropSystem::inBounds(const Vec2& tileIndex) const
{
    if (!_map) return false;
    Size mapTiles = _map->getMapSize();
    int x = (int)tileIndex.x;
    int y = (int)tileIndex.y;
    return x >= 0 && y >= 0 && x < (int)mapTiles.width && y < (int)mapTiles.height;
}

void CropSystem::placeOrUpdateSprite(const Vec2& tileIndex, CropInstance* inst)
{
    Vec2 pos = tileBottomCenterWorld(tileIndex);
    if (inst->sprite)
    {
        inst->sprite->setAnchorPoint(Vec2(0.5f, 0.0f));
        inst->sprite->setPosition(pos);
    }
}

void CropSystem::fitSpriteToTile(Sprite* sprite)
{
    if (!_map || !sprite) return;
    Size tileSize = _map->getTileSize();
    Size cs = sprite->getContentSize();
    if (cs.width <= 0 || cs.height <= 0) return;
    float scale = (tileSize.width * 0.9f) / cs.width;
    sprite->setScale(scale);
}

Vec2 CropSystem::tileBottomCenterWorld(const Vec2& tileIndex) const
{
    Size tileSize = _map->getTileSize();
    Size mapTiles = _map->getMapSize();
    float mapWidth = mapTiles.width * tileSize.width;
    float mapHeight = mapTiles.height * tileSize.height;
    float x = (tileIndex.x + 0.5f) * tileSize.width;
    float y = mapHeight - (tileIndex.y + 1.0f) * tileSize.height;
    return Vec2(x, y);
}

void CropSystem::darkenTile(const Vec2& tileIndex)
{
    if (!_groundLayer) return;
    auto tileSprite = _groundLayer->getTileAt(tileIndex);
    if (tileSprite)
    {
        tileSprite->setColor(Color3B(120, 100, 80));
    }
}

void CropSystem::waterTintTile(const Vec2& tileIndex)
{
    if (!_groundLayer) return;
    auto tileSprite = _groundLayer->getTileAt(tileIndex);
    if (tileSprite)
    {
        tileSprite->setColor(Color3B(90, 120, 170));
    }
}

void CropSystem::resetTileColor(const Vec2& tileIndex)
{
    if (!_groundLayer) return;
    auto tileSprite = _groundLayer->getTileAt(tileIndex);
    if (tileSprite)
    {
        tileSprite->setColor(Color3B::WHITE);
    }
}

bool CropSystem::isSeasonAllowed(CropType type, GameClock::Season s) const
{
    auto it = _data.find(type);
    if (it == _data.end()) return false;
    const auto& vec = it->second.allowedSeasons;
    for (auto ss : vec)
    {
        if (ss == s) return true;
    }
    return false;
}

int CropSystem::getSellPrice(CropType type) const
{
    auto it = _data.find(type);
    if (it == _data.end()) return 0;
    
    int basePrice = it->second.sellPrice;
    
    // Determine which skill affects this price
    SkillType skill = SkillType::Farming;
    if (type == CropType::Fish || type == CropType::Anchovy || type == CropType::Bream || type == CropType::LargemouthBass)
    {
        skill = SkillType::Fishing;
    }
    else if (type == CropType::Daffodil || type == CropType::Leek || type == CropType::WildHorseradish ||
             type == CropType::Grape || type == CropType::SpiceBerry || type == CropType::SweetPea ||
             type == CropType::Blackberry || type == CropType::CommonMushroom || type == CropType::WildPlum ||
             type == CropType::CrystalFruit || type == CropType::SnowYam || type == CropType::WinterRoot)
    {
        skill = SkillType::Foraging;
    }
    
    int level = ExperienceSystem::getInstance()->getLevel(skill);
    // Apply 5% increase per level
    float multiplier = 1.0f + (level * 0.05f);
    
    return (int)(basePrice * multiplier);
}

const CropData* CropSystem::getCropData(CropType type) const
{
    auto it = _data.find(type);
    if (it != _data.end())
    {
        return &it->second;
    }
    return nullptr;
}

const CropInstance* CropSystem::getCropAt(const cocos2d::Vec2& tileIndex) const
{
    if (!inBounds(tileIndex)) return nullptr;
    const auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    return slot.crop.get();
}
