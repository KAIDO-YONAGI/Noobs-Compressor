#include "MainWindow.h"

#include <QTextDocument>

QWidget* MainWindow::createDecompressionTab()
{
    QWidget *tab = new QWidget();
    tab->setStyleSheet("background: transparent;");

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
        "   placeholder-text-color: rgba(142, 149, 161, 190); "
        "}";

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

    QGroupBox *inputGroup = new QGroupBox(tr("Select Archive"));
    inputGroup->setStyleSheet(groupBoxStyle);
    QGridLayout *inputLayout = new QGridLayout(inputGroup);

    inputLayout->addWidget(new QLabel(tr("Archive File (.sy):")), 0, 0);
    m_decompressFilePathEdit = new QLineEdit();
    m_decompressFilePathEdit->setPlaceholderText(tr("Choose a .sy archive file"));
    inputLayout->addWidget(m_decompressFilePathEdit, 0, 1);
    QPushButton *browseFileBtn = new QPushButton(tr("Browse"));
    browseFileBtn->setStyleSheet(btnStyle);
    connect(browseFileBtn, &QPushButton::clicked, this, &MainWindow::onBrowseDecompressFileClicked);
    inputLayout->addWidget(browseFileBtn, 0, 2);

    leftLayout->addWidget(inputGroup);

    QGroupBox *outputGroup = new QGroupBox(tr("Output Settings"));
    outputGroup->setStyleSheet(groupBoxStyle);
    QGridLayout *outputLayout = new QGridLayout(outputGroup);

    outputLayout->addWidget(new QLabel(tr("Output Directory:")), 0, 0);
    m_decompressOutputDirEdit = new QLineEdit();
    m_decompressOutputDirEdit->setPlaceholderText(tr("Choose an output folder"));
    outputLayout->addWidget(m_decompressOutputDirEdit, 0, 1);
    QPushButton *browseOutBtn = new QPushButton(tr("Browse"));
    browseOutBtn->setStyleSheet(btnStyle);
    connect(browseOutBtn, &QPushButton::clicked, this, &MainWindow::onBrowseDecompressOutputClicked);
    outputLayout->addWidget(browseOutBtn, 0, 2);

    outputLayout->addWidget(new QLabel(tr("Subfolder Name:")), 1, 0);
    m_decompressSubfolderEdit = new QLineEdit();
    m_decompressSubfolderEdit->setPlaceholderText(tr("Optional: create a subfolder"));
    outputLayout->addWidget(m_decompressSubfolderEdit, 1, 1, 1, 2);

    outputLayout->addWidget(new QLabel(tr("Password:")), 2, 0);
    m_decompressPasswordEdit = new QLineEdit();
    m_decompressPasswordEdit->setEchoMode(QLineEdit::Password);
    m_decompressPasswordEdit->setPlaceholderText(tr("Enter a password if needed"));
    outputLayout->addWidget(m_decompressPasswordEdit, 2, 1, 1, 2);

    leftLayout->addWidget(outputGroup);

    QPushButton *resetDecompressBtn = new QPushButton(tr("Reset"));
    resetDecompressBtn->setStyleSheet(btnStyle);
    connect(resetDecompressBtn, &QPushButton::clicked, this, [this]() {
        m_decompressFilePathEdit->clear();
        m_decompressOutputDirEdit->clear();
        m_decompressSubfolderEdit->clear();
        m_decompressPasswordEdit->clear();
    });
    leftLayout->addWidget(resetDecompressBtn);
    leftLayout->addStretch();

    QWidget *rightColumn = new QWidget();
    rightColumn->setStyleSheet("background: transparent;");
    rightColumn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightColumn);
    rightLayout->setSpacing(10);
    rightLayout->setContentsMargins(0, 0, 0, 0);

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

    m_decompressCurrentFileLabel = new QLabel(tr("Ready"));
    m_decompressCurrentFileLabel->setStyleSheet(
        "QLabel { "
        "   background: transparent; "
        "   color: #4f5d6e; "
        "   padding: 6px; "
        "}"
    );
    m_decompressCurrentFileLabel->setWordWrap(false);
    m_decompressCurrentFileLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
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
        "   color: #4f5d6e; "
        "} "
        "QProgressBar::chunk { "
        "   background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #b0b0b0, stop:1 #e0e0e0); "
        "   border-radius: 4px; "
        "}"
    );
    progressLayout->addWidget(m_decompressProgressBar);

    m_decompressProgressLabel = new QLabel(tr("Overall: 0% | Current: 0%"));
    m_decompressProgressLabel->setStyleSheet(
        "QLabel { "
        "   background: transparent; "
        "   color: #667487; "
        "}"
    );
    progressLayout->addWidget(m_decompressProgressLabel);

    m_decompressLogEdit = new QTextEdit();
    m_decompressLogEdit->setReadOnly(true);
    m_decompressLogEdit->setTextInteractionFlags(Qt::NoTextInteraction);
    m_decompressLogEdit->setFocusPolicy(Qt::NoFocus);
    m_decompressLogEdit->setCursor(Qt::ArrowCursor);
    m_decompressLogEdit->viewport()->setCursor(Qt::ArrowCursor);
    m_decompressLogEdit->setLineWrapMode(QTextEdit::NoWrap);
    m_decompressLogEdit->setStyleSheet(
        "QTextEdit { "
        "   background: rgba(255, 255, 255, 160); "
        "   border: 1px solid rgba(200, 200, 200, 180); "
        "   border-radius: 5px; "
        "   padding: 4px; "
        "   color: #4f5d6e; "
        "}"
    );
    progressLayout->addWidget(m_decompressLogEdit);

    rightLayout->addWidget(progressGroup, 1);

    m_startDecompressBtn = new QPushButton(tr("Start Decompression"));
    m_startDecompressBtn->setMinimumHeight(45);
    m_startDecompressBtn->setStyleSheet(
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
    connect(m_startDecompressBtn, &QPushButton::clicked, this, &MainWindow::onStartDecompressionClicked);
    rightLayout->addWidget(m_startDecompressBtn);

    contentLayout->addWidget(leftColumn, 3);
    contentLayout->addWidget(rightColumn, 2);
    contentLayout->setStretch(0, 3);
    contentLayout->setStretch(1, 2);

    mainVLayout->addWidget(contentBox);
    return tab;
}

