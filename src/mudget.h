#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTableView>
#include <QtSql/QSQLDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSQLQuery>
#include <QtSql/QSQLRecord>
#include <QtSql/QSQLRelationalTableModel>
#include <memory>
#include <qcheckbox.h>
#include <qdebug.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qframe.h>
#include <qmessagebox.h>
#include <qsignalmapper.h>
#include <qtextedit.h>
#include <qtimer.h>
#include <qthread.h>
#include <set>
#include <string>
#include "ui_mudget.h"
#include "mudgetcategory.h"
#include "goal.h"
#include "goalbar.h"
#include "logger.h"

#define SAVE_LOAD_DIRECTORY "./moneyfiles/"
#define MOSS_FILE_EXT		".moss"
#define SETTINGS_FILE_NAME	"settings.mxt"
#define DATABASE_FILE_NAME	"safe.db"

class mudget : public QMainWindow
{
	Q_OBJECT

	Ui::mudgetClass ui;										// ui
	// main tab
	std::string lastLoginTime;								// timestamp of last login
	mudgetCategory* uiIncome;								// income category (displayed)
	std::vector<mudgetCategory*> expenses;					// all expense categories (displayed)
	std::vector<mudgetCategory*> tempExpenses;				// all expense categories for month(s) loaded while doing various calculations (not displayed)
	mudgetCategory* tempIncome;								// income category for month(s) while doing various calculations (not displayed)
	std::map<int, QString> monthMap;						// month number to text abbr (e.g. 0->Jan, 1->Feb,...)
	std::map<int, QString> yearMap;							// year number to text (e.g. 0->"2018", 1->"2019",...)
	std::map<int, QString> categoryMap;						// category index to category name
	std::map<QString, int> monthDaysMap;					// month abbr to number of days it contains (e.g. Jan->31, Feb->28, ...)
	std::map<QString, bool> monthsCalculateMap;				// whether or not a month is to be used in calculation(s)
	std::map<QString, bool> categoryCalculateMap;			// whether or noa a category is to be used in calculation(s)
	bool skipSlot;											// should next slot be skipped
	// goals tab
	std::vector<Goal *> goals;								// goals that user has set
	std::unique_ptr<QTimer> goalTimer;						// timer to start showing goal(s) progress
	std::unique_ptr<GoalBar> goalbar;						// progress bar for goal currently being displayed
	std::unique_ptr<QLabel> goalStringLabel;				// label displaying current goal whose progress is being displayed
	std::pair<std::unique_ptr<QLabel>, std::unique_ptr<QLabel> > goldTrophies;		// gold trophies won (pic and total)
	std::pair<std::unique_ptr<QLabel>, std::unique_ptr<QLabel> > silverTrophies;	// silver trophies won (pic and total)
	std::pair<std::unique_ptr<QLabel>, std::unique_ptr<QLabel> > bronzeTrophies;	// bronze trophies won (pic and total)
	// database
	std::unique_ptr<QSqlDatabase> db;						// db connection
	bool dbAvailable;										// does a db connection exist
	std::unique_ptr<QTableView> dbView;						// model for db info
	std::unique_ptr<QSqlRelationalTableModel> dbModel;		// view for db info

public:
	mudget(QWidget *parent = Q_NULLPTR);					// constructor
	~mudget();												// destructor

public slots:
	void addMudgetCategory();								// adds a new expense category to display
	void calculateGoalProgress();							// runs every 5 sec to display progress of each goal
	void load(QString openFName = "");						// loads in a .moss file (i.e. a month's financial data)
	void openDatabaseWindow();								// displayes db view
	void performWantedCalculation();						// performs one of many calculations user has requested
	void receiveRecord(QString exp, double amount, QString cat, int n, QString t);	// receives newley created record info to insert in db
	void save();											// save .moss file
	void setCalculationSettings();							// sets which months and categories should be used in calculations
	void setCategories();									// sets which categories are availble for expense categories
	void updateCurrentMonthYear(int);						// updates/loads current income and expenses displayed for month selected in combo
	void updateExpenses();									// recalculates total expenses for current month
	void updateGoals(bool creating);						// adds or removes a goal
	void updateIncome();									// recalculates total income for current month

protected:
	virtual void closeEvent(QCloseEvent * qce);				// when "X" button is pressed

private:
	mudgetCategory* add_first_available_expense_category();			// adds loaded category into first available space within ui
	bool auto_save_settings();										// saves settings to SETTINGS_FILE_NAME upon closing app
	void award_trophies();											// award any trophies that should be rewarded since last login
	double calculate_expenses(bool temp=true, bool calcAll=false);	// calculates expenses for currently loaded .moss file
	double calculate_income(bool temp=true) const;					// calculates income for currently loaded .moss file
	void clean_up_ui();												
	void clear_goals();												// removes all goals
	void create_income_category();									// initializes income category
	void delete_all();												// deallocates everything
	bool delete_old_db_records();									// removes db records older than 3 months ago
	void delete_temp();												// deallocates tempExpenses and tempIncome
	int display_message(const QString & msg, bool question = false);	// displayes message to user
	void find_matching_expenses(std::vector<mudgetCategory*> & matches, QString catname, bool temp=true);	// finds all expenses under specified category in current month
	void init_database();											// init db
	void init_display_case();										// inits display case on Goals tab
	void init_month_year_maps();									// init maps for month and year
	bool load_settings();											// load auto-saved settings
	std::string remove_newline(std::string & str);					// remove newline character
	void update_calculation_combo();								// updates calculation combo to match all categories in categoryMap
	void update_categories();										// updates categoryMap after setting categories
	void update_category_calculations();							// updates categoryCalculateMap after setting categories
	void update_goal_progress(Goal * g);							// updates progress of specified goal
	void update_monthly_goal(int needidx, int amount, QString category, QString tstamp);	// updates progress for monthly goal
	void update_weekly_goal(int needidx, int amount, QString category, QString tstamp);		// updates progress for weekly goal
	void update_yearly_goal(int needidx, int amount, QString category, QString tstamp);		// updates progress for yearly goal
	void update_profit();											// updates profit
};
