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
    , _map(nullptr)
{
}

AnimalSystem::~AnimalSystem()
{
    _animals.clear();
    _eggs.clear();
}

void AnimalSystem::init(Layer* layer, TMXTiledMap* map)
{
    _layer = layer;
    _map = map;
        
    if (_animals.empty())
    {
        if (map)
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
            if (_layer) _layer->addChild(animal, 5);
        }
    }
    
    // Re-create egg sprites
    for (auto& egg : _eggs)
    {
        if (egg.sprite) 
        {
            egg.sprite->removeFromParent();
            egg.sprite = nullptr;
        }
        
        // Recreate sprite
        std::string filename;
        if (egg.type == Animal::Type::Blue)
            filename = egg.isLarge ? "animals/Large Blue Egg.png" : "animals/Blue Egg.png"; // Assuming names
        else
            filename = egg.isLarge ? "animals/Large White Egg.png" : "animals/White Egg.png"; // Assuming names
            
        // Check if file exists, fallback if needed
        if (!FileUtils::getInstance()->isFileExist(filename))
        {
        }
        
        auto sprite = Sprite::create(filename);
        if (sprite)
        {
            Size tileSize = _map->getTileSize();
            Size mapSize = _map->getMapSize();
            float mapHeight = mapSize.height * tileSize.height;
            float cx = (egg.tilePos.x + 0.5f) * tileSize.width;
            float cy = mapHeight - (egg.tilePos.y + 0.5f) * tileSize.height;
            
            sprite->setPosition(Vec2(cx, cy));
            sprite->setScale(0.8f); // Eggs are small
            if (_layer) _layer->addChild(sprite, 2); // Below animals
            egg.sprite = sprite;
        }
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
    // 1. Grow Animals
    for (auto animal : _animals)
    {
        
        animal->incrementDaysAlive();
        if (animal->getAge() == Animal::Age::Baby && animal->getDaysAlive() >= DAYS_TO_MATURE)
        {
            animal->growUp(); 
        }
        
        // 2. Produce Eggs (Adults only)
        if (animal->getAge() == Animal::Age::Adult)
        {
            
            // Spawn Egg at animal position
            Vec2 pos = animal->getPosition();
            
            // Convert to tile coordinate
            if (_map)
            {
                // Simple conversion
                Size tileSize = _map->getTileSize();
                Size mapSize = _map->getMapSize();
                float mapHeight = mapSize.height * tileSize.height;
                
                int tx = pos.x / tileSize.width;
                int ty = (mapHeight - pos.y) / tileSize.height;
                
                // Avoid spawning on top of existing eggs?
                bool occupied = false;
                for (const auto& e : _eggs) {
                    if (e.tilePos.x == tx && e.tilePos.y == ty) { occupied = true; break; }
                }
                
                if (!occupied)
                {
                    // Large Egg Chance (e.g. 20%)
                    bool isLarge = (rand() % 100) < 20;
                    
                    EggData newEgg;
                    newEgg.tilePos = Vec2(tx, ty);
                    newEgg.isLarge = isLarge;
                    newEgg.type = animal->getType();
                    newEgg.sprite = nullptr; 
                    
                    // Create Sprite immediately
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
}

void AnimalSystem::update(float dt)
{
    // Update animals (movement AI)
    for (auto animal : _animals)
    {
        animal->update(dt);
        
        // Simple boundary check if map is set
        if (_map)
        {
            Size mapSize = _map->getMapSize();
            Size tileSize = _map->getTileSize();
            float mapW = mapSize.width * tileSize.width;
            float mapH = mapSize.height * tileSize.height;
            
            Vec2 pos = animal->getPosition();
            float halfSize = 16.0f; // Approx radius
            
            if (pos.x < halfSize) pos.x = halfSize;
            if (pos.x > mapW - halfSize) pos.x = mapW - halfSize;
            if (pos.y < halfSize) pos.y = halfSize;
            if (pos.y > mapH - halfSize) pos.y = mapH - halfSize;
            
            animal->setPosition(pos);
            
            // Z-Order
            if (_layer)
            {
                int z = static_cast<int>(mapH - pos.y);
                animal->setLocalZOrder(z);
            }
        }
    }
    
    // Update Incubators
    for (auto& inc : _incubators)
    {
        if (inc.active)
        {
            
            static float timer = 0;
            timer += dt;
            if (timer >= 0.05f) // Fast tick
            {
                // inc.minutesRemaining--; // Needs proper time syncing
                timer = 0;
            }
        }
    }
}

bool AnimalSystem::tryHarvestEgg(const Vec2& tilePos)
{
    // Check if egg exists at tilePos
    for (auto it = _eggs.begin(); it != _eggs.end(); ++it)
    {
        // Allow slight imprecision? Or exact tile?
        // Using exact tile coordinates
        if (it->tilePos.equals(tilePos))
        {
            // Harvest!
            // Add to inventory
            if (GameScene::sInventory)
            {
                Item eggItem;
                eggItem.type = ItemType::Resource; // Or AnimalProduct
                eggItem.name = it->isLarge ? "Large Egg" : "Egg"; // Generic name or specific?
                if (it->type == Animal::Type::Blue) eggItem.name = it->isLarge ? "Large Blue Egg" : "Blue Egg";
                else eggItem.name = it->isLarge ? "Large White Egg" : "White Egg";
                
                // Icon path
                if (it->type == Animal::Type::Blue)
                    eggItem.iconPath = it->isLarge ? "animals/Large Blue Egg.png" : "animals/Blue Egg.png";
                else
                    eggItem.iconPath = it->isLarge ? "animals/Large White Egg.png" : "animals/White Egg.png";
                    
                eggItem.quantity = 1;
                eggItem.maxStack = 99;
                
                GameScene::sInventory->addItem(eggItem);
                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("INVENTORY_UPDATED");
                
                // Show notification
                std::string msg = "You picked up a " + eggItem.name;
                Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
            }
            
            // Remove sprite
            if (it->sprite) it->sprite->removeFromParent();
            _eggs.erase(it);
            return true;
        }
    }
    return false;
}

bool AnimalSystem::tryIncubateEgg(const Vec2& tilePos, const std::string& eggName)
{
    // Check if tilePos matches Incubator position (EggBucket object in TMX)
    if (!_map) return false;
    
    auto objectGroup = _map->getObjectGroup("henhouse"); // User said "henhouse object layer"
    if (!objectGroup) return false;
    
    auto dict = objectGroup->getObject("eggbucket"); // User said "eggbucket"
    if (dict.empty()) return false;
    
    float x = dict["x"].asFloat();
    float y = dict["y"].asFloat();
    float w = dict["width"].asFloat();
    float h = dict["height"].asFloat();
    
    Size tileSize = _map->getTileSize();
    Size mapSize = _map->getMapSize();
    float mapH = mapSize.height * tileSize.height;
    
    Rect bucketRect(x, y, w, h);
    
    // Check if clicked tile is inside bucket rect
    float tx = tilePos.x * tileSize.width;
    float ty = (mapSize.height - 1 - tilePos.y) * tileSize.height; // Top-left origin tile to Y-up world
    Rect tileRect(tx, ty, tileSize.width, tileSize.height);
    
    if (bucketRect.intersectsRect(tileRect))
    {
        // Start Incubation
        IncubatorData data;
        data.active = true;
        data.minutesRemaining = 9000;
        data.tilePos = tilePos;
        
        // Determine hatch type from egg name
        if (eggName.find("Blue") != std::string::npos) data.hatchType = Animal::Type::Blue;
        else data.hatchType = Animal::Type::White;
        
        _incubators.push_back(data);
        
        std::string msg = "Egg placed in incubator.";
        Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("SHOW_NOTIFICATION", &msg);
        
        return true;
    }
    
    return false;
}
