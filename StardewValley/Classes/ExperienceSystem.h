#ifndef __EXPERIENCE_SYSTEM_H__
#define __EXPERIENCE_SYSTEM_H__

#include "cocos2d.h"
#include <unordered_map>
#include <vector>
#include <string>

enum class SkillType
{
    Farming,
    Fishing
};

class ExperienceSystem
{
public:
    static ExperienceSystem* getInstance();
    static void destroyInstance();

    // Initialize the system
    void init();

    // Add experience to a specific skill
    // Returns true if leveled up
    bool addExperience(SkillType skill, int amount);

    // Getters
    int getLevel(SkillType skill) const;
    int getCurrentExperience(SkillType skill) const;
    int getExperienceForNextLevel(SkillType skill) const;
    int getMaxLevel() const { return 10; }

    // Helper to get skill name string
    std::string getSkillName(SkillType skill) const;

private:
    ExperienceSystem();
    ~ExperienceSystem();

    static ExperienceSystem* s_instance;

    std::unordered_map<SkillType, int> _currentExperience;
    std::unordered_map<SkillType, int> _currentLevel;
    
    // XP required to reach next level. 
    // Index 0 = XP for Level 1 (from 0), Index 1 = XP for Level 2 (from Level 1)...
    // Actually simpler: Total XP required to reach Level X.
    std::vector<int> _xpThresholds;

    void loadXPThresholds();
    void checkLevelUp(SkillType skill);
};

#endif // __EXPERIENCE_SYSTEM_H__
