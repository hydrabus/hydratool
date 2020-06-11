/****************************************************************************
**
** Copyright (C) 2015 Benjamin Vernoux
**
****************************************************************************/

#ifndef CONSOLE_H
#define CONSOLE_H

#include <QPlainTextEdit>
#include <QObject>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE
class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;
QT_END_NAMESPACE

class LineNumberArea;

class Console : public QPlainTextEdit
{
    Q_OBJECT

signals:
    void getData(const QByteArray &data);

public:
    Console(QWidget *parent = 0);

    void putData(const QByteArray &data);

    void setLocalEchoEnabled(bool set);
    void setLocalVerticalScrollBarMaxEnabled(bool set);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();

protected:
    virtual void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;
    virtual void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    virtual void mouseDoubleClickEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
    virtual void contextMenuEvent(QContextMenuEvent *e) Q_DECL_OVERRIDE;

    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);

private:
    bool localEchoEnabled;
    bool localVerticalAutoScrollEnabled;
    QWidget *lineNumberArea;
    QElapsedTimer t;
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(Console *editor) : QWidget(editor) {
        console = editor;
    }

    QSize sizeHint() const Q_DECL_OVERRIDE {
        return QSize(console->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE {
        console->lineNumberAreaPaintEvent(event);
    }

private:
    Console *console;
};

#endif // CONSOLE_H
