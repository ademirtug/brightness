#ifndef __DATETIME_H__
#define __DATETIME_H__

#include <ctime>
#include <time.h>
#include <string>
#include <sstream>
#include <iomanip>

class datetime
{
	
	void retime()
	{
		time_t tt = mktime(&_val);
		_val = *localtime(&tt);
	}
public:
	struct tm _val;
	datetime()
	{
	}
	datetime(const datetime& rhs)
	{
		this->_val = rhs._val;
	}
	datetime(const std::string& value)
	{
		this->operator=(value);
	}
	~datetime()
	{
	}

	static datetime now()
	{
		datetime dt;

		time_t now = time(0);
		dt._val = *localtime(&now);

		return dt;
	}
	std::string tostring()
	{
		return tostring("%Y-%m-%dT%H:%M:%SZ");
	}
	std::string tostring(std::string format)
	{
		std::string val;

		char buff[100];
		strftime(buff, 100, format.c_str(), &_val);
		val += buff;

		return val;
	}
	void operator=(const std::string& val)
	{
		std::istringstream ss(val);
		ss >> std::get_time(&this->_val, "%Y-%m-%dT%H:%M:%SZ");
	}


	void add_seconds(int seconds)
	{
		_val.tm_sec += seconds;
		retime();
	}
	void add_minutes(int minutes)
	{
		_val.tm_min += minutes;
		retime();
	}
	void add_hours(int hours)
	{
		_val.tm_hour += hours;
		retime();
	}
	void add_days(int days)
	{
		_val.tm_mday += days;
		retime();
	}
	void add_months(int months)
	{
		_val.tm_mon += months;
		retime();
	}
	void add_years(int years)
	{
		_val.tm_year += years;
		retime();
	}

	inline int hour()
	{
		return _val.tm_hour;
	}
	inline int minute()
	{
		return _val.tm_min;
	}
	inline int second()
	{
		return _val.tm_sec;
	}

	inline int year()
	{
		return _val.tm_year;
	}
	inline int month()
	{
		return _val.tm_mon;
	}

};


#endif//