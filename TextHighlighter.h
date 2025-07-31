// TextHighlighter.h
#pragma once

#include "qregularexpression.h"
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class TextHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit TextHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightRule> rules;
};
