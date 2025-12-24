#include "Basket.h"

Basket::Basket()
{
}

bool Basket::addCrop(CropType type, int count)
{
    if (count <= 0) return false;
    if (_items.find(type) == _items.end())
    {
        if ((int)_items.size() >= 5) return false;
        _items[type] = 0;
    }
    _items[type] += count;
    return true;
}

int Basket::getCount(CropType type) const
{
    auto it = _items.find(type);
    if (it == _items.end()) return 0;
    return it->second;
}

std::unordered_map<CropType, int> Basket::getAll() const
{
    return _items;
}

void Basket::clear()
{
    _items.clear();
}

int Basket::distinctTypes() const
{
    return (int)_items.size();
}

int Basket::calculateTotalSellValue(const CropSystem* crops) const
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
