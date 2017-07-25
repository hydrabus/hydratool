#include <QtWidgets>

#include "console.h"

#include <QScrollBar>

//#include <QtCore/QDebug>

Console::Console(QWidget *parent)
    : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    //document()->setMaximumBlockCount(100);
    document()->setMaximumBlockCount(-1);
    //QPalette p = palette();
    //p.setColor(QPalette::Base, Qt::black);
    //p.setColor(QPalette::Text, Qt::green);
    //setPalette(p);

    QFont font;
    font.setFamily(QStringLiteral("Courier New"));
    this->setFont(font);
    //this->setTextInteractionFlags(Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    t.start();
}

void Console::putData(const QByteArray &data)
{
    appendPlainText(data);
    if(localVerticalAutoScrollEnabled == true)
    {
         QScrollBar *bar = verticalScrollBar();
         bar->setValue(bar->maximum());
    }
}

void Console::setLocalEchoEnabled(bool set)
{
    localEchoEnabled = set;
}

void Console::setLocalVerticalScrollBarMaxEnabled(bool set)
{
    localVerticalAutoScrollEnabled = set;
}

void Console::keyPressEvent(QKeyEvent *e)
{
    //Q_UNUSED(e);
    QPlainTextEdit::keyPressEvent(e);
    /*
    switch (e->key()) {
    case Qt::Key_Backspace:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
        break;
    default:
        if (localEchoEnabled)
            QPlainTextEdit::keyPressEvent(e);
        emit getData(e->text().toLocal8Bit());
    }
    */
}

void Console::mousePressEvent(QMouseEvent *e)
{
    QPlainTextEdit::mousePressEvent(e);
    //Q_UNUSED(e)
    //setFocus();
}

void Console::mouseDoubleClickEvent(QMouseEvent *e)
{
    //QPlainTextEdit::mouseDoubleClickEvent(e);
    Q_UNUSED(e)
}

void Console::contextMenuEvent(QContextMenuEvent *e)
{
    QPlainTextEdit::contextMenuEvent(e);
    //Q_UNUSED(e)
}

void Console::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

/*
    if (!isReadOnly())
*/
    {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

int Console::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;
    //int space = 80 + fontMetrics().width(QLatin1Char('9')) * digits;

    return space;
}

void Console::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void Console::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void Console::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void Console::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            //QString number = QString::asprintf("%d %dms", (blockNumber + 1), t.elapsed());
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

