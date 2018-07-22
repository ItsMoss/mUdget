#ifndef __mudgetCategory_H__
#define __mudgetCategory_H__

#include "expenseitem.h"
#include <map>
#include <qcombobox.h>
#include <qfile.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qobject.h>
#include <string>
#include <time.h>
#include <vector>

class mudgetCategory : public QFrame {
	Q_OBJECT

	std::map<int, QString> & categoryMap;	// map of expense categories
	QComboBox category;					// name of expense
	QGridLayout mainLayout;				// layout of widget
	QGridLayout itemLayout;				// layout of expense line items
	double total;						// total amount expenses
	std::vector<expenseItem> lineItems;	// all the expenses by name, value pair
	bool loading;						// are line items currently being loaded

	public:	// methods
		mudgetCategory(std::map<int, QString> & map, bool showIt=true);
		mudgetCategory(QString cat, std::map<int, QString> & map, bool showIt=true);
		mudgetCategory(const mudgetCategory & rhs);
		mudgetCategory & operator=(const mudgetCategory & rhs);
		~mudgetCategory();
		QString get_category_name() const;
		void set_category_name(QString name);
		double get_total() const;
		bool load(QFile & f);
		void reset();
		std::string save();
		void update_category();
	
	public slots:
		void createRecord(int itemNumber=-1);
		void updateTotal();
	
	private:
		std::string remove_newline(std::string & str);
	signals:
		void sendRecord(QString, double, QString, int, QString);
		void updateExpenses();

};

#endif // __mudgetCategory_H__
