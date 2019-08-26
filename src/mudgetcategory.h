#ifndef __mudgetCategory_H__
#define __mudgetCategory_H__

#include "expenseitem.h"
#include "melpers.h"
#include <map>
#include <qdialog.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qfile.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qobject.h>
#include <qsizepolicy.h>
#include <set>
#include <vector>

class mudgetCategory : public QFrame {
	Q_OBJECT

	QLineEdit category;					// name of expense
	QGridLayout mainLayout;				// layout of widget
	QGridLayout itemLayout;				// layout of expense line items
	QPushButton addButton;				// button to add a new expense item
	double total;						// total amount expenses
	std::vector<expenseItem> lineItems;	// all the expenses by name, value pair
	bool loading;						// are line items currently being loaded

	public:
		QPushButton addCategory;		// button to add category (public in order for mudget to access pressed SIGNAL)

	public:	// methods
		mudgetCategory(bool showIt=true);
		mudgetCategory(QString cat, bool showIt=true);
		mudgetCategory(const mudgetCategory & rhs);
		mudgetCategory & operator=(const mudgetCategory & rhs);
		~mudgetCategory();
		QString get_category_name() const;
		void set_category_name(QString name);
		double get_total() const;
		bool load(QFile & f);
		void reset(bool showAddCat=false);
		std::string save();
	
	public slots:
		void addLineItem();
		void createRecord(int itemNumber=-1);
		void updateTotal();
	
	private:
		std::string remove_newline(std::string & str);
	signals:
		void sendRecord(QString, double, QString, int, QString);
		void updateExpenses();

};

#endif // __mudgetCategory_H__
