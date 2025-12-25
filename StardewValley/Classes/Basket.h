#ifndef __BASKET_H__
#define __BASKET_H__

#include <unordered_map>
#include "CropSystem.h"
class CropSystem;

class Basket
{
public:
    Basket();
    bool addCrop(CropType type, int count);
    bool removeCrop(CropType type, int count);
    int getCount(CropType type) const;
    std::unordered_map<CropType, int> getAll() const;
    void clear();
    int distinctTypes() const;
    int getTotalCount() const;
    int calculateTotalSellValue(const CropSystem* crops) const;

private:
    std::unordered_map<CropType, int> _items;
};

inline int Basket::calculateTotalSellValue(const CropSystem* crops) const
{
    if (!crops) return 0;
    int total = 0;
    for (const auto& kv : _items)
    {
        int price = crops->getSellPrice(kv.first);
        total += price * kv.second;
    }
    return total;
}

#endif
