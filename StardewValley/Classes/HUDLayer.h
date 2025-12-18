#ifndef __HUD_LAYER_H__
#define __HUD_LAYER_H__

#include "cocos2d.h"
#include <string>
#include <vector>

class GameClock;
class Wallet;
class Inventory; // Forward decl

class HudLayer : public cocos2d::Layer
{
public:
    static HudLayer* create(GameClock* clock, Wallet* wallet, Inventory* inventory);

    bool initWithSystems(GameClock* clock, Wallet* wallet, Inventory* inventory);

    void refresh();
    void updateInventoryUI();

    void onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode, cocos2d::Event* event);
    void onScroll(cocos2d::Event* event); // Mouse scroll for toolbar selection
    void onMouseDown(cocos2d::Event* event); // Added mouse click handling for toolbar
    void onMouseUp(cocos2d::Event* event);
    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchMoved(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchEnded(cocos2d::Touch* touch, cocos2d::Event* event);
    void onTouchCancelled(cocos2d::Touch* touch, cocos2d::Event* event);
    void onMouseMove(cocos2d::Event* event); // Added for drag

    CREATE_FUNC(HudLayer);

public:
    bool isPointInToolbarWorld(const cocos2d::Vec2& p) const;
    int getSlotIndexFromPoint(const cocos2d::Vec2& p);
    bool isPointInCloseButton(const cocos2d::Vec2& p);
    bool isPointInTrashCan(const cocos2d::Vec2& p);
    bool isConsumingClick() const { return _consumingClick; }

private:
    cocos2d::EventListenerMouse* _mouseListener;
    cocos2d::EventListenerTouchOneByOne* _touchListener;
    cocos2d::DrawNode* _debugBounds;
    GameClock* _clock;
    Wallet* _wallet;
    Inventory* _inventory;

    cocos2d::Sprite* _toolbar;
    cocos2d::Sprite* _backpack;
    cocos2d::Sprite* _selector; // Highlight for selected tool

    // Stored layout data for mouse hit testing
    float _cachedToolbarLeft;
    float _cachedToolbarBottom;
    float _cachedScale;
    bool _consumingClick = false;
    
    // Pixel constants
    const float RAW_CELL_WIDTH = 71.0f;
    const float RAW_CELL_HEIGHT = 72.0f;
    const float RAW_LEFT_MARGIN = 23.0f;
    const float RAW_BOTTOM_MARGIN = 23.0f;
    const float RAW_GAP = 5.0f;

    // Backpack Pixel Constants
    const float BACKPACK_CELL_W = 59.0f;
    const float BACKPACK_CELL_H = 60.0f;
    const float BACKPACK_ROW1_LEFT = 153.0f;
    const float BACKPACK_ROW1_BOTTOM = 545.0f; // Bottom edge of row 1 cell from bottom of image
    const float BACKPACK_H_GAP = 4.0f;
    const float BACKPACK_V_GAP_1_2 = 21.0f; // Gap between row 1 and 2
    const float BACKPACK_V_GAP_2_3 = 9.0f;  // Gap between row 2 and 3
    
    // Close Button
    const float CLOSE_BTN_W = 45.0f;
    const float CLOSE_BTN_H = 44.0f;
    const float CLOSE_BTN_LEFT = 941.0f;
    const float CLOSE_BTN_BOTTOM = 689.0f;

    // Trash Can
    const float TRASH_W = 74.0f;
    const float TRASH_H = 105.0f;
    const float TRASH_LEFT = 990.0f;
    const float TRASH_BOTTOM = 229.0f;

    // Backpack Drag State
    bool _isDragging = false;
    int _dragSourceIndex = -1;
    cocos2d::Sprite* _draggedItemSprite = nullptr;
    cocos2d::Label* _draggedItemQty = nullptr;

    cocos2d::Label* _timeLabel;
    cocos2d::Label* _dateLabel;
    cocos2d::Label* _weekLabel;
    cocos2d::Label* _moneyLabel;

    std::vector<cocos2d::Sprite*> _itemSprites; // Sprites for items in slots
    std::vector<cocos2d::Label*> _quantityLabels; // Labels for quantities
};

#endif
