#include "AnimalSystem.h"
#include "GameScene.h"
#include "Item.h"
#include "CropSystem.h"

USING_NS_CC;

AnimalSystem* AnimalSystem::sInstance = nullptr;

AnimalSystem* AnimalSystem::getInstance()
{
    if (!sInstance)
    {
        sInstance = new AnimalSystem();
    }
    return sInstance;
}

AnimalSystem::AnimalSystem()
    : _layer(nullptr)
    , _bgLayer(nullptr)
    , _map(nullptr)
    , _hayStorage(0)
    , _hasHopper(false)
{
}

AnimalSystem::~AnimalSystem()
{
    _animals.clear();
    _eggs.clear();
    _troughs.clear();
}

void AnimalSystem::init(BackgroundLayer* layer, TMXTiledMap* map)
{
    _layer = layer;
    _bgLayer = layer;
    _map = map;
    for (auto& egg : _eggs) egg.sprite = nullptr;
    for (auto& t : _troughs) t.sprite = nullptr;

    _hasHopper = false;
    _hasIncubator = false;
    bool isHenhouse = false;
    if (layer)
    {
        isHenhouse = (layer->getType() == BackgroundType::Henhouse);
    }

    if (_map && isHenhouse)
    {
        if (_showHatchNotification)
        {
            _showHatchNotification = false;
            std::string msg = "A new chick has hatched!";
            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
        }

        // 1. Hopper
        auto bucketGroup = _map->getObjectGroup("bucket");
        if (bucketGroup)
        {
            auto objects = bucketGroup->getObjects();
            if (!objects.empty())
            {
                ValueMap hopperMap = objects.front().asValueMap();
                float x = hopperMap["x"].asFloat();
                float y = hopperMap["y"].asFloat();
                float w = hopperMap["width"].asFloat();
                float h = hopperMap["height"].asFloat();
                _hopperRect = Rect(x, y, w, h);
                _hasHopper = true;
            }
        }

        // 2. Troughs 
        auto troughGroup = _map->getObjectGroup("feedinghopper");
        if (troughGroup)
        {
            if (_troughs.empty())
            {
                auto objects = troughGroup->getObjects();
                for (const auto& obj : objects)
                {
                    ValueMap bucketMap = obj.asValueMap();
                    float x = bucketMap["x"].asFloat();
                    float y = bucketMap["y"].asFloat();
                    float w = bucketMap["width"].asFloat();
                    float h = bucketMap["height"].asFloat();

                    // Convert to tiles
                    Size tileSize = _map->getTileSize();
                    // Use rounding to avoid issues with slight size mismatches
                    int cols = std::max(1, (int)((w / tileSize.width) + 0.5f));
                    int rows = std::max(1, (int)((h / tileSize.height) + 0.5f));

                    for (int i = 0; i < cols; i++)
                    {
                        for (int j = 0; j < rows; j++)
                        {
                            float cx = x + (i + 0.5f) * tileSize.width;
                            float cy = y + (j + 0.5f) * tileSize.height;

                            Size mapSize = _map->getMapSize();
                            float mapHeight = mapSize.height * tileSize.height;

                            int tileX = cx / tileSize.width;
                            int tileY = (mapHeight - cy) / tileSize.height;

                            TroughData td;
                            td.tilePos = Vec2(tileX, tileY);
                            td.hasHay = false;
                            td.sprite = nullptr;
                            _troughs.push_back(td);
                        }
                    }
                }
            }
        }

        // 3. Incubator (EggBucket)
        auto incubatorGroup = _map->getObjectGroup("eggbucket");
        if (incubatorGroup)
        {
            auto objects = incubatorGroup->getObjects();
            if (!objects.empty())
            {
                ValueMap incMap = objects.front().asValueMap();
                float x = incMap["x"].asFloat();
                float y = incMap["y"].asFloat();
                float w = incMap["width"].asFloat();
                float h = incMap["height"].asFloat();
                _incubatorRect = Rect(x, y, w, h);
                _hasIncubator = true;
            }
        }
    }

    if (_animals.empty())
    {
        if (map)
        {
            Size mapSize = map->getMapSize();
            Size tileSize = map->getTileSize();
            float centerX = mapSize.width * tileSize.width * 0.5f;
            float centerY = mapSize.height * tileSize.height * 0.5f;

            // Add initial animals
            addAnimal(Animal::Type::Blue, Animal::Age::Baby, Vec2(centerX - 50, centerY));
            addAnimal(Animal::Type::White, Animal::Age::Baby, Vec2(centerX + 50, centerY));

            if (!isHenhouse)
            {
                for (auto animal : _animals)
                {
                    animal->setLocation(Animal::Location::Outside);

                    if (GameScene::sHasFarmHenhouseDoorPos)
                    {
                        float offX = (rand() % 100) - 50;
                        float offY = (rand() % 100) - 100;
                        animal->setPosition(GameScene::sFarmHenhouseDoorPos + Vec2(offX, offY));
                    }
                    else
                    {
                        animal->setPosition(Vec2(centerX, centerY));
                    }
                }
            }
        }
    }
    else
    {
        for (auto animal : _animals)
        {
            if (animal->getParent()) animal->removeFromParent();

            // Render logic based on location
            bool shouldRender = false;
            if (isHenhouse && animal->getLocation() == Animal::Location::Inside) shouldRender = true;
            else if (!isHenhouse && animal->getLocation() == Animal::Location::Outside) shouldRender = true;

            if (shouldRender && _layer)
            {
                if (_map)
                {
                    Size mapSize = _map->getMapSize();
                    Size tileSize = _map->getTileSize();
                    float w = mapSize.width * tileSize.width;
                    float h = mapSize.height * tileSize.height;
                    Vec2 pos = animal->getPosition();

                    // Allow small margin or strict check
                    if (pos.x < 0 || pos.x > w || pos.y < 0 || pos.y > h)
                    {
                        // Reset to center
                        animal->setPosition(Vec2(w * 0.5f, h * 0.5f));
                    }
                }

                _layer->addChild(animal, 5);
            }
        }
    }

    // Process Pending Eggs 
    if (_bgLayer && _bgLayer->getType() == BackgroundType::Farm && _map)
    {
        Size tileSize = _map->getTileSize();
        Size mapSize = _map->getMapSize();
        float mapHeight = mapSize.height * tileSize.height;

        for (auto& egg : _eggs)
        {
            if (egg.tilePos.x == -1 && egg.tilePos.y == -1)
            {
                // Assign a valid position on the Farm
                bool foundSpot = false;
                int attempts = 30; // Try harder to find a spot

                int centerX = mapSize.width / 2;
                int centerY = mapSize.height / 2;

                if (GameScene::sHasFarmHenhouseDoorPos)
                {
                    Vec2 doorPos = GameScene::sFarmHenhouseDoorPos;
                    centerX = doorPos.x / tileSize.width;
                    centerY = (mapHeight - doorPos.y) / tileSize.height;
                }

                for (int i = 0; i < attempts; i++)
                {
                    // Random offset 
                    int rX = centerX + (rand() % 30) - 15;
                    int rY = centerY + (rand() % 20) - 10;

                    // Bounds check
                    if (rX < 0 || rX >= mapSize.width || rY < 0 || rY >= mapSize.height) continue;

                    // Check validity using BackgroundLayer 
                    if (!_bgLayer->isValidSpawnPosition(rX, rY)) continue;

                    // Check if occupied by other eggs
                    bool occupied = false;
                    for (const auto& otherEgg : _eggs) {
                        if (otherEgg.tilePos.x == rX && otherEgg.tilePos.y == rY) { occupied = true; break; }
                    }
                    if (occupied) continue;

                    // Found spot
                    egg.tilePos = Vec2(rX, rY);

                    float wx = (rX + 0.5f) * tileSize.width;
                    float wy = mapHeight - (rY + 0.5f) * tileSize.height;
                    egg.worldPos = Vec2(wx, wy);

                    foundSpot = true;
                    break;
                }

            }
        }
    }

    // Re-create egg sprites
    if (_map)
    {
        for (auto& egg : _eggs)
        {
            if (egg.tilePos.x == -1 && egg.tilePos.y == -1) continue;

            if (egg.sprite)
            {
                egg.sprite->removeFromParent();
                egg.sprite = nullptr;
            }

            if (!isHenhouse)
            {
                std::string filename;
                filename = egg.isLarge ? "animals/Large_Egg.png" : "animals/Egg.png";

                if (!FileUtils::getInstance()->isFileExist(filename)) continue;

                auto sprite = Sprite::create(filename);
                if (sprite)
                {
                    if (egg.worldPos.lengthSquared() > 0.1f)
                    {
                        sprite->setPosition(egg.worldPos);
                    }
                    else
                    {
                        Size tileSize = _map->getTileSize();
                        Size mapSize = _map->getMapSize();
                        float mapHeight = mapSize.height * tileSize.height;
                        float cx = (egg.tilePos.x + 0.5f) * tileSize.width;
                        float cy = mapHeight - (egg.tilePos.y + 0.5f) * tileSize.height;
                        sprite->setPosition(Vec2(cx, cy));
                    }

                    // Scale to fit tile
                    Size tileSize = _map->getTileSize();
                    float targetSize = tileSize.width * 0.8f;
                    float scale = targetSize / sprite->getContentSize().width;
                    sprite->setScale(scale);

                    if (_layer) _layer->addChild(sprite, 2); // Egg z-order 2 (below player 3)
                    egg.sprite = sprite;
                }
            }
        }
    }

    // Re-create Hay Sprites
    if (isHenhouse)
    {
        for (auto& t : _troughs)
        {
            if (t.sprite)
            {
                t.sprite->removeFromParent();
                t.sprite = nullptr;
            }

            if (t.hasHay)
            {
                auto sprite = Sprite::create("block/Hay.png");
                if (!sprite) {
                    sprite = Sprite::create("Hay.png");
                }
                if (sprite)
                {
                    Size tileSize = _map->getTileSize();
                    Size mapSize = _map->getMapSize();
                    float mapHeight = mapSize.height * tileSize.height;
                    float cx = (t.tilePos.x + 0.5f) * tileSize.width;
                    float cy = mapHeight - (t.tilePos.y + 0.5f) * tileSize.height;

                    sprite->setPosition(Vec2(cx, cy));

                    // Scale to fit tile
                    float targetW = tileSize.width * 0.8f;
                    float targetH = tileSize.height * 0.8f;
                    float scaleX = targetW / sprite->getContentSize().width;
                    float scaleY = targetH / sprite->getContentSize().height;
                    sprite->setScale(std::min(scaleX, scaleY));

                    // Use Local Z Order on Layer (Consistent with init)
                    if (_layer) _layer->addChild(sprite, 200);
                    t.sprite = sprite;
                }
            }
        }
    }
}
void AnimalSystem::cleanupVisuals(BackgroundLayer* layer)
{
    if (_layer == layer)
    {
        _layer = nullptr;
        _map = nullptr;
        for (auto& egg : _eggs) egg.sprite = nullptr;
        for (auto& trough : _troughs) trough.sprite = nullptr;
    }
}

