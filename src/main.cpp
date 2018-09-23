#include "mudget.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	mudget w;
	w.show();
	w.load();
	w.setWindowIcon(QPixmap(":mudget/Resources/icon.png"));
	return a.exec();
}
