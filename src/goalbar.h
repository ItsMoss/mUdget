#ifndef __GOALBAR_H__
#define __GOALBAR_H__

#include <QProgressBar>
#include <QString>
#include <map>

class GoalBar : public QProgressBar {
	float percentReached;
	std::map<float, QString> percent2Color;

public:
	GoalBar(QWidget * parent = 0);
	GoalBar(const GoalBar & rhs);
	GoalBar & operator=(const GoalBar & rhs);
	~GoalBar();
	void update(int max, int value);

};

#endif // __GOALBAR_H__