void AnimalSystem::addAnimal(Animal::Type type, Animal::Age age, const Vec2& position)
{
    auto animal = Animal::create(type, age);
    if (animal)
    {
        animal->setPosition(position);
        animal->retain(); // Keep alive
        _animals.push_back(animal);
        if (_layer) _layer->addChild(animal, 5);
    }
}

void AnimalSystem::updateDailyGrowth()
{
    // Feeding Logic (Inside Henhouse)
    for (auto animal : _animals)
    {
        bool fed = animal->isFed();
        if (GameScene::sDebugMode) fed = true;

        // 1. Try eat Hay from Trough
        if (!fed && animal->getLocation() == Animal::Location::Inside)
        {
            for (auto& trough : _troughs)
            {
                if (trough.hasHay)
                {
                    trough.hasHay = false;
                    if (trough.sprite)
                    {
                        trough.sprite->removeFromParent();
                        trough.sprite = nullptr;
                    }
                    fed = true;

                    // Play Eat Animation
                    animal->startEating();

                    break; // One hay per animal
                }
            }
        }
        else
        {
            fed = true;
        }

        animal->setLocation(Animal::Location::Outside);

        if (GameScene::sHasFarmHenhouseDoorPos)
        {
            float offX = (rand() % 100) - 50;
            float offY = (rand() % 100) - 100; // Move down from door
            animal->setPosition(GameScene::sFarmHenhouseDoorPos + Vec2(offX, offY));
        }
        else
        {
            // Fallback
            animal->setPosition(Vec2(500, 300));
        }

        // 2. Growth / Produce
        animal->incrementDaysAlive();

        if (animal->getAge() == Animal::Age::Baby && animal->getDaysAlive() >= DAYS_TO_MATURE)
        {
            animal->growUp();
        }

        // Produce Eggs (Adults only, and only if fed)
        if (fed && animal->getAge() == Animal::Age::Adult)
        {
            bool isLarge = (rand() % 100) < 20;
            EggData newEgg;
            newEgg.tilePos = Vec2(-1, -1);
            newEgg.worldPos = Vec2::ZERO;
            newEgg.isLarge = isLarge;
            newEgg.type = animal->getType();
            newEgg.sprite = nullptr;

            _eggs.push_back(newEgg);
        }   // Reset for new day
        animal->setFed(false);
    }

    for (auto& inc : _incubators)
    {
        if (inc.active)
        {
            inc.minutesRemaining -= 1200;
            if (inc.minutesRemaining <= 0)
            {
                // Hatch
                inc.active = false;

                // Add new baby animal
                addAnimal(inc.hatchType, Animal::Age::Baby, Vec2(400, 300));

                // Flag to show notification next time we enter Henhouse
                _showHatchNotification = true;
            }
        }
    }

    // Cleanup inactive incubators
    _incubators.erase(std::remove_if(_incubators.begin(), _incubators.end(),
        [](const IncubatorData& d) { return !d.active; }), _incubators.end());

    if (_bgLayer && _bgLayer->getType() == BackgroundType::Farm && _map)
    {
        Size tileSize = _map->getTileSize();
        Size mapSize = _map->getMapSize();
        float mapHeight = mapSize.height * tileSize.height;

        for (auto& egg : _eggs)
        {
            if (egg.tilePos.x == -1 && egg.tilePos.y == -1)
            {
                // Assign a valid position on the Farm
                bool foundSpot = false;
                int attempts = 30;

                int centerX = mapSize.width / 2;
                int centerY = mapSize.height / 2;

                if (GameScene::sHasFarmHenhouseDoorPos)
                {
                    Vec2 doorPos = GameScene::sFarmHenhouseDoorPos;
                    centerX = doorPos.x / tileSize.width;
                    centerY = (mapHeight - doorPos.y) / tileSize.height;
                }

                for (int i = 0; i < attempts; i++)
                {
                    int rX = centerX + (rand() % 30) - 15;
                    int rY = centerY + (rand() % 20) - 10;

                    if (rX < 0 || rX >= mapSize.width || rY < 0 || rY >= mapSize.height) continue;
                    if (!_bgLayer->isValidSpawnPosition(rX, rY)) continue;

                    bool occupied = false;
                    for (const auto& otherEgg : _eggs) {
                        if (otherEgg.tilePos.x == rX && otherEgg.tilePos.y == rY) { occupied = true; break; }
                    }
                    if (occupied) continue;

                    egg.tilePos = Vec2(rX, rY);
                    float wx = (rX + 0.5f) * tileSize.width;
                    float wy = mapHeight - (rY + 0.5f) * tileSize.height;
                    egg.worldPos = Vec2(wx, wy);

                    foundSpot = true;

                    // Create sprite immediately since we are on Farm
                    std::string filename = egg.isLarge ? "animals/Large_Egg.png" : "animals/Egg.png";
                    if (FileUtils::getInstance()->isFileExist(filename))
                    {
                        auto sprite = Sprite::create(filename);
                        if (sprite)
                        {
                            sprite->setPosition(egg.worldPos);
                            float targetSize = tileSize.width * 0.8f;
                            float scale = targetSize / sprite->getContentSize().width;
                            sprite->setScale(scale);
                            if (_layer) _layer->addChild(sprite, 2);
                            egg.sprite = sprite;
                        }
                    }

                    break;
                }
            }
        }
    }
}

