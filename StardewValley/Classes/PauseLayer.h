#ifndef __PAUSE_LAYER_H__
#define __PAUSE_LAYER_H__

#include "cocos2d.h"

class PauseLayer : public cocos2d::LayerColor
{
public:
    static PauseLayer* create();
    virtual bool init();

    void show(cocos2d::Node* parent);
    void hide();
    
    static bool isGamePaused() { return sIsPaused; }

private:
    void onResume();
    void onExitGame();
    
    // Swallow touches to prevent interaction with game while paused
    bool onTouchBegan(cocos2d::Touch* touch, cocos2d::Event* event);

    static bool sIsPaused;
};

#endif // __PAUSE_LAYER_H__
