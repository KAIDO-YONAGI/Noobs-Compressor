# 第三方组件与许可 / Third-Party Notices

本文件列出 SFC（Simple Files Compressor）使用或随包分发的第三方组件及其许可。
**本项目自身代码的许可是 GPL-3.0**，完整原文见 [LICENSE](LICENSE)；
第三方组件不在该授权范围内，各自保留其许可证。

## 1. Qt 6.2.4（LGPL v3，静态链接）
This application uses Qt 6.2.4, licensed under the GNU Lesser General
Public License v3.0 (LGPL v3).

In compliance with LGPL v3 Section 6, the object files (.o) required for
relinking this application with modified versions of the Qt libraries are
available upon request or bundled alongside this distribution as:
  qt-6.2.4-object-files.tar

The Qt 6.2.4 source code is available at:
  https://download.qt.io/archive/qt/6.2/6.2.4/single/

Build configuration:
  configure -static -release -optimize-size -ltcg -opensource -confirm-license
  -platform win32-g++ -no-feature-opengl -no-feature-dbus -no-feature-sql
  -qt-libpng -qt-libjpeg -qt-freetype -qt-pcre -qt-doubleconversion

For the full text of LGPL v3:
  https://www.gnu.org/licenses/lgpl-3.0.txt

> 维护提示：`qt-6.2.4-object-files.tar` 目前**不在仓库中**。若发布二进制，
> 需按 LGPL v3 第 4 条(d)(0) 与 GPL v3 第 6 条二选一履行义务：
> ① 随发布包附带该对象文件归档；或 ② 附书面要约（承诺三年内应请求提供）。
> 二者都做更稳妥。

## 2. 本项目自带的美术与文档资源

| 资源 | 位置 | 来源 |
| --- | --- | --- |
| 应用图标 `YONAGII_512x512.ico` | `SFC_GUI/` | 项目作者 |
| 界面图标（由 `resources.qrc` 引用） | `SFC_GUI/` | 项目作者 |
| 流程图 / UML / 结构图 | `DevFiles/FlowCharts/` | 项目作者 |
| 中文翻译 `.ts` / `.qm` | `SFC_GUI/translations/` | 项目作者 |

下表为**当前版本**随仓库分发的资源，均为项目作者自制。

界面背景为**可选**：`SFC_GUI/resources.qrc` 未引用背景图，应用在无背景图的情况下正常运行。
若要自定义背景，请放入你自己拥有权利的图片，并在该文件中登记。

GPL-3.0 只覆盖作者拥有版权的部分，覆盖不了他人素材。若你发现本仓库中任何资源的来源或授权
存在问题，请提交 issue，我们会核实并处理。
## 3. 压缩与加密实现

按仓库内源码与文档，Huffman 压缩与 AES 加密均为本项目自行实现，加密经 Windows 提供的
加密服务（CSP）实现，未引入第三方压缩 / 加密库；构建仅依赖 Qt 与 CMake / 编译器工具链。

---

本文件是授权事实的汇总，**不是法律意见**。