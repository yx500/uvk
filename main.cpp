#include <QCoreApplication>
#include "uvk_central.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    UVK_Central uvk;
    if (!uvk.init("./M.xml")){
        QString errolLofFileName="./uvk_errors.log";
                QFile txtfile1(errolLofFileName);
                if (txtfile1.open(QFile::WriteOnly | QFile::Truncate)) {
                    QTextStream textStream(&txtfile1);
                    textStream.setCodec("Windows-1251");
                    for (int i=0;i<uvk.errLog.size();i++) {textStream <<uvk.errLog[i] <<endl;};
                }
        qFatal("fail uvk.init see uvk_errors.log.");
        return -1;
    } else {
        uvk.start();
        return a.exec();
    }
}