void AnimalSystem::update(float dt)
{
    // Check Time: If 6 PM (18:00), return animals to Henhouse
    if (GameScene::sClock && GameScene::sClock->getHour() >= 18)
    {
        bool isHenhouse = (_bgLayer && _bgLayer->getType() == BackgroundType::Henhouse);
        bool isFarm = (_bgLayer && _bgLayer->getType() == BackgroundType::Farm);

        for (auto animal : _animals)
        {
            // 1. Update Logical State
            if (animal->getLocation() == Animal::Location::Outside)
            {
                animal->setLocation(Animal::Location::Inside);
            }

            // 2. Update Visual State
            if (isFarm)
            {
                if (animal->getParent()) animal->removeFromParent();
                animal->setPosition(Vec2(400, 300));
            }
            // If we are currently in Henhouse, add them if missing
            else if (isHenhouse)
            {
                if (!animal->getParent() && _layer)
                {
                    if (_map)
                    {
                        Size mapSize = _map->getMapSize();
                        Size tileSize = _map->getTileSize();
                        float cx = mapSize.width * tileSize.width * 0.5f;
                        float cy = mapSize.height * tileSize.height * 0.5f;
                        animal->setPosition(Vec2(cx, cy));
                    }
                    _layer->addChild(animal, 5);
                }
            }
        }
    }


    bool isFarm = false;

    for (auto animal : _animals)
    {
        Vec2 oldPos = animal->getPosition();
        animal->update(dt);

        // Eating Grass Logic (Outside)
        if (animal->getLocation() == Animal::Location::Outside &&
            animal->getCurrentState() == Animal::State::Eat)
        {
            if (_map && _layer)
            {
                Size tileSize = _map->getTileSize();
                Size mapSize = _map->getMapSize();
                float mapHeight = mapSize.height * tileSize.height;
                Vec2 pos = animal->getPosition();

                int tx = pos.x / tileSize.width;
                int ty = (mapHeight - pos.y) / tileSize.height;

                if (_bgLayer && _bgLayer->getType() == BackgroundType::Farm)
                {
                    // Use newly added method
                    if (_bgLayer->tryEatGrass(Vec2(tx, ty)))
                    {
                        // Ate grass!
                        animal->startEating();
                        animal->setFed(true);
                    }
                }
            }
        }

        if (_map)
        {
            Vec2 newPos = animal->getPosition();

            // Check collisions and boundaries
            if (!isWalkable(newPos))
            {
                // Revert position
                animal->setPosition(oldPos);

                animal->setState(Animal::State::Idle);
                animal->setStateTimer(0.5f + (rand() % 10) / 10.0f); // Short pause
            }
            else
            {
                Size mapSize = _map->getMapSize();
                Size tileSize = _map->getTileSize();
                float mapHeight = mapSize.height * tileSize.height;
                animal->setLocalZOrder(mapHeight - newPos.y);
            }
        }
    }
}

