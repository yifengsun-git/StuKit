#include "MarkdownPlanFileWriter.h"

#include <QFileInfo>
#include <QSaveFile>

namespace stukit::planning {

bool MarkdownPlanFileWriter::writeMarkdown(
    const QString &filePath, const QString &markdownContent, QString &error)
{
    error.clear();
    QString target = filePath.trimmed();
    if (target.isEmpty() || markdownContent.trimmed().isEmpty()) {
        error = QStringLiteral("导出路径和规划正文不能为空");
        return false;
    }
    if (QFileInfo(target).suffix().isEmpty()) {
        target += QStringLiteral(".md");
    }
    QSaveFile file(target);
    if (!file.open(QIODevice::WriteOnly)) {
        error = QStringLiteral("无法创建 Markdown 文件：%1")
                    .arg(file.errorString());
        return false;
    }
    const QByteArray bytes = markdownContent.toUtf8();
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        error = QStringLiteral("Markdown 文件写入失败：%1")
                    .arg(file.errorString());
        return false;
    }
    return true;
}

} // namespace stukit::planning
