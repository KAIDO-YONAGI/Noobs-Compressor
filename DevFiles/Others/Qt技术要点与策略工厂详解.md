# Qt 技术要点与策略工厂详解

> 基于 `SFC_GUI/src/` 的实际代码，面向未系统学习 Qt 的读者。
> 涉及文件：`main.cpp`、`MainWindow.h/.cpp`、`MainWindow_Compression.cpp`、`MainWindow_Decompression.cpp`、`CompressionWorker.h/.cpp`、`PlaceholderLineEdit.h`、`StrategyFactory.h/.cpp`

---

## 目录

- [1. Qt 程序的基本骨架](#1-qt-程序的基本骨架)
- [2. 信号与槽机制](#2-信号与槽机制)
- [3. 多线程机制——moveToThread 模式](#3-多线程机制movetothread-模式)
- [4. UI 搭建——纯代码布局](#4-ui-搭建纯代码布局)
- [5. 事件处理——拖拽与窗口缩放](#5-事件处理拖拽与窗口缩放)
- [6. 国际化——tr() 与 QTranslator](#6-国际化tr-与-qtranslator)
- [7. 策略工厂与智能指针的具体实现](#7-策略工厂与智能指针的具体实现)
- [8. 原子变量与协作式取消](#8-原子变量与协作式取消)
- [9. 进度节流机制](#9-进度节流机制)
- [10. 文件摘要](#10-文件摘要)

---

## 1. Qt 程序的基本骨架

每个 Qt Widgets 程序都有一个固定的入口结构，见 `main.cpp`：

```cpp
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);   // ① 创建应用实例，管理事件循环和全局设置
    // ... 配置应用名称、字体、翻译 ...
    MainWindow window;              // ② 创建主窗口
    window.show();                  // ③ 显示窗口（此时窗口还不可见，进入事件循环后才渲染）
    return app.exec();              // ④ 进入事件循环，程序开始接收用户操作
}
```

**关键概念**：

| 概念 | 作用 | 本项目中的体现 |
|------|------|---------------|
| `QApplication` | 管理整个 GUI 应用的生命周期、事件循环、全局设置 | `main.cpp:135`，全局唯一 |
| `app.exec()` | 启动事件循环，程序在此阻塞，直到窗口关闭 | `main.cpp:156`，所有用户交互都由事件循环分发 |
| `QWidget::show()` | 将窗口标记为"可见"，下次事件循环迭代时渲染 | `main.cpp:154` |

**高 DPI 支持**（`main.cpp:130-133`）：

```cpp
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
```

Qt 6 默认启用高 DPI 缩放，Qt 5 需要手动设置。这段代码保证两个版本都能正确处理 4K 屏幕缩放。

---

## 2. 信号与槽机制

### 2.1 基本原理

信号槽是 Qt 的**观察者模式**实现，本质是类型安全的回调函数系统。

```
发送方                               接收方
┌──────────────────┐               ┌──────────────────┐
│ emit signal(args) │──── 通知 ────→│ slot(args)       │
│ (信号声明)         │               │ (槽函数实现)      │
└──────────────────┘               └──────────────────┘

连接方式：connect(sender, &Sender::signal, receiver, &Receiver::slot)
```

### 2.2 本项目中的信号槽

**CompressionWorker 的信号声明**（`CompressionWorker.h:55-57`）：

```cpp
signals:
    void detailedProgress(const QString &filename, double fileProgress,
                          double overallProgress, const QString &status);
    void finished(bool success, const QString &message);
```

**信号发送方——Worker（子线程）**：

```cpp
// CompressionWorker.cpp:201-205
emit detailedProgress(EncodingUtils::utf8ToQString(filename),
                      fileProgress, mappedProgress,
                      EncodingUtils::utf8ToQString(status));

// CompressionWorker.cpp:224
emit finished(true, QStringLiteral("Compression successful!..."));
```

**信号接收方——MainWindow（主线程）**（`MainWindow_Compression.cpp:479-480`）：

```cpp
connect(m_worker, &CompressionWorker::detailedProgress,
        this, &MainWindow::onCompressionDetailedProgress);
connect(m_worker, &CompressionWorker::finished,
        this, &MainWindow::onCompressionFinished);
```

### 2.3 跨线程的信号槽

当信号发送者和接收者不在同一线程时，Qt 自动使用 `Qt::QueuedConnection`：

```
子线程                                主线程
Worker::doCompression()               MainWindow
  │                                      │
  ├─ emit detailedProgress(...)          │
  │    └─ Qt 将参数拷贝到事件队列 ──────→ │ 从队列取出
  │                                      │ 调用 onCompressionDetailedProgress()
  │    （Worker 继续执行，不阻塞）         │ （安全地更新 UI 控件）
```

**为什么安全**：参数被拷贝到队列中，槽函数在接收者所在线程执行。所以 Worker 在子线程发信号，MainWindow 的槽函数在主线程更新 UI，不存在并发问题。

### 2.4 本项目中 connect 的三种用法

**① 信号 → 槽（跨线程通信）**：

```cpp
// MainWindow_Compression.cpp:479
connect(m_worker, &CompressionWorker::detailedProgress,
        this, &MainWindow::onCompressionDetailedProgress);
```

**② 信号 → 槽（同线程 UI 事件）**：

```cpp
// MainWindow_Compression.cpp:108
connect(addFileBtn, &QPushButton::clicked,
        this, &MainWindow::onAddFilesClicked);
```

**③ 信号 → Lambda（内联处理）**：

```cpp
// MainWindow_Compression.cpp:91-94
connect(m_fileListWidget, &QListWidget::itemSelectionChanged,
        this, [this]() {
    updateOutputDirectory();
    updateOutputFileName();
});
```

**④ 线程启动 → Worker 开始工作**：

```cpp
// MainWindow_Compression.cpp:478
connect(m_workerThread, &QThread::started,
        m_worker, &CompressionWorker::doCompression);
```

这里 `QThread::started` 是 QThread 的内置信号，线程启动后自动发出。

---

## 3. 多线程机制——moveToThread 模式

### 3.1 为什么需要多线程

Qt 的 GUI 运行在主线程（也叫 UI 线程）。如果在主线程执行耗时操作（压缩一个 1GB 的文件），事件循环被阻塞，窗口无法响应任何操作——拖不动、点不了、系统显示"未响应"。

### 3.2 Qt 的三种多线程方式对比

| 方式 | 特点 | 本项目是否使用 |
|------|------|--------------|
| 继承 `QThread` 重写 `run()` | 简单，但要求逻辑写在单个类的 `run()` 中 | 否 |
| `QObject::moveToThread()` | Worker 对象整个搬到子线程，所有槽函数自动在子线程执行 | **是** |
| `QtConcurrent::run()` | 一次性任务，不适合需要信号回传的场景 | 否 |

### 3.3 moveToThread 的完整生命周期

以压缩为例，完整时序如下：

```
用户点击"开始压缩"
│
▼
onStartCompressionClicked()                          ← 主线程
│
├─ 1. 校验参数
│     ├─ 文件列表非空？
│     ├─ 输出目录有效？
│     ├─ 密码已输入？
│     └─ 不通过则 QMessageBox::warning() 直接返回
│
├─ 2. 创建线程和 Worker
│     m_workerThread = new QThread();                ← 创建线程（此时未启动）
│     m_worker = new CompressionWorker();            ← 创建工作者（此时在主线程）
│
├─ 3. 设置参数
│     m_worker->setCompressionParams(files, outputDir,
│                                    fileName, password, mode);
│
├─ 4. 搬家！
│     m_worker->moveToThread(m_workerThread);
│     // 此后 m_worker 的所有槽函数都在 m_workerThread 中执行
│     // 但 m_worker 的指针在主线程仍然有效，可以调用非槽函数
│
├─ 5. 连接信号槽
│     connect(m_workerThread, &QThread::started,
│             m_worker, &CompressionWorker::doCompression);
│     connect(m_worker, &CompressionWorker::detailedProgress,
│             this, &MainWindow::onCompressionDetailedProgress);
│     connect(m_worker, &CompressionWorker::finished,
│             this, &MainWindow::onCompressionFinished);
│
├─ 6. 冻结 UI
│     m_startCompressBtn->setEnabled(false);
│     m_tabWidget->setTabEnabled(1, false);          ← 禁用另一个 Tab
│
└─ 7. 启动线程
      m_workerThread->start();
      // QThread 发出 started 信号
      // → 触发 doCompression() 在子线程中执行

                          子线程开始执行
                          │
                          ▼
                    doCompression()                    ← 子线程
                    │
                    ├─ validateCompressionParams()
                    ├─ StrategyFactory::createModules()
                    ├─ HeaderWriter::headerWriter()
                    ├─ CompressionLoop::compressionLoop()
                    │    └─ 进度回调中:
                    │         emit detailedProgress()  → 主线程更新进度条
                    │         if (isStopRequested())
                    │             throw ...            → 协作式取消
                    │
                    └─ emit finished(true/false, msg)  → 主线程收尾

                          主线程收到 finished 信号
                          │
                          ▼
                    onCompressionFinished()             ← 主线程
                    │
                    ├─ 1. 更新 UI
                    │     ├─ 恢复按钮状态
                    │     ├─ 显示成功/失败弹窗
                    │     └─ 更新进度条到 100% 或标记失败
                    │
                    ├─ 2. 停止并清理线程
                    │     m_workerThread->quit();       ← 请求线程退出事件循环
                    │     m_workerThread->wait();       ← 阻塞等待线程真正退出
                    │     delete m_workerThread;        ← 释放线程对象
                    │     m_workerThread = nullptr;
                    │
                    └─ 3. 清理 Worker
                          delete m_worker;             ← 释放工作者对象
                          m_worker = nullptr;
```

### 3.4 为什么用 moveToThread 而不是继承 QThread

`moveToThread` 的优势在于**职责分离**：

- `CompressionWorker` 只关心"怎么压缩"，不需要知道线程的存在
- `MainWindow` 负责线程的创建、管理和销毁
- Worker 的 `doCompression()` 是一个普通 slot，可以被线程启动信号触发，也可以被其他方式调用（如单元测试）

### 3.5 窗口关闭时的线程安全

`MainWindow` 的析构函数（`MainWindow.cpp:21-40`）必须正确清理线程：

```cpp
MainWindow::~MainWindow()
{
    if (m_worker) {
        m_worker->requestStop();             // ① 请求 Worker 停止
    }

    if (m_workerThread) {
        if (!m_workerThread->wait(3000)) {   // ② 最多等 3 秒
            m_workerThread->terminate();     // ③ 超时则强制终止（不推荐，但作为最后手段）
            m_workerThread->wait();          // ④ 等终止完成
        }
        delete m_workerThread;               // ⑤ 释放线程
    }

    if (m_worker) {
        delete m_worker;                     // ⑥ 释放 Worker
    }
}
```

**顺序很重要**：必须先 `requestStop()` 让 Worker 自行退出，再 `wait()` 等待，最后才能 `delete`。如果直接 `terminate()` 杀线程，Worker 可能正处于文件写入中间状态，导致数据损坏。

### 3.6 关键规则总结

1. **Worker 创建后、moveToThread 之前**设置所有参数，不能在 moveToThread 之后调用 Worker 的普通成员函数
2. **Worker 的 slot 函数**在子线程执行（由信号触发）
3. **Worker 的信号**通过队列传递到主线程
4. **主线程只做 UI 操作**，不调用 Worker 的耗时函数
5. **线程用完必须 quit + wait + delete**，否则资源泄漏

---

## 4. UI 搭建——纯代码布局

本项目没有使用 Qt Designer（.ui 文件），所有界面通过 C++ 代码构建。

### 4.1 布局系统

Qt 提供三种基本布局，本项目全部用到了：

| 布局类 | 排列方式 | 本项目使用位置 |
|--------|---------|--------------|
| `QVBoxLayout` | 垂直排列（从上到下） | 每个 Tab 的主布局、各 GroupBox 内部 |
| `QHBoxLayout` | 水平排列（从左到右） | 左右分栏、按钮行 |
| `QGridLayout` | 网格排列（行+列） | 输出设置区域（标签+输入框+浏览按钮） |

**布局示例——压缩 Tab 的结构**（`MainWindow_Compression.cpp:8-358`）：

```
CompressionTab
└── QVBoxLayout (mainVLayout)
    └── QHBoxLayout (contentLayout)          ← 左右分栏
        ├── leftColumn (QVBoxLayout)         ← 左侧 3:2 比例
        │   ├── QGroupBox "Files to Compress"
        │   │   └── QVBoxLayout
        │   │       ├── QListWidget           ← 文件列表
        │   │       └── QHBoxLayout           ← 按钮行
        │   │           ├── "Add Files" btn
        │   │           ├── "Add Folder" btn
        │   │           ├── "Remove" btn
        │   │           └── "Clear" btn
        │   ├── QGroupBox "Output Settings"
        │   │   └── QGridLayout               ← 表格式布局
        │   │       ├── (0,0) "Output Directory:"   (0,1) LineEdit   (0,2) "Browse" btn
        │   │       ├── (1,0) "Output Filename:"    (1,1-2) LineEdit
        │   │       └── (2,0) "Password:"           (2,1-2) LineEdit
        │   └── stretch                        ← 弹性空间，推内容往上
        └── rightColumn (QVBoxLayout)         ← 右侧 2:3 比例
            ├── QGroupBox "Compression Mode"
            │   └── QComboBox                 ← 模式下拉框
            ├── QGroupBox "Progress"
            │   └── QVBoxLayout
            │       ├── QLabel                ← 当前文件名
            │       ├── QProgressBar          ← 进度条
            │       ├── QLabel                ← 百分比文字
            │       └── QTextEdit             ← 日志
            └── QPushButton "Start Compression" ← 大按钮
```

### 4.2 比例控制

```cpp
// MainWindow_Compression.cpp:351-354
contentLayout->addWidget(leftColumn, 3);     // 左侧占 3 份
contentLayout->addWidget(rightColumn, 2);    // 右侧占 2 份
contentLayout->setStretch(0, 3);
contentLayout->setStretch(1, 2);
```

`addWidget(widget, stretch)` 的第二个参数是拉伸比例。左右比例为 3:2，即左栏占 60%，右栏占 40%。

### 4.3 样式表（QSS）

Qt 使用类似 CSS 的语法控制控件外观。本项目的 QSS 分散在各控件创建处：

```cpp
// MainWindow_Compression.cpp:17-36，按钮样式
"QPushButton { "
"   background: rgba(255, 255, 255, 160); "     // 半透明白色背景
"   border: 1px solid rgba(180, 180, 180, 180); "// 灰色边框
"   border-radius: 5px; "                        // 圆角
"   padding: 8px 18px; "                         // 内边距
"   font-weight: bold; "                         // 粗体
"   color: #4f5d6e; "                            // 文字颜色
"} "
"QPushButton:hover { "                           // 鼠标悬停
"   background: rgba(255, 255, 255, 200); "
"} "
"QPushButton:pressed { "                         // 按下
"   background: rgba(220, 220, 220, 180); "
"} "
"QPushButton:disabled { "                        // 禁用状态
"   background: rgba(200, 200, 200, 140); "
"   color: #8d98a6; "
"}"
```

QSS 的关键是 `rgba()` — 支持半透明，配合背景图片实现毛玻璃效果。

### 4.4 常用控件对照表

| Qt 控件 | 等价于 | 用途 |
|---------|-------|------|
| `QLabel` | HTML `<span>` | 显示文本 |
| `QLineEdit` | HTML `<input type="text">` | 单行文本输入 |
| `QTextEdit` | HTML `<textarea>` | 多行文本（用作日志区） |
| `QPushButton` | HTML `<button>` | 按钮 |
| `QListWidget` | HTML `<select multiple>` | 文件列表 |
| `QComboBox` | HTML `<select>` | 下拉选择（模式选择器） |
| `QProgressBar` | HTML `<progress>` | 进度条 |
| `QTabWidget` | 浏览器标签页 | 压缩/解压两个 Tab |
| `QGroupBox` | HTML `<fieldset>` | 分组框 |
| `QFileDialog` | 系统文件选择对话框 | 浏览文件/目录 |
| `QMessageBox` | 系统弹窗 | 成功/失败提示 |

---

## 5. 事件处理——拖拽与窗口缩放

### 5.1 拖拽事件

Qt 通过重写虚函数处理事件，类似 JavaScript 的 `addEventListener`：

```cpp
// MainWindow.h:57-58 声明重写
void dragEnterEvent(QDragEnterEvent *event) override;   // 拖入窗口时
void dropEvent(QDropEvent *event) override;             // 松开鼠标时

// MainWindow.cpp:113-143 实现
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {    // 判断拖入的是否是文件
        event->acceptProposedAction();     // 接受这个拖拽操作
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QList<QUrl> &urls = event->mimeData()->urls();
    const int currentTab = m_tabWidget->currentIndex();

    if (currentTab == 0) {                 // 压缩 Tab：把文件加入列表
        addDroppedPaths(urls);
    } else if (currentTab == 1) {          // 解压 Tab：把 .sy 文件填入输入框
        // ... 处理 .sy 文件路径 ...
    }
}
```

启用拖拽需要在构造时设置（`MainWindow.cpp:48`）：

```cpp
setAcceptDrops(true);
```

### 5.2 窗口缩放——背景图片自适应

`resizeEvent` 在窗口大小变化时被调用（`MainWindow.cpp:167-211`）：

```cpp
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);    // 先调用基类处理
    updateBackground();                 // 重新计算背景
}
```

`updateBackground()` 实现了类似 CSS `background-size: cover` 的效果：
1. 计算窗口和图片的宽高比
2. 按较大比例缩放图片，确保完全覆盖窗口
3. 居中裁剪多余部分
4. 设置为窗口调色板的背景画刷

---

## 6. 国际化——tr() 与 QTranslator

### 6.1 tr() 标记

所有面向用户的字符串用 `tr()` 包裹：

```cpp
setWindowTitle(tr("Compressor By Yonagi"));
QPushButton *addFileBtn = new QPushButton(tr("Add Files"));
```

`tr()` 在运行时查找翻译表，如果没找到则返回原始英文。因此写英文原文 + tr() 标记 = 默认英文界面 + 可翻译为中文。

### 6.2 翻译加载流程

`main.cpp` 中的 `loadApplicationTranslator()` 实现了翻译加载：

```
1. 检测系统语言（QLocale::system()）
2. 如果是中文，构建候选列表 ["zh_CN", "zh"]
3. 依次尝试加载翻译文件：
   ├─ 尝试 <exe目录>/translations/SimpleFilesCompressorGUI_zh_CN.qm
   └─ 尝试 Qt 资源 :/i18n/SimpleFilesCompressorGUI_zh_CN.qm
4. 加载成功则 installTranslator()，此后所有 tr() 返回中文
```

### 6.3 Worker 消息的特殊翻译

Worker 运行在子线程，发出的消息是英文硬编码字符串。`localizeWorkerDialogMessage()`（`MainWindow.cpp:244-330`）负责将这些字符串匹配并翻译：

```cpp
const QString fileNotFoundPrefix = QStringLiteral("File not found: ");
if (message.startsWith(fileNotFoundPrefix)) {
    return translate("File not found: %1").arg(message.mid(fileNotFoundPrefix.size()));
}
```

这种间接翻译的原因是：Worker 的消息在子线程中构建，不能直接调用 `tr()`（`tr()` 依赖于 GUI 线程的翻译器上下文）。

---

## 7. 策略工厂与智能指针的具体实现

### 7.1 设计目标

用户在 GUI 中选择压缩模式后，需要根据模式组合不同的"压缩算法 + 加密算法"。这些算法必须满足：

1. **运行时可替换** — 不同模式用不同组合
2. **调用方无需知道具体类型** — CompressionLoop 只依赖接口
3. **自动管理内存** — 不能忘记 delete

### 7.2 接口定义

```cpp
// ICompression 接口
class ICompression {
public:
    virtual ~ICompression() = default;
    virtual void compress(const DataBlock &input, DataBlock &output) = 0;
    virtual void decompress(const DataBlock &input, DataBlock &output) = 0;
};

// IEncryption 接口
class IEncryption {
public:
    virtual ~IEncryption() = default;
    virtual void encrypt(const DataBlock &input, DataBlock &output) = 0;
    virtual void decrypt(const DataBlock &input, DataBlock &output) = 0;
};
```

四个具体实现类：

| 类 | 实现的接口 | 行为 |
|----|-----------|------|
| `HuffmanCompression` | `ICompression` | 哈夫曼压缩/解压 |
| `NullCompression` | `ICompression` | 数据直接透传（9字节桩格式） |
| `AesEncryption` | `IEncryption` | AES 加密/解密 |
| `NullEncryption` | `IEncryption` | 恒等拷贝（inline 空操作） |

### 7.3 StrategyModules — 智能指针持有策略组合

```cpp
// StrategyFactory.h:16-20
struct StrategyModules
{
    std::unique_ptr<ICompression> compression;    // 独占所有权的压缩策略
    std::unique_ptr<IEncryption> encryption;      // 独占所有权的加密策略
};
```

**为什么用 `unique_ptr` 而不是裸指针或 `shared_ptr`**：

| 方案 | 问题 |
|------|------|
| `ICompression*` 裸指针 | 谁负责 delete？工厂创建但调用方使用，生命周期管理混乱 |
| `shared_ptr` | 所有权共享，但这里所有权是独占的——工厂创建、Worker 使用、用完销毁，不存在多方共享 |
| **`unique_ptr`** | 所有权明确：工厂创建 → Worker 持有 → StrategyModules 析构时自动 delete |

`unique_ptr` 的核心特性：
- **独占所有权**：同一时刻只能有一个 `unique_ptr` 指向对象
- **零开销**：编译后与裸指针一样快，没有引用计数的开销
- **自动释放**：`StrategyModules` 析构时，`unique_ptr` 的析构函数自动调用 `delete`
- **可移动**：通过 `std::move()` 转移所有权，但不能复制

### 7.4 StrategyFactory::createModules() — 工厂如何组装策略

```cpp
// StrategyFactory.cpp:33-61
StrategyModules StrategyFactory::createModules(CompressionMode mode, const std::string &password)
{
    StrategyModules modules;    // ① 创建空的模块容器

    switch (mode)
    {
    case CompressionMode::HuffmanAES:    // ② 根据模式进入对应分支
        modules.compression = std::make_unique<HuffmanCompression>(1);
        // ③ make_unique 在堆上创建对象，返回 unique_ptr
        modules.encryption = std::make_unique<OwnedAesEncryption>(
            std::make_unique<Aes>(password.c_str()));
        // ④ 嵌套的 make_unique：先创建 Aes 对象，再传给 OwnedAesEncryption
        break;

    case CompressionMode::HuffmanOnly:
        modules.compression = std::make_unique<HuffmanCompression>(1);
        modules.encryption = std::make_unique<NullEncryption>();
        // ⑤ NullEncryption 是空操作，不需要参数
        break;

    case CompressionMode::AESOnly:
        modules.compression = std::make_unique<NullCompression>();
        modules.encryption = std::make_unique<OwnedAesEncryption>(
            std::make_unique<Aes>(password.c_str()));
        break;

    case CompressionMode::PackOnly:
        modules.compression = std::make_unique<NullCompression>();
        modules.encryption = std::make_unique<NullEncryption>();
        // ⑥ 两个都是空操作，数据直接透传
        break;
    }

    return modules;    // ⑦ 返回时触发移动语义（不是拷贝！）
}
```

**第 ⑦ 步的关键细节**：`StrategyModules` 包含 `unique_ptr` 成员，`unique_ptr` 不可复制，但可以移动。编译器自动使用移动构造函数，将 `modules` 中的 `unique_ptr` 转移到返回值中。整个过程零拷贝。

### 7.5 OwnedAesEncryption — 解决裸指针的生命周期问题

这是整个策略工厂中最精巧的设计。

**问题**：`AesEncryption` 类的构造函数接收 `Aes*` 裸指针但不拥有它：

```cpp
class AesEncryption : public IEncryption {
    Aes* aes_;    // 裸指针，不拥有！
public:
    AesEncryption(Aes* aes) : aes_(aes) {}
    // ...
};
```

如果直接写：
```cpp
modules.encryption = std::make_unique<AesEncryption>(new Aes(password));
// 问题：谁 delete 这个 Aes？AesEncryption 不会 delete 它！
```

**解决方案**：在匿名命名空间中创建一个包装类（`StrategyFactory.cpp:9-27`）：

```cpp
namespace   // 匿名命名空间，只在此文件可见
{
    class OwnedAesEncryption : public Y_flib::IEncryption
    {
        std::unique_ptr<Aes> ownedAes;     // 拥有 Aes 对象
        Y_flib::AesEncryption impl;        // 实际干活的加密器

    public:
        OwnedAesEncryption(std::unique_ptr<Aes> aes)
            : ownedAes(std::move(aes))     // 接管 Aes 的所有权
            , impl(ownedAes.get())         // 把裸指针给 AesEncryption 使用
        {}

        void encrypt(const DataBlock &input, DataBlock &output) override
        {
            impl.encrypt(input, output);   // 委托给实际实现
        }

        void decrypt(const DataBlock &input, DataBlock &output) override
        {
            impl.decrypt(input, output);   // 委托给实际实现
        }
    };
}
```

**所有权链**：

```
StrategyModules::encryption (unique_ptr<IEncryption>)
    └── 指向 OwnedAesEncryption 对象
            ├── ownedAes (unique_ptr<Aes>)   ← 拥有并管理 Aes 生命周期
            └── impl (AesEncryption)         ← 使用 ownedAes.get() 提供的裸指针
                    └── aes_ (Aes*)           ← 裸指针，指向 ownedAes 管理的对象
```

**析构顺序**（由 C++ 成员析构顺序保证）：
1. `OwnedAesEncryption` 析构 → `impl` 先析构（后声明先析构）→ `ownedAes` 后析构
2. `impl` 析构时，`aes_` 裸指针变为悬空指针，但 `impl` 的析构函数不访问它
3. `ownedAes` 析构时，`delete Aes` 对象

这个顺序是安全的：使用者在 Aes 之前析构，不会出现访问已释放内存的问题。

**为什么放在匿名命名空间**：`OwnedAesEncryption` 是一个内部实现细节，不需要暴露给其他文件。匿名命名空间相当于 `static`，限制其可见性为当前编译单元。

### 7.6 调用方如何使用——完全不知道底层类型

`CompressionWorker::doCompression()` 中（`CompressionWorker.cpp:167,207`）：

```cpp
auto modules = Y_flib::StrategyFactory::createModules(m_mode, password);
// ...
compressor.compressionLoop(filePathToScan, *modules.encryption, *modules.compression, m_mode);
```

`CompressionLoop` 的接口：

```cpp
void compressionLoop(const std::vector<std::string>& paths,
                     IEncryption& encryption,      // 接口引用，不是具体类
                     ICompression& compression,    // 接口引用，不是具体类
                     CompressionMode mode);
```

**解引用 `*modules.encryption`**：`modules.encryption` 是 `unique_ptr<IEncryption>`，`*` 得到 `IEncryption&` 引用。`CompressionLoop` 只看到接口，完全不知道底层是 `OwnedAesEncryption` 还是 `NullEncryption`。

这就是策略模式的核心——**算法与使用方解耦**。未来如果增加 LZ77 压缩或 ChaCha20 加密，只需：
1. 新增 `Lz77Compression : public ICompression`
2. 在 `StrategyFactory::createModules()` 中增加新的 case 分支
3. CompressionLoop 和 CompressionWorker 不需要任何修改

### 7.7 解压时的策略自动识别

压缩时，策略号被写入文件头（`Header.strategy`）。解压时，不需要用户选择模式，而是自动读取：

```cpp
// CompressionWorker.cpp:295-301
Y_flib::Header fileHeader;
std::memcpy(&fileHeader, headerBuf.data(), sizeof(Y_flib::Header));

const Y_flib::CompressionMode detectedMode =
    Y_flib::StrategyFactory::idToMode(fileHeader.strategy);

// CompressionWorker.cpp:303
if (Y_flib::StrategyFactory::hasEncryption(detectedMode) && m_decompressPassword.isEmpty())
{
    // 如果需要加密但没提供密码，提示用户
}
```

`idToMode()` 将数字 ID 映射回枚举值：

```cpp
// StrategyFactory.h:28-37
static CompressionMode idToMode(CompressStrategy id)
{
    switch (id)
    {
    case 0: return CompressionMode::HuffmanAES;
    case 1: return CompressionMode::HuffmanOnly;
    case 2: return CompressionMode::AESOnly;
    case 3: return CompressionMode::PackOnly;
    default: throw std::runtime_error("Unsupported strategy: " + std::to_string(id));
    }
}
```

这样旧版本生成的 `.sy` 文件（策略号 0 = HuffmanAES）也能被新版本正确解压。

### 7.8 内存生命周期全景图

```
createModules() 返回 StrategyModules
    │
    │  返回值通过移动语义传递（零拷贝）
    ▼
modules 变量在 doCompression() 栈上
    │
    ├─ modules.compression → unique_ptr → HuffmanCompression 或 NullCompression
    └─ modules.encryption  → unique_ptr → OwnedAesEncryption 或 NullEncryption
                                        └─ (OwnedAesEncryption 内部)
                                            └─ ownedAes → unique_ptr → Aes
    │
    │  压缩循环中使用 *modules.compression 和 *modules.encryption
    ▼
doCompression() 函数结束，modules 离开作用域
    │
    ├─ StrategyModules 析构
    │   ├─ compression (unique_ptr) 析构 → delete HuffmanCompression/NullCompression
    │   └─ encryption (unique_ptr) 析构 → delete OwnedAesEncryption/NullEncryption
    │       └─ (OwnedAesEncryption 析构)
    │           ├─ impl 析构（不 delete Aes）
    │           └─ ownedAes (unique_ptr) 析构 → delete Aes
    │
    └─ 全部自动完成，无需手动 delete
```

---

## 8. 原子变量与协作式取消

### 8.1 停止标志

```cpp
// CompressionWorker.h:74
std::atomic<bool> m_stopRequested{false};
```

`std::atomic<bool>` 保证在多线程环境下的读写安全：
- 主线程调用 `requestStop()` 设置为 `true`
- 子线程在各检查点读取值，发现为 `true` 则退出

### 8.2 检查点的位置

`doCompression()` 中有多处检查点（`CompressionWorker.cpp:133,159,169,181,209`）：

```cpp
if (isStopRequested())
{
    emit finished(false, tr("Compression cancelled by user"));
    return;
}
```

压缩循环内部通过回调检查（`CompressionWorker.cpp:193-196`）：

```cpp
compressor.setProgressCallback([this](...) {
    if (isStopRequested())
    {
        throw std::runtime_error("Operation cancelled by user");
    }
    // ...
});
```

**为什么用异常而不是 return**：进度回调是从 `CompressionLoop` 内部调用的，回调函数无法直接让外层函数 return。抛出异常可以穿越多层调用栈回到 `doCompression()` 的 catch 块。

### 8.3 为什么是"协作式"而非"强制式"

```
协作式取消                         强制式取消（如 terminate()）
├─ Worker 在安全点自行检查          ├─ 外部强行杀线程
├─ 可以清理资源（关闭文件等）        ├─ 可能在任意位置被杀
├─ 数据一致性有保证                 ├─ 可能留下半写文件
└─ 缺点：响应有延迟                └─ 缺点：危险，仅作最后手段
```

---

## 9. 进度节流机制

### 9.1 为什么需要节流

压缩循环每处理一个数据块就回调一次进度。如果文件很小、块很多（比如几万个 1KB 文件），每秒可能产生上千次回调。每次回调都 `emit` 信号会导致 Qt 事件队列堆积，UI 线程忙于处理进度更新而卡顿。

### 9.2 实现细节

```cpp
// CompressionWorker.cpp:12-28
bool CompressionWorker::shouldEmitProgress(double currentProgress)
{
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_lastProgressTime).count();

    const bool shouldEmit = (elapsed >= PROGRESS_INTERVAL_MS) ||      // 距上次 ≥ 200ms
                            (currentProgress - m_lastEmittedProgress
                             >= PROGRESS_DELTA) ||                     // 进度变化 ≥ 2%
                            (currentProgress >= 100.0);                // 到 100% 必发

    if (shouldEmit)
    {
        m_lastProgressTime = now;
        m_lastEmittedProgress = currentProgress;
    }

    return shouldEmit;
}
```

三个条件任一满足即发送：
1. **时间条件**（200ms）：保证 UI 有最低更新频率
2. **变化条件**（2%）：保证进度条看起来在动
3. **完成条件**（100%）：保证最终状态一定被发送

### 9.3 进度映射

压缩循环的 `overallProgress` 范围是 0~100，但前面有参数验证和文件头写入（约占 20%），所以需要线性映射：

```cpp
// 压缩：前 20% 用于准备，后 80% 用于实际压缩
const double mappedProgress = 20.0 + overallProgress * 0.75;
// overallProgress 0   → mappedProgress 20%
// overallProgress 100 → mappedProgress 95%
// 最后 5% 留给图标关联和收尾

// 解压：前 15% 用于准备，后 85% 用于实际解压
const double mappedProgress = 15.0 + overallProgress * 0.80;
```

---

## 10. 文件摘要

| 文件 | 行数 | 核心职责 |
|------|------|---------|
| `main.cpp` | 157 | 程序入口、高 DPI 设置、国际化加载、字体配置 |
| `MainWindow.h` | 132 | 主窗口声明：控件指针、槽函数、Worker 线程指针 |
| `MainWindow.cpp` | 331 | 构造/析构、拖拽事件、背景缩放、路径工具、消息翻译 |
| `MainWindow_Compression.cpp` | 606 | 压缩 Tab UI 搭建 + 压缩流程的信号槽调度 |
| `MainWindow_Decompression.cpp` | 389 | 解压 Tab UI 搭建 + 解压流程的信号槽调度 |
| `CompressionWorker.h` | 80 | Worker 声明：参数、信号、原子停止标志、节流控制 |
| `CompressionWorker.cpp` | 380 | Worker 实现：参数校验、压缩/解压主流程、进度控制 |
| `PlaceholderLineEdit.h` | 67 | 自定义输入框：空内容时绘制半透明提示文字 |
| `StrategyFactory.h` | 49 | 策略工厂声明：模块结构体、模式转换、加密检测 |
| `StrategyFactory.cpp` | 63 | 策略工厂实现：模式→模块映射、OwnedAesEncryption 包装 |