bool AnimalSystem::isWalkable(const cocos2d::Vec2& pos)
{
    if (!_map) return false;

    Size tileSize = _map->getTileSize();
    Size mapSize = _map->getMapSize();
    float mapWidth = mapSize.width * tileSize.width;
    float mapHeight = mapSize.height * tileSize.height;

    // 1. Boundary Check
    int marginTiles = 1;
    // Top wall is usually 2 tiles high
    int topMarginTiles = 2;

    float leftLimit = marginTiles * tileSize.width;
    float rightLimit = mapWidth - marginTiles * tileSize.width;
    float bottomLimit = marginTiles * tileSize.height;
    float topLimit = mapHeight - topMarginTiles * tileSize.height;

    if (pos.x < leftLimit || pos.x > rightLimit ||
        pos.y < bottomLimit || pos.y > topLimit)
    {
        return false;
    }

    // 2. Object Collision
    // Define a small footprint for the animal (feet)
    Rect animalRect(pos.x - 6, pos.y - 6, 12, 12);

    // Hopper (Bucket)
    if (_hasHopper && _hopperRect.intersectsRect(animalRect)) return false;

    // Troughs (FeedingHopper)
    for (const auto& t : _troughs)
    {
        // Calculate Trough Rect
        float cx = (t.tilePos.x + 0.5f) * tileSize.width;
        float cy = mapHeight - (t.tilePos.y + 0.5f) * tileSize.height;

        // Troughs are solid obstacles
        Rect troughRect(cx - tileSize.width * 0.4f, cy - tileSize.height * 0.4f, tileSize.width * 0.8f, tileSize.height * 0.8f);

        if (troughRect.intersectsRect(animalRect)) return false;
    }

    // 3. Collision Layer Check (Generic Obstacles & Floor Check)
    if (_map)
    {
        // Use the same logic as GameScene to find the main layer
        auto layer = _map->getLayer("图块层 1");
        if (!layer) layer = _map->getLayer("ground");
        if (!layer) layer = _map->getLayer("Back");
        if (!layer)
        {
            if (_map->getChildrenCount() > 0)
                layer = dynamic_cast<TMXLayer*>(_map->getChildren().at(0));
        }

        if (layer)
        {
            int tx = pos.x / tileSize.width;
            int ty = (mapHeight - pos.y) / tileSize.height;

            // Bounds check for tile coords
            if (tx >= 0 && tx < mapSize.width && ty >= 0 && ty < mapSize.height)
            {
                int gid = layer->getTileGIDAt(Vec2(tx, ty));

                if (gid == 0) return false;

                if (gid > 0)
                {
                    Value properties = _map->getPropertiesForGID(gid);
                    if (!properties.isNull())
                    {
                        ValueMap dict = properties.asValueMap();
                        if (dict.find("Collidable") != dict.end())
                        {
                            if (dict["Collidable"].asString() == "true")
                                return false;
                        }
                    }
                }
            }
            else
            {
                // Out of map bounds
                return false;
            }
        }
    }

    // 4. Farm-Specific Checks
    if (_bgLayer && _bgLayer->getType() == BackgroundType::Farm)
    {
        // Check Water
        if (_bgLayer->isWater(pos)) return false;

        Rect animalRect(pos.x - 6, pos.y - 6, 12, 12);

        if (_bgLayer->isColliding(animalRect)) return false;
    }

    return true;
}

