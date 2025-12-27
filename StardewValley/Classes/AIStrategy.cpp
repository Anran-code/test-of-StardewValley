#include "AIStrategy.h"
#include "Animal.h"

void Chicken_Rabbit_AI::decideNextState(Animal* animal)
{
    // Simple Random AI (Chicken/Rabbit)
    int r = rand() % 100;

    // 60% Chance to Walk
    if (r < 60)
    {
        animal->setState(Animal::State::Walk);
        animal->setStateTimer(1.0f + (rand() % 20) / 10.0f);

        int d = rand() % 4;
        animal->setDirection(static_cast<Animal::Direction>(d));
    }
    // 20% Chance to Idle
    else if (r < 80)
    {
        animal->setState(Animal::State::Idle);
        animal->setStateTimer(1.0f + (rand() % 30) / 10.0f);
    }
    // 20% Chance to Sit/Sleep
    else
    {
        animal->setState(Animal::State::Sit);
        animal->setStateTimer(3.0f + (rand() % 50) / 10.0f);
    }
}

void CatAI::decideNextState(Animal* animal)
{
    int r = rand() % 100;
    // 40% Walk
    if (r < 40)
    {
        animal->setState(Animal::State::Walk);
        animal->setStateTimer(2.0f + (rand() % 30) / 10.0f);
        int d = rand() % 4;
        animal->setDirection(static_cast<Animal::Direction>(d));
    }
    // 20% Sit
    else if (r < 60)
    {
        animal->setState(Animal::State::Sit);
        animal->setStateTimer(3.0f + (rand() % 30) / 10.0f);
    }
    // 15% Groom
    else if (r < 75)
    {
        animal->setState(Animal::State::Groom);
        animal->setStateTimer(4.0f); // Fixed duration for grooming loop
    }
    // 10% Lie Down
    else if (r < 85)
    {
        animal->setState(Animal::State::LieDown);
        animal->setStateTimer(5.0f);
    }
    // 15% Sleep
    else
    {
        animal->setState(Animal::State::Sleep);
        animal->setStateTimer(8.0f + (rand() % 50) / 10.0f);
    }
}
