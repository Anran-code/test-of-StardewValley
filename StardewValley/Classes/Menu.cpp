#include "Menu.h"
#include"HomeScene.h"
USING_NS_CC;

Scene* MenuScene::createScene()
{
    return MenuScene::create();
}

// Print useful error message instead of segfaulting when files are not there.
static void problemLoading(const char* filename)
{
    printf("Error while loading: %s\n", filename);
    printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in HelloWorldScene.cpp\n");
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

    // 1.����
    auto bgsprite = Sprite::create("ui/menu_bg.png");
    Size targrtSize = visibleSize;
    Size imgSize = bgsprite->getContentSize();
    float scaleX = targrtSize.width / imgSize.width;
    float scaleY = targrtSize.height / imgSize.height;
    float scale = std::min(scaleX, scaleY);
    bgsprite->setScale(scale);
    // position the sprite on the center of the screen
    bgsprite->setPosition(Vec2(visibleSize.width / 2 + origin.x, visibleSize.height / 2 + origin.y));

    // add the sprite as a child to this layer
    this->addChild(bgsprite);

    // 2.����
    auto ttsprite = Sprite::create("ui/menu_logo.png");
    ttsprite->setPosition(Vec2(visibleSize.width / 2 + origin.x, visibleSize.height*0.75 + origin.y));
    ttsprite->setScale(0.5);
    // add the sprite as a child to this layer
    this->addChild(ttsprite);

    auto closeItem = MenuItemImage::create(
        "ui/btn_exit.png",
        "ui/btn_exit_close.png",
        CC_CALLBACK_1(MenuScene::menuCloseCallback, this));
    if (closeItem == nullptr ||
        closeItem->getContentSize().width <= 0 ||
        closeItem->getContentSize().height <= 0)
    {
        problemLoading("'ui/btn_exit.png' and 'ui/btn_exit_close.png'");
    }
    else
    {
        float x1 = origin.x + 0.75 * visibleSize.width - closeItem->getContentSize().width / 2;
        float y1 = origin.y + 0.15 * visibleSize.height + closeItem->getContentSize().height / 2;
        closeItem->setPosition(Vec2(x1, y1));
        closeItem->setScale(2.0);
    }
    auto newItem = MenuItemImage::create(
        "ui/btn_new.png",
        "ui/btn_new_close.png",
        CC_CALLBACK_1(MenuScene::menuNewCallback, this));
    if (newItem == nullptr ||
        newItem->getContentSize().width <= 0 ||
        newItem->getContentSize().height <= 0)
    {
        problemLoading("'ui/btn_new.png' and 'ui/btn_new_close.png'");
    }
    else
    {
        float x2 = origin.x + 0.25 * visibleSize.width - newItem->getContentSize().width / 2;
        float y2 = origin.y + 0.15 * visibleSize.height + newItem->getContentSize().height / 2;
        newItem->setPosition(Vec2(x2, y2));
        newItem->setScale(2.0);
    }
    auto menu1 = Menu::create(closeItem, NULL);
    menu1->setPosition(Vec2::ZERO);
    this->addChild(menu1);
    auto menu2 = Menu::create(newItem, NULL);
    menu2->setPosition(Vec2::ZERO);
    this->addChild(menu2);

    return true;
}

void MenuScene::menuCloseCallback(Ref* pSender)
{
    Director::getInstance()->end();
}

void MenuScene::menuNewCallback(Ref* pSender)
{
    // 1. ���� HomeScene ����
    // HomeScene::createScene() ���� HomeScene.h �ж���ľ�̬����
    auto scene = HomeScene::createScene();

    // 2. ʹ�� Director �л�����
    // ʹ�� replaceScene �滻��ǰ���еĳ���
    Director::getInstance()->replaceScene(scene);
}
