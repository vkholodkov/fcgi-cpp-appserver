
#ifndef _DATE_
#define _DATE_

#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>

#define UNIX_EPOCH_JULIAN_DAY   2440587.5
#define SECONDS_PER_DAY         (24 * 60 * 60)
#define SECONDS_PER_QUANT       (15 * 60)
#define QUANTS_PER_DAY          (24 * 4)

class Date {
public:
    Date()
        : m_julian_day(epoch_to_julian_day(time(NULL)))
    {
    }

    Date(const Date &_d)
        : m_julian_day(_d.m_julian_day)
    {
    }

    Date(const std::string &_yyyymmdd)
        : m_julian_day(yyyymmdd_to_julian_day(_yyyymmdd))
    {
    }

    std::string yyyymmdd() const {
        double z = m_julian_day + 0.5;
        double w = ::floor((z - 1867216.25) / 36524.25);
        double x = ::floor(w / 4);
        double a = z + 1 + w - x;
        double b = a + 1524;
        double c = ::floor((b - 122.1) / 365.25);
        double d = ::floor(365.25 * c);
        double e = ::floor((b - d) / 30.6001);
        double f = ::floor(30.6001 * e);

        unsigned int day = b - d - f;
        unsigned int month = (e - 1 <= 12) ? e - 1 : e - 13;
        unsigned int year = (month == 1 || month == 2) ? c - 4715 : c - 4716;

        std::string result;

        result.append(1, '0' + (year / 1000) % 10);
        result.append(1, '0' + (year / 100) % 10);
        result.append(1, '0' + (year / 10) % 10);
        result.append(1, '0' + year % 10);

        result.append(1, '0' + (month / 10) % 10);
        result.append(1, '0' + month % 10);

        result.append(1, '0' + (day / 10) % 10);
        result.append(1, '0' + day % 10);

        return result;
    }

    unsigned int start_quant_id() const {
        return (unsigned int)(m_julian_day - UNIX_EPOCH_JULIAN_DAY) * QUANTS_PER_DAY;
    }

    unsigned int end_quant_id() const {
        return (unsigned int)(m_julian_day - UNIX_EPOCH_JULIAN_DAY) * QUANTS_PER_DAY + QUANTS_PER_DAY - 1;
    }

/*
    std::string day_of_week() {
        static unsigned int centuries[] = { 4, 2, 0, 6 };
        static unsigned int months[] = { 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };

        unsigned int a = centuries[0];

        return "";
    }
*/

    Date& operator=(const Date &_v) {
        m_julian_day = _v.m_julian_day;
        return *this;
    }

    Date& operator+=(unsigned int days) {
        m_julian_day += days;
        return *this;
    }

    Date& operator-=(unsigned int days) {
        m_julian_day -= days;
        return *this;
    }

    bool operator==(const Date &_v) const {
        return ::floor(this->m_julian_day) == ::floor(_v.m_julian_day);
    }

    bool operator!=(const Date &_v) const {
        return ::floor(this->m_julian_day) != ::floor(_v.m_julian_day);
    }

    Date& operator++() {
        m_julian_day += 1.0f;
        return *this;
    }

    Date& operator++(int) {
        m_julian_day += 1.0f;
        return *this;
    }

    static double yyyymmdd_to_julian_day(const std::string &_yyyymmdd)
    {
        unsigned int year = (_yyyymmdd[0] - '0') * 1000
            + (_yyyymmdd[1] - '0') * 100
            + (_yyyymmdd[2] - '0') * 10
            + (_yyyymmdd[3] - '0');

        unsigned int month = (_yyyymmdd[4] - '0') * 10
            + (_yyyymmdd[5] - '0');

        unsigned int day = (_yyyymmdd[6] - '0') * 10
            + (_yyyymmdd[7] - '0');

        return ymd_to_julian_day(year, month, day);
    }

    static double ymd_to_julian_day(unsigned int year, unsigned int month, unsigned int day)
    {
        if(month == 1 || month == 2) {
            year--;
            month += 12;
        }

        double a = ::floor(year / 100);
        double b = ::floor(a / 4);
        double c = 2 - a + b;
        double e = ::floor(365.25 * (year + 4716));
        double f = ::floor(30.6001 * (month + 1));

        return c + day + e + f - 1524.5;
    }

    static double epoch_to_julian_day(time_t epoch)
    {
        return (double)(epoch / SECONDS_PER_DAY) + UNIX_EPOCH_JULIAN_DAY;
    }

    double m_julian_day;
};

#endif //_DATE_