void MainWindow::onBrowseDecompressFileClicked()
{
    const QString file = QFileDialog::getOpenFileName(this,
        tr("Select Archive"), QString(), tr("Simple Archives (*.sy);;All Files (*)"));

    if (!file.isEmpty()) {
        m_decompressFilePathEdit->setText(makeValidPath(file));
        QFileInfo fileInfo(file);
        m_decompressOutputDirEdit->setText(fileInfo.absolutePath());
    }
}

void MainWindow::onBrowseDecompressOutputClicked()
{
    const QString dir = QFileDialog::getExistingDirectory(this,
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

    const QString inputPath = m_decompressFilePathEdit->text().trimmed();
    if (inputPath.isEmpty() || !pathExists(inputPath)) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a valid archive file."));
        return;
    }

    if (!inputPath.toLower().endsWith(".sy")) {
        QMessageBox::warning(this, tr("Error"), tr("Only .sy files can be decompressed."));
        return;
    }

    const QString password = m_decompressPasswordEdit->text();
    QString outputDir = m_decompressOutputDirEdit->text().trimmed();

    const QString subfolder = m_decompressSubfolderEdit->text().trimmed();
    if (!subfolder.isEmpty()) {
        outputDir = outputDir + "/" + subfolder;
    }

    m_isProcessing = true;
    m_tabWidget->setCurrentIndex(1);
    m_tabWidget->setTabEnabled(0, false);
    m_startCompressBtn->setEnabled(false);
    m_startDecompressBtn->setEnabled(false);
    m_startDecompressBtn->setText(tr("Processing..."));
    m_decompressProgressBar->setValue(0);
    m_decompressProgressLabel->setText(tr("Overall: 0%"));
    m_decompressCurrentFileLabel->setText(tr("Initializing..."));
    m_decompressLogEdit->clear();
    m_decompressLogEdit->append(tr("Starting decompression..."));

    m_workerThread = new QThread();
    m_worker = new CompressionWorker();
    m_worker->setDecompressionParams(inputPath, outputDir, password);
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, m_worker, &CompressionWorker::doDecompression);
    connect(m_worker, &CompressionWorker::detailedProgress, this, &MainWindow::onDecompressionDetailedProgress);
    connect(m_worker, &CompressionWorker::finished, this, &MainWindow::onDecompressionFinished);

    m_workerThread->start();
}

void MainWindow::onDecompressionDetailedProgress(const QString &filename, double fileProgress, double overallProgress, const QString &status)
{
    const int overallInt = static_cast<int>(overallProgress);
    const int fileInt = static_cast<int>(fileProgress);

    m_decompressProgressBar->setValue(overallInt);
    m_decompressProgressLabel->setText(tr("Overall: %1% | Current file: %2%").arg(overallInt).arg(fileInt));

    if (!filename.isEmpty()) {
        const QString displayText = tr("Processing: %1").arg(elideText(filename, 250));
        m_decompressCurrentFileLabel->setText(displayText);

        QTextDocument *doc = m_decompressLogEdit->document();
        if (doc->lineCount() > 500) {
            QTextCursor cursor(doc);
            cursor.movePosition(QTextCursor::End);
            cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::Up, QTextCursor::KeepAnchor, 500);
            cursor.removeSelectedText();
        }

        m_decompressLogEdit->append(tr("[%1%] %2 - %3 (%4%)")
            .arg(overallInt, 3)
            .arg(status)
            .arg(filename)
            .arg(fileInt, 3));
        return;
    }

    m_decompressCurrentFileLabel->setText(status);
    m_decompressLogEdit->append(tr("[%1%] %2").arg(overallInt, 3).arg(status));
}

void MainWindow::onDecompressionFinished(bool success, const QString &message)
{
    const QString dialogMessage = localizeWorkerDialogMessage(message);

    m_isProcessing = false;
    m_tabWidget->setTabEnabled(0, true);
    m_tabWidget->setTabEnabled(1, true);
    m_startCompressBtn->setEnabled(true);
    m_startDecompressBtn->setEnabled(true);
    m_startDecompressBtn->setText(tr("Start Decompression"));

    if (success) {
        m_decompressProgressBar->setValue(100);
        m_decompressProgressLabel->setText(tr("Overall: 100% | Completed"));
        m_decompressCurrentFileLabel->setText(tr("Completed successfully"));
        m_decompressLogEdit->append(tr("Decompression completed successfully!"));
        QMessageBox::information(this, tr("Success"), dialogMessage);
    } else {
        m_decompressCurrentFileLabel->setText(tr("Failed"));
        m_decompressLogEdit->append(tr("Decompression failed: %1").arg(message));
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
