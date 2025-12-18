#include "ShopLayer.h"
#include "Wallet.h"
#include "Basket.h"
#include "CropSystem.h"

USING_NS_CC;

ShopLayer* ShopLayer::create(Wallet* wallet, Basket* basket, CropSystem* crops)
{
    ShopLayer* ret = new (std::nothrow) ShopLayer();
    if (ret && ret->initWithSystems(wallet, basket, crops))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool ShopLayer::initWithSystems(Wallet* wallet, Basket* basket, CropSystem* crops)
{
    if (!Layer::init())
    {
        return false;
    }
    _wallet = wallet;
    _basket = basket;
    _crops = crops;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    float leftPanelX = origin.x + visibleSize.width * 0.3f;
    float rightPanelX = origin.x + visibleSize.width * 0.7f;
    float topY = origin.y + visibleSize.height * 0.8f;

    _items.clear();
    _itemLabels.clear();
    _items.push_back({ "Parsnip Seeds", 20, 20, "Crop/Parsnip_Seeds.png", [this]() -> bool {
        if (_wallet && _wallet->spendMoney(20)) { this->refreshBasketView(); return true; }
        return false;
    }});
    _items.push_back({ "Cauliflower Seeds", 80, 10, "Crop/Cauliflower_Seeds.png", [this]() -> bool {
        if (_wallet && _wallet->spendMoney(80)) { this->refreshBasketView(); return true; }
        return false;
    }});
    _items.push_back({ "Potato Seeds", 50, 15, "Crop/Potato_Seeds.png", [this]() -> bool {
        if (_wallet && _wallet->spendMoney(50)) { this->refreshBasketView(); return true; }
        return false;
    }});
    _items.push_back({ "Hoe", 250, 2, "tools/hoe.png", [this]() -> bool {
        if (_wallet && _wallet->spendMoney(250)) { this->refreshBasketView(); return true; }
        return false;
    }});
    _items.push_back({ "Axe", 250, 2, "tools/axe.png", [this]() -> bool {
        if (_wallet && _wallet->spendMoney(250)) { this->refreshBasketView(); return true; }
        return false;
    }});
    _items.push_back({ "Pickaxe", 250, 2, "tools/pickaxe.png", [this]() -> bool {
        if (_wallet && _wallet->spendMoney(250)) { this->refreshBasketView(); return true; }
        return false;
    }});
    _items.push_back({ "Watering Can", 200, 2, "tools/watering_Can.png", [this]() -> bool {
        if (_wallet && _wallet->spendMoney(200)) { this->refreshBasketView(); return true; }
        return false;
    }});
    _items.push_back({ "Fishing Rod", 500, 1, "tools/Pole.png", [this]() -> bool {
        if (_wallet && _wallet->spendMoney(500)) { this->refreshBasketView(); return true; }
        return false;
    }});

    float rowY = topY;
    float rowGap = 90.0f;
    Vector<MenuItem*> menuItems;

    for (size_t i = 0; i < _items.size(); ++i)
    {
        const auto& it = _items[i];
        auto icon = MenuItemImage::create(it.image, it.image, [this, i]() {
            auto& item = _items[i];
            if (item.stock <= 0) return;
            bool ok = item.onBuy ? item.onBuy() : false;
            if (!ok) return;
            if (item.stock > 0) item.stock -= 1;
            if (i < _itemLabels.size() && _itemLabels[i])
            {
                char buf2[128];
                std::snprintf(buf2, sizeof(buf2), "%s  Price: %d  Stock: %d", item.name.c_str(), item.price, item.stock);
                _itemLabels[i]->setString(buf2);
            }
            if (item.stock <= 0 && item.icon)
            {
                item.icon->setEnabled(false);
                item.icon->setColor(Color3B(150, 150, 150));
            }
        });
        if (icon)
        {
            icon->setPosition(Vec2(leftPanelX - 100.0f, rowY));
            float scale = 0.8f;
            icon->setScale(scale);
            _items[i].icon = icon;
            menuItems.pushBack(icon);
        }
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%s  Price: %d  Stock: %d", it.name.c_str(), it.price, it.stock);
        auto label = Label::createWithSystemFont(buf, "Arial", 24);
        if (label)
        {
            label->setAnchorPoint(Vec2(0.0f, 0.5f));
            label->setPosition(Vec2(leftPanelX - 50.0f, rowY));
            addChild(label, 1);
            _itemLabels.push_back(label);
        }
        else
        {
            _itemLabels.push_back(nullptr);
        }
        rowY -= rowGap;
    }
    auto menu = Menu::create(menuItems);
    if (menu)
    {
        menu->setPosition(Vec2::ZERO);
        addChild(menu, 1);
    }

    _basketTitle = Label::createWithSystemFont("Shipping Bin", "Arial", 28);
    if (_basketTitle)
    {
        _basketTitle->setAnchorPoint(Vec2(0.5f, 0.5f));
        _basketTitle->setPosition(Vec2(rightPanelX, topY + 20.0f));
        addChild(_basketTitle, 1);
    }

    _moneyLabel = Label::createWithSystemFont("Money: 0", "Arial", 28);
    if (_moneyLabel)
    {
        _moneyLabel->setAnchorPoint(Vec2(0.0f, 0.5f));
        _moneyLabel->setPosition(Vec2(leftPanelX - 100.0f, topY + 20.0f));
        _moneyLabel->setColor(Color3B::YELLOW);
        addChild(_moneyLabel, 1);
    }

    _basketDetails = Label::createWithSystemFont("", "Arial", 22);
    if (_basketDetails)
    {
        _basketDetails->setAnchorPoint(Vec2(0.5f, 1.0f));
        _basketDetails->setPosition(Vec2(rightPanelX, topY - 20.0f));
        addChild(_basketDetails, 1);
    }
    _basketTotal = Label::createWithSystemFont("", "Arial", 24);
    if (_basketTotal)
    {
        _basketTotal->setAnchorPoint(Vec2(0.5f, 0.5f));
        _basketTotal->setPosition(Vec2(rightPanelX, origin.y + visibleSize.height * 0.25f));
        addChild(_basketTotal, 1);
    }

    auto sellAllLabel = Label::createWithSystemFont("Sell All", "Arial", 26);
    MenuItemLabel* sellItem = nullptr;
    if (sellAllLabel)
    {
        sellItem = MenuItemLabel::create(sellAllLabel, [this]() {
            if (!_basket || !_crops || !_wallet) return;
            int total = _basket->calculateTotalSellValue(_crops);
            if (total > 0)
            {
                _wallet->addMoney(total);
                _basket->clear();
                refreshBasketView();
            }
        });
    }
    if (sellItem)
    {
        sellItem->setPosition(Vec2(rightPanelX, origin.y + visibleSize.height * 0.2f));
        auto sellMenu = Menu::create(sellItem, nullptr);
        sellMenu->setPosition(Vec2::ZERO);
        addChild(sellMenu, 1);
    }

    refreshBasketView();
    return true;
}

void ShopLayer::refreshBasketView()
{
    if (!_basketDetails || !_basketTotal || !_basket) return;
    auto all = _basket->getAll();
    std::string lines;
    for (const auto& kv : all)
    {
        std::string name;
        switch (kv.first)
        {
        case CropType::Parsnip: name = "Parsnip"; break;
        case CropType::Cauliflower: name = "Cauliflower"; break;
        case CropType::Potato: name = "Potato"; break;
        }
        char buf[128];
        std::snprintf(buf, sizeof(buf), "%s x%d", name.c_str(), kv.second);
        if (!lines.empty()) lines += "\n";
        lines += buf;
    }
    if (lines.empty()) lines = "Empty";
    _basketDetails->setString(lines);
    int total = _crops ? _basket->calculateTotalSellValue(_crops) : 0;
    char t[64];
    std::snprintf(t, sizeof(t), "Total: %d", total);
    _basketTotal->setString(t);

    if (_moneyLabel && _wallet)
    {
        char m[64];
        std::snprintf(m, sizeof(m), "Money: %d", _wallet->getMoney());
        _moneyLabel->setString(m);
    }
}
