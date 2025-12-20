#ifndef __ANIMAL_SYSTEM_H__
#define __ANIMAL_SYSTEM_H__

#include "cocos2d.h"
#include "Animal.h"
#include "GameClock.h"
#include <vector>

class AnimalSystem
{
public:
    static AnimalSystem* getInstance();
    
    // Lifecycle
    void init(cocos2d::Layer* layer, cocos2d::TMXTiledMap* map);
    void updateDailyGrowth(); // Called when day changes
    void update(float dt); // Called every frame for movement/AI
    
    // Animal Management
    void addAnimal(Animal::Type type, Animal::Age age, const cocos2d::Vec2& position);
    void removeAnimal(Animal* animal);
    
    // Interaction
    bool tryHarvestEgg(const cocos2d::Vec2& tilePos);
    bool tryIncubateEgg(const cocos2d::Vec2& tilePos, const std::string& eggName);
    
    // Getters
    const std::vector<Animal*>& getAnimals() const { return _animals; }
    
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
        cocos2d::Sprite* sprite;
        bool isLarge;
        Animal::Type type; // Blue or White egg
    };

    static AnimalSystem* sInstance;
    
    cocos2d::Layer* _layer;
    cocos2d::TMXTiledMap* _map;
    
    std::vector<Animal*> _animals;
    std::vector<EggData> _eggs;
    std::vector<IncubatorData> _incubators; // Can be multiple if multiple incubators exist
    
    // Constants
    const int DAYS_TO_MATURE = 3;
    const int INCUBATION_MINUTES = 9000;
};

#endif // __ANIMAL_SYSTEM_H__
