#ifndef __SHOP_LAYER_H__
#define __SHOP_LAYER_H__

#include "cocos2d.h"
#include <vector>
#include <string>
#include <functional>

class Wallet;
class Basket;
class CropSystem;
class Inventory;

class ShopLayer : public cocos2d::Layer
{
public:
    static ShopLayer* create(Wallet* wallet, Basket* basket, CropSystem* crops, Inventory* inventory);
    bool initWithSystems(Wallet* wallet, Basket* basket, CropSystem* crops, Inventory* inventory);
    void refreshBasketView();

    CREATE_FUNC(ShopLayer);

private:
    struct StoreItem
    {
        std::string name;
        int price;
        int stock;
        std::string image;
        std::function<bool()> onBuy;
        cocos2d::MenuItemImage* icon;
        bool isTool;
    };

    Wallet* _wallet;
    Basket* _basket;
    CropSystem* _crops;
    Inventory* _inventory;
    std::vector<StoreItem> _items;
    cocos2d::Label* _basketTitle;
    cocos2d::Label* _basketDetails;
    cocos2d::Label* _basketTotal;
    cocos2d::Label* _moneyLabel;
    std::vector<cocos2d::Label*> _itemLabels;
    cocos2d::Sprite* _basketIcon;
};

#endif
