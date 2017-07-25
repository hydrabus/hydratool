#ifndef PROGRESS_H
#define PROGRESS_H

#include <QProgressDialog>
#include <QTimer>

class progress : public QObject
{
    Q_OBJECT
public:
    explicit progress(QObject *parent = 0);

signals:

public slots:
    void perform();
    void cancel();

private:
    int steps;
    QProgressDialog *pd;
    QTimer *t;
};

#endif // PROGRESS_H
