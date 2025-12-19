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
    void resetEnergy(float multiplier = 1.0f);
    
    float getCurrentEnergy() const { return _currentEnergy; }
    float getMaxEnergy() const { return _maxEnergy; }
    
    bool isExhausted() const { return _currentEnergy <= 0.0f; }

    // Constants
    static const float DEFAULT_MAX_ENERGY;
    static const float TOOL_USAGE_COST; // Legacy, kept for compatibility if needed
    static const float COST_HOE;
    static const float COST_WATERING_CAN;
    static const float COST_AXE;
    static const float COST_PICKAXE;
    static const float COST_FISHING;

private:
    EnergySystem();
    static EnergySystem* sInstance;

    float _currentEnergy;
    float _maxEnergy;
};

#endif
