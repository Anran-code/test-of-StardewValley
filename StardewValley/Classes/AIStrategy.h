#ifndef __AI_STRATEGY_H__
#define __AI_STRATEGY_H__

#include "cocos2d.h"

class Animal; // Forward declaration

class AIStrategy {
public:
    virtual ~AIStrategy() {}
    virtual void decideNextState(Animal* animal) = 0;
};

class Chicken_Rabbit_AI : public AIStrategy {
public:
    void decideNextState(Animal* animal) override;
};

class CatAI : public AIStrategy {
public:
    void decideNextState(Animal* animal) override;
};

#endif // __AI_STRATEGY_H__
