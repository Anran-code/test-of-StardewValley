#include "Menu.h"

USING_NS_CC;

Scene* MenuScene::createScene()
{
    return MenuScene::create();
}

bool MenuScene::init()
{
    // 1. super init first
    if (!Scene::init())
    {
        return false;
    }

    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();


}


void MenuScene::menuCloseCallback(Ref* pSender)
{
    Director::getInstance()->end();
}