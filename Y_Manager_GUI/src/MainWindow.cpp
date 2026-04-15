#include "MainWindow.h"
#include "CompressionWorker.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>
#include <QPalette>
#include <QBrush>
#include <QFont>

#ifdef _WIN32
#include <windows.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_isProcessing(false)
{
    setupUI();
}

MainWindow::~MainWindow()
{
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_workerThread;
    }
}

void MainWindow::setupUI()
{
    setWindowTitle(tr("Secure Files Compressor"));
    setMinimumSize(700, 500);
    resize(800, 600);

    // 设置窗口图标
    setWindowIcon(QIcon(":/YONAGII_512x512.ico"));

    // 设置背景图片
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 加载背景图片
    QPixmap backgroundPixmap(":/images/background.png");
    if (!backgroundPixmap.isNull()) {
        QPalette palette;
        palette.setBrush(QPalette::Window, QBrush(backgroundPixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)));
        setPalette(palette);
    }

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 创建选项卡
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->setStyleSheet(
        "QTabWidget::pane { "
        "   border: 1px solid rgba(180, 180, 180, 120); "
        "   background: rgba(255, 255, 255, 220); "
        "   border-radius: 8px; "
        "} "
        "QTabBar::tab { "
        "   background: rgba(240, 240, 240, 200); "
        "   padding: 10px 25px; "
        "   margin-right: 2px; "
        "   border-top-left-radius: 6px; "
        "   border-top-right-radius: 6px; "
        "   font-weight: bold; "
        "   font-size: 13px; "
        "   color: #333; "
        "} "
        "QTabBar::tab:selected { "
        "   background: rgba(255, 255, 255, 240); "
        "   color: #000; "
        "} "
        "QTabBar::tab:hover { "
        "   background: rgba(255, 255, 255, 220); "
        "}"
    );
    tabWidget->addTab(createCompressionTab(), tr("Compress"));
    tabWidget->addTab(createDecompressionTab(), tr("Decompress"));

    mainLayout->addWidget(tabWidget);
}

