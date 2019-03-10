#ifndef __GOAL_H__
#define __GOAL_H__

#include <QLabel>
#include <QDoubleSpinBox>
#include <QWidget>
#include <QComboBox>
#include <QHBoxLayout>
#include <QFile>
#include <QPushButton>
#include <QStringList>
#include <memory>

#define GOLD_THRESHOLD		0.8
#define SILVER_THRESHOLD	0.6
#define BRONZE_THRESHOLD	0.2

enum class GoalNeed {
	SpendLess = 1,
	MakeProfit = 2
};

enum class GoalTime {
	Weekly = 1,
	Monthly = 2,
	Yearly = 3
};

enum class GoalTrophy {
	None,
	Bronze,
	Silver,
	Gold
};

class Goal : public QWidget {
	Q_OBJECT
	
	std::map<int, QString> & categoryMap;	// map of expense categories
	QLabel label;							// starts off goal by reading "I need to"
	QComboBox need;							// combo box for user to select need
	QSpinBox amount;						// $ amount corresponding with need
	QComboBox category;						// category (or categories) that $ amount corresponds to
	QComboBox time;							// is goal weekly, monthly, or yearly
	QPushButton deleteButton;				// button to delete goal
	QHBoxLayout hLayout;					// layout
	bool locked;							// can goal be modified (cannot if locked)
	double ytdNet;							/* year-to-date net amount for goal
											   Ex: if goal is to spend less than $50/week on X, then
											       this is the value of difference b/w $50 * n weeks this year
												   and actual $ amount spent on X this year */
	std::map<GoalTrophy, size_t> ytdTrophies;	// year-to-date trophy counts for this goal

public:
	Goal(std::map<int, QString> & map);		// constructor 1
	Goal(std::map<int, QString> & map, int needIndex, double amt, QString cat, int timeIndex);	// constructor 2
	~Goal();								// destructor
	const QPushButton * getDelete() const;	// returns pointer to deleteButton
	QString getNeedText() const;			// returns need current text
	int getNeedIndex() const;				// returns need current index
	QString getAmountStr() const;			// returns amount current value as text
	int getAmount() const;					// returns amount current value as int
	QString getCategoryText() const;		// returns category current text
	int getCategoryIndex() const;			// returns category current index
	QString getTimeText() const;			// returns time current text
	int getTimeIndex() const;				// returns time current index
	bool load(std::string line);			// populates goal (when loading on app bootup)
	std::string save();						// returns goal as a string (when saving on app close)
	void setLock(bool lck);					// set variable locked
	void update_category();					// updates variable categoryMap
	void setYtdNet(double net);				// set variable ytdNet
	void setYtdTrophies(std::map<GoalTrophy, size_t> trophies);		// set variable ytdTrophies
	void setYtdTrophies(int gold, int silver, int bronze, int failed);		// set variable ytdTrophies
	double getYtdNet() const;				// returns ytdNet
	std::map<GoalTrophy, size_t> getYtdTrophies() const;	// returns ytdTrophies
	void addYtdNet(int toAdd);				// adds to YTD net
	bool addYtdTrophy(GoalTrophy trophy_t);	// increments YTD trophies

private:
	std::string remove_newline(std::string & str);		// removes newline char from string

public slots:
	void resetGoal();						// resets goal to empty values and unlocks
	void validate();						// determines whether current goal is valid (i.e. all fields populated aptly)

signals:
	void broadcast(bool);					// signal that alerts receiver this goal is valid or being removed

};

#endif // __GOAL_H__
