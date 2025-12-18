#ifndef __GAME_CLOCK_H__
#define __GAME_CLOCK_H__

#include <string>

class GameClock
{
public:
    enum class Season
    {
        Spring,
        Summer,
        Fall,
        Winter
    };

    GameClock();

    void update(float dt);
    void setSecondsPerMinute(float spm);
    void setSecondsPerTenMinutes(float sptm);
    
    // Pause/Resume
    void pause() { _paused = true; }
    void resume() { _paused = false; }
    bool isPaused() const { return _paused; }

    int getHour() const;
    int getMinute() const;
    int getDay() const;
    int getYear() const;
    Season getSeason() const;

    void setHour(int h);
    void setMinute(int m);
    void setDay(int d);
    void setYear(int y);
    void setSeason(Season s);

    void addDay(int days);
    void addHour(int hours);

    // Checks if it's 2:00 AM or later, and advances day if so
    void checkTurnOfDay();

    int getWeekdayIndex() const;
    int getWeekOfSeason() const;
    std::string getWeekdayString() const;
    std::string getTimeString() const;
    std::string getDateString() const;

private:
    int _hour;
    int _minute;
    int _day;
    int _year;
    Season _season;
    float _acc;
    float _secondsPerTenMinutes;
    bool _paused = false;
};

#endif
