#ifndef __MENU_H__
#define __MENU_H__

#include "cocos2d.h"

class MenuScene : public cocos2d::Scene
{
    static cocos2d::Scene* createScene();

    virtual bool init();

    // a selector callback
    void menuCloseCallback(cocos2d::Ref* pSender);

    // implement the "static create()" method manually
    CREATE_FUNC(MenuScene);
};


#endif // __MENU_H__

