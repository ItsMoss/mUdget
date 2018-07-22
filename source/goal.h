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

	QLabel label;
	QComboBox need;
	QSpinBox amount;
	QComboBox category;
	QComboBox time;
	QHBoxLayout hLayout;

public:
	Goal();
	Goal(int needIndex, double amt, QString cat, int timeIndex);
	~Goal();
	bool save(QFile & f);

public slots:
	void validate();

signals:
	void broadcast();

};

#endif // __GOAL_H__
