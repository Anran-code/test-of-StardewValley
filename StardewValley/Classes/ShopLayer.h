#ifndef __SHOP_LAYER_H__
#define __SHOP_LAYER_H__

#include "cocos2d.h"
#include <vector>
#include <string>
#include <functional>

class Wallet;
class Basket;
class CropSystem;

class ShopLayer : public cocos2d::Layer
{
public:
    static ShopLayer* create(Wallet* wallet, Basket* basket, CropSystem* crops);
    bool initWithSystems(Wallet* wallet, Basket* basket, CropSystem* crops);
    void refreshBasketView();

    CREATE_FUNC(ShopLayer);

private:
    struct StoreItem
    {
        std::string name;
        int price;
        int stock;
        std::string image;
        std::function<void()> onBuy;
    };

    Wallet* _wallet;
    Basket* _basket;
    CropSystem* _crops;
    std::vector<StoreItem> _items;
    cocos2d::Label* _basketTitle;
    cocos2d::Label* _basketDetails;
    cocos2d::Label* _basketTotal;
};

#endif
