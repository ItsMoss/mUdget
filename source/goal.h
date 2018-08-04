#ifndef __GOAL_H__
#define __GOAL_H__

#include <QLabel>
#include <QDoubleSpinBox>
#include <QWidget>
#include <QComboBox>
#include <QHBoxLayout>
#include <QFile>
#include <QStringList>
#include <memory>

class Goal : public QWidget {
	Q_OBJECT
	
	std::map<int, QString> & categoryMap;	// map of expense categories
	QLabel label;
	QComboBox need;
	QSpinBox amount;
	QComboBox category;
	QComboBox time;
	QHBoxLayout hLayout;
	bool locked;

public:
	Goal(std::map<int, QString> & map);
	Goal(std::map<int, QString> & map, int needIndex, double amt, QString cat, int timeIndex);
	~Goal();
	QString getNeedText() const;
	int getNeedIndex() const;
	QString getAmountStr() const;
	int getAmount() const;
	QString getCategoryText() const;
	int getCategoryIndex() const;
	QString getTimeText() const;
	int getTimeIndex() const;
	bool load(std::string line);
	std::string save();
	void setLock(bool lck);
	void update_category();

private:
	std::string remove_newline(std::string & str);

public slots:
	void validate();

signals:
	void broadcast();

};

#endif // __GOAL_H__
