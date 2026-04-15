#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <memory>
#include <vector>
#include <string>

// 前向声明
class CompressionWorker;
class Aes;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 压缩模式槽函数
    void onAddFilesClicked();
    void onAddFolderClicked();
    void onRemoveFileClicked();
    void onClearFilesClicked();
    void onBrowseOutputDirClicked();
    void onStartCompressionClicked();

    // 解压模式槽函数
    void onBrowseDecompressFileClicked();
    void onBrowseDecompressOutputClicked();
    void onStartDecompressionClicked();

    // 进度和状态槽函数
    void onCompressionDetailedProgress(const QString &filename, double fileProgress, double overallProgress, const QString &status);
    void onCompressionFinished(bool success, const QString &message);
    void onDecompressionDetailedProgress(const QString &filename, double fileProgress, double overallProgress, const QString &status);
    void onDecompressionFinished(bool success, const QString &message);

private:
    void setupUI();
    QWidget* createCompressionTab();
    QWidget* createDecompressionTab();
    QString getExeDirectory();
    bool pathExists(const QString &path);
    QString makeValidPath(const QString &input);

    // 压缩模式控件
    QListWidget *m_fileListWidget;
    QLineEdit *m_outputDirEdit;
    QLineEdit *m_outputFileNameEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_startCompressBtn;
    QProgressBar *m_compressProgressBar;
    QTextEdit *m_compressLogEdit;

    // 解压模式控件
    QLineEdit *m_decompressFilePathEdit;
    QLineEdit *m_decompressOutputDirEdit;
    QLineEdit *m_decompressPasswordEdit;
    QPushButton *m_startDecompressBtn;
    QProgressBar *m_decompressProgressBar;
    QTextEdit *m_decompressLogEdit;

    // 工作线程
    QThread *m_workerThread;
    CompressionWorker *m_worker;

    // 状态
    bool m_isProcessing;
};
