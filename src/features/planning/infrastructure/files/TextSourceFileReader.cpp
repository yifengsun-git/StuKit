#include "TextSourceFileReader.h"

#include <QFile>
#include <QFileInfo>
#include <QStringConverter>

namespace stukit::planning {
namespace {

constexpr qint64 MaximumImportBytes = 2 * 1024 * 1024;

std::optional<SourceDocumentFormat> formatForSuffix(const QString &suffix)
{
    if (suffix == QStringLiteral("txt")) {
        return SourceDocumentFormat::PlainText;
    }
    if (suffix == QStringLiteral("md")
        || suffix == QStringLiteral("markdown")) {
        return SourceDocumentFormat::Markdown;
    }
    if (suffix == QStringLiteral("csv")) {
        return SourceDocumentFormat::Csv;
    }
    return std::nullopt;
}

} // namespace

std::optional<SourceFileContent> TextSourceFileReader::read(
    const QString &filePath, QString &error) const
{
    error.clear();
    const QFileInfo info(filePath);
    const auto format = formatForSuffix(info.suffix().toLower());
    if (!format) {
        error = QStringLiteral("仅支持 .txt、.md、.markdown 和 .csv 文件");
        return std::nullopt;
    }
    if (!info.exists() || !info.isFile()) {
        error = QStringLiteral("选择的资料文件不存在");
        return std::nullopt;
    }
    if (info.size() > MaximumImportBytes) {
        error = QStringLiteral("单个资料文件不能超过 2 MiB");
        return std::nullopt;
    }

    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("无法读取资料文件：%1").arg(file.errorString());
        return std::nullopt;
    }
    const QByteArray bytes = file.readAll();
    QStringDecoder decoder(QStringDecoder::Utf8);
    const QString content = decoder.decode(bytes);
    if (decoder.hasError()) {
        error = QStringLiteral("资料文件必须使用 UTF-8 编码");
        return std::nullopt;
    }

    return SourceFileContent{
        .fileName = info.fileName(),
        .content = content,
        .format = *format,
    };
}

} // namespace stukit::planning
