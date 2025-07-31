#include "JsonHighlighter.h"

JsonHighlighter::JsonHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {
    keyFormat.setForeground(QColor("#4da6ff"));   // 蓝色键名
    keyFormat.setFontWeight(QFont::Bold);

    stringFormat.setForeground(QColor("#ffff99")); // 黄色字符串

    numberFormat.setForeground(QColor("#ffcc66")); // 橙色数字

    boolFormat.setForeground(QColor("#ff99cc"));   // 粉色 true/false

    nullFormat.setForeground(QColor("#999999"));   // 灰色 null

    punctuationFormat.setForeground(QColor("#bbbbbb")); // 淡灰色标点
}

void JsonHighlighter::highlightBlock(const QString &text) {
    // 键名: "key":
    QRegularExpression keyRegex(R"(\"([^\"\\]*(\\.[^\"\\]*)*)\"\s*:)");
    QRegularExpressionMatchIterator keyIt = keyRegex.globalMatch(text);
    while (keyIt.hasNext()) {
        QRegularExpressionMatch match = keyIt.next();
        setFormat(match.capturedStart(1), match.capturedLength(1), keyFormat);
    }

    // 字符串值: "value"
    QRegularExpression stringRegex(R"(:\s*\"([^"\\]*(\\.[^"\\]*)*)\")");
    QRegularExpressionMatchIterator strIt = stringRegex.globalMatch(text);
    while (strIt.hasNext()) {
        QRegularExpressionMatch match = strIt.next();
        setFormat(match.capturedStart(1), match.capturedLength(1), stringFormat);
    }

    // 数字
    QRegularExpression numberRegex(R"(:\s*(-?\d+(\.\d+)?([eE][+-]?\d+)?))");
    QRegularExpressionMatchIterator numIt = numberRegex.globalMatch(text);
    while (numIt.hasNext()) {
        QRegularExpressionMatch match = numIt.next();
        setFormat(match.capturedStart(1), match.capturedLength(1), numberFormat);
    }

    // 布尔值 true/false
    QRegularExpression boolRegex(R"(\btrue\b|\bfalse\b)");
    QRegularExpressionMatchIterator boolIt = boolRegex.globalMatch(text);
    while (boolIt.hasNext()) {
        QRegularExpressionMatch match = boolIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), boolFormat);
    }

    // null
    QRegularExpression nullRegex(R"(\bnull\b)");
    QRegularExpressionMatchIterator nullIt = nullRegex.globalMatch(text);
    while (nullIt.hasNext()) {
        QRegularExpressionMatch match = nullIt.next();
        setFormat(match.capturedStart(), match.capturedLength(), nullFormat);
    }

    // 花括号、方括号、逗号、冒号
    QRegularExpression punctuationRegex(R"([{}\[\]:,])");
    QRegularExpressionMatchIterator puncIt = punctuationRegex.globalMatch(text);
    while (puncIt.hasNext()) {
        QRegularExpressionMatch match = puncIt.next();
        setFormat(match.capturedStart(), 1, punctuationFormat);
    }
}
