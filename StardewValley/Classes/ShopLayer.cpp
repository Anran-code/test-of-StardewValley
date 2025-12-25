#include "ShopLayer.h"
#include "Wallet.h"
#include "Basket.h"
#include "CropSystem.h"
#include "Inventory.h"

USING_NS_CC;

ShopLayer* ShopLayer::create(Wallet* wallet, Basket* basket, CropSystem* crops, Inventory* inventory)
{
    ShopLayer* ret = new (std::nothrow) ShopLayer();
    if (ret && ret->initWithSystems(wallet, basket, crops, inventory))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool ShopLayer::initWithSystems(Wallet* wallet, Basket* basket, CropSystem* crops, Inventory* inventory)
{
    if (!Layer::init())
    {
        return false;
    }
    _wallet = wallet;
    _basket = basket;
    _crops = crops;
    _inventory = inventory;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // 1. Background
    auto bg = Sprite::create("ui/menu_bg.png");
    if (bg)
    {
        bg->setPosition(origin + visibleSize / 2);
        // Scale to fit screen if needed, or just center it
        // Assuming 1280x720 design, let's keep it centered
        addChild(bg, 0);
    }
    else
    {
        // Fallback
        auto colorLayer = LayerColor::create(Color4B(0, 0, 0, 200));
        addChild(colorLayer, 0);
    }

    // 2. Close Button
    auto closeBtn = MenuItemImage::create("ui/btn_exit.png", "ui/btn_exit.png", [this](Ref* sender) {
        this->removeFromParent();
    });
    if (closeBtn)
    {
        closeBtn->setPosition(Vec2(origin.x + visibleSize.width - 50, origin.y + visibleSize.height - 50));
        auto closeMenu = Menu::create(closeBtn, nullptr);
        closeMenu->setPosition(Vec2::ZERO);
        addChild(closeMenu, 10);
    }

    float leftPanelX = origin.x + visibleSize.width * 0.3f;
    float rightPanelX = origin.x + visibleSize.width * 0.7f;
    float topY = origin.y + visibleSize.height * 0.8f;

    _items.clear();
    _itemLabels.clear();

    // Helper to add item
    auto addSeed = [&](const std::string& name, int price, int stock, const std::string& img, CropType type) {
        _items.push_back({ name, price, stock, img, [this, price, type, name, img]() -> bool {
            if (_wallet && _wallet->spendMoney(price)) {
                if (_inventory) {
                    _inventory->addItem(Item::createSeed(type, name, img, 1));
                }
                this->refreshBasketView();
                return true;
            }
            return false;
        }, nullptr });
    };

    auto addTool = [&](const std::string& name, int price, int stock, const std::string& img, ToolType type) {
        _items.push_back({ name, price, stock, img, [this, price, type, name, img]() -> bool {
            if (_wallet && _wallet->spendMoney(price)) {
                 if (_inventory) {
                    _inventory->addItem(Item::createTool(type, name, img));
                }
                this->refreshBasketView();
                return true;
            }
            return false;
        }, nullptr });
    };

    addSeed("Parsnip Seeds", 20, 20, "Crop/Parsnip_Seeds.png", CropType::Parsnip);
    addSeed("Cauliflower Seeds", 80, 10, "Crop/Cauliflower_Seeds.png", CropType::Cauliflower);
    addSeed("Potato Seeds", 50, 15, "Crop/Potato_Seeds.png", CropType::Potato);
    
    addTool("Hoe", 250, 2, "tools/hoe.png", ToolType::Hoe);
    addTool("Axe", 250, 2, "tools/axe.png", ToolType::Axe);
    addTool("Pickaxe", 250, 2, "tools/pickaxe.png", ToolType::Pickaxe);
    addTool("Watering Can", 200, 2, "tools/watering_Can.png", ToolType::WateringCan);
    addTool("Fishing Rod", 500, 1, "tools/Pole.png", ToolType::FishingRod);

    float rowY = topY;
    float rowGap = 55.0f;
    Vector<MenuItem*> menuItems;

    for (size_t i = 0; i < _items.size(); ++i)
    {
        const auto& it = _items[i];
        auto icon = MenuItemImage::create(it.image, it.image, [this, i](Ref* sender) {
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
    auto menu = Menu::createWithArray(menuItems);
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
        sellItem = MenuItemLabel::create(sellAllLabel, [this](Ref* sender) {
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
    
    // Consume clicks so game doesn't receive them
    auto listener = EventListenerTouchOneByOne::create();
    listener->setSwallowTouches(true);
    listener->onTouchBegan = [](Touch* t, Event* e) { return true; };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
    
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
        case CropType::Blueberry: name = "Blueberry"; break;
        case CropType::Melon: name = "Melon"; break;
        case CropType::Starfruit: name = "Starfruit"; break;
        case CropType::Pumpkin: name = "Pumpkin"; break;
        case CropType::Eggplant: name = "Eggplant"; break;
        case CropType::Yam: name = "Yam"; break;
        case CropType::Powdermelon: name = "Powdermelon"; break;
        case CropType::Fish: name = "Fish"; break;
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
