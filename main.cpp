#include <QCoreApplication>
#include <QTextCodec>
#include "uvk_central.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

#ifdef Q_OS_WIN64
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("IBM 866"));
#endif
#ifdef Q_OS_LINUX
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

    UVK_Central uvk;
    if (!uvk.init("./uvk.ini")){
        qDebug()<<"ERRORS:\n";
        QString errolLofFileName="./uvk_errors.log";
        QFile txtfile1(errolLofFileName);
        if (txtfile1.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream textStream(&txtfile1);
            textStream.setCodec("Windows-1251");
            for (int i=0;i<uvk.errLog.size();i++) {
                textStream <<uvk.errLog[i] <<endl;
                qDebug()<<uvk.errLog[i];
            };
            txtfile1.close();
        }
        QFileInfo fi(txtfile1);
        qFatal("fail uvk.init, see %s", qPrintable(fi.absoluteFilePath()));
        return -1;
    } else {
        uvk.start();
        return a.exec();
    }
}
