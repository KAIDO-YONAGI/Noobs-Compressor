#include "MainWindow.h"
#include "PlaceholderLineEdit.h"
#include "../CompressorFileSystem/Commons/include/FileLibrary.h"
#include "../CompressorFileSystem/Strategy/include/StrategyFactory.h"

#include <QTextDocument>

QWidget* MainWindow::createCompressionTab()
{
    QWidget *tab = new QWidget();
    tab->setStyleSheet("background: transparent;");

    QVBoxLayout *mainVLayout = new QVBoxLayout(tab);
    mainVLayout->setSpacing(0);
    mainVLayout->setContentsMargins(0, 0, 0, 0);

    QString btnStyle =
        "QPushButton { "
        "   background: rgba(255, 255, 255, 160); "
        "   border: 1px solid rgba(180, 180, 180, 180); "
        "   border-radius: 5px; "
        "   padding: 8px 18px; "
        "   font-weight: bold; "
        "   color: #4f5d6e; "
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
        "   color: #8d98a6; "
        "}";

    QString groupBoxStyle =
        "QGroupBox { "
        "   background: rgba(255, 255, 255, 130); "
        "   border: 1px solid rgba(200, 200, 200, 150); "
        "   border-radius: 8px; "
        "   margin-top: 12px; "
        "   padding-top: 12px; "
        "   font-weight: bold; "
        "   color: #4f5d6e; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   left: 12px; "
        "   padding: 0 8px 8px 8px; "
        "} "
        "QLabel { background: transparent; color: #4f5d6e; } "
        "QLineEdit { "
        "   background: rgba(255, 255, 255, 180); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 4px; "
        "   padding: 6px; "
        "   color: #4f5d6e; "
        "}";

    QWidget *contentBox = new QWidget();
    contentBox->setStyleSheet("background: transparent;");
    QHBoxLayout *contentLayout = new QHBoxLayout(contentBox);
    contentLayout->setSpacing(10);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *leftColumn = new QWidget();
    leftColumn->setStyleSheet("background: transparent;");
    leftColumn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setSpacing(10);
    leftLayout->setContentsMargins(0, 0, 0, 0);

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
        "   color: #4f5d6e; "
        "} "
        "QListWidget::item { padding: 6px; } "
        "QListWidget::item:selected { background: rgba(200, 200, 200, 150); }"
    );
    connect(m_fileListWidget, &QListWidget::itemSelectionChanged, this, [this]() {
        updateOutputDirectory();
        updateOutputFileName();
    });
    fileLayout->addWidget(m_fileListWidget);

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

    QGroupBox *outputGroup = new QGroupBox(tr("Output Settings"));
    outputGroup->setStyleSheet(groupBoxStyle);
    QGridLayout *outputLayout = new QGridLayout(outputGroup);

    outputLayout->addWidget(new QLabel(tr("Output Directory:")), 0, 0);
    m_outputDirEdit = new PlaceholderLineEdit();
    applyPlaceholderPalette(m_outputDirEdit);
    setPlaceholderHint(m_outputDirEdit, tr("Auto-filled from selected items"));
    outputLayout->addWidget(m_outputDirEdit, 0, 1);
    QPushButton *browseOutDirBtn = new QPushButton(tr("Browse"));
    browseOutDirBtn->setStyleSheet(btnStyle);
    connect(browseOutDirBtn, &QPushButton::clicked, this, &MainWindow::onBrowseOutputDirClicked);
    outputLayout->addWidget(browseOutDirBtn, 0, 2);

    outputLayout->addWidget(new QLabel(tr("Output Filename:")), 1, 0);
    m_outputFileNameEdit = new PlaceholderLineEdit();
    applyPlaceholderPalette(m_outputFileNameEdit);
    setPlaceholderHint(m_outputFileNameEdit, tr("Example: SHINKU_YONAGI"));
    outputLayout->addWidget(m_outputFileNameEdit, 1, 1, 1, 2);

    outputLayout->addWidget(new QLabel(tr("Password:")), 2, 0);
    m_passwordEdit = new PlaceholderLineEdit();
    applyPlaceholderPalette(m_passwordEdit);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    setPlaceholderHint(m_passwordEdit, tr("Enter a password"));
    outputLayout->addWidget(m_passwordEdit, 2, 1, 1, 2);

    leftLayout->addWidget(outputGroup);
    leftLayout->addStretch();

    QWidget *rightColumn = new QWidget();
    rightColumn->setStyleSheet("background: transparent;");
    rightColumn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightColumn);
    rightLayout->setSpacing(10);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox *modeGroup = new QGroupBox(tr("Compression Mode"));
    modeGroup->setStyleSheet(
        "QGroupBox { "
        "   background: rgba(255, 255, 255, 130); "
        "   border: 1px solid rgba(200, 200, 200, 150); "
        "   border-radius: 8px; "
        "   margin-top: 12px; "
        "   padding-top: 12px; "
        "   font-weight: bold; "
        "   color: #4f5d6e; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   left: 12px; "
        "   padding: 0 8px 8px 8px; "
        "}"
    );
    QVBoxLayout *modeLayout = new QVBoxLayout(modeGroup);

    m_compressModeCombo = new QComboBox();
    m_compressModeCombo->addItem(tr("Huffman Only (Default)"),
        static_cast<int>(Y_flib::CompressionMode::HuffmanOnly));
    m_compressModeCombo->addItem(tr("Huffman + AES"),
        static_cast<int>(Y_flib::CompressionMode::HuffmanAES));
    m_compressModeCombo->addItem(tr("AES Only"),
        static_cast<int>(Y_flib::CompressionMode::AESOnly));
    m_compressModeCombo->addItem(tr("Pack Only"),
        static_cast<int>(Y_flib::CompressionMode::PackOnly));
    m_compressModeCombo->setCurrentIndex(0);
    m_compressModeCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_compressModeCombo->setMinimumContentsLength(16);
    m_compressModeCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_compressModeCombo->setStyleSheet(
        "QComboBox { "
        "   background: rgba(255, 255, 255, 180); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 4px; "
        "   padding: 6px 20px 6px 6px; "
        "   color: #4f5d6e; "
        "} "
        "QComboBox:hover { "
        "   background: rgba(255, 255, 255, 220); "
        "   border: 1px solid rgba(150, 150, 150, 200); "
        "} "
        "QComboBox:disabled { "
        "   background: rgba(200, 200, 200, 140); "
        "   color: #8d98a6; "
        "} "
        "QComboBox::drop-down { "
        "   subcontrol-origin: padding; "
        "   subcontrol-position: center right; "
        "   width: 20px; "
        "   border-left: 1px solid rgba(200, 200, 200, 180); "
        "   border-top-right-radius: 4px; "
        "   border-bottom-right-radius: 4px; "
        "} "
        "QComboBox::down-arrow { "
        "   width: 8px; "
        "   height: 8px; "
        "   background: #666; "
        "} "
        "QComboBox::down-arrow:disabled { "
        "   background: #aaa; "
        "} "
        "QComboBox QAbstractItemView { "
        "   background: rgba(255, 255, 255, 220); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   color: #4f5d6e; "
        "   selection-background-color: rgba(200, 200, 200, 150); "
        "}"
    );
    connect(m_compressModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
        const Y_flib::CompressionMode mode =
            static_cast<Y_flib::CompressionMode>(m_compressModeCombo->currentData().toInt());
        const bool needsEncryption = Y_flib::StrategyFactory::hasEncryption(mode);
        m_passwordEdit->setEnabled(needsEncryption);
        if (!needsEncryption) {
            setPlaceholderHint(m_passwordEdit, tr("No password needed for this mode"));
        } else {
            setPlaceholderHint(m_passwordEdit, tr("Enter a password"));
        }
    });
    emit m_compressModeCombo->currentIndexChanged(m_compressModeCombo->currentIndex());

    modeLayout->addWidget(m_compressModeCombo);
    rightLayout->addWidget(modeGroup);

    QGroupBox *progressGroup = new QGroupBox(tr("Progress"));
    progressGroup->setStyleSheet(
        "QGroupBox { "
        "   background: rgba(255, 255, 255, 130); "
        "   border: 1px solid rgba(200, 200, 200, 150); "
        "   border-radius: 8px; "
        "   margin-top: 12px; "
        "   padding-top: 12px; "
        "   font-weight: bold; "
        "   color: #4f5d6e; "
        "} "
        "QGroupBox::title { "
        "   subcontrol-origin: margin; "
        "   left: 12px; "
        "   padding: 0 8px 8px 8px; "
        "}"
    );
    QVBoxLayout *progressLayout = new QVBoxLayout(progressGroup);
    progressLayout->setSpacing(8);

    m_compressCurrentFileLabel = new QLabel(tr("Ready"));
    m_compressCurrentFileLabel->setStyleSheet(
        "QLabel { "
        "   background: transparent; "
        "   color: #4f5d6e; "
        "   padding: 6px; "
        "}"
    );
    m_compressCurrentFileLabel->setWordWrap(false);
    m_compressCurrentFileLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    progressLayout->addWidget(m_compressCurrentFileLabel);

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
        "   color: #4f5d6e; "
        "} "
        "QProgressBar::chunk { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #a0a0a0, stop:1 #d0d0d0); "
        "   border-radius: 4px; "
        "}"
    );
    progressLayout->addWidget(m_compressProgressBar);

    m_compressProgressLabel = new QLabel(tr("Overall: 0% | Current: 0%"));
    m_compressProgressLabel->setStyleSheet(
        "QLabel { "
        "   background: transparent; "
        "   color: #667487; "
        "}"
    );
    progressLayout->addWidget(m_compressProgressLabel);

    m_compressLogEdit = new QTextEdit();
    m_compressLogEdit->setReadOnly(true);
    m_compressLogEdit->setTextInteractionFlags(Qt::NoTextInteraction);
    m_compressLogEdit->setFocusPolicy(Qt::NoFocus);
    m_compressLogEdit->setCursor(Qt::ArrowCursor);
    m_compressLogEdit->viewport()->setCursor(Qt::ArrowCursor);
    m_compressLogEdit->setLineWrapMode(QTextEdit::NoWrap);
    m_compressLogEdit->setStyleSheet(
        "QTextEdit { "
        "   background: rgba(255, 255, 255, 160); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 5px; "
        "   padding: 4px; "
        "   color: #4f5d6e; "
        "}"
    );
    progressLayout->addWidget(m_compressLogEdit);

    rightLayout->addWidget(progressGroup, 1);

    m_startCompressBtn = new QPushButton(tr("Start Compression"));
    m_startCompressBtn->setMinimumHeight(45);
    m_startCompressBtn->setStyleSheet(
        "QPushButton { "
        "   background: rgba(255, 255, 255, 160); "
        "   color: #4f5d6e; "
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
        "   color: #8d98a6; "
        "}"
    );
    connect(m_startCompressBtn, &QPushButton::clicked, this, &MainWindow::onStartCompressionClicked);
    rightLayout->addWidget(m_startCompressBtn);

    contentLayout->addWidget(leftColumn, 3);
    contentLayout->addWidget(rightColumn, 2);
    contentLayout->setStretch(0, 3);
    contentLayout->setStretch(1, 2);

    mainVLayout->addWidget(contentBox);
    return tab;
}

