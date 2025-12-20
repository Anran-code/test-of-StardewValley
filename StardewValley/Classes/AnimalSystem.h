#ifndef __ANIMAL_SYSTEM_H__
#define __ANIMAL_SYSTEM_H__

#include "cocos2d.h"
#include "Animal.h"
#include "GameClock.h"
#include <vector>

class BackgroundLayer;

class AnimalSystem
{
public:
    static AnimalSystem* getInstance();
    
    // Lifecycle
    void init(BackgroundLayer* layer, cocos2d::TMXTiledMap* map);
    void cleanupVisuals();
    void updateDailyGrowth(); // Called when day changes
    void update(float dt); // Called every frame for movement/AI
    
    // Check if a position is valid for animal movement
    bool isWalkable(const cocos2d::Vec2& pos);
    
    // Animal Management
    void addAnimal(Animal::Type type, Animal::Age age, const cocos2d::Vec2& position);
    void removeAnimal(Animal* animal);
    
    // Interaction
    bool tryHarvestEgg(const cocos2d::Vec2& tilePos);
    bool tryIncubateEgg(const cocos2d::Vec2& tilePos, const std::string& eggName);
    
    // Feeding
    bool tryDepositHay(const cocos2d::Vec2& tilePos); // Put Hay into Hopper
    bool tryWithdrawHay(const cocos2d::Vec2& tilePos); // Take Hay from Hopper
    bool tryPlaceHay(const cocos2d::Vec2& tilePos); // Put Hay into Trough
    
    // Getters
    const std::vector<Animal*>& getAnimals() const { return _animals; }
    int getHayCount() const { return _hayStorage; }
    
private:
    AnimalSystem();
    ~AnimalSystem();
    
    struct IncubatorData {
        bool active;
        int minutesRemaining;
        Animal::Type hatchType;
        cocos2d::Vec2 tilePos;
    };
    
    struct EggData {
        cocos2d::Vec2 tilePos;
        cocos2d::Vec2 worldPos;
        cocos2d::Sprite* sprite;
        bool isLarge;
        Animal::Type type; // Blue or White egg
    };
    
    struct TroughData {
        cocos2d::Vec2 tilePos;
        bool hasHay;
        cocos2d::Sprite* sprite; // Hay sprite
    };

    static AnimalSystem* sInstance;
    
    BackgroundLayer* _bgLayer;
    cocos2d::Layer* _layer;
    cocos2d::TMXTiledMap* _map;
    
    std::vector<Animal*> _animals;
    std::vector<EggData> _eggs;
    std::vector<IncubatorData> _incubators; 
    std::vector<TroughData> _troughs;
    
    cocos2d::Rect _hopperRect;
    bool _hasHopper = false;
    
    int _hayStorage = 0;
    const int MAX_HAY_STORAGE = 20;
    
    // Constants
    const int DAYS_TO_MATURE = 3;
    const int INCUBATION_MINUTES = 9000;
};

#endif // __ANIMAL_SYSTEM_H__
