#ifndef __ANIMAL_H__
#define __ANIMAL_H__

#include "cocos2d.h"

class Animal : public cocos2d::Sprite
{
public:
    enum class Type { Blue, White };
    enum class Age { Baby, Adult };
    enum class Direction { Down, Right, Up, Left };
    enum class State { Idle, Walk, Eat, Sleep, Sit };

    static Animal* create(Type type, Age age);

    bool init(Type type, Age age);
    void update(float dt) override;

    // Actions
    void setDirection(Direction dir);
    void setState(State state);
    void walkRandomly();
    void stopMoving();
    
    // Growth
    void growUp();
    void incrementDaysAlive() { _daysAlive++; }
    int getDaysAlive() const { return _daysAlive; }
    Age getAge() const { return _age; }
    Type getType() const { return _type; }
    
    void pickNewState();

    // Force start eating action
    void startEating();

    bool isFed() const { return _isFed; }
    void setFed(bool val) { _isFed = val; }

    // Location
    enum class Location { Inside, Outside };
    void setLocation(Location loc) { _location = loc; }
    Location getLocation() const { return _location; }
    State getCurrentState() const { return _currentState; }

private:
    void initAnimations();
    void updateAnimation();
    cocos2d::Vec2 getDirectionVector() const;

private:
    Type _type;
    Age _age;
    Location _location;
    Direction _currentDirection;
    State _currentState;
    
    float _stateTimer; 
    float _moveSpeed;
    int _daysAlive;
    bool _isFed;
    
    // Animation Cache
    cocos2d::Map<std::string, cocos2d::Animation*> _animations;
    
    // Texture details
    std::string _textureFile;
    cocos2d::Size _cellSize; 
};

#endif // __ANIMAL_H__
