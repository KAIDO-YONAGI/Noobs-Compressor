#include "MainWindow.h"
#include "../CompressorFileSystem/DataCommunication/include/EncodingUtils.h"

#include <filesystem>
#include <QCoreApplication>

using Y_flib::EncodingUtils;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_isProcessing(false)
{
    m_backgroundPixmap.load(":/images/background.jpg");
    setupUI();
}

MainWindow::~MainWindow()
{
    if (m_worker) {
        m_worker->requestStop();
    }

    if (m_workerThread) {
        if (!m_workerThread->wait(3000)) {
            m_workerThread->terminate();
            m_workerThread->wait();
        }
        delete m_workerThread;
        m_workerThread = nullptr;
    }

    if (m_worker) {
        delete m_worker;
        m_worker = nullptr;
    }
}

void MainWindow::setupUI()
{
    setWindowTitle(tr("Compressor By Yonagi"));
    setMinimumSize(600, 400);
    resize(720, 450);
    setWindowIcon(QIcon(":/YONAGII_512x512.ico"));
    setAcceptDrops(true);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    updateBackground();

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane { "
        "   border-left: 1px solid rgba(180, 180, 180, 120); "
        "   border-right: 1px solid rgba(180, 180, 180, 120); "
        "   border-bottom: 1px solid rgba(180, 180, 180, 120); "
        "   border-top: 0px; "
        "   background: rgba(255, 255, 255, 160); "
        "   border-radius: 8px; "
        "   border-top-left-radius: 0px; "
        "} "
        "QTabBar::tab { "
        "   background: rgba(118, 176, 232, 79); "
        "   padding: 10px 25px; "
        "   margin-right: 2px; "
        "   border-top-left-radius: 6px; "
        "   border-top-right-radius: 6px; "
        "   font-weight: bold; "
        "   color: rgba(248, 251, 255, 235); "
        "} "
        "QTabBar::tab:selected { "
        "   background: rgba(83, 149, 217, 100); "
        "   color: rgba(255, 255, 255, 245); "
        "} "
        "QTabBar::tab:hover { "
        "   background: rgba(98, 164, 230, 89); "
        "}"
    );
    m_tabWidget->addTab(createCompressionTab(), tr("Compress"));
    m_tabWidget->addTab(createDecompressionTab(), tr("Decompress"));

    mainLayout->addWidget(m_tabWidget);
}

bool MainWindow::pathExists(const QString &path)
{
    try {
        return std::filesystem::exists(EncodingUtils::qStringToPath(path));
    } catch (...) {
        return false;
    }
}

QString MainWindow::makeValidPath(const QString &input)
{
    QString result = input.trimmed();
    if (result.startsWith('"') && result.endsWith('"')) {
        result = result.mid(1, result.length() - 2);
    }
    if (result.startsWith('\'') && result.endsWith('\'')) {
        result = result.mid(1, result.length() - 2);
    }
    return QDir::cleanPath(result);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> &urls = event->mimeData()->urls();
    const int currentTab = m_tabWidget->currentIndex();

    if (currentTab == 0) {
        addDroppedPaths(urls);
        return;
    }

    if (currentTab == 1 && !urls.isEmpty()) {
        QString path = urls.first().toLocalFile();
        if (path.isEmpty()) {
            path = urls.first().toString();
        }

        const QString cleanPath = makeValidPath(path);
        if (!cleanPath.isEmpty() && cleanPath.toLower().endsWith(".sy")) {
            m_decompressFilePathEdit->setText(cleanPath);
            QFileInfo fileInfo(cleanPath);
            m_decompressOutputDirEdit->setText(fileInfo.absolutePath());
        }
    }
}

void MainWindow::addDroppedPaths(const QList<QUrl> &urls)
{
    for (const QUrl &url : urls) {
        QString path = url.toLocalFile();
        if (path.isEmpty()) {
            path = url.toString();
        }

        const QString cleanPath = makeValidPath(path);
        if (cleanPath.isEmpty()) {
            continue;
        }

        if (m_fileListWidget->findItems(cleanPath, Qt::MatchExactly).isEmpty()) {
            m_fileListWidget->addItem(cleanPath);
        }
    }

    updateOutputDirectory();
    updateOutputFileName();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateBackground();
}

