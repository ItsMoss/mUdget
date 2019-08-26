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
#include <qscrollarea.h>
#include <qsignalmapper.h>
#include <qtextedit.h>
#include <qtimer.h>
#include <qthread.h>
#include <iterator>
#include <set>
#include <string>
#include "ui_mudget.h"
#include "mudgetcategory.h"
#include "goal.h"
#include "goalbar.h"
#include "investmenttypes.h"
#include "investmentwidget.h"
#include "logger.h"

#define SAVE_LOAD_DIRECTORY "./moneyfiles/"
#define MOSS_FILE_EXT		".moss"
#define SETTINGS_FILE_NAME	"settings.mxt"
#define DATABASE_FILE_NAME	"safe.db"
#define DB_TABLE_HISTORY	"LAST3MONTHS"
#define DB_TABLE_TROPHIES	"TROPHIES"
#define DB_TABLE_INVESTMENTS "INVESTMENTS"

class mudget : public QMainWindow
{
	Q_OBJECT

	Ui::mudgetClass ui;										// ui
	// main tab
	std::string loginTime, lastLoginTime;					// timestamp of current and last login
	mudgetCategory* uiIncome;								// income category (displayed)
	std::vector<mudgetCategory*> expenses;					// all expense categories (displayed)
	std::vector<mudgetCategory*> tempExpenses;				// all expense categories for month(s) loaded while doing various calculations (not displayed)
	mudgetCategory* tempIncome;								// income category for month(s) while doing various calculations (not displayed)
	std::map<int, QString> monthMap;						// month number to text abbr (e.g. 0->Jan, 1->Feb,...)
	std::map<int, QString> yearMap;							// year number to text (e.g. 0->"2018", 1->"2019",...)
	std::set<QString> expenseCategories;					// category names
	std::set<QString> availableCategories;					// categories available to be assigned to new expense boxes
	std::map<QString, int> monthDaysMap;					// month abbr to number of days it contains (e.g. Jan->31, Feb->28, ...)
	std::map<QString, bool> monthsCalculateMap;				// whether or not a month is to be used in calculation(s)
	std::map<QString, bool> categoryCalculateMap;			// whether or noa a category is to be used in calculation(s)
	bool skipSlot;											// should next slot be skipped
	// goals tab
	std::vector<Goal *> goals;								// goals that user has set
	std::unique_ptr<QTimer> goalTimer;						// timer to start showing goal(s) progress
	std::unique_ptr<GoalBar> goalbar;						// progress bar for goal currently being displayed
	std::unique_ptr<QLabel> goalStringLabel;				// label displaying current goal whose progress is being displayed
	std::map<QString, std::unique_ptr<QLabel> > progressFrameHeadings;				// maps string to corresponding heading label within progress frame
	std::map<QString, std::pair<std::unique_ptr<QLabel>, std::unique_ptr<QLabel> > > progressFrameTrophyCts;			// trophy counts for current goal whose progress is being displayed
	std::unique_ptr<QLabel> ytdNetValueLabel;				// year-to-date net amount for goal whose progress is being displayed
	std::pair<std::unique_ptr<QLabel>, std::unique_ptr<QLabel> > goldTrophies;		// gold trophies won (pic and total)
	std::pair<std::unique_ptr<QLabel>, std::unique_ptr<QLabel> > silverTrophies;	// silver trophies won (pic and total)
	std::pair<std::unique_ptr<QLabel>, std::unique_ptr<QLabel> > bronzeTrophies;	// bronze trophies won (pic and total)
	std::unique_ptr<QLabel> possibleTrophyCountLabel;		// label that displays total possible trophies
	// invest tab
	std::vector<InvestmentWidget*> investments;				// investment accounts user has
	// database
	std::unique_ptr<QSqlDatabase> db;						// db connection
	bool dbAvailable;										// does a db connection exist
	std::unique_ptr<QTableView> dbView;						// model for db info
	std::unique_ptr<QSqlRelationalTableModel> dbModel;		// view for db info

public:
	mudget(QWidget *parent = Q_NULLPTR);					// constructor
	~mudget();												// destructor

public slots:
	void addCategoryPressed();								// when "Add Category" pressed to add category to expense box
	void addInvestmentAccount();							// when "Add Account" pressed to add investment widget
	void calculateGoalProgress();							// runs every 5 sec to display progress of each goal
	void load(QString openFName = "");						// loads in a .moss file (i.e. a month's financial data)							** UPDATE **
	void openDatabaseWindow(int);							// displayes db view
	void performWantedCalculation();						// performs one of many calculations user has requested								** FIX **
	void receiveExpenseRecord(QString exp, double amount,	// receives newly created expense record info to insert in db
		QString cat, int n, QString t);
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
	mudgetCategory* get_first_available_expense_category();			// gets first available expense space within ui
	bool auto_save_settings();										// saves settings to SETTINGS_FILE_NAME upon closing app
	void award_trophies();											// award any trophies that should be rewarded since last login
	double calculate_expenses(bool temp=true, bool calcAll=false);	// calculates expenses for currently loaded .moss file
	double calculate_income(bool temp=true) const;					// calculates income for currently loaded .moss file
	void clean_up_ui();												
	void clear_goals();												// removes all goals
	void create_empty_expenses();									// initializes all expense categories
	void create_income_category();									// initializes income category
	void delete_all();												// deallocates everything
	bool delete_old_db_records();									// removes db records older than 3 months ago
	void delete_temp();												// deallocates tempExpenses and tempIncome
	int display_message(const QString & msg, bool question=false);	// displayes message to user
	void evaluate_monthly_goal(GoalNeed needidx, int amount,	// updates progress for monthly goal
		QString category, QString tstamp, bool update);
	void evaluate_weekly_goal(GoalNeed needidx, int amount,		// updates progress for weekly goal
		QString category, QString tstamp, bool update);
	void evaluate_yearly_goal(GoalNeed needidx, int amount,		// updates progress for yearly goal
		QString category, QString tstamp, bool update);
	void find_matching_expenses(std::vector<mudgetCategory*> & matches,	// finds all expenses under specified category in current month
		QString catname, bool temp=true);
	mudgetCategory* get_existing_expense_category(std::string catname);		// returns category box for an already existing category, o/w NULL
	void initialize();												// wrapper for all initialization functions called in constructor
	void init_database();											// init db
	void init_display_case();										// inits display case on Goals tab
	void init_month_year_maps();									// init maps for month and year
	void init_progress_frame();										// inits progress frame on Goals tab
	void insert_trophy(GoalTrophy type, QString desc, QString t,	// inserts trophy record in db
		bool won, float margin);
	bool load_investments();										// load investment account info from db
	bool load_settings();											// load auto-saved settings
	InvestmentWidget* produce_investment_widget(int id,				// produces an investment widget given an ID...like a factory or something :)
		QString accountname, double balance=0.0);			
	std::string remove_newline(std::string & str);					// remove newline character
	bool save_investments();										// saves investments to database
	void setYear2Dates4Goal(Goal * g);								// sets ytdNet and ytdTrophies for a Goal
	void update_available_categories();								// updates availableCategories accordingly (after setting categories, and loading)
	void update_calculation_combo();								// updates calculation combo to match all categories
	void update_category_calculations();							// updates categoryCalculateMap after setting categories
	void update_goal_progress(Goal * g);							// updates progress of specified goal
	void update_profit();											// updates profit
	void update_ytd_for_goal_via_description(QString desc,			// updates YTD for a goal that is being awarded a trophy
		int add2net, GoalTrophy trophy_t);			
	bool valid_last_login();										// is lastLoginTime valid
};
