#include "connect_qaction.h"

Connect_QAction::Connect_QAction(QObject *parent) :
    QAction(parent)
{
    this->disconnect();
}

void StateAction::connect()
{
    this->setIcon(QIcon(":/images/connect.png"));
}

void StateAction::disconnect()
{
    this->setIcon(QIcon(":/images/disconnect.png"));
}
