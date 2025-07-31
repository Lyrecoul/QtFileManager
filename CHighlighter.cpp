#include "CHighlighter.h"

CHighlighter::CHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent) {
    // 关键词
    keywordFormat.setForeground(Qt::cyan);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywords = {
        "auto", "bool", "break", "case", "catch", "char", "class", "const", "constexpr", "continue",
        "default", "delete", "do", "double", "else", "enum", "explicit", "extern", "false", "float",
        "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new",
        "nullptr", "operator", "private", "protected", "public", "register", "reinterpret_cast",
        "return", "short", "signed", "sizeof", "static", "static_cast", "struct", "switch", "template",
        "this", "throw", "true", "try", "typedef", "typename", "union", "unsigned", "using", "virtual",
        "void", "volatile", "while"
    };
    for (const QString &word : keywords) {
        rules.append({QRegularExpression("\\b" + word + "\\b"), keywordFormat});
    }

    // 单行注释
    singleLineCommentFormat.setForeground(QColor("#6AA84F"));  // 绿色
    rules.append({QRegularExpression("//[^\n]*"), singleLineCommentFormat});

    // 字符串
    stringFormat.setForeground(QColor("#FFFF99"));  // 黄色
    rules.append({QRegularExpression(R"("([^"\\]|\\.)*")"), stringFormat});

    // 字符常量
    charFormat.setForeground(QColor("#FFCC66"));  // 橙色
    rules.append({QRegularExpression(R"('([^'\\]|\\.)')"), charFormat});

    // 数字
    numberFormat.setForeground(QColor("#FF9966"));  // 橙色
    rules.append({QRegularExpression(R"(\b\d+(\.\d+)?\b)"), numberFormat});

    // include 语句
    includeFormat.setForeground(QColor("#9999FF"));  // 蓝紫
    rules.append({QRegularExpression(R"(^\s*#\s*include\s*[<"].*[>"])"), includeFormat});

    // 多行注释格式
    multiLineCommentFormat.setForeground(QColor("#6AA84F"));  // 绿色
    multiLineCommentStart = QRegularExpression(R"(/\*)");
    multiLineCommentEnd = QRegularExpression(R"(\*/)");
}

void CHighlighter::highlightBlock(const QString &text) {
    // 应用正则规则
    for (const HighlightRule &rule : qAsConst(rules)) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // 多行注释处理
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1) {
        QRegularExpressionMatch match = multiLineCommentStart.match(text);
        startIndex = match.hasMatch() ? match.capturedStart() : -1;
    }

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch = multiLineCommentEnd.match(text, startIndex);
        int endIndex = endMatch.hasMatch() ? endMatch.capturedEnd() : -1;

        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex;
        }

        setFormat(startIndex, commentLength, multiLineCommentFormat);

        // 查找下一段注释
        startIndex = multiLineCommentStart.match(text, startIndex + commentLength).capturedStart();
    }
}
