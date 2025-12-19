#include "ExperienceSystem.h"

USING_NS_CC;

ExperienceSystem* ExperienceSystem::s_instance = nullptr;

ExperienceSystem* ExperienceSystem::getInstance()
{
    if (!s_instance)
    {
        s_instance = new ExperienceSystem();
        s_instance->init();
    }
    return s_instance;
}

void ExperienceSystem::destroyInstance()
{
    if (s_instance)
    {
        delete s_instance;
        s_instance = nullptr;
    }
}

ExperienceSystem::ExperienceSystem()
{
}

ExperienceSystem::~ExperienceSystem()
{
}

void ExperienceSystem::init()
{
    loadXPThresholds();
    
    // Initialize skills to Level 0, 0 XP
    _currentExperience[SkillType::Farming] = 0;
    _currentLevel[SkillType::Farming] = 0;
    
    _currentExperience[SkillType::Fishing] = 0;
    _currentLevel[SkillType::Fishing] = 0;
}

void ExperienceSystem::loadXPThresholds()
{
    // Stardew Valley XP Curve (Total XP required to reach level N)
    // Level 0 -> 1: 100
    // Level 1 -> 2: 380
    // ...
    _xpThresholds = { 
        100,    // Level 1
        380,    // Level 2
        770,    // Level 3
        1300,   // Level 4
        2150,   // Level 5
        3300,   // Level 6
        4800,   // Level 7
        6900,   // Level 8
        10000,  // Level 9
        15000   // Level 10
    };
}

std::string ExperienceSystem::getSkillName(SkillType skill) const
{
    switch (skill)
    {
        case SkillType::Farming: return "Farming";
        case SkillType::Fishing: return "Fishing";
        default: return "Unknown";
    }
}

bool ExperienceSystem::addExperience(SkillType skill, int amount)
{
    if (amount <= 0) return false;
    
    int currentLevel = _currentLevel[skill];
    if (currentLevel >= getMaxLevel()) return false; // Max level reached

    _currentExperience[skill] += amount;
    
    // Notification for XP gain
    std::string skillName = getSkillName(skill);
    std::string msg = "+" + std::to_string(amount) + " XP (" + skillName + ")";
    
    // Dispatch event for UI
    EventCustom event("SHOW_NOTIFICATION");
    event.setUserData(&msg);
    Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);

    checkLevelUp(skill);
    return true;
}

void ExperienceSystem::checkLevelUp(SkillType skill)
{
    int currentXP = _currentExperience[skill];
    int currentLevel = _currentLevel[skill];
    
    // Check if we can level up
    // Thresholds are 0-indexed for Level 1..10
    // e.g. Level 0 -> need _xpThresholds[0] (100) to be Level 1
    
    while (currentLevel < getMaxLevel())
    {
        int requiredXP = _xpThresholds[currentLevel]; // Threshold for next level
        if (currentXP >= requiredXP)
        {
            currentLevel++;
            _currentLevel[skill] = currentLevel;
            
            // Level Up Notification
            std::string skillName = getSkillName(skill);
            std::string msg = "LEVEL UP! " + skillName + " is now Level " + std::to_string(currentLevel);
            
            EventCustom event("SHOW_NOTIFICATION");
            event.setUserData(&msg);
            Director::getInstance()->getEventDispatcher()->dispatchEvent(&event);
            
            // Play sound? (Optional)
        }
        else
        {
            break;
        }
    }
}

int ExperienceSystem::getLevel(SkillType skill) const
{
    auto it = _currentLevel.find(skill);
    if (it != _currentLevel.end()) return it->second;
    return 0;
}

int ExperienceSystem::getCurrentExperience(SkillType skill) const
{
    auto it = _currentExperience.find(skill);
    if (it != _currentExperience.end()) return it->second;
    return 0;
}

int ExperienceSystem::getExperienceForNextLevel(SkillType skill) const
{
    int lvl = getLevel(skill);
    if (lvl >= getMaxLevel()) return 0; // Max level
    
    // Return total XP required for next level
    return _xpThresholds[lvl];
}