void AnimalSystem::removeAnimal(Animal* animal)
{
    // Implementation if needed
}

bool AnimalSystem::hasEgg(const Vec2& tilePos) const
{
    for (const auto& egg : _eggs)
    {
        if (egg.tilePos.equals(tilePos)) return true;
    }
    return false;
}

bool AnimalSystem::tryHarvestEgg(const Vec2& tilePos)
{
    std::vector<Vec2> checkPositions = {
        tilePos,
        tilePos + Vec2(1, 0), tilePos + Vec2(-1, 0),
        tilePos + Vec2(0, 1), tilePos + Vec2(0, -1),
        tilePos + Vec2(1, 1), tilePos + Vec2(-1, -1),
        tilePos + Vec2(1, -1), tilePos + Vec2(-1, 1)
    };

    for (const auto& checkPos : checkPositions)
    {
        for (auto it = _eggs.begin(); it != _eggs.end(); ++it)
        {
            if (it->tilePos.equals(checkPos))
            {
                if (GameScene::sInventory)
                {
                    Item eggItem;
                    eggItem.type = ItemType::Resource;
                    eggItem.name = it->isLarge ? "Large Egg" : "Egg";
                    eggItem.quantity = 1;
                    eggItem.maxStack = 99;

                    eggItem.iconPath = it->isLarge ? "animals/Large_Egg.png" : "animals/Egg.png";

                    GameScene::sInventory->addItem(eggItem);

                    if (true)
                    {
                        if (it->sprite)
                        {
                            it->sprite->removeFromParent();
                        }

                        _eggs.erase(it);
                        return true;
                    }
                }
                return false;
            }
        }
    }
    return false;
}

