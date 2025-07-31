#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>
#include <QRegularExpression>

class CHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit CHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<HighlightRule> rules;

    QTextCharFormat keywordFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat multiLineCommentFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat includeFormat;
    QTextCharFormat charFormat;

    QRegularExpression multiLineCommentStart;
    QRegularExpression multiLineCommentEnd;
};
