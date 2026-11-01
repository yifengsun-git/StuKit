#include "ThemePackageReader.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QSet>
#include <QStringList>
#include <QXmlStreamReader>

#include <miniz.h>

#include <limits>

namespace stukit::app
{
namespace
{

constexpr qint64 MaximumLegacyThemeSize = 256 * 1024;
constexpr qint64 MaximumArchiveSize = 5 * 1024 * 1024;
constexpr quint64 MaximumResourceSize = 4 * 1024 * 1024;
constexpr quint64 MaximumTotalExpandedSize = 10 * 1024 * 1024;
constexpr mz_uint MaximumArchiveEntries = 64;
constexpr qint64 MaximumSvgSize = 512 * 1024;
constexpr int MaximumImageDimension = 4096;
constexpr qint64 MaximumImagePixels = 16 * 1024 * 1024;
constexpr int MaximumSvgElements = 2000;
constexpr int MaximumSvgDepth = 32;

bool isZipArchive(const QByteArray &contents)
{
    return contents.size() >= 4 && contents.at(0) == 'P' && contents.at(1) == 'K' &&
           (contents.at(2) == 3 || contents.at(2) == 5 || contents.at(2) == 7) &&
           (contents.at(3) == 4 || contents.at(3) == 6 || contents.at(3) == 8);
}

bool isSafeRelativePath(const QString &path)
{
    if (path.isEmpty() || path.startsWith(QLatin1Char('/')) || path.contains(QLatin1Char('\\')) ||
        path.contains(QLatin1Char(':')) || path.contains(QChar::Null) ||
        path.contains(QChar::ReplacementCharacter))
    {
        return false;
    }

    QString normalizedPath = path;
    if (normalizedPath.endsWith(QLatin1Char('/')))
    {
        normalizedPath.chop(1);
    }
    if (normalizedPath.isEmpty() || QDir::cleanPath(normalizedPath) != normalizedPath)
    {
        return false;
    }

    const QStringList parts = normalizedPath.split(QLatin1Char('/'));
    for (const QString &part : parts)
    {
        if (part.isEmpty() || part == QStringLiteral(".") || part == QStringLiteral(".."))
        {
            return false;
        }
    }
    return true;
}

bool isAllowedPackagePath(const QString &path, bool directory)
{
    if (directory)
    {
        const QString directoryPath = path.endsWith(QLatin1Char('/')) ? path.chopped(1) : path;
        return directoryPath == QStringLiteral("assets") ||
               directoryPath.startsWith(QStringLiteral("assets/"));
    }

    if (path == QStringLiteral("theme.json") || path == QStringLiteral("preview.png") ||
        path == QStringLiteral("preview.jpg") || path == QStringLiteral("preview.jpeg") ||
        path == QStringLiteral("preview.webp") || path == QStringLiteral("LICENSE.txt") ||
        path == QStringLiteral("LICENSE.md"))
    {
        return true;
    }
    if (!path.startsWith(QStringLiteral("assets/")))
    {
        return false;
    }

    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == QStringLiteral("png") || suffix == QStringLiteral("jpg") ||
           suffix == QStringLiteral("jpeg") || suffix == QStringLiteral("webp") ||
           suffix == QStringLiteral("svg");
}

bool validateRasterImage(const QString &path, const QByteArray &data, QString &errorMessage)
{
    QBuffer buffer;
    buffer.setData(data);
    if (!buffer.open(QIODevice::ReadOnly))
    {
        errorMessage = QStringLiteral("无法检查图片资源：%1").arg(path);
        return false;
    }

    QImageReader reader(&buffer);
    reader.setDecideFormatFromContent(true);
    const QByteArray detectedFormat = reader.format().toLower();
    const QString suffix = QFileInfo(path).suffix().toLower();
    const bool formatMatches =
        (suffix == QStringLiteral("png") && detectedFormat == "png") ||
        ((suffix == QStringLiteral("jpg") || suffix == QStringLiteral("jpeg")) &&
         (detectedFormat == "jpg" || detectedFormat == "jpeg")) ||
        (suffix == QStringLiteral("webp") && detectedFormat == "webp");
    if (!reader.canRead() || !formatMatches)
    {
        errorMessage = QStringLiteral("图片内容与扩展名不匹配或格式不受支持：%1").arg(path);
        return false;
    }

    const QSize size = reader.size();
    if (!size.isValid() || size.width() > MaximumImageDimension ||
        size.height() > MaximumImageDimension ||
        static_cast<qint64>(size.width()) * size.height() > MaximumImagePixels)
    {
        errorMessage = QStringLiteral("图片尺寸无效或超过 4096×4096：%1").arg(path);
        return false;
    }
    return true;
}

bool validateSvg(const QString &path, const QByteArray &data, QString &errorMessage)
{
    if (data.isEmpty() || data.size() > MaximumSvgSize)
    {
        errorMessage = QStringLiteral("SVG 不能为空且不能超过 512 KB：%1").arg(path);
        return false;
    }

    static const QSet<QString> allowedElements{
        QStringLiteral("svg"),
        QStringLiteral("g"),
        QStringLiteral("path"),
        QStringLiteral("rect"),
        QStringLiteral("circle"),
        QStringLiteral("ellipse"),
        QStringLiteral("line"),
        QStringLiteral("polyline"),
        QStringLiteral("polygon"),
        QStringLiteral("defs"),
        QStringLiteral("linearGradient"),
        QStringLiteral("radialGradient"),
        QStringLiteral("stop"),
        QStringLiteral("clipPath"),
        QStringLiteral("mask"),
        QStringLiteral("title"),
        QStringLiteral("desc"),
        QStringLiteral("use"),
    };

    QXmlStreamReader xml(data);
    int elementCount = 0;
    int depth = 0;
    while (!xml.atEnd())
    {
        const auto token = xml.readNext();
        if (token == QXmlStreamReader::DTD || token == QXmlStreamReader::EntityReference)
        {
            errorMessage = QStringLiteral("SVG 不允许 DTD 或实体引用：%1").arg(path);
            return false;
        }
        if (token == QXmlStreamReader::StartElement)
        {
            ++elementCount;
            ++depth;
            const QString elementName = xml.name().toString();
            if (!allowedElements.contains(elementName) || elementCount > MaximumSvgElements ||
                depth > MaximumSvgDepth)
            {
                errorMessage = QStringLiteral("SVG 包含不允许或过于复杂的元素：%1").arg(path);
                return false;
            }

            for (const QXmlStreamAttribute &attribute : xml.attributes())
            {
                const QString name = attribute.name().toString().toLower();
                const QString value = attribute.value().toString().trimmed();
                const QString lowerValue = value.toLower();
                if (name.startsWith(QStringLiteral("on")) ||
                    lowerValue.contains(QStringLiteral("javascript:")) ||
                    lowerValue.contains(QStringLiteral("http:")) ||
                    lowerValue.contains(QStringLiteral("https:")) ||
                    lowerValue.contains(QStringLiteral("file:")) ||
                    lowerValue.contains(QStringLiteral("data:")) ||
                    (lowerValue.contains(QStringLiteral("url(")) &&
                     !lowerValue.contains(QStringLiteral("url(#"))) ||
                    ((name == QStringLiteral("href") || name == QStringLiteral("xlink:href")) &&
                     !value.startsWith(QLatin1Char('#'))))
                {
                    errorMessage = QStringLiteral("SVG 包含脚本或外部资源引用：%1").arg(path);
                    return false;
                }
            }
        }
        else if (token == QXmlStreamReader::EndElement)
        {
            --depth;
        }
    }
    if (xml.hasError() || elementCount == 0 || depth != 0)
    {
        errorMessage = QStringLiteral("SVG XML 格式无效：%1").arg(path);
        return false;
    }
    return true;
}

bool validateResourceContents(const QString &path, const QByteArray &data, QString &errorMessage)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QStringLiteral("svg"))
    {
        return validateSvg(path, data, errorMessage);
    }
    if (suffix == QStringLiteral("png") || suffix == QStringLiteral("jpg") ||
        suffix == QStringLiteral("jpeg") || suffix == QStringLiteral("webp"))
    {
        return validateRasterImage(path, data, errorMessage);
    }
    return true;
}