bool AnimalSystem::tryIncubateEgg(const Vec2& tilePos, const std::string& eggName)
{
    if (!_hasIncubator) return false;

    if (!_map) return false;
    Size tileSize = _map->getTileSize();
    Size mapSize = _map->getMapSize();
    float mapHeight = mapSize.height * tileSize.height;

    Rect tileRect(tilePos.x * tileSize.width, mapHeight - (tilePos.y + 1) * tileSize.height, tileSize.width, tileSize.height);

    if (_incubatorRect.intersectsRect(tileRect))
    {

        bool alreadyActive = false;
        for (const auto& inc : _incubators) {
            if (inc.active) { alreadyActive = true; break; }
        }

        if (!alreadyActive)
        {
            IncubatorData data;
            data.active = true;
            data.minutesRemaining = INCUBATION_MINUTES;
            data.hatchType = (eggName.find("Blue") != std::string::npos) ? Animal::Type::Blue : Animal::Type::White;

            data.hatchType = (rand() % 10 == 0) ? Animal::Type::Blue : Animal::Type::White;

            _incubators.push_back(data);

            std::string msg = "Incubating egg...";
            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);

            return true;
        }
        else
        {
            std::string msg = "Incubator is full.";
            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
        }
    }

    return false;
}

bool AnimalSystem::tryDepositHay(const cocos2d::Vec2& tilePos)
{
    if (!_hasHopper) return false;


    if (!_map) return false;
    Size tileSize = _map->getTileSize();
    Size mapSize = _map->getMapSize();
    float mapHeight = mapSize.height * tileSize.height;

    // Convert tilePos to rect
    Rect tileRect(tilePos.x * tileSize.width, mapHeight - (tilePos.y + 1) * tileSize.height, tileSize.width, tileSize.height);

    if (_hopperRect.intersectsRect(tileRect))
    {
        if (_hayStorage < MAX_HAY_STORAGE)
        {
            _hayStorage++;
            return true;
        }
        else
        {
            std::string msg = "Hay Hopper is full.";
            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
        }
    }
    return false;
}

