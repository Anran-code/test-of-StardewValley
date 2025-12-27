#ifndef __INVENTORY_H__
#define __INVENTORY_H__

#include "Item.h"
#include <vector>

class Inventory
{
public:
    static const int TOOLBAR_SIZE = 12;
    static const int BACKPACK_SIZE = 36; 

    Inventory();

    void addItem(Item item);
    bool hasItem(int slotIndex) const;
    const Item& getItem(int slotIndex) const;
    Item& getItem(int slotIndex); // Mutable access
    void removeItem(int slotIndex, int count = 1);
    void swapItems(int slotA, int slotB);
    
    int getSelectedSlot() const { return _selectedSlot; }
    void setSelectedSlot(int slot) { 
        if (slot >= 0 && slot < TOOLBAR_SIZE) _selectedSlot = slot; 
    }
    
    const Item* getSelectedItem() const {
        if (hasItem(_selectedSlot)) return &_items[_selectedSlot];
        return nullptr;
    }

private:
    std::vector<Item> _items; // 0-11 are toolbar
    int _selectedSlot;
};

#endif