void MainWindow::updateBackground()
{
    if (m_backgroundPixmap.isNull()) {
        return;
    }

    const QSize windowSize = size();
    const QSize imageSize = m_backgroundPixmap.size();
    const double imageRatio = static_cast<double>(imageSize.width()) / imageSize.height();
    const double windowRatio = static_cast<double>(windowSize.width()) / windowSize.height();

    QSize scaledSize;
    if (windowRatio > imageRatio) {
        scaledSize.setWidth(windowSize.width());
        scaledSize.setHeight(static_cast<int>(windowSize.width() / imageRatio));
    } else {
        scaledSize.setHeight(windowSize.height());
        scaledSize.setWidth(static_cast<int>(windowSize.height() * imageRatio));
    }

    const QPixmap scaledPixmap = m_backgroundPixmap.scaled(
        scaledSize,
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation
    );

    const int x = (scaledPixmap.width() - windowSize.width()) / 2;
    const int y = (scaledPixmap.height() - windowSize.height()) / 2;
    const QPixmap croppedPixmap = scaledPixmap.copy(
        qMax(0, x),
        qMax(0, y),
        windowSize.width(),
        windowSize.height()
    );

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(croppedPixmap));
    setPalette(palette);
}

QString MainWindow::elideText(const QString &text, int maxWidth)
{
    QFontMetrics fm(m_compressCurrentFileLabel->font());
    return fm.elidedText(text, Qt::ElideMiddle, maxWidth);
}

QString MainWindow::localizeWorkerDialogMessage(const QString &message) const
{
    const auto translate = [](const char *sourceText) {
        return QCoreApplication::translate("CompressionWorker", sourceText);
    };

    const QString fileNotFoundPrefix = QStringLiteral("File not found: ");
    if (message.startsWith(fileNotFoundPrefix)) {
        return translate("File not found: %1").arg(message.mid(fileNotFoundPrefix.size()));
    }

    const QString invalidPathPrefix = QStringLiteral("Invalid path: ");
    if (message.startsWith(invalidPathPrefix)) {
        return translate("Invalid path: %1").arg(message.mid(invalidPathPrefix.size()));
    }

    const QString outputDirNotFoundPrefix = QStringLiteral("Output directory not found: ");
    if (message.startsWith(outputDirNotFoundPrefix)) {
        return translate("Output directory not found: %1").arg(message.mid(outputDirNotFoundPrefix.size()));
    }

    const QString invalidOutputDirPrefix = QStringLiteral("Invalid output directory: ");
    if (message.startsWith(invalidOutputDirPrefix)) {
        return translate("Invalid output directory: %1").arg(message.mid(invalidOutputDirPrefix.size()));
    }

    const QString archiveNotFoundPrefix = QStringLiteral("Archive file not found: ");
    if (message.startsWith(archiveNotFoundPrefix)) {
        return translate("Archive file not found: %1").arg(message.mid(archiveNotFoundPrefix.size()));
    }

    const QString invalidArchivePrefix = QStringLiteral("Invalid archive path: ");
    if (message.startsWith(invalidArchivePrefix)) {
        return translate("Invalid archive path: %1").arg(message.mid(invalidArchivePrefix.size()));
    }

    if (message == QStringLiteral("Only .sy files can be decompressed")) {
        return translate("Only .sy files can be decompressed");
    }

    const QString compressionSuccessPrefix = QStringLiteral("Compression successful!\nOutput file: ");
    if (message.startsWith(compressionSuccessPrefix)) {
        return translate("Compression successful!\nOutput file: %1")
            .arg(message.mid(compressionSuccessPrefix.size()));
    }

    const QString compressionFailedPrefix = QStringLiteral("Compression failed: ");
    if (message.startsWith(compressionFailedPrefix)) {
        return translate("Compression failed: %1").arg(message.mid(compressionFailedPrefix.size()));
    }

    if (message == QStringLiteral("Compression failed due to unknown error")) {
        return translate("Compression failed due to unknown error");
    }

    if (message == QStringLiteral("This archive requires a password. Please enter the decryption key.")) {
        return translate("This archive requires a password. Please enter the decryption key.");
    }

    const QString decompressionSuccessPrefix = QStringLiteral("Decompression successful!\nOutput directory: ");
    if (message.startsWith(decompressionSuccessPrefix)) {
        return translate("Decompression successful!\nOutput directory: %1")
            .arg(message.mid(decompressionSuccessPrefix.size()));
    }

    const QString decompressionFailedPrefix = QStringLiteral("Decompression failed: ");
    const QString decompressionFailedSuffix = QStringLiteral("\n\nPossible reasons:\n"
                                                             "1. Incorrect decryption key\n"
                                                             "2. Corrupted or incompatible .sy file\n"
                                                             "3. Insufficient disk space");
    if (message.startsWith(decompressionFailedPrefix) && message.endsWith(decompressionFailedSuffix)) {
        const QString reason = message.mid(
            decompressionFailedPrefix.size(),
            message.size() - decompressionFailedPrefix.size() - decompressionFailedSuffix.size());
        return translate("Decompression failed: %1\n\nPossible reasons:\n"
                         "1. Incorrect decryption key\n"
                         "2. Corrupted or incompatible .sy file\n"
                         "3. Insufficient disk space")
            .arg(reason);
    }

    if (message == QStringLiteral("Decompression failed due to unknown error")) {
        return translate("Decompression failed due to unknown error");
    }

    return message;
}