QString archiveError(mz_zip_archive &archive, const QString &prefix)
{
    const char *details = mz_zip_get_error_string(mz_zip_get_last_error(&archive));
    return details ? QStringLiteral("%1：%2").arg(prefix, QString::fromLatin1(details)) : prefix;
}

bool readArchive(const QByteArray &contents, ThemePackageData &package, QString &errorMessage)
{
    mz_zip_archive archive{};
    mz_zip_zero_struct(&archive);
    if (!mz_zip_reader_init_mem(&archive, contents.constData(),
                                static_cast<size_t>(contents.size()), 0))
    {
        errorMessage = archiveError(archive, QStringLiteral("无法打开主题压缩包"));
        return false;
    }

    struct ArchiveGuard
    {
        mz_zip_archive &archive;
        ~ArchiveGuard()
        {
            mz_zip_reader_end(&archive);
        }
    } guard{archive};

    const mz_uint entryCount = mz_zip_reader_get_num_files(&archive);
    if (entryCount == 0 || entryCount > MaximumArchiveEntries)
    {
        errorMessage = QStringLiteral("主题压缩包必须包含 1～64 个条目。");
        return false;
    }

    QSet<QString> normalizedPaths;
    quint64 totalExpandedSize = 0;
    bool foundThemeDefinition = false;
    for (mz_uint index = 0; index < entryCount; ++index)
    {
        mz_zip_archive_file_stat fileInfo{};
        if (!mz_zip_reader_file_stat(&archive, index, &fileInfo))
        {
            errorMessage = archiveError(archive, QStringLiteral("无法读取主题包文件信息"));
            return false;
        }

        const mz_uint requiredNameSize = mz_zip_reader_get_filename(&archive, index, nullptr, 0);
        if (requiredNameSize <= 1 || requiredNameSize > 1024)
        {
            errorMessage = QStringLiteral("主题包中存在无效或过长的文件名。");
            return false;
        }
        QByteArray encodedName(static_cast<qsizetype>(requiredNameSize), '\0');
        mz_zip_reader_get_filename(&archive, index, encodedName.data(), requiredNameSize);
        const QString path = QString::fromUtf8(encodedName.constData());
        if (!isSafeRelativePath(path) || !isAllowedPackagePath(path, fileInfo.m_is_directory))
        {
            errorMessage = QStringLiteral("主题包包含不允许的路径：%1").arg(path);
            return false;
        }

        const QString collisionKey = path.toCaseFolded();
        if (normalizedPaths.contains(collisionKey))
        {
            errorMessage = QStringLiteral("主题包包含重复路径：%1").arg(path);
            return false;
        }
        normalizedPaths.insert(collisionKey);

        if (fileInfo.m_is_directory)
        {
            continue;
        }
        if (fileInfo.m_is_encrypted || !fileInfo.m_is_supported)
        {
            errorMessage = QStringLiteral("主题包包含加密文件或不支持的压缩格式：%1").arg(path);
            return false;
        }

        const quint64 maximumSize =
            path == QStringLiteral("theme.json") ? MaximumLegacyThemeSize : MaximumResourceSize;
        if (fileInfo.m_uncomp_size > maximumSize ||
            totalExpandedSize > MaximumTotalExpandedSize - fileInfo.m_uncomp_size)
        {
            errorMessage = QStringLiteral("主题包解压后的文件过大：%1").arg(path);
            return false;
        }
        totalExpandedSize += fileInfo.m_uncomp_size;

        if (fileInfo.m_uncomp_size > static_cast<mz_uint64>(std::numeric_limits<qsizetype>::max()))
        {
            errorMessage = QStringLiteral("主题资源超出当前平台支持的大小。");
            return false;
        }
        QByteArray data(static_cast<qsizetype>(fileInfo.m_uncomp_size), '\0');
        if (!mz_zip_reader_extract_to_mem(&archive, index, data.data(),
                                          static_cast<size_t>(data.size()), 0))
        {
            errorMessage = archiveError(archive, QStringLiteral("解压主题资源失败"));
            return false;
        }

        if (path == QStringLiteral("theme.json"))
        {
            if (data.isEmpty())
            {
                errorMessage = QStringLiteral("theme.json 不能为空。");
                return false;
            }
            package.themeDefinition = data;
            foundThemeDefinition = true;
        }
        else
        {
            if (!validateResourceContents(path, data, errorMessage))
            {
                return false;
            }
            package.resources.insert(path, data);
        }
    }

    if (!foundThemeDefinition)
    {
        errorMessage = QStringLiteral("主题压缩包根目录缺少 theme.json。");
        return false;
    }
    package.archive = true;
    return true;
}

} // namespace

bool ThemePackageReader::read(const QString &filePath, ThemePackageData &package,
                              QString &errorMessage)
{
    package = {};
    QFile source(filePath);
    if (!source.exists() || !source.open(QIODevice::ReadOnly))
    {
        errorMessage = QStringLiteral("无法读取所选主题文件。");
        return false;
    }
    if (source.size() <= 0 || source.size() > MaximumArchiveSize)
    {
        errorMessage = QStringLiteral("主题文件必须小于 5 MB，且内容不能为空。");
        return false;
    }

    const QByteArray contents = source.readAll();
    if (isZipArchive(contents))
    {
        return readArchive(contents, package, errorMessage);
    }
    if (contents.size() > MaximumLegacyThemeSize)
    {
        errorMessage = QStringLiteral("旧版 JSON 主题文件不能超过 256 KB。");
        return false;
    }

    package.themeDefinition = contents;
    return true;
}

} // namespace stukit::app
