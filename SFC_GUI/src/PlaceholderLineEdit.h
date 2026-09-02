#pragma once

#include <QColor>
#include <QLineEdit>
#include <QPainter>
#include <QPaintEvent>
#include <QStyle>
#include <QStyleOptionFrame>

class PlaceholderLineEdit : public QLineEdit
{
public:
    explicit PlaceholderLineEdit(QWidget *parent = nullptr)
        : QLineEdit(parent)
    {
    }

    void setGhostText(const QString &text)
    {
        m_ghostText = text;
        update();
    }

    void setGhostTextColor(const QColor &color)
    {
        m_ghostTextColor = color;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QLineEdit::paintEvent(event);

        if (!text().isEmpty() || m_ghostText.isEmpty()) {
            return;
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        QFont ghostFont = font();
        const qreal currentSize = ghostFont.pointSizeF();
        if (currentSize > 0.0) {
            ghostFont.setPointSizeF(qMax<qreal>(currentSize - 1.0, 8.0));
        }
        painter.setFont(ghostFont);
        painter.setPen(isEnabled() ? m_ghostTextColor : m_ghostTextColor.darker(110));

        QStyleOptionFrame option;
        initStyleOption(&option);

        QRect contentRect = style()->subElementRect(QStyle::SE_LineEditContents, &option, this);
        const QString visibleText =
            painter.fontMetrics().elidedText(m_ghostText, Qt::ElideRight, contentRect.width());

        painter.drawText(
            contentRect.adjusted(1, 0, -1, 0),
            Qt::AlignVCenter | Qt::AlignLeft,
            visibleText
        );
    }

private:
    QString m_ghostText;
    QColor m_ghostTextColor = QColor(178, 186, 196);
};
