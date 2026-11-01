#include "ThemePackageWriter.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringList>

#include <miniz.h>

namespace stukit::app
{
namespace
{

constexpr qint64 MaximumExportedFileSize = 4 * 1024 * 1024;
constexpr qint64 MaximumExportedTotalSize = 10 * 1024 * 1024;
constexpr int MaximumExportedFiles = 64;

bool isAllowedExportPath(const QString &path)
{
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

} // namespace

bool ThemePackageWriter::write(const QString &themeDirectory, const QString &targetPath,
                               QString &errorMessage)
{
    QDir sourceDirectory(themeDirectory);
    if (!sourceDirectory.exists() ||
        !QFileInfo(sourceDirectory.filePath(QStringLiteral("theme.json"))).isFile())
    {
        errorMessage = QStringLiteral("已安装主题目录不完整，无法导出。");
        return false;
    }

    struct Entry
    {
        QByteArray name;
        QByteArray contents;
    };
    QList<Entry> entries;
    qint64 totalSize = 0;
    QDirIterator iterator(themeDirectory, QDir::Files | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext())
    {
        const QString path = iterator.next();
        const QString relativePath =
            sourceDirectory.relativeFilePath(path).replace(QLatin1Char('\\'), QLatin1Char('/'));
        const QFileInfo fileInfo(path);
        if (!isAllowedExportPath(relativePath) || fileInfo.isSymLink() || fileInfo.size() < 0 ||
            fileInfo.size() > MaximumExportedFileSize ||
            totalSize > MaximumExportedTotalSize - fileInfo.size())
        {
            errorMessage = QStringLiteral("主题包含无法安全导出的文件：%1").arg(relativePath);
            return false;
        }
        if (entries.size() >= MaximumExportedFiles)
        {
            errorMessage = QStringLiteral("主题文件数量超过 64 个，无法导出。");
            return false;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
        {
            errorMessage = QStringLiteral("无法读取主题资源：%1").arg(relativePath);
            return false;
        }
        entries.push_back({relativePath.toUtf8(), file.readAll()});
        totalSize += fileInfo.size();
    }

    mz_zip_archive archive{};
    mz_zip_zero_struct(&archive);
    if (!mz_zip_writer_init_heap(&archive, 0, static_cast<size_t>(totalSize + 4096)))
    {
        errorMessage = QStringLiteral("无法初始化主题导出器。");
        return false;
    }

    bool succeeded = true;
    for (const Entry &entry : entries)
    {
        if (!mz_zip_writer_add_mem(&archive, entry.name.constData(), entry.contents.constData(),
                                   static_cast<size_t>(entry.contents.size()), MZ_BEST_COMPRESSION))
        {
            succeeded = false;
            break;
        }
    }

    void *archiveData = nullptr;
    size_t archiveSize = 0;
    succeeded =
        succeeded && mz_zip_writer_finalize_heap_archive(&archive, &archiveData, &archiveSize);
    QByteArray packageContents;
    if (succeeded)
    {
        packageContents =
            QByteArray(static_cast<const char *>(archiveData), static_cast<qsizetype>(archiveSize));
    }
    mz_free(archiveData);
    mz_zip_writer_end(&archive);
    if (!succeeded)
    {
        errorMessage = QStringLiteral("生成主题压缩包失败。");
        return false;
    }

    QSaveFile target(targetPath);
    if (!target.open(QIODevice::WriteOnly) ||
        target.write(packageContents) != packageContents.size() || !target.commit())
    {
        errorMessage = QStringLiteral("无法保存导出的主题包。");
        return false;
    }
    return true;
}

} // namespace stukit::app