QWidget* MainWindow::createCompressionTab()
{
    QWidget *tab = new QWidget();
    tab->setStyleSheet("background: transparent;");
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setSpacing(10);

    // 统一的按钮样式 - 白色/灰色风格
    QString btnStyle =
        "QPushButton { "
        "   background: rgba(255, 255, 255, 230); "
        "   border: 1px solid rgba(180, 180, 180, 180); "
        "   border-radius: 5px; "
        "   padding: 8px 18px; "
        "   font-weight: bold; "
        "   font-size: 12px; "
        "   color: #333; "
        "} "
        "QPushButton:hover { "
        "   background: rgba(245, 245, 245, 250); "
        "   border: 1px solid rgba(150, 150, 150, 200); "
        "} "
        "QPushButton:pressed { "
        "   background: rgba(220, 220, 220, 230); "
        "}"
        "QPushButton:disabled { "
        "   background: rgba(200, 200, 200, 180); "
        "   color: #888; "
        "}";

    // 统一的GroupBox样式
    QString groupBoxStyle =
        "QGroupBox { "
        "   background: rgba(255, 255, 255, 200); "
        "   border: 1px solid rgba(200, 200, 200, 150); "
        "   border-radius: 8px; "
        "   margin-top: 12px; "
        "   padding-top: 12px; "
        "   font-weight: bold; "
        "   font-size: 13px; "
        "   color: #333; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   left: 12px; "
        "   padding: 0 6px; "
        "} "
        "QLabel { background: transparent; font-size: 12px; color: #333; } "
        "QLineEdit { "
        "   background: rgba(255, 255, 255, 240); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 4px; "
        "   padding: 6px; "
        "   font-size: 12px; "
        "}";

    // === 文件列表区域 ===
    QGroupBox *fileGroup = new QGroupBox(tr("Files to Compress"));
    fileGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout *fileLayout = new QVBoxLayout(fileGroup);

    m_fileListWidget = new QListWidget();
    m_fileListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileListWidget->setStyleSheet(
        "QListWidget { "
        "   background: rgba(255, 255, 255, 240); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 5px; "
        "   font-size: 12px; "
        "} "
        "QListWidget::item { padding: 6px; } "
        "QListWidget::item:selected { background: rgba(200, 200, 200, 150); }"
    );
    fileLayout->addWidget(m_fileListWidget);

    // 文件操作按钮
    QHBoxLayout *fileBtnLayout = new QHBoxLayout();
    QPushButton *addFileBtn = new QPushButton(tr("Add Files"));
    QPushButton *addFolderBtn = new QPushButton(tr("Add Folder"));
    QPushButton *removeBtn = new QPushButton(tr("Remove"));
    QPushButton *clearBtn = new QPushButton(tr("Clear"));

    addFileBtn->setStyleSheet(btnStyle);
    addFolderBtn->setStyleSheet(btnStyle);
    removeBtn->setStyleSheet(btnStyle);
    clearBtn->setStyleSheet(btnStyle);

    connect(addFileBtn, &QPushButton::clicked, this, &MainWindow::onAddFilesClicked);
    connect(addFolderBtn, &QPushButton::clicked, this, &MainWindow::onAddFolderClicked);
    connect(removeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveFileClicked);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearFilesClicked);

    fileBtnLayout->addWidget(addFileBtn);
    fileBtnLayout->addWidget(addFolderBtn);
    fileBtnLayout->addWidget(removeBtn);
    fileBtnLayout->addWidget(clearBtn);
    fileBtnLayout->addStretch();
    fileLayout->addLayout(fileBtnLayout);

    layout->addWidget(fileGroup);

    // === 输出设置区域 ===
    QGroupBox *outputGroup = new QGroupBox(tr("Output Settings"));
    outputGroup->setStyleSheet(groupBoxStyle);
    QGridLayout *outputLayout = new QGridLayout(outputGroup);

    // 输出目录
    outputLayout->addWidget(new QLabel(tr("Output Directory:")), 0, 0);
    m_outputDirEdit = new QLineEdit(getExeDirectory());
    outputLayout->addWidget(m_outputDirEdit, 0, 1);
    QPushButton *browseOutDirBtn = new QPushButton(tr("Browse"));
    browseOutDirBtn->setStyleSheet(btnStyle);
    connect(browseOutDirBtn, &QPushButton::clicked, this, &MainWindow::onBrowseOutputDirClicked);
    outputLayout->addWidget(browseOutDirBtn, 0, 2);

    // 输出文件名
    outputLayout->addWidget(new QLabel(tr("Output Filename:")), 1, 0);
    m_outputFileNameEdit = new QLineEdit("SHINKU_YONAGI");
    outputLayout->addWidget(m_outputFileNameEdit, 1, 1, 1, 2);

    // 密码
    outputLayout->addWidget(new QLabel(tr("Encryption Key:")), 2, 0);
    m_passwordEdit = new QLineEdit("LOVEYONAGI");
    m_passwordEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    outputLayout->addWidget(m_passwordEdit, 2, 1, 1, 2);

    layout->addWidget(outputGroup);

    // === 进度和日志区域 ===
    QGroupBox *progressGroup = new QGroupBox(tr("Progress"));
    progressGroup->setStyleSheet(
        "QGroupBox { "
        "   background: rgba(255, 255, 255, 200); "
        "   border: 1px solid rgba(200, 200, 200, 150); "
        "   border-radius: 8px; "
        "   margin-top: 12px; "
        "   padding-top: 12px; "
        "   font-weight: bold; "
        "   font-size: 13px; "
        "   color: #333; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   left: 12px; "
        "   padding: 0 6px; "
        "}"
    );
    QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);

    m_compressProgressBar = new QProgressBar();
    m_compressProgressBar->setValue(0);
    m_compressProgressBar->setTextVisible(true);
    m_compressProgressBar->setStyleSheet(
        "QProgressBar { "
        "   background: rgba(255, 255, 255, 240); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 5px; "
        "   text-align: center; "
        "   font-size: 12px; "
        "   height: 22px; "
        "} "
        "QProgressBar::chunk { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #a0a0a0, stop:1 #d0d0d0); "
        "   border-radius: 4px; "
        "}"
    );
    progressLayout->addWidget(m_compressProgressBar);

    m_compressLogEdit = new QTextEdit();
    m_compressLogEdit->setReadOnly(true);
    m_compressLogEdit->setMaximumHeight(100);
    m_compressLogEdit->setStyleSheet(
        "QTextEdit { "
        "   background: rgba(255, 255, 255, 240); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 5px; "
        "   font-size: 12px; "
        "}"
    );
    progressLayout->addWidget(m_compressLogEdit);

    layout->addWidget(progressGroup);

    // === 开始按钮 ===
    m_startCompressBtn = new QPushButton(tr("Start Compression"));
    m_startCompressBtn->setMinimumHeight(45);
    m_startCompressBtn->setStyleSheet(
        "QPushButton { "
        "   background: rgba(255, 255, 255, 240); "
        "   color: #333; "
        "   border: 2px solid rgba(150, 150, 150, 200); "
        "   border-radius: 8px; "
        "   font-size: 14px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { "
        "   background: rgba(245, 245, 245, 250); "
        "   border: 2px solid rgba(120, 120, 120, 220); "
        "} "
        "QPushButton:pressed { "
        "   background: rgba(230, 230, 230, 240); "
        "} "
        "QPushButton:disabled { "
        "   background: rgba(220, 220, 220, 200); "
        "   color: #888; "
        "}"
    );
    connect(m_startCompressBtn, &QPushButton::clicked, this, &MainWindow::onStartCompressionClicked);
    layout->addWidget(m_startCompressBtn);

    layout->addStretch();
    return tab;
}

