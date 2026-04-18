#include <QApplication>
#include <QDir>
#include <QFont>
#include <QLocale>
#include <QTranslator>

#include "MainWindow.h"

namespace
{
QString normalizeLocaleName(QString localeName)
{
    localeName = localeName.trimmed();
    localeName.replace('-', '_');
    return localeName;
}

bool isChineseLocale(const QString &localeName)
{
    return normalizeLocaleName(localeName).startsWith(QStringLiteral("zh"), Qt::CaseInsensitive);
}

void appendChineseFallback(QStringList &candidates)
{
    const QString simplifiedChinese = QStringLiteral("zh_CN");
    if (!candidates.contains(simplifiedChinese, Qt::CaseInsensitive))
    {
        candidates.append(simplifiedChinese);
    }

    const QString chineseLanguage = QStringLiteral("zh");
    if (!candidates.contains(chineseLanguage, Qt::CaseInsensitive))
    {
        candidates.append(chineseLanguage);
    }
}

void appendLocaleCandidate(QStringList &candidates, const QString &localeName)
{
    const QString normalized = normalizeLocaleName(localeName);
    if (normalized.isEmpty())
    {
        return;
    }

    if (!candidates.contains(normalized, Qt::CaseInsensitive))
    {
        candidates.append(normalized);
    }

    const int separatorIndex = normalized.indexOf('_');
    if (separatorIndex > 0)
    {
        const QString languageOnly = normalized.left(separatorIndex);
        if (!candidates.contains(languageOnly, Qt::CaseInsensitive))
        {
            candidates.append(languageOnly);
        }
    }
}

QStringList preferredLocales()
{
    QStringList locales;

    const QString languageOverride = normalizeLocaleName(qEnvironmentVariable("SFC_GUI_LANGUAGE"));
    if (!languageOverride.isEmpty() && languageOverride.compare("system", Qt::CaseInsensitive) != 0)
    {
        if (!isChineseLocale(languageOverride))
        {
            return locales;
        }

        appendLocaleCandidate(locales, languageOverride);

        const QLocale overrideLocale(languageOverride);
        for (const QString &uiLanguage : overrideLocale.uiLanguages())
        {
            appendLocaleCandidate(locales, uiLanguage);
        }
        appendLocaleCandidate(locales, overrideLocale.name());
        appendChineseFallback(locales);
    }
    else
    {
        const QLocale systemLocale = QLocale::system();
        bool hasChinesePreference = false;

        for (const QString &uiLanguage : systemLocale.uiLanguages())
        {
            hasChinesePreference = hasChinesePreference || isChineseLocale(uiLanguage);
            appendLocaleCandidate(locales, uiLanguage);
        }
        hasChinesePreference = hasChinesePreference || isChineseLocale(systemLocale.name());

        if (!hasChinesePreference)
        {
            return {};
        }

        appendLocaleCandidate(locales, systemLocale.name());
        appendChineseFallback(locales);
    }

    return locales;
}

bool loadApplicationTranslator(QApplication &app, QTranslator &translator)
{
    const QString translationsDir = QDir(QCoreApplication::applicationDirPath()).filePath("translations");
    const QString translationBaseName = QStringLiteral("SimpleFilesCompressorGUI");

    for (const QString &localeName : preferredLocales())
    {
        const QString fileBaseName = translationBaseName + "_" + localeName;
        if (translator.load(fileBaseName, translationsDir) ||
            translator.load(QStringLiteral(":/i18n/%1.qm").arg(fileBaseName)))
        {
            app.installTranslator(&translator);
            return true;
        }
    }

    return false;
}
} // namespace

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    QTranslator appTranslator;

    app.setApplicationName("Simple Files Compressor");
    app.setApplicationVersion("2.1.0");
    app.setOrganizationName("YONAGI");

    loadApplicationTranslator(app, appTranslator);

    QFont font = app.font();
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setHintingPreference(QFont::PreferFullHinting);
    app.setFont(font);

    MainWindow window;
    window.show();

    return app.exec();
}
