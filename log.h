#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

#ifdef __cplusplus

#include <QDialog>
#include <QString>
#include <QScrollBar>
#include <QProgressBar>
#include <QLineEdit>

namespace Ui {
class Log;
}

class Log : public QDialog
{
    Q_OBJECT

private:
    void reject();

public:
    static bool logOpen;
    explicit Log(QWidget *parent = 0);
    void set_up(QProgressBar *bar, QLineEdit *edit);
    ~Log();
    Ui::Log *ui;
    QString *text;
    QScrollBar *bar;
    int progressed;

private slots:
    void on_buttonClose_clicked();
    void on_buttonClear_clicked();
    void write(char *msg);

signals:
    void call_write(char *msg);
    void progress_set(int va);
    void ticker_set(QString text);
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
EXPORT_C void r_printf(const char *format, ...);
EXPORT_C void set_progress_bar_(Log *ptr, int va);
EXPORT_C void set_progress_bar(int va);
EXPORT_C void add_progress_bar_(Log *ptr, int va);
EXPORT_C void add_progress_bar(int va);
EXPORT_C void set_ticker_(Log *ptr, const char *text);
EXPORT_C void set_ticker(const char *text);

#endif // LOG_H
