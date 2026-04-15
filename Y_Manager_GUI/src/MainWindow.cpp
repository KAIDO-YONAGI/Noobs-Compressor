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
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QResizeEvent>

#ifdef _WIN32
#include <windows.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_workerThread(nullptr)
    , m_worker(nullptr)
    , m_isProcessing(false)
{
    // 加载背景图片（只加载一次）
    m_backgroundPixmap.load(":/images/background.png");

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
    setMinimumSize(700, 500);  // 最小尺寸
    resize(900, 650);  // 默认尺寸，确保内容完整显示

    // 设置窗口图标
    setWindowIcon(QIcon(":/YONAGII_512x512.ico"));

    // 启用拖放
    setAcceptDrops(true);

    // 设置背景图片 - 使用裁剪模式
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 设置初始背景
    updateBackground();

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 创建选项卡
    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->setStyleSheet(
        "QTabWidget::pane { "
        "   border: 1px solid rgba(180, 180, 180, 120); "
        "   background: rgba(255, 255, 255, 160); "
        "   border-radius: 8px; "
        "} "
        "QTabBar::tab { "
        "   background: rgba(240, 240, 240, 140); "
        "   padding: 10px 25px; "
        "   margin-right: 2px; "
        "   border-top-left-radius: 6px; "
        "   border-top-right-radius: 6px; "
        "   font-weight: bold; "
        "   color: #333; "
        "} "
        "QTabBar::tab:selected { "
        "   background: rgba(255, 255, 255, 180); "
        "   color: #000; "
        "} "
        "QTabBar::tab:hover { "
        "   background: rgba(255, 255, 255, 160); "
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

    // 主垂直布局
    QVBoxLayout *mainVLayout = new QVBoxLayout(tab);
    mainVLayout->setSpacing(0);
    mainVLayout->setContentsMargins(0, 0, 0, 0);

    // 统一的按钮样式 - 白色/灰色风格
    QString btnStyle =
        "QPushButton { "
        "   background: rgba(255, 255, 255, 160); "
        "   border: 1px solid rgba(180, 180, 180, 180); "
        "   border-radius: 5px; "
        "   padding: 8px 18px; "
        "   font-weight: bold; "
        "   color: #333; "
        "} "
        "QPushButton:hover { "
        "   background: rgba(255, 255, 255, 200); "
        "   border: 1px solid rgba(150, 150, 150, 200); "
        "} "
        "QPushButton:pressed { "
        "   background: rgba(220, 220, 220, 180); "
        "} "
        "QPushButton:disabled { "
        "   background: rgba(200, 200, 200, 140); "
        "   color: #888; "
        "}";

    // 统一的GroupBox样式
    QString groupBoxStyle =
        "QGroupBox { "
        "   background: rgba(255, 255, 255, 130); "
        "   border: 1px solid rgba(200, 200, 200, 150); "
        "   border-radius: 8px; "
        "   margin-top: 12px; "
        "   padding-top: 12px; "
        "   font-weight: bold; "
        "   color: #333; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   left: 12px; "
        "   padding: 0 6px; "
        "} "
        "QLabel { background: transparent; color: #333; } "
        "QLineEdit { "
        "   background: rgba(255, 255, 255, 180); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 4px; "
        "   padding: 6px; "
        "}";

    // =============================================
    // 内容区域容器 - 左右两列作为一个整体
    // =============================================
    QWidget *contentBox = new QWidget();
    contentBox->setStyleSheet("background: transparent;");
    QHBoxLayout *contentLayout = new QHBoxLayout(contentBox);
    contentLayout->setSpacing(10);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    // =============================================
    // 左列 - 输入信息填写区域
    // =============================================
    QWidget *leftColumn = new QWidget();
    leftColumn->setStyleSheet("background: transparent;");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setSpacing(10);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // === 文件列表区域 ===
    QGroupBox *fileGroup = new QGroupBox(tr("Files to Compress"));
    fileGroup->setStyleSheet(groupBoxStyle);
    QVBoxLayout *fileLayout = new QVBoxLayout(fileGroup);

    m_fileListWidget = new QListWidget();
    m_fileListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_fileListWidget->setStyleSheet(
        "QListWidget { "
        "   background: rgba(255, 255, 255, 160); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 5px; "
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
    fileLayout->addLayout(fileBtnLayout);

    leftLayout->addWidget(fileGroup);

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
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("Leave empty for default"));
    outputLayout->addWidget(m_passwordEdit, 2, 1, 1, 2);

    leftLayout->addWidget(outputGroup);
    leftLayout->addStretch();

    // =============================================
    // 右列 - 输出信息区域
    // =============================================
    QWidget *rightColumn = new QWidget();
    rightColumn->setStyleSheet("background: transparent;");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightColumn);
    rightLayout->setSpacing(10);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // === 进度和日志区域 ===
    QGroupBox *progressGroup = new QGroupBox(tr("Progress"));
    progressGroup->setStyleSheet(
        "QGroupBox { "
        "   background: rgba(255, 255, 255, 130); "
        "   border: 1px solid rgba(200, 200, 200, 150); "
        "   border-radius: 8px; "
        "   margin-top: 12px; "
        "   padding-top: 12px; "
        "   font-weight: bold; "
        "   color: #333; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   left: 12px; "
        "   padding: 0 6px; "
        "}"
    );
    QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);
    progressLayout->setSpacing(8);

    // 当前文件标签
    m_compressCurrentFileLabel = new QLabel(tr("Ready"));
    m_compressCurrentFileLabel->setStyleSheet(
        "QLabel { "
        "   background: transparent; "
        "   color: #333; "
        "   padding: 6px; "
        "}"
    );
    m_compressCurrentFileLabel->setWordWrap(true);
    progressLayout->addWidget(m_compressCurrentFileLabel);

    // 进度条
    m_compressProgressBar = new QProgressBar();
    m_compressProgressBar->setValue(0);
    m_compressProgressBar->setTextVisible(true);
    m_compressProgressBar->setMinimumHeight(28);
    m_compressProgressBar->setStyleSheet(
        "QProgressBar { "
        "   background: rgba(255, 255, 255, 160); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 5px; "
        "   text-align: center; "
        "   font-weight: bold; "
        "} "
        "QProgressBar::chunk { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #a0a0a0, stop:1 #d0d0d0); "
        "   border-radius: 4px; "
        "}"
    );
    progressLayout->addWidget(m_compressProgressBar);

    // 进度百分比标签
    m_compressProgressLabel = new QLabel(tr("Overall: 0% | Current: 0%"));
    m_compressProgressLabel->setStyleSheet(
        "QLabel { "
        "   background: transparent; "
        "   color: #555; "
        "}"
    );
    progressLayout->addWidget(m_compressProgressLabel);

    // 日志框 - 禁用自动换行
    m_compressLogEdit = new QTextEdit();
    m_compressLogEdit->setReadOnly(true);
    m_compressLogEdit->setLineWrapMode(QTextEdit::NoWrap);
    m_compressLogEdit->setStyleSheet(
        "QTextEdit { "
        "   background: rgba(255, 255, 255, 160); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 5px; "
        "   padding: 4px; "
        "}"
    );
    progressLayout->addWidget(m_compressLogEdit);

    rightLayout->addWidget(progressGroup, 1);

    // === 开始按钮（移到右列下方） ===
    m_startCompressBtn = new QPushButton(tr("Start Compression"));
    m_startCompressBtn->setMinimumHeight(45);
    m_startCompressBtn->setStyleSheet(
        "QPushButton { "
        "   background: rgba(255, 255, 255, 160); "
        "   color: #333; "
        "   border: 2px solid rgba(150, 150, 150, 200); "
        "   border-radius: 8px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { "
        "   background: rgba(255, 255, 255, 200); "
        "   border: 2px solid rgba(120, 120, 120, 220); "
        "} "
        "QPushButton:pressed { "
        "   background: rgba(220, 220, 220, 180); "
        "} "
        "QPushButton:disabled { "
        "   background: rgba(200, 200, 200, 140); "
        "   color: #888; "
        "}"
    );
    connect(m_startCompressBtn, &QPushButton::clicked, this, &MainWindow::onStartCompressionClicked);
    rightLayout->addWidget(m_startCompressBtn);

    // 添加左右两列到内容区域，设置拉伸比例（左:右 = 3:2）
    contentLayout->addWidget(leftColumn, 3);
    contentLayout->addWidget(rightColumn, 2);

    // 内容区域添加到主布局
    mainVLayout->addWidget(contentBox);

    return tab;
}

