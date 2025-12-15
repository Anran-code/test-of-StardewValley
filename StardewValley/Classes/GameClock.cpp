#include "GameClock.h"
#include <cstdio>

GameClock::GameClock()
    : _hour(6)
    , _minute(0)
    , _day(1)
    , _year(1)
    , _season(Season::Spring)
    , _acc(0.0f)
    , _secondsPerTenMinutes(7.0f)
{
}

void GameClock::update(float dt)
{
    _acc += dt;
    while (_acc >= _secondsPerTenMinutes)
    {
        _acc -= _secondsPerTenMinutes;
        _minute += 10;
        if (_minute >= 60)
        {
            _minute = 0;
            _hour += 1;
        }
        if (_hour >= 24)
        {
            _hour -= 24;
        }

        int minutesSinceSixAM = 0;
        if (_hour >= 6)
        {
            minutesSinceSixAM = (_hour - 6) * 60 + _minute;
        }
        else
        {
            minutesSinceSixAM = (24 - 6 + _hour) * 60 + _minute;
        }

        if (minutesSinceSixAM >= 1200)
        {
            _hour = 6;
            _minute = 0;
            _day += 1;
            if (_day > 28)
            {
                _day = 1;
                int s = static_cast<int>(_season);
                s += 1;
                if (s > static_cast<int>(Season::Winter))
                {
                    s = static_cast<int>(Season::Spring);
                    _year += 1;
                }
                _season = static_cast<Season>(s);
            }
        }
    }
}

void GameClock::setSecondsPerMinute(float spm)
{
    _secondsPerTenMinutes = spm * 10.0f;
}

void GameClock::setSecondsPerTenMinutes(float sptm)
{
    _secondsPerTenMinutes = sptm;
}

int GameClock::getHour() const { return _hour; }
int GameClock::getMinute() const { return _minute; }
int GameClock::getDay() const { return _day; }
int GameClock::getYear() const { return _year; }
GameClock::Season GameClock::getSeason() const { return _season; }

void GameClock::setHour(int h) { _hour = h; }
void GameClock::setMinute(int m) { _minute = m; }
void GameClock::setDay(int d) { _day = d; }
void GameClock::setYear(int y) { _year = y; }
void GameClock::setSeason(Season s) { _season = s; }

int GameClock::getWeekdayIndex() const
{
    return (_day - 1) % 7;
}

int GameClock::getWeekOfSeason() const
{
    return (_day - 1) / 7 + 1;
}

std::string GameClock::getWeekdayString() const
{
    static const char* names[7] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
    return std::string(names[getWeekdayIndex()]);
}

std::string GameClock::getTimeString() const
{
    int h12 = _hour % 12;
    if (h12 == 0) h12 = 12;
    const char* ampm = (_hour < 12) ? "AM" : "PM";
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d:%02d %s", h12, _minute, ampm);
    return std::string(buf);
}

std::string GameClock::getDateString() const
{
    const char* s =
        (_season == Season::Spring) ? "Spring" :
        (_season == Season::Summer) ? "Summer" :
        (_season == Season::Fall)   ? "Fall" : "Winter";
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s Day %d Year %d", s, _day, _year);
    return std::string(buf);
}