void MainWindow::onAddFilesClicked()
{
    const QStringList files = QFileDialog::getOpenFileNames(this, tr("Select Files"),
        QString(), tr("All Files (*)"));

    for (const QString &file : files) {
        const QString cleanPath = makeValidPath(file);
        if (!cleanPath.isEmpty() && m_fileListWidget->findItems(cleanPath, Qt::MatchExactly).isEmpty()) {
            m_fileListWidget->addItem(cleanPath);
        }
    }

    updateOutputDirectory();
    updateOutputFileName();
}

void MainWindow::onAddFolderClicked()
{
    const QString folder = QFileDialog::getExistingDirectory(this, tr("Select Folder"),
        QString(), QFileDialog::ShowDirsOnly);

    if (!folder.isEmpty()) {
        const QString cleanPath = makeValidPath(folder);
        if (!cleanPath.isEmpty() && m_fileListWidget->findItems(cleanPath, Qt::MatchExactly).isEmpty()) {
            m_fileListWidget->addItem(cleanPath);
        }
    }

    updateOutputDirectory();
    updateOutputFileName();
}

void MainWindow::onRemoveFileClicked()
{
    const QList<QListWidgetItem*> items = m_fileListWidget->selectedItems();
    for (QListWidgetItem *item : items) {
        delete m_fileListWidget->takeItem(m_fileListWidget->row(item));
    }

    updateOutputDirectory();
    updateOutputFileName();
}

