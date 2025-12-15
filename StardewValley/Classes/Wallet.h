#ifndef __WALLET_H__
#define __WALLET_H__

class Wallet
{
public:
    Wallet();

    int getMoney() const;
    void setMoney(int money);
    void addMoney(int amount);
    bool spendMoney(int amount);

private:
    int _money;
};

#endif
