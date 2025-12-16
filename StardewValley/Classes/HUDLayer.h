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

    CREATE_FUNC(HudLayer);

private:
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
    
    // Pixel constants
    const float RAW_CELL_WIDTH = 198.0f;
    const float RAW_CELL_HEIGHT = 190.0f;
    const float RAW_LEFT_MARGIN = 54.0f;
    const float RAW_BOTTOM_MARGIN = 75.0f;
    const float RAW_GAP = 30.0f;

    cocos2d::Label* _timeLabel;
    cocos2d::Label* _dateLabel;
    cocos2d::Label* _weekLabel;
    cocos2d::Label* _moneyLabel;

    std::vector<cocos2d::Sprite*> _itemSprites; // Sprites for items in slots
    std::vector<cocos2d::Label*> _quantityLabels; // Labels for quantities
};

#endif