void MainWindow::onClearFilesClicked()
{
    m_fileListWidget->clear();
    updateOutputDirectory();
    updateOutputFileName();
}

void MainWindow::onBrowseOutputDirClicked()
{
    const QString dir = QFileDialog::getExistingDirectory(this,
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

    if (m_fileListWidget->count() == 0) {
        QMessageBox::warning(this, tr("Error"), tr("Please add files to compress."));
        return;
    }

    const QString outputDir = m_outputDirEdit->text().trimmed();
    if (outputDir.isEmpty() || !pathExists(outputDir)) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a valid output directory."));
        return;
    }

    const QString fileName = m_outputFileNameEdit->text().trimmed();
    if (fileName.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter an output filename."));
        return;
    }

    const Y_flib::CompressionMode mode =
        static_cast<Y_flib::CompressionMode>(m_compressModeCombo->currentData().toInt());

    QString password;
    if (Y_flib::StrategyFactory::hasEncryption(mode)) {
        password = m_passwordEdit->text();
        if (password.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Please enter a password for encrypted mode."));
            return;
        }
    }

    QStringList files;
    for (int i = 0; i < m_fileListWidget->count(); ++i) {
        files.append(m_fileListWidget->item(i)->text());
    }

    m_isProcessing = true;
    m_tabWidget->setCurrentIndex(0);
    m_tabWidget->setTabEnabled(1, false);
    m_startCompressBtn->setEnabled(false);
    m_startDecompressBtn->setEnabled(false);
    m_startCompressBtn->setText(tr("Processing..."));
    m_compressModeCombo->setEnabled(false);
    m_compressProgressBar->setValue(0);
    m_compressProgressLabel->setText(tr("Overall: 0%"));
    m_compressCurrentFileLabel->setText(tr("Initializing..."));
    m_compressLogEdit->clear();
    m_compressLogEdit->append(tr("Starting compression..."));

    m_workerThread = new QThread();
    m_worker = new CompressionWorker();
    m_worker->setCompressionParams(files, outputDir, fileName, password, mode);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &CompressionWorker::doCompression);
    connect(m_worker, &CompressionWorker::detailedProgress, this, &MainWindow::onCompressionDetailedProgress);
    connect(m_worker, &CompressionWorker::finished, this, &MainWindow::onCompressionFinished);

    m_workerThread->start();
}