QWidget* MainWindow::createDecompressionTab()
{
    QWidget *tab = new QWidget();
    tab->setStyleSheet("background: transparent;");
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setSpacing(10);

    QString groupBoxStyle =
        "QGroupBox { "
        "   background: rgba(255, 255, 255, 200); "
        "   border: 1px solid rgba(200, 200, 200, 150); "
        "   border-radius: 8px; "
        "   margin-top: 12px; "
        "   padding-top: 12px; "
        "   font-weight: bold; "
        "   font-size: 13px; "
        "   color: #333; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   left: 12px; "
        "   padding: 0 6px; "
        "} "
        "QLabel { background: transparent; font-size: 12px; color: #333; } "
        "QLineEdit { "
        "   background: rgba(255, 255, 255, 240); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 4px; "
        "   padding: 6px; "
        "   font-size: 12px; "
        "}";

    QString btnStyle =
        "QPushButton { "
        "   background: rgba(255, 255, 255, 230); "
        "   border: 1px solid rgba(180, 180, 180, 180); "
        "   border-radius: 5px; "
        "   padding: 8px 18px; "
        "   font-weight: bold; "
        "   font-size: 12px; "
        "   color: #333; "
        "} "
        "QPushButton:hover { "
        "   background: rgba(245, 245, 245, 250); "
        "   border: 1px solid rgba(150, 150, 150, 200); "
        "} "
        "QPushButton:pressed { "
        "   background: rgba(220, 220, 220, 230); "
        "}"
        "QPushButton:disabled { "
        "   background: rgba(200, 200, 200, 180); "
        "   color: #888; "
        "}";

    // === 输入文件区域 ===
    QGroupBox *inputGroup = new QGroupBox(tr("Select Archive"));
    inputGroup->setStyleSheet(groupBoxStyle);
    QGridLayout *inputLayout = new QGridLayout(inputGroup);

    inputLayout->addWidget(new QLabel(tr("Archive File (.sy):")), 0, 0);
    m_decompressFilePathEdit = new QLineEdit();
    inputLayout->addWidget(m_decompressFilePathEdit, 0, 1);
    QPushButton *browseFileBtn = new QPushButton(tr("Browse"));
    browseFileBtn->setStyleSheet(btnStyle);
    connect(browseFileBtn, &QPushButton::clicked, this, &MainWindow::onBrowseDecompressFileClicked);
    inputLayout->addWidget(browseFileBtn, 0, 2);

    layout->addWidget(inputGroup);

    // === 输出设置区域 ===
    QGroupBox *outputGroup = new QGroupBox(tr("Output Settings"));
    outputGroup->setStyleSheet(groupBoxStyle);
    QGridLayout *outputLayout = new QGridLayout(outputGroup);

    outputLayout->addWidget(new QLabel(tr("Output Directory:")), 0, 0);
    m_decompressOutputDirEdit = new QLineEdit();
    outputLayout->addWidget(m_decompressOutputDirEdit, 0, 1);
    QPushButton *browseOutBtn = new QPushButton(tr("Browse"));
    browseOutBtn->setStyleSheet(btnStyle);
    connect(browseOutBtn, &QPushButton::clicked, this, &MainWindow::onBrowseDecompressOutputClicked);
    outputLayout->addWidget(browseOutBtn, 0, 2);

    outputLayout->addWidget(new QLabel(tr("Decryption Key:")), 1, 0);
    m_decompressPasswordEdit = new QLineEdit();
    m_decompressPasswordEdit->setEchoMode(QLineEdit::Password);
    outputLayout->addWidget(m_decompressPasswordEdit, 1, 1, 1, 2);

    layout->addWidget(outputGroup);

    // === 进度和日志区域 ===
    QGroupBox *progressGroup = new QGroupBox(tr("Progress"));
    progressGroup->setStyleSheet(
        "QGroupBox { "
        "   background: rgba(255, 255, 255, 200); "
        "   border: 1px solid rgba(200, 200, 200, 150); "
        "   border-radius: 8px; "
        "   margin-top: 12px; "
        "   padding-top: 12px; "
        "   font-weight: bold; "
        "   font-size: 13px; "
        "   color: #333; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   left: 12px; "
        "   padding: 0 6px; "
        "}"
    );
    QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);

    m_decompressProgressBar = new QProgressBar();
    m_decompressProgressBar->setValue(0);
    m_decompressProgressBar->setTextVisible(true);
    m_decompressProgressBar->setStyleSheet(
        "QProgressBar { "
        "   background: rgba(255, 255, 255, 240); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 5px; "
        "   text-align: center; "
        "   font-size: 12px; "
        "   height: 22px; "
        "} "
        "QProgressBar::chunk { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #b0b0b0, stop:1 #e0e0e0); "
        "   border-radius: 4px; "
        "}"
    );
    progressLayout->addWidget(m_decompressProgressBar);

    m_decompressLogEdit = new QTextEdit();
    m_decompressLogEdit->setReadOnly(true);
    m_decompressLogEdit->setMaximumHeight(100);
    m_decompressLogEdit->setStyleSheet(
        "QTextEdit { "
        "   background: rgba(255, 255, 255, 240); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 5px; "
        "   font-size: 12px; "
        "}"
    );
    progressLayout->addWidget(m_decompressLogEdit);

    layout->addWidget(progressGroup);

    // === 开始按钮 ===
    m_startDecompressBtn = new QPushButton(tr("Start Decompression"));
    m_startDecompressBtn->setMinimumHeight(45);
    m_startDecompressBtn->setStyleSheet(
        "QPushButton { "
        "   background: rgba(255, 255, 255, 240); "
        "   color: #333; "
        "   border: 2px solid rgba(150, 150, 150, 200); "
        "   border-radius: 8px; "
        "   font-size: 14px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { "
        "   background: rgba(245, 245, 245, 250); "
        "   border: 2px solid rgba(120, 120, 120, 220); "
        "} "
        "QPushButton:pressed { "
        "   background: rgba(230, 230, 230, 240); "
        "} "
        "QPushButton:disabled { "
        "   background: rgba(220, 220, 220, 200); "
        "   color: #888; "
        "}"
    );
    connect(m_startDecompressBtn, &QPushButton::clicked, this, &MainWindow::onStartDecompressionClicked);
    layout->addWidget(m_startDecompressBtn);

    layout->addStretch();
    return tab;
}

