#include "Inventory.h"

Inventory::Inventory()
{
    _items.resize(BACKPACK_SIZE); // Initialize empty slots
    _selectedSlot = 0;
    
    _items[0] = Item::createTool(ToolType::Hoe, "Hoe", "tools/hoe.png");
    _items[1] = Item::createTool(ToolType::Axe, "Axe", "tools/axe.png");
    _items[2] = Item::createTool(ToolType::Pickaxe, "Pickaxe", "tools/pickaxe.png");
    _items[3] = Item::createTool(ToolType::WateringCan, "Watering Can", "tools/watering_Can.png");
    _items[4] = Item::createTool(ToolType::Scythe, "Scythe", "tools/Scythe.png");
    _items[5] = Item::createTool(ToolType::FishingRod, "Fishing Rod", "tools/Pole.png");
    _items[6] = Item::createSeed(CropType::Parsnip, "Parsnip Seeds", "Crop/Parsnip_Seeds.png", 15);
    _items[7] = Item::createSeed(CropType::Cauliflower, "Cauliflower Seeds", "Crop/Cauliflower_Seeds.png", 15);
    _items[8] = Item::createSeed(CropType::Potato, "Potato Seeds", "Crop/Potato_Seeds.png", 15);
    _items[9] = Item::createSeed(CropType::Blueberry, "Blueberry Seeds", "Crop/Blueberry_Seeds.png", 15);
    _items[10] = Item::createSeed(CropType::Melon, "Melon Seeds", "Crop/Melon_Seeds.png", 15);
    _items[11] = Item::createSeed(CropType::Starfruit, "Starfruit Seeds", "Crop/Starfruit_Seeds.png", 15);
    _items[12] = Item::createSeed(CropType::Pumpkin, "Pumpkin Seeds", "Crop/Pumpkin_Seeds.png", 15);
    _items[13] = Item::createSeed(CropType::Eggplant, "Eggplant Seeds", "Crop/Eggplant_Seeds.png", 15);
    _items[14] = Item::createSeed(CropType::Yam, "Yam Seeds", "Crop/Yam_Seeds.png", 15);
    _items[15] = Item::createSeed(CropType::Powdermelon, "Powdermelon Seeds", "Crop/Powdermelon_Seeds.png", 15);
}

void Inventory::addItem(Item item)
{
    // Try to stack first
    if (item.maxStack > 1)
    {
        for (auto& slot : _items)
        {
            if (slot.quantity > 0 && slot.type == item.type && slot.name == item.name && slot.quantity < slot.maxStack)
            {
                int space = slot.maxStack - slot.quantity;
                int add = std::min(space, item.quantity);
                slot.quantity += add;
                item.quantity -= add;
                if (item.quantity <= 0) return;
            }
        }
    }

    // Add to first empty slot
    for (auto& slot : _items)
    {
        if (slot.quantity == 0)
        {
            slot = item;
            return;
        }
    }
}

bool Inventory::hasItem(int slotIndex) const
{
    if (slotIndex < 0 || slotIndex >= (int)_items.size()) return false;
    return _items[slotIndex].quantity > 0;
}

const Item& Inventory::getItem(int slotIndex) const
{
    return _items[slotIndex];
}

Item& Inventory::getItem(int slotIndex)
{
    return _items[slotIndex];
}

void Inventory::removeItem(int slotIndex, int count)
{
    if (slotIndex < 0 || slotIndex >= (int)_items.size()) return;
    
    // Tools cannot be removed/destroyed
    if (_items[slotIndex].type == ItemType::Tool) return;

    if (_items[slotIndex].quantity > 0)
    {
        _items[slotIndex].quantity -= count;
        if (_items[slotIndex].quantity < 0) _items[slotIndex].quantity = 0;
    }
}

void Inventory::swapItems(int slotA, int slotB)
{
    if (slotA < 0 || slotA >= (int)_items.size()) return;
    if (slotB < 0 || slotB >= (int)_items.size()) return;
    if (slotA == slotB) return;

    std::swap(_items[slotA], _items[slotB]);
}