QWidget* MainWindow::createDecompressionTab()
{
    QWidget *tab = new QWidget();
    tab->setStyleSheet("background: transparent;");

    // 主垂直布局
    QVBoxLayout *mainVLayout = new QVBoxLayout(tab);
    mainVLayout->setSpacing(0);
    mainVLayout->setContentsMargins(0, 0, 0, 0);

    QString groupBoxStyle =
        "QGroupBox { "
        "   background: rgba(255, 255, 255, 130); "
        "   border: 1px solid rgba(200, 200, 200, 150); "
        "   border-radius: 8px; "
        "   margin-top: 12px; "
        "   padding-top: 12px; "
        "   font-weight: bold; "
        "   color: #333; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   left: 12px; "
        "   padding: 0 6px; "
        "} "
        "QLabel { background: transparent; color: #333; } "
        "QLineEdit { "
        "   background: rgba(255, 255, 255, 180); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 4px; "
        "   padding: 6px; "
        "}";

    QString btnStyle =
        "QPushButton { "
        "   background: rgba(255, 255, 255, 160); "
        "   border: 1px solid rgba(180, 180, 180, 180); "
        "   border-radius: 5px; "
        "   padding: 8px 18px; "
        "   font-weight: bold; "
        "   color: #333; "
        "} "
        "QPushButton:hover { "
        "   background: rgba(255, 255, 255, 200); "
        "   border: 1px solid rgba(150, 150, 150, 200); "
        "} "
        "QPushButton:pressed { "
        "   background: rgba(220, 220, 220, 180); "
        "} "
        "QPushButton:disabled { "
        "   background: rgba(200, 200, 200, 140); "
        "   color: #888; "
        "}";

    // =============================================
    // 内容区域容器 - 左右两列作为一个整体
    // =============================================
    QWidget *contentBox = new QWidget();
    contentBox->setStyleSheet("background: transparent;");
    QHBoxLayout *contentLayout = new QHBoxLayout(contentBox);
    contentLayout->setSpacing(10);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    // =============================================
    // 左列 - 输入信息填写区域
    // =============================================
    QWidget *leftColumn = new QWidget();
    leftColumn->setStyleSheet("background: transparent;");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setSpacing(10);
    leftLayout->setContentsMargins(0, 0, 0, 0);

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

    leftLayout->addWidget(inputGroup);

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
    m_decompressPasswordEdit->setPlaceholderText(tr("Leave empty for default"));
    outputLayout->addWidget(m_decompressPasswordEdit, 1, 1, 1, 2);

    leftLayout->addWidget(outputGroup);
    leftLayout->addStretch();

    // =============================================
    // 右列 - 输出信息区域
    // =============================================
    QWidget *rightColumn = new QWidget();
    rightColumn->setStyleSheet("background: transparent;");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightColumn);
    rightLayout->setSpacing(10);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // === 进度和日志区域 ===
    QGroupBox *progressGroup = new QGroupBox(tr("Progress"));
    progressGroup->setStyleSheet(
        "QGroupBox { "
        "   background: rgba(255, 255, 255, 130); "
        "   border: 1px solid rgba(200, 200, 200, 150); "
        "   border-radius: 8px; "
        "   margin-top: 12px; "
        "   padding-top: 12px; "
        "   font-weight: bold; "
        "   color: #333; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   left: 12px; "
        "   padding: 0 6px; "
        "}"
    );
    QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);
    progressLayout->setSpacing(8);

    // 当前文件标签
    m_decompressCurrentFileLabel = new QLabel(tr("Ready"));
    m_decompressCurrentFileLabel->setStyleSheet(
        "QLabel { "
        "   background: transparent; "
        "   color: #333; "
        "   padding: 6px; "
        "}"
    );
    m_decompressCurrentFileLabel->setWordWrap(true);
    progressLayout->addWidget(m_decompressCurrentFileLabel);

    m_decompressProgressBar = new QProgressBar();
    m_decompressProgressBar->setValue(0);
    m_decompressProgressBar->setTextVisible(true);
    m_decompressProgressBar->setMinimumHeight(28);
    m_decompressProgressBar->setStyleSheet(
        "QProgressBar { "
        "   background: rgba(255, 255, 255, 160); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 5px; "
        "   text-align: center; "
        "   font-weight: bold; "
        "} "
        "QProgressBar::chunk { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #b0b0b0, stop:1 #e0e0e0); "
        "   border-radius: 4px; "
        "}"
    );
    progressLayout->addWidget(m_decompressProgressBar);

    // 进度百分比标签
    m_decompressProgressLabel = new QLabel(tr("Overall: 0% | Current: 0%"));
    m_decompressProgressLabel->setStyleSheet(
        "QLabel { "
        "   background: transparent; "
        "   color: #555; "
        "}"
    );
    progressLayout->addWidget(m_decompressProgressLabel);

    // 日志框 - 禁用自动换行
    m_decompressLogEdit = new QTextEdit();
    m_decompressLogEdit->setReadOnly(true);
    m_decompressLogEdit->setLineWrapMode(QTextEdit::NoWrap);
    m_decompressLogEdit->setStyleSheet(
        "QTextEdit { "
        "   background: rgba(255, 255, 255, 160); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 5px; "
        "   padding: 4px; "
        "}"
    );
    progressLayout->addWidget(m_decompressLogEdit);

    rightLayout->addWidget(progressGroup, 1);

    // === 开始按钮（移到右列下方） ===
    m_startDecompressBtn = new QPushButton(tr("Start Decompression"));
    m_startDecompressBtn->setMinimumHeight(45);
    m_startDecompressBtn->setStyleSheet(
        "QPushButton { "
        "   background: rgba(255, 255, 255, 160); "
        "   color: #333; "
        "   border: 2px solid rgba(150, 150, 150, 200); "
        "   border-radius: 8px; "
        "   font-weight: bold; "
        "} "
        "QPushButton:hover { "
        "   background: rgba(255, 255, 255, 200); "
        "   border: 2px solid rgba(120, 120, 120, 220); "
        "} "
        "QPushButton:pressed { "
        "   background: rgba(220, 220, 220, 180); "
        "} "
        "QPushButton:disabled { "
        "   background: rgba(200, 200, 200, 140); "
        "   color: #888; "
        "}"
    );
    connect(m_startDecompressBtn, &QPushButton::clicked, this, &MainWindow::onStartDecompressionClicked);
    rightLayout->addWidget(m_startDecompressBtn);

    // 添加左右两列到内容区域，设置拉伸比例（左:右 = 3:2）
    contentLayout->addWidget(leftColumn, 3);
    contentLayout->addWidget(rightColumn, 2);

    // 内容区域添加到主布局
    mainVLayout->addWidget(contentBox);

    return tab;
}

