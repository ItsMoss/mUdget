#include "logger.h"

void log(std::string message, int level) {
	static logger loggerFile;
	loggerFile.log(message, static_cast<LogLevel>(level));
}

logger::logger() : initialized(false), fileName("mog.txt") {
	stream.open(fileName.c_str());
	initialized = stream.is_open();
	levelMap[LogLevel::Info] = "INFO";
	levelMap[LogLevel::Debug] = "DEBG";
	levelMap[LogLevel::Warn] = "WARN";
	levelMap[LogLevel::Error] = "EROR";
	levelMap[LogLevel::Severe] = "SEVR";
}

logger::logger(std::string fname) : initialized(false), fileName(fname) {
	stream.open(fileName.c_str());
	initialized = stream.is_open();
	levelMap[LogLevel::Info] = "INFO";
	levelMap[LogLevel::Debug] = "DEBG";
	levelMap[LogLevel::Warn] = "WARN";
	levelMap[LogLevel::Error] = "EROR";
	levelMap[LogLevel::Severe] = "SEVR";
}

logger::~logger() {
	if (stream.is_open()) {
		stream.close();
	}
}

void logger::log(std::string message, LogLevel level) {
	if (initialized && stream.is_open() && stream.good()) {
		stream << levelMap[level] << ":" << message << std::endl;
	}
}