// === 压缩模式槽函数 ===

void MainWindow::onAddFilesClicked()
{
    QStringList files = QFileDialog::getOpenFileNames(this,
        tr("Select Files"), QString(), tr("All Files (*)"));

    for (const QString &file : files) {
        QString cleanPath = makeValidPath(file);
        if (!cleanPath.isEmpty() && m_fileListWidget->findItems(cleanPath, Qt::MatchExactly).isEmpty()) {
            m_fileListWidget->addItem(cleanPath);
        }
    }
}

void MainWindow::onAddFolderClicked()
{
    QString folder = QFileDialog::getExistingDirectory(this,
        tr("Select Folder"), QString(), QFileDialog::ShowDirsOnly);

    if (!folder.isEmpty()) {
        QString cleanPath = makeValidPath(folder);
        if (!cleanPath.isEmpty() && m_fileListWidget->findItems(cleanPath, Qt::MatchExactly).isEmpty()) {
            m_fileListWidget->addItem(cleanPath);
        }
    }
}

void MainWindow::onRemoveFileClicked()
{
    QList<QListWidgetItem*> items = m_fileListWidget->selectedItems();
    for (QListWidgetItem *item : items) {
        delete m_fileListWidget->takeItem(m_fileListWidget->row(item));
    }
}

void MainWindow::onClearFilesClicked()
{
    m_fileListWidget->clear();
}

