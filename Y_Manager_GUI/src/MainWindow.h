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
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QPixmap>
#include <QResizeEvent>
#include <QSizePolicy>
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

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

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
    void addDroppedPaths(const QList<QUrl> &urls);
    void updateBackground();
    void updateOutputDirectory();
    QString elideText(const QString &text, int maxWidth);

    // 压缩模式控件
    QListWidget *m_fileListWidget;
    QLineEdit *m_outputDirEdit;
    QLineEdit *m_outputFileNameEdit;
    QLineEdit *m_passwordEdit;
    QPushButton *m_startCompressBtn;
    QProgressBar *m_compressProgressBar;
    QLabel *m_compressCurrentFileLabel;
    QLabel *m_compressProgressLabel;
    QTextEdit *m_compressLogEdit;

    // 解压模式控件
    QLineEdit *m_decompressFilePathEdit;
    QLineEdit *m_decompressOutputDirEdit;
    QLineEdit *m_decompressPasswordEdit;
    QPushButton *m_startDecompressBtn;
    QProgressBar *m_decompressProgressBar;
    QLabel *m_decompressCurrentFileLabel;
    QLabel *m_decompressProgressLabel;
    QTextEdit *m_decompressLogEdit;

    // 工作线程
    QThread *m_workerThread;
    CompressionWorker *m_worker;

    // 状态
    bool m_isProcessing;

    // UI组件
    QTabWidget *m_tabWidget;

    // 背景图片
    QPixmap m_backgroundPixmap;
};
