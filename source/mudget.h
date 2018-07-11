#pragma once

#include <QtWidgets/QMainWindow>
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
#include <set>
#include <string>
#include "ui_mudget.h"
#include "mudgetcategory.h"
#include "logger.h"

#define SAVE_LOAD_DIRECTORY "./moneyfiles/"
#define MOSS_FILE_EXT		".moss"
#define SETTINGS_FILE_NAME	"settings.mxt"

class mudget : public QMainWindow
{
	Q_OBJECT

	Ui::mudgetClass ui;
	mudgetCategory* uiIncome;
	std::vector<mudgetCategory*> expenses;
	std::vector<mudgetCategory*> tempExpenses;
	mudgetCategory* tempIncome;
	std::map<int, QString> monthMap;
	std::map<int, QString> yearMap;
	std::map<int, QString> categoryMap;
	std::map<QString, int> monthDaysMap;
	std::map<QString, bool> monthsCalculateMap;
	std::map<QString, bool> categoryCalculateMap;
	QTimer* loadTimer;
	bool skipSlot;

public:
	mudget(QWidget *parent = Q_NULLPTR);
	~mudget();

public slots:
	void addMudgetCategory();
	void load(QString openFName = "");
	void performWantedCalculation();
	void save();
	void setCalculationSettings();
	void setCategories();
	void updateCurrentMonthYear(int);
	void updateExpenses();
	void updateIncome();

protected:
	virtual void closeEvent(QCloseEvent * qce);

private:
	mudgetCategory* add_first_available_expense_category();
	bool auto_save_settings();
	double calculate_expenses(bool temp=true, bool calcAll=false);
	double calculate_income(bool temp=true) const;
	void clean_up_ui();
	void create_income_category();
	void delete_all();
	void delete_temp();
	int display_message(const QString & msg, bool question = false);
	void find_matching_expenses(std::vector<mudgetCategory*> & matches, QString catname, bool temp=true);
	void init_month_year_maps();
	bool load_settings();
	std::string remove_newline(std::string & str);
	void update_calculation_combo();
	void update_categories();
	void update_category_calculations();
	void update_profit();
};
