#include "Wallet.h"

Wallet::Wallet()
    : _money(500)
{
}

int Wallet::getMoney() const
{
    return _money;
}

void Wallet::setMoney(int money)
{
    _money = money;
}

void Wallet::addMoney(int amount)
{
    _money += amount;
}

bool Wallet::spendMoney(int amount)
{
    if (_money < amount) return false;
    _money -= amount;
    return true;
}
