#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

#ifdef __cplusplus

#include <QDialog>
#include <QString>
#include <QScrollBar>

namespace Ui {
class Log;
}

class Log : public QDialog
{
    Q_OBJECT

public:
    static bool logOpen;
    explicit Log(QWidget *parent = 0);

    ~Log();
    Ui::Log *ui;
    QString *text;
    QScrollBar *bar;

private slots:
    void on_buttonClose_clicked();
    void on_buttonClear_clicked();
    void write(char *msg);

signals:
    void call_write(char *msg);

};

#else

typedef struct Log Log;

#endif // __cplusplus

#ifdef __cplusplus
#define EXPORT_C extern "C"
#else
#define EXPORT_C
#endif

extern Log *logptr;

EXPORT_C void write_c(Log *ptr, char *msg);
EXPORT_C void r_printf(char *format, ...);

#endif // LOG_H
