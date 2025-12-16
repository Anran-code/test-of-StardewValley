#include "CropSystem.h"
#include "cocos2d.h"

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
    , _selected(CropType::Parsnip)
    , _lastProcessedDay(-1)
    , _lastProcessedSeason(-1)
{
}

void CropSystem::init(TMXTiledMap* map, GameClock* clock, Wallet* wallet)
{
    if (map) _map = map;
    if (_map)
    {
        _groundLayer = _map->getLayer("ground");
        if (!_groundLayer && _map->getChildrenCount() > 0)
        {
            _groundLayer = dynamic_cast<TMXLayer*>(_map->getChildren().at(0));
        }
    }
    if (clock) _clock = clock;
    if (wallet) _wallet = wallet;
    loadCropData();
    ensureGridSize();
    if (_clock)
    {
        _lastProcessedDay = _clock->getDay();
        _lastProcessedSeason = (int)_clock->getSeason();
    }
}

void CropSystem::setSelectedCrop(CropType type)
{
    _selected = type;
}

void CropSystem::tillTile(const Vec2& tileIndex)
{
    if (!inBounds(tileIndex)) return;
    auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];

    // Only till if not already tilled
    if (!slot.tilled)
    {
        slot.tilled = true;
        darkenTile(tileIndex);
    }
}

bool CropSystem::plantSelected(const Vec2& tileIndex)
{
    if (!inBounds(tileIndex)) return false;
    auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    if (!slot.tilled) return false;
    if (slot.crop) return false;

    // Check season
    if (!isSeasonAllowed(_selected, _clock->getSeason()))
    {
        cocos2d::log("Crop not allowed in this season");
        return false;
    }

    const auto& d = _data[_selected];

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
    return true;
}

void CropSystem::waterTile(const Vec2& tileIndex)
{
    if (!inBounds(tileIndex)) return;
    auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    if (!slot.tilled) return;
    slot.watered = true;
    waterTintTile(tileIndex);
}

bool CropSystem::harvestTile(const Vec2& tileIndex)
{
    if (!inBounds(tileIndex)) return false;
    auto& slot = _tiles[(int)tileIndex.x][(int)tileIndex.y];
    if (!slot.crop) return false;
    auto* inst = slot.crop.get();
    const auto& d = _data[inst->type];
    if (inst->withered) return false;
    if (inst->daysWatered < d.growthDays) return false;

    int yieldCount = 1;
    if (inst->type == CropType::Potato)
    {
        float r = RandomHelper::random_real<float>(0.0f, 1.0f);
        if (r < 0.25f) yieldCount += 1;
    }
    if (_wallet) _wallet->addMoney(d.sellPrice * yieldCount);
    if (inst->sprite)
    {
        inst->sprite->removeFromParent();
    }
    slot.crop.reset();
    return true;
}

void CropSystem::updateDailyGrowth()
{
    if (!_clock || !_map) return;
    int day = _clock->getDay();
    if (_lastProcessedDay == day) return;
    auto season = _clock->getSeason();
    bool seasonChanged = (_lastProcessedSeason != (int)season);

    Size mapTiles = _map->getMapSize();
    for (int x = 0; x < (int)mapTiles.width; ++x)
    {
        for (int y = 0; y < (int)mapTiles.height; ++y)
        {
            auto& slot = _tiles[x][y];
            if (slot.crop)
            {
                auto* inst = slot.crop.get();
                const auto& d = _data[inst->type];
                if (seasonChanged && !isSeasonAllowed(inst->type, season))
                {
                    inst->withered = true;
                    if (inst->sprite)
                    {
                        inst->sprite->setTexture("Crop/Wilted_crop.png");
                        fitSpriteToTile(inst->sprite);
                        placeOrUpdateSprite(Vec2(x, y), inst);
                    }
                }
                if (!inst->withered)
                {
                    if (slot.watered)
                    {
                        inst->daysWatered += 1;
                    }
                    slot.watered = false;

                    int newStage = 0;
                    for (int i = 0; i < (int)d.sproutThresholdDays.size(); ++i)
                    {
                        if (inst->daysWatered >= d.sproutThresholdDays[i]) newStage = i + 1;
                    }
                    if (inst->daysWatered >= d.growthDays) newStage = (int)d.sproutSprites.size() + 1;

                    if (newStage != inst->stageIndex)
                    {
                        inst->stageIndex = newStage;
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
                    }
                }
                if (slot.tilled)
                {
                    darkenTile(Vec2(x, y));
                }
                else
                {
                    resetTileColor(Vec2(x, y));
                }
            }
            else
            {
                if (slot.tilled)
                {
                    if (!slot.watered)
                    {
                        float r = RandomHelper::random_real<float>(0.0f, 1.0f);
                        if (r < 0.3f)
                        {
                            slot.tilled = false;
                            resetTileColor(Vec2(x, y));
                        }
                        else
                        {
                            darkenTile(Vec2(x, y));
                        }
                    }
                    else
                    {
                        darkenTile(Vec2(x, y));
                    }
                    slot.watered = false;
                }
                else
                {
                    resetTileColor(Vec2(x, y));
                }
            }
        }
    }

    _lastProcessedDay = day;
    _lastProcessedSeason = (int)season;
}

void CropSystem::loadCropData()
{
    _data.clear();
    CropData parsnip;
    parsnip.growthDays = 4;
    parsnip.sellPrice = 35;
    parsnip.seedlingSprite = "Crop/Parsnip_seedling.png";
    parsnip.sproutSprites = { "Crop/Parsnip_sprout_1.png", "Crop/Parsnip_sprout_2.png", "Crop/Parsnip_sprout_3.png" };
    parsnip.matureSprite = "Crop/Parsnip.png";
    parsnip.sproutThresholdDays = { 1, 2, 3 };

    CropData cauliflower;
    cauliflower.growthDays = 12;
    cauliflower.sellPrice = 175;
    cauliflower.seedlingSprite = "Crop/Cauliflower_seedling.png";
    cauliflower.sproutSprites = { "Crop/Cauliflower_sprout_1.png", "Crop/Cauliflower_sprout_2.png", "Crop/Cauliflower_sprout_3.png", "Crop/Cauliflower_sprout_4.png" };
    cauliflower.matureSprite = "Crop/Cauliflower.png";
    cauliflower.sproutThresholdDays = { 3, 6, 9, 11 };

    CropData potato;
    potato.growthDays = 6;
    potato.sellPrice = 80;
    potato.seedlingSprite = "Crop/Potato_seedling.png";
    potato.sproutSprites = { "Crop/Potato_sprout_1.png", "Crop/Potato_sprout_2.png", "Crop/Potato_sprout_3.png", "Crop/Potato_sprout_4.png", "Crop/Potato_sprout_5.png" };
    potato.matureSprite = "Crop/Potato.png";
    potato.sproutThresholdDays = { 1, 2, 3, 4, 5 };
    parsnip.allowedSeasons = { GameClock::Season::Spring };
    cauliflower.allowedSeasons = { GameClock::Season::Spring };
    potato.allowedSeasons = { GameClock::Season::Spring };

    _data[CropType::Parsnip] = parsnip;
    _data[CropType::Cauliflower] = cauliflower;
    _data[CropType::Potato] = potato;
}

void CropSystem::ensureGridSize()
{
    if (!_map) return;
    Size mapTiles = _map->getMapSize();
    _tiles.clear();
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
