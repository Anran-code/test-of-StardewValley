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
    _hasHopper = false;
    // _troughs.clear(); // Don't clear immediately, we might want to persist hay?
    // Actually, if we reload map, positions are same.
    // If _troughs is empty, parse. If not, just rebind sprites.
    
    // Determine context (Farm or Henhouse)
    // We can infer from map name or layer type but AnimalSystem doesn't know.
    // However, init is called by GameScene.
    // Let's assume if map has "henhouse" object group, it's henhouse.
    // If it has "farm" logic or we are in Farm scene...
    // Actually, we should know if we are in Farm or Henhouse.
    // But since we don't have that param, let's detect.
    
    bool isHenhouse = false;
    if (_map) {
        if (_map->getObjectGroup("henhouse")) isHenhouse = true;
    }
    
    // Parse Objects (Only if Henhouse)
    if (isHenhouse && _map)
    {
        auto objectGroup = _map->getObjectGroup("henhouse");
        if (objectGroup)
        {
            // Hopper (Bucket in TMX, stores hay)
            auto hopperMap = objectGroup->getObject("bucket");
            if (!hopperMap.empty())
            {
                 float x = hopperMap["x"].asFloat();
                 float y = hopperMap["y"].asFloat();
                 float w = hopperMap["width"].asFloat();
                 float h = hopperMap["height"].asFloat();
                 _hopperRect = Rect(x, y, w, h);
                 _hasHopper = true;
            }
            
            // Troughs (FeedingHopper in TMX, eating spot)
            auto bucketMap = objectGroup->getObject("feedinghopper");
            if (!bucketMap.empty())
            {
                 // If it's the first time, populate _troughs
                 if (_troughs.empty())
                 {
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
    }
        
    if (_animals.empty())
    {
        if (map && isHenhouse)
        {
            Size mapSize = map->getMapSize();
            Size tileSize = map->getTileSize();
            float centerX = mapSize.width * tileSize.width * 0.5f;
            float centerY = mapSize.height * tileSize.height * 0.5f;
            
            addAnimal(Animal::Type::Blue, Animal::Age::Baby, Vec2(centerX - 50, centerY));
            addAnimal(Animal::Type::White, Animal::Age::Baby, Vec2(centerX + 50, centerY));
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
                 // Safety Check: Ensure position is within bounds
                 // If moving from Farm (Large) to Henhouse (Small), pos might be out of bounds.
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
    
    // Re-create egg sprites (Only in Henhouse)
    if (isHenhouse)
    {
        for (auto& egg : _eggs)
        {
            if (egg.sprite) 
            {
                egg.sprite->removeFromParent();
                egg.sprite = nullptr;
            }
            
            std::string filename;
            if (egg.type == Animal::Type::Blue)
                filename = egg.isLarge ? "animals/Large Blue Egg.png" : "animals/Blue Egg.png";
            else
                filename = egg.isLarge ? "animals/Large White Egg.png" : "animals/White Egg.png";
                
            if (!FileUtils::getInstance()->isFileExist(filename)) continue;
            
            auto sprite = Sprite::create(filename);
            if (sprite)
            {
                Size tileSize = _map->getTileSize();
                Size mapSize = _map->getMapSize();
                float mapHeight = mapSize.height * tileSize.height;
                float cx = (egg.tilePos.x + 0.5f) * tileSize.width;
                float cy = mapHeight - (egg.tilePos.y + 0.5f) * tileSize.height;
                
                sprite->setPosition(Vec2(cx, cy));
                sprite->setScale(0.8f);
                if (_layer) _layer->addChild(sprite, 2);
                egg.sprite = sprite;
            }
        }
        
        // Re-create Hay Sprites
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
                if (sprite)
                {
                    Size tileSize = _map->getTileSize();
                    Size mapSize = _map->getMapSize();
                    float mapHeight = mapSize.height * tileSize.height;
                    float cx = (t.tilePos.x + 0.5f) * tileSize.width;
                    float cy = mapHeight - (t.tilePos.y + 0.5f) * tileSize.height;
                    
                    sprite->setPosition(Vec2(cx, cy));
                    // sprite->setScale(0.8f); 
                    if (_layer) _layer->addChild(sprite, 2);
                    t.sprite = sprite;
                }
            }
        }
    }
}
void AnimalSystem::cleanupVisuals()
{
    _layer = nullptr;
    _map = nullptr;
    for (auto& egg : _eggs) egg.sprite = nullptr;
    for (auto& trough : _troughs) trough.sprite = nullptr;
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
        
        // 1. Try eat Hay from Trough (Only if Inside and not yet fed)
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
            // If outside, assume they ate grass (handled in update loop usually, but for simplicity assume fed if outside in non-winter)
            // Stardew logic: Animals eat grass during the day. If they ate, they are full.
            // If they didn't find grass, they are hungry.
            // But updateDailyGrowth runs at NIGHT/MORNING transition.
            // If they were outside yesterday, did they eat?
            // Let's assume for now if they are outside, they are fed (unless Winter/Rain).
            
            fed = true; // <--- FIX: Assume fed if outside
            
            // For now, let's keep it simple: Reset location to Inside at night?
            // Usually animals go back inside at 5pm.
            // If door is closed, they stay outside and get grumpy/attacked by wolves.
            // User request: "set a feature where chickens can go out ... at 6 am".
            
            // Logic:
            // At 6 AM (New Day):
            // Check weather (not implemented yet, assume Sunny).
            // Move animals Outside.
            // Set position to Farm Henhouse Door.
            
            // Wait, updateDailyGrowth is called at New Day (6 AM).
            // So we move them out here.
            
            animal->setLocation(Animal::Location::Outside);
            
            // Set position (Need Farm info, but we might be in House scene)
            // When we load Farm scene, init() will place them.
            // But we should set a default spawn pos on Farm.
            // Henhouse door on Farm is at GameScene::sFarmHenhouseDoorPos
            if (GameScene::sHasFarmHenhouseDoorPos)
            {
                // Add some random offset so they don't stack
                float offX = (rand() % 100) - 50;
                float offY = (rand() % 100) - 100; // Move down from door
                animal->setPosition(GameScene::sFarmHenhouseDoorPos + Vec2(offX, offY));
            }
            else
            {
                // Fallback
                animal->setPosition(Vec2(500, 300));
            }
        }
        
        // 2. Growth / Produce
        // ... (Existing logic)
        if (fed)
        {
            animal->incrementDaysAlive();
            if (animal->getAge() == Animal::Age::Baby && animal->getDaysAlive() >= DAYS_TO_MATURE)
            {
                animal->growUp(); 
            }
            
            // Produce Eggs (Adults only)
            if (animal->getAge() == Animal::Age::Adult)
            {
                Vec2 pos = animal->getPosition();
                if (_map) // If map is not loaded (sleeping elsewhere), this might fail to spawn egg visually
                {
                    // If map is null, we can't calculate tile pos easily unless we track animal tile pos separately.
                    // For now, assume eggs only spawn if map is loaded or handle deferred spawn?
                    // Let's assume egg is spawned where animal IS.
                    // If animal is off-screen/mapless, we skip egg or use last known pos?
                    // Animals are persistent, so their position is valid.
                    
                    // But we need tile size to snap.
                    // If _map is null (we are at Home), we can't spawn egg into the scene.
                    // But we should record the egg existence.
                    
                    // Issue: If we are at Home, _map is Home map.
                    // AnimalSystem _map might be stale or null.
                    // If _map is null, we can't convert pos to tile.
                    
                    // Workaround: Store last known tile pos in Animal?
                    // Or just skip egg production if not in Henhouse? (Unfair)
                    // Or auto-collect?
                    
                    // For now, let's just proceed with existing logic.
                    // If _map is valid (Henhouse loaded), spawn.
                    // If not, maybe just skip for this iteration of development.
                }
                
                if (_map)
                {
                    Size tileSize = _map->getTileSize();
                    Size mapSize = _map->getMapSize();
                    float mapHeight = mapSize.height * tileSize.height;
                    
                    int tx = pos.x / tileSize.width;
                    int ty = (mapHeight - pos.y) / tileSize.height;
                    
                    bool occupied = false;
                    for (const auto& e : _eggs) {
                        if (e.tilePos.x == tx && e.tilePos.y == ty) { occupied = true; break; }
                    }
                    
                    if (!occupied)
                    {
                        bool isLarge = (rand() % 100) < 20;
                        EggData newEgg;
                        newEgg.tilePos = Vec2(tx, ty);
                        newEgg.isLarge = isLarge;
                        newEgg.type = animal->getType();
                        newEgg.sprite = nullptr;
                        
                        std::string filename;
                        if (newEgg.type == Animal::Type::Blue)
                            filename = isLarge ? "animals/Large Blue Egg.png" : "animals/Blue Egg.png";
                        else
                            filename = isLarge ? "animals/Large White Egg.png" : "animals/White Egg.png";

                        auto sprite = Sprite::create(filename);
                        if (sprite)
                        {
                            float cx = (tx + 0.5f) * tileSize.width;
                            float cy = mapHeight - (ty + 0.5f) * tileSize.height;
                            sprite->setPosition(Vec2(cx, cy));
                            sprite->setScale(0.8f);
                            if (_layer) _layer->addChild(sprite, 2);
                            newEgg.sprite = sprite;
                        }
                        _eggs.push_back(newEgg);
                    }
                }
            }
        }
        else
        {
            // Starve
            // Optional: visual feedback
        }

        // Reset for new day
        animal->setFed(false);
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
            
            // 2. Update Visual State (Continuous Check)
            // If we are currently on Farm, remove them regardless of previous state
            if (isFarm)
            {
                if (animal->getParent()) animal->removeFromParent();
            }
            // If we are currently in Henhouse, add them if missing
            else if (isHenhouse)
            {
                if (!animal->getParent() && _layer)
                {
                     // Reset position to center of Henhouse if just added
                     // (We can't easily track if they were just added this frame or earlier, 
                     //  but resetting pos every frame is bad. 
                     //  However, if they are already in parent, we skip this block.
                     //  So this only happens ONCE when they reappear.)
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

    // Check if we are in Farm scene to handle eating grass
    bool isFarm = false;
    // We don't have direct access to Scene type here easily, but we can check if animals are Outside and _layer is valid.
    // If animals are outside and rendered, we are likely in Farm.
    
    // Better: init() sets context. If we initialized in Farm (no henhouse group), isFarm = true?
    // Let's use a member var or just check map name if possible?
    // Or just check if animals are outside.
    
    for (auto animal : _animals)
    {
        Vec2 oldPos = animal->getPosition();
        animal->update(dt);
        
        // Eating Grass Logic (Outside)
        if (animal->getLocation() == Animal::Location::Outside && 
            animal->getCurrentState() == Animal::State::Eat)
        {
            // Try to find grass at current position
            // Access obstacles from BackgroundLayer?
            // AnimalSystem doesn't have friend access to BackgroundLayer private members.
            // But we can check via GameScene::getBackgroundLayer()? 
            // Or just assume we can cast _layer to BackgroundLayer if we include it.
            
            // For now, let's use a simple approach:
            // If we are in Farm (inferred), check tile.
            if (_map && _layer)
            {
                Size tileSize = _map->getTileSize();
                Size mapSize = _map->getMapSize();
                float mapHeight = mapSize.height * tileSize.height;
                Vec2 pos = animal->getPosition();
                
                int tx = pos.x / tileSize.width;
                int ty = (mapHeight - pos.y) / tileSize.height;
                
                // We need a way to check/remove grass.
                // BackgroundLayer has public hasObstacle/getObstacleType, but remove is private.
                // Wait, removeObstacle is private in BackgroundLayer.
                // We might need to expose it or use an event.
                // Or maybe just let them eat "virtual" grass for now?
                // User said "chicken can eat grass in the farm".
                
                // Let's try to cast _layer to BackgroundLayer
                 // auto bgLayer = dynamic_cast<BackgroundLayer*>(_layer);
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
                // Force AI to pick new direction/state to avoid getting stuck
                animal->pickNewState();
            }
            else
            {
                // Z-order based on Y
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

    // 1. Boundary Check (Henhouse Interior Walls)
    // Use the actual tile map size to determine boundaries
    // Tiled Map usually has "wall" tiles. 
    // If we want to be precise, we should check for "Collidable" property on tiles.
    // But for now, we'll use a hardcoded margin based on typical map layout.
    
    // Assuming 1 tile border around the room is wall
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
                
                // If no tile (gid == 0), it's a void/black area -> Not Walkable
                // EXCEPT on Farm where ground might be on different layers or handled differently?
                // On Farm, we might have grass tiles.
                // But for Henhouse Interior, void is definitely wall.
                // Let's assume if gid == 0 it's not walkable for now.
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
    
    // 4. Farm-Specific Checks (Buildings, Water, etc.)
    // We can access GameScene singleton or check _layer type
    // But direct access is cleaner if we cast _layer
    // auto bgLayer = dynamic_cast<BackgroundLayer*>(_layer);
    if (_bgLayer && _bgLayer->getType() == BackgroundType::Farm)
    {
        // Check Water
        if (_bgLayer->isWater(pos)) return false;
        
        // Check Collisions (Buildings, Obstacles)
        // We need to check if 'pos' (feet) is inside any obstacle or building rect
        // Create a small rect for the animal
        Rect animalRect(pos.x - 6, pos.y - 6, 12, 12);
        
        if (_bgLayer->isColliding(animalRect)) return false;
    }

    return true;
}

void AnimalSystem::removeAnimal(Animal* animal)
{
    // Implementation if needed
}

bool AnimalSystem::tryHarvestEgg(const Vec2& tilePos)
{
    for (auto it = _eggs.begin(); it != _eggs.end(); ++it)
    {
        if (it->tilePos.equals(tilePos))
        {
            if (GameScene::sInventory)
            {
                Item eggItem;
                eggItem.type = ItemType::Resource; 
                eggItem.name = it->isLarge ? "Large Egg" : "Egg"; 
                if (it->type == Animal::Type::Blue) eggItem.name = it->isLarge ? "Large Blue Egg" : "Blue Egg";
                else eggItem.name = it->isLarge ? "Large White Egg" : "White Egg";
                
                if (it->type == Animal::Type::Blue)
                    eggItem.iconPath = it->isLarge ? "animals/Large Blue Egg.png" : "animals/Blue Egg.png";
                else
                    eggItem.iconPath = it->isLarge ? "animals/Large White Egg.png" : "animals/White Egg.png";
                    
                eggItem.quantity = 1;
                eggItem.maxStack = 99;
                
                GameScene::sInventory->addItem(eggItem);
                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
                
                std::string msg = "You picked up a " + eggItem.name;
                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
            }
            
            if (it->sprite) it->sprite->removeFromParent();
            _eggs.erase(it);
            return true;
        }
    }
    return false;
}

bool AnimalSystem::tryIncubateEgg(const Vec2& tilePos, const std::string& eggName)
{
    // Simplified: Just consume egg and start timer
    // We need to check if tilePos matches an incubator object
    // For now, assume any click on incubator works.
    // ... Implementation logic from previous steps or placeholders ...
    return false; // Placeholder
}

bool AnimalSystem::tryDepositHay(const cocos2d::Vec2& tilePos)
{
    if (!_hasHopper) return false;
    
    // Check if tilePos is inside hopper rect
    // Need to convert tilePos to world or rect to tiles.
    // tilePos is in Grid coordinates.
    // _hopperRect is in Pixels (Cocos space).
    
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
                
                // Create sprite
                if (_layer && _map)
                {
                    Size tileSize = _map->getTileSize();
                    Size mapSize = _map->getMapSize();
                    float mapHeight = mapSize.height * tileSize.height;
                    float cx = (t.tilePos.x + 0.5f) * tileSize.width;
                    float cy = mapHeight - (t.tilePos.y + 0.5f) * tileSize.height;
                    
                    auto sprite = Sprite::create("block/Hay.png");
                    if (sprite)
                    {
                        sprite->setPosition(Vec2(cx, cy));
                        _layer->addChild(sprite, 2);
                        t.sprite = sprite;
                    }
                }
                return true;
            }
            return false; // Already has hay
        }
    }
    return false;
}