void MainWindow::onBrowseOutputDirClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Select Output Directory"), m_outputDirEdit->text(), QFileDialog::ShowDirsOnly);

    if (!dir.isEmpty()) {
        m_outputDirEdit->setText(makeValidPath(dir));
    }
}

void MainWindow::onStartCompressionClicked()
{
    if (m_isProcessing) {
        return;
    }

    // 验证输入
    if (m_fileListWidget->count() == 0) {
        QMessageBox::warning(this, tr("Error"), tr("Please add files to compress."));
        return;
    }

    QString outputDir = m_outputDirEdit->text().trimmed();
    if (outputDir.isEmpty() || !pathExists(outputDir)) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a valid output directory."));
        return;
    }

    QString fileName = m_outputFileNameEdit->text().trimmed();
    if (fileName.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter an output filename."));
        return;
    }

    QString password = m_passwordEdit->text();
    if (password.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter an encryption key."));
        return;
    }

    // 收集文件列表
    QStringList files;
    for (int i = 0; i < m_fileListWidget->count(); ++i) {
        files.append(m_fileListWidget->item(i)->text());
    }

    // 设置UI状态
    m_isProcessing = true;
    m_startCompressBtn->setEnabled(false);
    m_startCompressBtn->setText(tr("Processing..."));
    m_compressProgressBar->setValue(0);
    m_compressLogEdit->clear();
    m_compressLogEdit->append(tr("Starting compression..."));

    // 创建工作线程
    m_workerThread = new QThread();
    m_worker = new CompressionWorker();
    m_worker->setCompressionParams(files, outputDir, fileName, password);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &CompressionWorker::doCompression);
    connect(m_worker, &CompressionWorker::detailedProgress, this, &MainWindow::onCompressionDetailedProgress);
    connect(m_worker, &CompressionWorker::finished, this, &MainWindow::onCompressionFinished);

    m_workerThread->start();
}

// === 解压模式槽函数 ===

void MainWindow::onBrowseDecompressFileClicked()
{
    QString file = QFileDialog::getOpenFileName(this,
        tr("Select Archive"), QString(), tr("Secure Archives (*.sy);;All Files (*)"));

    if (!file.isEmpty()) {
        m_decompressFilePathEdit->setText(makeValidPath(file));

        // 自动设置输出目录为文件所在目录
        QFileInfo fileInfo(file);
        m_decompressOutputDirEdit->setText(fileInfo.absolutePath());
    }
}

void MainWindow::onBrowseDecompressOutputClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Select Output Directory"), m_decompressOutputDirEdit->text(), QFileDialog::ShowDirsOnly);

    if (!dir.isEmpty()) {
        m_decompressOutputDirEdit->setText(makeValidPath(dir));
    }
}

void MainWindow::onStartDecompressionClicked()
{
    if (m_isProcessing) {
        return;
    }

    // 验证输入
    QString inputPath = m_decompressFilePathEdit->text().trimmed();
    if (inputPath.isEmpty() || !pathExists(inputPath)) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a valid archive file."));
        return;
    }

    if (!inputPath.toLower().endsWith(".sy")) {
        QMessageBox::warning(this, tr("Error"), tr("Only .sy files can be decompressed."));
        return;
    }

    QString password = m_decompressPasswordEdit->text();
    if (password.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter the decryption key."));
        return;
    }

    QString outputDir = m_decompressOutputDirEdit->text().trimmed();

    // 设置UI状态
    m_isProcessing = true;
    m_startDecompressBtn->setEnabled(false);
    m_startDecompressBtn->setText(tr("Processing..."));
    m_decompressProgressBar->setValue(0);
    m_decompressLogEdit->clear();
    m_decompressLogEdit->append(tr("Starting decompression..."));

    // 创建工作线程
    m_workerThread = new QThread();
    m_worker = new CompressionWorker();
    m_worker->setDecompressionParams(inputPath, outputDir, password);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &CompressionWorker::doDecompression);
    connect(m_worker, &CompressionWorker::detailedProgress, this, &MainWindow::onDecompressionDetailedProgress);
    connect(m_worker, &CompressionWorker::finished, this, &MainWindow::onDecompressionFinished);

    m_workerThread->start();
}