// === 压缩模式槽函数 ===

void MainWindow::onAddFilesClicked()
{
    // 使用非原生对话框支持多选文件
    QFileDialog dialog(this, tr("Select Files"));
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() == QDialog::Accepted) {
        QStringList files = dialog.selectedFiles();
        for (const QString &file : files) {
            QString cleanPath = makeValidPath(file);
            if (!cleanPath.isEmpty() && m_fileListWidget->findItems(cleanPath, Qt::MatchExactly).isEmpty()) {
                m_fileListWidget->addItem(cleanPath);
            }
        }
    }
}

void MainWindow::onAddFolderClicked()
{
    // 使用非原生对话框支持多选文件夹
    QFileDialog dialog(this, tr("Select Folders"));
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly, true);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    if (dialog.exec() == QDialog::Accepted) {
        QStringList folders = dialog.selectedFiles();
        for (const QString &folder : folders) {
            QString cleanPath = makeValidPath(folder);
            if (!cleanPath.isEmpty() && m_fileListWidget->findItems(cleanPath, Qt::MatchExactly).isEmpty()) {
                m_fileListWidget->addItem(cleanPath);
            }
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
        password = "LOVEYONAGI";  // 使用默认密码
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
    m_compressProgressLabel->setText(tr("Overall: 0%"));
    m_compressCurrentFileLabel->setText(tr("Initializing..."));
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
        password = "LOVEYONAGI";  // 使用默认密码
    }

    QString outputDir = m_decompressOutputDirEdit->text().trimmed();

    // 设置UI状态
    m_isProcessing = true;
    m_startDecompressBtn->setEnabled(false);
    m_startDecompressBtn->setText(tr("Processing..."));
    m_decompressProgressBar->setValue(0);
    m_decompressProgressLabel->setText(tr("Overall: 0%"));
    m_decompressCurrentFileLabel->setText(tr("Initializing..."));
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
    int overallInt = static_cast<int>(overallProgress);
    int fileInt = static_cast<int>(fileProgress);

    m_compressProgressBar->setValue(overallInt);
    m_compressProgressLabel->setText(tr("Overall: %1% | Current file: %2%").arg(overallInt).arg(fileInt));

    if (!filename.isEmpty()) {
        m_compressCurrentFileLabel->setText(tr("Processing: %1").arg(filename));
        m_compressLogEdit->append(tr("[%1%] %2 - %3 (%4%)")
            .arg(overallInt, 3)
            .arg(status)
            .arg(filename)
            .arg(fileInt, 3));
    } else {
        m_compressCurrentFileLabel->setText(status);
        m_compressLogEdit->append(tr("[%1%] %2").arg(overallInt, 3).arg(status));
    }
}

