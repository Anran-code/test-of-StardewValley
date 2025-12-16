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
    int getCount(CropType type) const;
    std::unordered_map<CropType, int> getAll() const;
    void clear();
    int distinctTypes() const;
    int calculateTotalSellValue(const CropSystem* crops) const;

private:
    std::unordered_map<CropType, int> _items;
};

#endif
