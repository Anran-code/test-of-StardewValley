#ifndef __ENERGY_SYSTEM_H__
#define __ENERGY_SYSTEM_H__

#include "cocos2d.h"

class EnergySystem
{
public:
    static EnergySystem* getInstance();

    bool init();
    
    // Energy management
    void consumeEnergy(float amount);
    void restoreEnergy(float amount);
    void setEnergy(float amount);
    void resetEnergy();
    
    float getCurrentEnergy() const { return _currentEnergy; }
    float getMaxEnergy() const { return _maxEnergy; }
    
    bool isExhausted() const { return _currentEnergy <= 0.0f; }

    // Constants
    static const float DEFAULT_MAX_ENERGY;
    static const float TOOL_USAGE_COST;

private:
    EnergySystem();
    static EnergySystem* sInstance;

    float _currentEnergy;
    float _maxEnergy;
};

#endif
