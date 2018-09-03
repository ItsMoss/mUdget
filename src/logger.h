#pragma once
#include <iostream>
#include <fstream>
#include <map>
#include <string>

#define INFO(x)		log(x, 0)
#define DEBUG(x)	log(x, 1)
#define WARN(x)		log(x, 2)
#define ERROR(x)	log(x, 3)
#define SEVERE(x)	log(x, 4)

void log(std::string message, int level);

enum class LogLevel {
	Info,
	Debug,
	Warn,
	Error,
	Severe
};

struct logger {
	bool initialized;
	std::string fileName;
	std::ofstream stream;
	std::map<LogLevel, std::string> levelMap;
public:
	logger();
	logger(std::string fname);
	~logger();
	void log(std::string message, LogLevel level);
};