void MainWindow::onCompressionDetailedProgress(const QString &filename, double fileProgress, double overallProgress, const QString &status)
{
    const int overallInt = static_cast<int>(overallProgress);
    const int fileInt = static_cast<int>(fileProgress);

    m_compressProgressBar->setValue(overallInt);
    m_compressProgressLabel->setText(tr("Overall: %1% | Current file: %2%").arg(overallInt).arg(fileInt));

    if (!filename.isEmpty()) {
        const QString displayText = tr("Processing: %1").arg(elideText(filename, 250));
        m_compressCurrentFileLabel->setText(displayText);

        QTextDocument *doc = m_compressLogEdit->document();
        if (doc->lineCount() > 500) {
            QTextCursor cursor(doc);
            cursor.movePosition(QTextCursor::End);
            cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::Up, QTextCursor::KeepAnchor, 500);
            cursor.removeSelectedText();
        }

        m_compressLogEdit->append(tr("[%1%] %2 - %3 (%4%)")
            .arg(overallInt, 3)
            .arg(status)
            .arg(filename)
            .arg(fileInt, 3));
        return;
    }

    m_compressCurrentFileLabel->setText(status);
    m_compressLogEdit->append(tr("[%1%] %2").arg(overallInt, 3).arg(status));
}

void MainWindow::onCompressionFinished(bool success, const QString &message)
{
    const QString dialogMessage = localizeWorkerDialogMessage(message);

    m_isProcessing = false;
    m_tabWidget->setTabEnabled(0, true);
    m_tabWidget->setTabEnabled(1, true);
    m_startCompressBtn->setEnabled(true);
    m_startDecompressBtn->setEnabled(true);
    m_startCompressBtn->setText(tr("Start Compression"));
    m_compressModeCombo->setEnabled(true);

    if (success) {
        m_compressProgressBar->setValue(100);
        m_compressProgressLabel->setText(tr("Overall: 100% | Completed"));
        m_compressCurrentFileLabel->setText(tr("Completed successfully"));
        m_compressLogEdit->append(tr("Compression completed successfully!"));
        QMessageBox::information(this, tr("Success"), dialogMessage);
    } else {
        m_compressCurrentFileLabel->setText(tr("Failed"));
        m_compressLogEdit->append(tr("Compression failed: %1").arg(message));
        QMessageBox::critical(this, tr("Error"), dialogMessage);
    }

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

void MainWindow::updateOutputDirectory()
{
    const QString inputPath = primaryCompressionInputPath();
    if (!inputPath.isEmpty()) {
        QFileInfo fileInfo(inputPath);
        m_outputDirEdit->setText(fileInfo.absolutePath());
    } else {
        m_outputDirEdit->clear();
    }
}

void MainWindow::updateOutputFileName()
{
    const QString inputPath = primaryCompressionInputPath();
    if (inputPath.isEmpty()) {
        m_outputFileNameEdit->clear();
        return;
    }

    m_outputFileNameEdit->setText(suggestedOutputFileName(inputPath));
}

QString MainWindow::primaryCompressionInputPath() const
{
    const QList<QListWidgetItem *> selectedItems = m_fileListWidget->selectedItems();
    if (!selectedItems.isEmpty()) {
        return selectedItems.first()->text();
    }

    if (m_fileListWidget->count() > 0) {
        return m_fileListWidget->item(0)->text();
    }

    return {};
}

QString MainWindow::suggestedOutputFileName(const QString &inputPath) const
{
    const QFileInfo fileInfo(inputPath);
    QString suggestedName;

    if (fileInfo.isDir()) {
        suggestedName = fileInfo.fileName();
    } else {
        suggestedName = fileInfo.completeBaseName();
        if (suggestedName.isEmpty()) {
            suggestedName = fileInfo.fileName();
        }
    }

    return suggestedName.trimmed();
}
