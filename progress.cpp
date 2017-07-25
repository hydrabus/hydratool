#include "progress.h"

progress::progress(QObject *parent) :
    QObject(parent), steps(0)
{
    pd = new QProgressDialog("Task in progress.", "Cancel", 0, 100000);
    connect(pd, SIGNAL(canceled()), this, SLOT(cancel()));
    t = new QTimer(this);
    connect(t, SIGNAL(timeout()), this, SLOT(perform()));
    t->start(0);
}

void progress::perform()
{
    pd->setValue(steps);

    steps++;
    if (steps > pd->maximum())
        t->stop();
}

void progress::cancel()
{
    t->stop();
}
