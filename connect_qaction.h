#ifndef CONNECT_QACTION_H
#define CONNECT_QACTION_H


class Connect_QAction : public QAction
{
    Q_OBJECT
public:
    explicit Connect_QAction(QObject *parent = 0);

public slots:
    void connect();
    void disconnect();

};

#endif // CONNECT_QACTION_H
