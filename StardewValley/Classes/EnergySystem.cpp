#include "EnergySystem.h"

EnergySystem* EnergySystem::sInstance = nullptr;

const float EnergySystem::DEFAULT_MAX_ENERGY = 270.0f;
const float EnergySystem::TOOL_USAGE_COST = 2.0f;
const float EnergySystem::COST_HOE = 2.0f;
const float EnergySystem::COST_WATERING_CAN = 2.0f;
const float EnergySystem::COST_AXE = 2.0f;
const float EnergySystem::COST_PICKAXE = 2.0f;
const float EnergySystem::COST_FISHING = 8.0f;

EnergySystem* EnergySystem::getInstance()
{
    if (!sInstance)
    {
        sInstance = new EnergySystem();
        sInstance->init();
    }
    return sInstance;
}

EnergySystem::EnergySystem() 
: _currentEnergy(DEFAULT_MAX_ENERGY)
, _maxEnergy(DEFAULT_MAX_ENERGY)
{
}

bool EnergySystem::init()
{
    _currentEnergy = _maxEnergy;
    return true;
}

void EnergySystem::consumeEnergy(float amount)
{
    _currentEnergy -= amount;
    if (_currentEnergy < 0.0f) _currentEnergy = 0.0f;
    
    // Notify UI updates
    cocos2d::Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("ENERGY_UPDATED");
}

void EnergySystem::restoreEnergy(float amount)
{
    _currentEnergy += amount;
    if (_currentEnergy > _maxEnergy) _currentEnergy = _maxEnergy;
    
    // Notify UI updates
    cocos2d::Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("ENERGY_UPDATED");
}

void EnergySystem::setEnergy(float amount)
{
    _currentEnergy = amount;
    if (_currentEnergy < 0.0f) _currentEnergy = 0.0f;
    if (_currentEnergy > _maxEnergy) _currentEnergy = _maxEnergy;
    
    // Notify UI updates
    cocos2d::Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("ENERGY_UPDATED");
}

void EnergySystem::resetEnergy(float multiplier)
{
    _currentEnergy = _maxEnergy * multiplier;
    if (_currentEnergy > _maxEnergy) _currentEnergy = _maxEnergy;
    // Notify UI updates
    cocos2d::Director::getInstance()->getEventDispatcher()->dispatchCustomEvent("ENERGY_UPDATED");
}
