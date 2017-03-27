#!/bin/sh

echo
qmake --version
echo
g++ -v
echo

DIR=`mktemp -d`
cd $DIR

echo "#include <QApplication>\n" \
     "#include <QPushButton>\n" \
     "" \
     "int main(int argc, char *argv[])\n" \
     "{\n" \
     "    QApplication a(argc, argv);\n" \
     "" \
     "    QPushButton button (\"Hello world !\");\n" \
     "    button.show();\n" \
     "" \
     "    return a.exec();\n" \
     "}\n" \
     > test.cpp

echo "QT += core gui widgets\n" \
     "TARGET = test\n" \
     "TEMPLATE = app\n" \
     "SOURCES += test.cpp\n" \
     > test.pro

qmake test.pro
make