void MainWindow::onCompressionFinished(bool success, const QString &message)
{
    m_isProcessing = false;
    m_startCompressBtn->setEnabled(true);
    m_startCompressBtn->setText(tr("Start Compression"));

    if (success) {
        m_compressProgressBar->setValue(100);
        m_compressProgressLabel->setText(tr("Overall: 100% | Completed"));
        m_compressCurrentFileLabel->setText(tr("Completed successfully"));
        m_compressLogEdit->append(tr("Compression completed successfully!"));
        QMessageBox::information(this, tr("Success"), message);
    } else {
        m_compressCurrentFileLabel->setText(tr("Failed"));
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
    int overallInt = static_cast<int>(overallProgress);
    int fileInt = static_cast<int>(fileProgress);

    m_decompressProgressBar->setValue(overallInt);
    m_decompressProgressLabel->setText(tr("Overall: %1% | Current file: %2%").arg(overallInt).arg(fileInt));

    if (!filename.isEmpty()) {
        m_decompressCurrentFileLabel->setText(tr("Processing: %1").arg(filename));
        m_decompressLogEdit->append(tr("[%1%] %2 - %3 (%4%)")
            .arg(overallInt, 3)
            .arg(status)
            .arg(filename)
            .arg(fileInt, 3));
    } else {
        m_decompressCurrentFileLabel->setText(status);
        m_decompressLogEdit->append(tr("[%1%] %2").arg(overallInt, 3).arg(status));
    }
}

void MainWindow::onDecompressionFinished(bool success, const QString &message)
{
    m_isProcessing = false;
    m_startDecompressBtn->setEnabled(true);
    m_startDecompressBtn->setText(tr("Start Decompression"));

    if (success) {
        m_decompressProgressBar->setValue(100);
        m_decompressProgressLabel->setText(tr("Overall: 100% | Completed"));
        m_decompressCurrentFileLabel->setText(tr("Completed successfully"));
        m_decompressLogEdit->append(tr("Decompression completed successfully!"));
        QMessageBox::information(this, tr("Success"), message);
    } else {
        m_decompressCurrentFileLabel->setText(tr("Failed"));
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

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> &urls = event->mimeData()->urls();
    addDroppedPaths(urls);
}

void MainWindow::addDroppedPaths(const QList<QUrl> &urls)
{
    for (const QUrl &url : urls) {
        QString path = url.toLocalFile();
        if (path.isEmpty()) {
            path = url.toString();
        }

        QString cleanPath = makeValidPath(path);
        if (cleanPath.isEmpty()) {
            continue;
        }

        // 检查是否已存在
        if (m_fileListWidget->findItems(cleanPath, Qt::MatchExactly).isEmpty()) {
            m_fileListWidget->addItem(cleanPath);
        }
    }
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

    // 获取窗口大小
    QSize windowSize = this->size();

    // 计算保持宽高比的缩放尺寸（裁剪模式：缩放到完全覆盖窗口）
    QSize imageSize = m_backgroundPixmap.size();
    double imageRatio = static_cast<double>(imageSize.width()) / imageSize.height();
    double windowRatio = static_cast<double>(windowSize.width()) / windowSize.height();

    QSize scaledSize;
    if (windowRatio > imageRatio) {
        // 窗口更宽 - 按宽度缩放，高度会超出
        scaledSize.setWidth(windowSize.width());
        scaledSize.setHeight(static_cast<int>(windowSize.width() / imageRatio));
    } else {
        // 窗口更高 - 按高度缩放，宽度会超出
        scaledSize.setHeight(windowSize.height());
        scaledSize.setWidth(static_cast<int>(windowSize.height() * imageRatio));
    }

    // 缩放图片
    QPixmap scaledPixmap = m_backgroundPixmap.scaled(
        scaledSize,
        Qt::KeepAspectRatioByExpanding,
        Qt::SmoothTransformation
    );

    // 居中裁剪
    int x = (scaledPixmap.width() - windowSize.width()) / 2;
    int y = (scaledPixmap.height() - windowSize.height()) / 2;

    QPixmap croppedPixmap = scaledPixmap.copy(
        qMax(0, x),
        qMax(0, y),
        windowSize.width(),
        windowSize.height()
    );

    // 设置背景
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(croppedPixmap));
    setPalette(palette);
}
