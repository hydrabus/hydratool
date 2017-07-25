#ifndef TERMINALWINDOW_H
#define TERMINALWINDOW_H

#include <QMainWindow>

namespace Ui {
class TerminalWindow;
}

class TerminalWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TerminalWindow(QWidget *parent = 0);
    ~TerminalWindow();

private:
    Ui::TerminalWindow *ui;
};

#endif // TERMINALWINDOW_H
