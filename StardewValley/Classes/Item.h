#ifndef __ITEM_H__
#define __ITEM_H__

#include <string>
#include "CropSystem.h" // For CropType

enum class ItemType
{
    Tool,
    Seed,
    Crop,
    Resource
};

enum class ToolType
{
    None,
    Hoe,
    Axe,
    Pickaxe,
    WateringCan,
    Scythe,
    FishingRod
};

struct Item
{
    ItemType type;
    std::string name;
    std::string iconPath;
    int quantity;
    int maxStack;

    // Specific data
    ToolType toolType = ToolType::None;
    CropType cropType = CropType::Parsnip; // Default
    
    // Tool Stats
    float currentWater = 0.0f;
    float maxWater = 0.0f;
    int toolLevel = 0; // 0=Basic, 1=Copper, 2=Steel, 3=Gold, 4=Iridium

    Item() : type(ItemType::Tool), quantity(0), maxStack(1) {}
    
    static Item createTool(ToolType t, const std::string& name, const std::string& path) {
        Item i;
        i.type = ItemType::Tool;
        i.toolType = t;
        i.name = name;
        i.iconPath = path;
        i.quantity = 1;
        i.maxStack = 1;
        i.toolLevel = 0;
        
        if (t == ToolType::WateringCan)
        {
            i.maxWater = 20.0f; // Capacity 20
            i.currentWater = 20.0f; // Start full
        }
        
        return i;
    }

    static Item createSeed(CropType c, const std::string& name, const std::string& path, int qty) {
        Item i;
        i.type = ItemType::Seed;
        i.cropType = c;
        i.name = name;
        i.iconPath = path;
        i.quantity = qty;
        i.maxStack = 999;
        return i;
    }

    static Item createCrop(CropType c, const std::string& name, const std::string& path, int qty) {
        Item i;
        i.type = ItemType::Crop;
        i.cropType = c;
        i.name = name;
        i.iconPath = path;
        i.quantity = qty;
        i.maxStack = 999;
        return i;
    }
};

#endif