// === 进度和状态槽函数 ===

void MainWindow::onCompressionDetailedProgress(const QString &filename, double fileProgress, double overallProgress, const QString &status)
{
    m_compressProgressBar->setValue(static_cast<int>(overallProgress));

    QString logMessage;
    if (!filename.isEmpty()) {
        logMessage = tr("[%1%] %2 - %3 (%4%)")
            .arg(static_cast<int>(overallProgress), 3)
            .arg(status)
            .arg(filename)
            .arg(static_cast<int>(fileProgress), 3);
    } else {
        logMessage = tr("[%1%] %2")
            .arg(static_cast<int>(overallProgress), 3)
            .arg(status);
    }
    m_compressLogEdit->append(logMessage);
}

void MainWindow::onCompressionFinished(bool success, const QString &message)
{
    m_isProcessing = false;
    m_startCompressBtn->setEnabled(true);
    m_startCompressBtn->setText(tr("Start Compression"));

    if (success) {
        m_compressProgressBar->setValue(100);
        m_compressLogEdit->append(tr("Compression completed successfully!"));
        QMessageBox::information(this, tr("Success"), message);
    } else {
        m_compressLogEdit->append(tr("Compression failed: %1").arg(message));
        QMessageBox::critical(this, tr("Error"), message);
    }

    // 清理工作线程
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_workerThread;
        m_workerThread = nullptr;
    }
    if (m_worker) {
        delete m_worker;
        m_worker = nullptr;
    }
}

void MainWindow::onDecompressionDetailedProgress(const QString &filename, double fileProgress, double overallProgress, const QString &status)
{
    m_decompressProgressBar->setValue(static_cast<int>(overallProgress));

    QString logMessage;
    if (!filename.isEmpty()) {
        logMessage = tr("[%1%] %2 - %3 (%4%)")
            .arg(static_cast<int>(overallProgress), 3)
            .arg(status)
            .arg(filename)
            .arg(static_cast<int>(fileProgress), 3);
    } else {
        logMessage = tr("[%1%] %2")
            .arg(static_cast<int>(overallProgress), 3)
            .arg(status);
    }
    m_decompressLogEdit->append(logMessage);
}

void MainWindow::onDecompressionFinished(bool success, const QString &message)
{
    m_isProcessing = false;
    m_startDecompressBtn->setEnabled(true);
    m_startDecompressBtn->setText(tr("Start Decompression"));

    if (success) {
        m_decompressProgressBar->setValue(100);
        m_decompressLogEdit->append(tr("Decompression completed successfully!"));
        QMessageBox::information(this, tr("Success"), message);
    } else {
        m_decompressLogEdit->append(tr("Decompression failed: %1").arg(message));
        QMessageBox::critical(this, tr("Error"), message);
    }

    // 清理工作线程
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_workerThread;
        m_workerThread = nullptr;
    }
    if (m_worker) {
        delete m_worker;
        m_worker = nullptr;
    }
}

// === 辅助函数 ===

QString MainWindow::getExeDirectory()
{
#ifdef _WIN32
    wchar_t exePath[MAX_PATH];
    DWORD length = GetModuleFileNameW(NULL, exePath, MAX_PATH);
    if (length > 0 && length < MAX_PATH) {
        QString path = QString::fromWCharArray(exePath, length);
        QFileInfo fi(path);
        return fi.absolutePath();
    }
#endif
    return QDir::currentPath();
}

bool MainWindow::pathExists(const QString &path)
{
    return QFileInfo::exists(path);
}

QString MainWindow::makeValidPath(const QString &input)
{
    QString result = input.trimmed();
    // 移除可能的引号
    if (result.startsWith('"') && result.endsWith('"')) {
        result = result.mid(1, result.length() - 2);
    }
    if (result.startsWith('\'') && result.endsWith('\'')) {
        result = result.mid(1, result.length() - 2);
    }
    return QDir::cleanPath(result);
}
