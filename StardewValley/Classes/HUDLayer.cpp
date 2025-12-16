#include "HudLayer.h"
#include "GameClock.h"
#include "Wallet.h"
#include "Inventory.h"

USING_NS_CC;

HudLayer* HudLayer::create(GameClock* clock, Wallet* wallet, Inventory* inventory)
{
    HudLayer* ret = new (std::nothrow) HudLayer();
    if (ret && ret->initWithSystems(clock, wallet, inventory))
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool HudLayer::initWithSystems(GameClock* clock, Wallet* wallet, Inventory* inventory)
{
    if (!Layer::init())
    {
        return false;
    }

    _clock = clock;
    _wallet = wallet;
    _inventory = inventory;

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();

    // Toolbar
    _toolbar = Sprite::create("Toolbar.png");
    if (_toolbar)
    {
        float targetWidth = visibleSize.width * 0.5f;
        float scale = targetWidth / _toolbar->getContentSize().width;
        _toolbar->setScale(scale);
        _toolbar->setAnchorPoint(Vec2(0.5f, 0.0f));
        _toolbar->setPosition(Vec2(visibleSize.width * 0.5f + origin.x, origin.y));
        addChild(_toolbar, 1);
    }

    // Backpack
    _backpack = Sprite::create("backpack_test.png");
    if (_backpack)
    {
        float targetWidth = visibleSize.width * 0.5f;
        float scale = targetWidth / _backpack->getContentSize().width;
        _backpack->setScale(scale);
        _backpack->setPosition(Vec2(visibleSize.width * 0.5f + origin.x, visibleSize.height * 0.5f + origin.y));
        _backpack->setVisible(false);
        addChild(_backpack, 2);
    }

    // Selector
    _selector = Sprite::create("CloseSelected.png"); // Using CloseSelected as a placeholder for selector frame, or create a simple rect
    if (!_selector)
    {
         // If texture not found, create a red rect
         _selector = Sprite::create();
         _selector->setTextureRect(Rect(0, 0, 40, 40));
         _selector->setColor(Color3B::RED);
         _selector->setOpacity(128);
    }
    if (_toolbar && _selector) {
        _selector->setAnchorPoint(Vec2(0.5f, 0.5f));
        _selector->retain(); // Keep it to add/remove or just toggle visibility/position
        addChild(_selector, 3);
    }

    float margin = 10.0f;
    Vec2 topRight(origin.x + visibleSize.width - margin, origin.y + visibleSize.height - margin);

    _timeLabel = Label::createWithSystemFont("", "Arial", 24);
    _dateLabel = Label::createWithSystemFont("", "Arial", 24);
    _weekLabel = Label::createWithSystemFont("", "Arial", 24);
    _moneyLabel = Label::createWithSystemFont("", "Arial", 24);

    if (_timeLabel)
    {
        _timeLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        _timeLabel->setPosition(topRight);
        addChild(_timeLabel, 1000);
    }
    if (_dateLabel)
    {
        _dateLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        _dateLabel->setPosition(Vec2(topRight.x, topRight.y - 28.0f));
        addChild(_dateLabel, 1000);
    }
    if (_weekLabel)
    {
        _weekLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        _weekLabel->setPosition(Vec2(topRight.x, topRight.y - 56.0f));
        addChild(_weekLabel, 1000);
    }
    if (_moneyLabel)
    {
        _moneyLabel->setAnchorPoint(Vec2(1.0f, 1.0f));
        _moneyLabel->setPosition(Vec2(topRight.x, topRight.y - 84.0f));
        addChild(_moneyLabel, 1000);
    }

    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = CC_CALLBACK_2(HudLayer::onKeyPressed, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);

    auto mouseListener = EventListenerMouse::create();
    mouseListener->onMouseScroll = CC_CALLBACK_1(HudLayer::onScroll, this);
    mouseListener->onMouseDown = CC_CALLBACK_1(HudLayer::onMouseDown, this);
    _eventDispatcher->addEventListenerWithSceneGraphPriority(mouseListener, this);

    auto updateListener = EventListenerCustom::create("INVENTORY_UPDATED", [this](EventCustom* event) {
        this->updateInventoryUI();
    });
    _eventDispatcher->addEventListenerWithSceneGraphPriority(updateListener, this);

    refresh();
    updateInventoryUI();
    
    return true;
}

void HudLayer::onKeyPressed(EventKeyboard::KeyCode keyCode, Event* event)
{
    if (keyCode == EventKeyboard::KeyCode::KEY_E)
    {
        if (_backpack)
        {
            _backpack->setVisible(!_backpack->isVisible());
        }
    }
    
    // Number keys for toolbar selection
    int slot = -1;
    if (keyCode >= EventKeyboard::KeyCode::KEY_1 && keyCode <= EventKeyboard::KeyCode::KEY_9)
    {
        slot = (int)keyCode - (int)EventKeyboard::KeyCode::KEY_1;
    }
    else if (keyCode == EventKeyboard::KeyCode::KEY_0)
    {
        slot = 9;
    }
    else if (keyCode == EventKeyboard::KeyCode::KEY_MINUS)
    {
        slot = 10;
    }
    else if (keyCode == EventKeyboard::KeyCode::KEY_EQUAL) // often +
    {
        slot = 11;
    }

    if (slot != -1 && _inventory)
    {
        _inventory->setSelectedSlot(slot);
        updateInventoryUI();
    }
}

void HudLayer::onScroll(Event* event)
{
    EventMouse* e = (EventMouse*)event;
    float scrollY = e->getScrollY();
    if (scrollY != 0 && _inventory)
    {
        int current = _inventory->getSelectedSlot();
        if (scrollY > 0)
        {
            current--;
            if (current < 0) current = Inventory::TOOLBAR_SIZE - 1;
        }
        else
        {
            current++;
            if (current >= Inventory::TOOLBAR_SIZE) current = 0;
        }
        _inventory->setSelectedSlot(current);
        updateInventoryUI();
    }
}

void HudLayer::updateInventoryUI()
{
    if (!_inventory || !_toolbar) return;

    // Clear old sprites
    for (auto sp : _itemSprites) sp->removeFromParent();
    _itemSprites.clear();
    for (auto lbl : _quantityLabels) lbl->removeFromParent();
    _quantityLabels.clear();

    Size toolbarSize = _toolbar->getContentSize();
    float scale = _toolbar->getScale();
    Vec2 toolbarPos = _toolbar->getPosition();
    
    // Toolbar has 12 slots.
    // Assuming Toolbar.png is a single row of 12 squares.
    // We need to calculate the position of each slot relative to the toolbar sprite.
    // Let's assume standard Stardew toolbar layout.
    // If we don't know the exact pixel offsets, we can approximate.
    // Toolbar width is toolbarSize.width. Each slot is roughly width / 12.
    
    // Toolbar.png 像素信息:
    // 假设 Toolbar.png 的原始宽度对应这些像素值
    // 每个格子 198x190
    // 左边框到第一个格子左边缘: 54
    // 下边框到第一个格子下边缘: 75 (原点在左下角，所以Y轴偏移是75)
    // 格子间隙: 30
    // 
    // 我们需要将这些像素值转换为相对于 toolbar 缩放后的屏幕坐标。
    
    // 获取 toolbar 原始尺寸
    Size originalSize = _toolbar->getContentSize();
    
    // Pixel constants
    // These constants are defined in the header file, no need to redefine
    
    // 计算缩放后的值
    float scaledLeftMargin = RAW_LEFT_MARGIN * scale;
    float scaledBottomMargin = RAW_BOTTOM_MARGIN * scale;
    float scaledCellWidth = RAW_CELL_WIDTH * scale;
    float scaledCellHeight = RAW_CELL_HEIGHT * scale;
    float scaledGap = RAW_GAP * scale;
    
    // Toolbar 在屏幕上的左下角坐标
    // Anchor(0.5, 0.0), Pos(cx, y)
    // Left = Pos.x - (ContentW * Scale * 0.5)
    // Bottom = Pos.y
    float toolbarScreenLeft = toolbarPos.x - (originalSize.width * scale * 0.5f);
    float toolbarScreenBottom = toolbarPos.y;
    
    // Cache for hit testing
    _cachedToolbarLeft = toolbarScreenLeft;
    _cachedToolbarBottom = toolbarScreenBottom;
    _cachedScale = scale;

    // 第一个格子的中心点
    // CenterX = Left + Margin + CellW/2
    // CenterY = Bottom + Margin + CellH/2
    float startCenterX = toolbarScreenLeft + scaledLeftMargin + (scaledCellWidth * 0.5f);
    float startCenterY = toolbarScreenBottom + scaledBottomMargin + (scaledCellHeight * 0.5f);
    
    // 每个格子之间的步长
    float stepX = scaledCellWidth + scaledGap;

    // Update items
    for (int i = 0; i < Inventory::TOOLBAR_SIZE; i++)
    {
        if (_inventory->hasItem(i))
        {
            const Item& item = _inventory->getItem(i);
            auto sprite = Sprite::create(item.iconPath);
            if (sprite)
            {
                // Fit into slot (leave some padding, e.g. 10%)
                float maxIconW = scaledCellWidth * 0.8f;
                float maxIconH = scaledCellHeight * 0.8f;
                
                float sX = maxIconW / sprite->getContentSize().width;
                float sY = maxIconH / sprite->getContentSize().height;
                float iconScale = std::min(sX, sY);
                
                sprite->setScale(iconScale);
                
                // Calculate position based on index
                float cx = startCenterX + (i * stepX);
                float cy = startCenterY;
                
                sprite->setPosition(Vec2(cx, cy));
                addChild(sprite, 10); // Above toolbar
                _itemSprites.push_back(sprite);

                // Quantity
                if (item.maxStack > 1 && item.quantity > 1)
                {
                    auto label = Label::createWithSystemFont(std::to_string(item.quantity), "Arial", 16); // Font size might need scaling
                    if (label)
                    {
                        label->setAnchorPoint(Vec2(1.0f, 0.0f));
                        // Position at bottom-right of the cell content area
                        label->setPosition(Vec2(cx + scaledCellWidth * 0.4f, cy - scaledCellHeight * 0.4f));
                        addChild(label, 11);
                        _quantityLabels.push_back(label);
                    }
                }
            }
        }
    }

    // Update Selector position
    if (_selector)
    {
        int sel = _inventory->getSelectedSlot();
        float cx = startCenterX + (sel * stepX);
        float cy = startCenterY;
        _selector->setPosition(Vec2(cx, cy));
        
        // Scale selector to fit slot size
        Size selSize = _selector->getContentSize();
        if (selSize.width > 0) {
            // Make selector slightly larger than cell
            float targetSelW = scaledCellWidth * 1.1f;
            _selector->setScale(targetSelW / selSize.width);
        }
    }
}

void HudLayer::onMouseDown(Event* event)
{
    if (!_inventory || !_toolbar) return;

    EventMouse* e = (EventMouse*)event;
    if (e->getMouseButton() != EventMouse::MouseButton::BUTTON_LEFT) return;

    Vec2 clickPos = e->getLocationInView();
    clickPos.y = Director::getInstance()->getWinSize().height - clickPos.y;

    // Check if click is within Toolbar bounds roughly
    // Or just check individual slots
    
    // We can reverse the logic from updateInventoryUI
    // x = startCenterX + i * stepX
    // y = startCenterY
    
    // Better: convert click pos to local space relative to toolbar origin
    
    float scale = _cachedScale;
    if (scale <= 0.0f) return; // Not initialized?

    float scaledLeftMargin = RAW_LEFT_MARGIN * scale;
    float scaledBottomMargin = RAW_BOTTOM_MARGIN * scale;
    float scaledCellWidth = RAW_CELL_WIDTH * scale;
    float scaledCellHeight = RAW_CELL_HEIGHT * scale;
    float scaledGap = RAW_GAP * scale;
    float stepX = scaledCellWidth + scaledGap;
    
    float localX = clickPos.x - _cachedToolbarLeft;
    float localY = clickPos.y - _cachedToolbarBottom;
    
    // Check Y range
    if (localY < scaledBottomMargin || localY > scaledBottomMargin + scaledCellHeight)
    {
        return; // Click not in the row of cells
    }
    
    // Check X range and find slot
    // startX of first cell content = scaledLeftMargin
    // cell i starts at scaledLeftMargin + i * stepX
    // cell i ends at ... + scaledCellWidth
    
    float relativeX = localX - scaledLeftMargin;
    
    if (relativeX < 0) return; // Clicked on left margin
    
    // Estimate index
    // i = relativeX / stepX?
    // Not exactly because of gap.
    
    // Let's iterate or calc
    // relativeX = i * (width + gap) + offset_within_cell
    
    int index = static_cast<int>(relativeX / stepX);
    
    if (index >= 0 && index < Inventory::TOOLBAR_SIZE)
    {
        // Check if within the cell width (exclude gap)
        float offsetInStep = relativeX - (index * stepX);
        if (offsetInStep <= scaledCellWidth)
        {
            // Clicked on slot 'index'
            _inventory->setSelectedSlot(index);
            updateInventoryUI();
            
            // Consume event so BackgroundLayer doesn't process it?
            // HudLayer is z=1000, but event listeners graph priority...
            // If we want to swallow, we need stopPropagation.
            event->stopPropagation();
        }
    }
}

void HudLayer::refresh()
{
    if (_clock && _timeLabel)
    {
        _timeLabel->setString(_clock->getTimeString());
    }
    if (_clock && _dateLabel)
    {
        const char* s =
            (_clock->getSeason() == GameClock::Season::Spring) ? "Spring" :
            (_clock->getSeason() == GameClock::Season::Summer) ? "Summer" :
            (_clock->getSeason() == GameClock::Season::Fall)   ? "Fall" : "Winter";
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s Day %d Year %d", s, _clock->getDay(), _clock->getYear());
        _dateLabel->setString(buf);
    }
    if (_clock && _weekLabel)
    {
        std::string w = _clock->getWeekdayString();
        int wk = _clock->getWeekOfSeason();
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s, Week %d", w.c_str(), wk);
        _weekLabel->setString(buf);
    }
    if (_wallet && _moneyLabel)
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "Gold: %d", _wallet->getMoney());
        _moneyLabel->setString(buf);
    }
}
