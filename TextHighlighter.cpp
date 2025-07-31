// TextHighlighter.cpp
#include "TextHighlighter.h"

TextHighlighter::TextHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(Qt::cyan);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns = {
        "\\bif\\b", "\\belse\\b", "\\bwhile\\b", "\\bfor\\b", "\\breturn\\b",
        "\\bclass\\b", "\\bvoid\\b", "\\bint\\b", "\\bfloat\\b", "\\bconst\\b"
    };
    for (const QString &pattern : keywordPatterns) {
        rules.append({QRegularExpression(pattern), keywordFormat});
    }

    QTextCharFormat commentFormat;
    commentFormat.setForeground(Qt::green);
    rules.append({QRegularExpression("//[^\n]*"), commentFormat});

    QTextCharFormat stringFormat;
    stringFormat.setForeground(Qt::yellow);
    rules.append({QRegularExpression(R"(".*?")"), stringFormat});
}

void TextHighlighter::highlightBlock(const QString &text) {
    for (const HighlightRule &rule : qAsConst(rules)) {
        QRegularExpressionMatchIterator i = rule.pattern.globalMatch(text);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