bool AnimalSystem::tryWithdrawHay(const cocos2d::Vec2& tilePos)
{
    if (!_hasHopper) return false;
    if (!_map) return false;

    Size tileSize = _map->getTileSize();
    Size mapSize = _map->getMapSize();
    float mapHeight = mapSize.height * tileSize.height;

    Rect tileRect(tilePos.x * tileSize.width, mapHeight - (tilePos.y + 1) * tileSize.height, tileSize.width, tileSize.height);

    if (_hopperRect.intersectsRect(tileRect))
    {
        if (_hayStorage > 0)
        {
            _hayStorage--;

            // Give Hay to Player
            if (GameScene::sInventory)
            {
                Item hay;
                hay.type = ItemType::Resource;
                hay.name = "Hay";
                hay.iconPath = "block/Hay.png";
                hay.quantity = 1;
                hay.maxStack = 99;
                GameScene::sInventory->addItem(hay);
                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
            }
            return true;
        }
        else
        {
            std::string msg = "Hay Hopper is empty.";
            Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
        }
    }
    return false;
}

bool AnimalSystem::tryPlaceHay(const cocos2d::Vec2& tilePos)
{
    for (auto& t : _troughs)
    {
        if (t.tilePos.equals(tilePos))
        {
            if (!t.hasHay)
            {
                t.hasHay = true;

                // Create sprite immediately
                if (_layer && _map)
                {
                    Size tileSize = _map->getTileSize();
                    Size mapSize = _map->getMapSize();
                    float mapHeight = mapSize.height * tileSize.height;
                    float cx = (t.tilePos.x + 0.5f) * tileSize.width;
                    float cy = mapHeight - (t.tilePos.y + 0.5f) * tileSize.height;

                    auto sprite = Sprite::create("block/Hay.png");
                    if (!sprite) {
                        sprite = Sprite::create("Hay.png");
                    }
                    if (sprite)
                    {
                        sprite->setPosition(Vec2(cx, cy));

                        // Scale to fit tile (leaving a small margin)
                        float targetW = tileSize.width * 0.8f; // Reduced to 0.8 to be safe
                        float targetH = tileSize.height * 0.8f;
                        float scaleX = targetW / sprite->getContentSize().width;
                        float scaleY = targetH / sprite->getContentSize().height;
                        sprite->setScale(std::min(scaleX, scaleY));

                        if (_layer) _layer->addChild(sprite, 200);

                        t.sprite = sprite;
                    }
                    else
                    {
                        // Debug fallback
                        if (_layer) {
                            auto debugNode = DrawNode::create();
                            debugNode->drawDot(Vec2(cx, cy), 4.0f, Color4F::ORANGE);
                            _layer->addChild(debugNode, 200);
                        }
                    }
                }

                return true;
            }
            return false; // Already has hay
        }
    }
    return false;
}

