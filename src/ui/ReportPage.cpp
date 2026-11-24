#include "ReportPage.h"
#include "AppContext.h"
#include "Components.h"
#include "ExperimentPages.h"
#include "ReportDocument.h"
#include "ExportService.h"
#include "LanguageManager.h"
#include "RecentExportPanel.h"
#include "ThemeManager.h"
#include "ui_ReportPage.h"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDialog>
#include <QLocale>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QLabel>
#include <QPageLayout>
#include <QPageSize>
#include <QPalette>
#include <QPainter>
#include <QPen>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QShowEvent>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QToolButton>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <algorithm>

namespace phy104 {
namespace {

QString localized(const QString& source) {
    return LanguageManager::instance().text(source);
}

std::string localized(const std::string& source) {
    static const std::string actionPrefix = "ACTION: ";
    if (source.rfind(actionPrefix, 0) == 0)
        return localized(QStringLiteral("ACTION: ")).toStdString() + localized(source.substr(actionPrefix.size()));
    return localized(QString::fromStdString(source)).toStdString();
}

/// Renders a widget (even a hidden one inside the page stack) into a 2x image for print quality.
QImage grabWidget(QWidget* widget) {
    if (!widget || widget->width() < 60 || widget->height() < 60) return {};
    constexpr qreal scale = 2.0;
    QImage image(widget->size() * scale, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.scale(scale, scale);
    widget->render(&painter);
    painter.end();
    return image;
}

} // namespace

ReportPage::ReportPage(AppContext* ctx, QWidget* parent)
    : BasePage("PDF Report", "PRINT / Notes, registers, diagrams and charts of every experiment in one document",
               ctx, parent, false) {
    // ---------------------------------------------------------------- Designer-defined report workspaces; live widgets plug into explicit form slots
    auto* reportFormHost = new QWidget;
    Ui::ReportPage ui;
    ui.setupUi(reportFormHost);
    content()->addWidget(reportFormHost, 1);

    ui.optionsPanel->setObjectName("panel");
    ui.optionsHeader->setObjectName("panelHeader");
    ui.optionsBody->setObjectName("panelBody");
    ui.optionsTitle->setObjectName("panelTitle");
    ui.optionsMeta->setObjectName("panelMeta");
    ui.includeLabel->setObjectName("caption");
    ui.paperLabel->setObjectName("caption");
    ui.optionsHint->setObjectName("hint");
    m_chkDiagrams = ui.diagramsCheck;
    m_chkCharts = ui.chartsCheck;
    m_chkRegs = ui.registersCheck;
    m_chkNotes = ui.notesCheck;
    m_chkRuns = ui.runsCheck;
    for (QCheckBox* check : {m_chkDiagrams, m_chkCharts, m_chkRegs, m_chkNotes, m_chkRuns})
        connect(check, &QCheckBox::toggled, this, [this] { refreshPreview(); });
    m_paper = new ModeTabs({"A4 portrait", "A4 landscape"});
    ui.paperTabsLayout->addWidget(m_paper);
    connect(m_paper, &ModeTabs::currentChanged, this, [this] { refreshPreview(); });

    ui.previewPanel->setObjectName("panel");
    ui.previewHeader->setObjectName("panelHeader");
    ui.previewBody->setObjectName("panelBody");
    ui.previewTitle->setObjectName("panelTitle");
    ui.previewMeta->setObjectName("panelMeta");
    m_previewMeta = ui.previewMeta;
    m_preview = ui.reportPreview;
    const auto applyPreviewTheme = [this] {
        const Theme& theme = ThemeManager::instance().theme();
        m_preview->setStyleSheet(QStringLiteral(
            "QTextBrowser#reportPreview { background: %1; color: %2; border: 1px solid %3; }")
            .arg(theme.surface.name(), theme.text.name(), theme.border.name()));
        QPalette palette = m_preview->palette();
        palette.setColor(QPalette::Base, theme.surface);
        palette.setColor(QPalette::Text, theme.text);
        m_preview->setPalette(palette);
    };
    applyPreviewTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, applyPreviewTheme);

    ui.exportPanel->setObjectName("panel");
    ui.exportHeader->setObjectName("panelHeader");
    ui.exportBody->setObjectName("panelBody");
    ui.exportTitle->setObjectName("panelTitle");
    ui.exportMeta->setObjectName("panelMeta");
    ui.fileNameLabel->setObjectName("fieldLabel");
    ui.folderLabel->setObjectName("fieldLabel");
    ui.exportHint->setObjectName("hint");
    m_fileName = new UnitField(".pdf");
    m_fileName->setText(suggestedName());
    ui.fileNameFieldLayout->addWidget(m_fileName);
    m_folder = ui.folderCombo;
    m_folder->addItem("Documents / PHY104", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/PHY104");
    m_folder->addItem("Desktop", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    m_folder->addItem("Choose folder…", QString());
    connect(m_folder, &QComboBox::activated, this, [this](int index) {
        if (m_folder->itemData(index).toString().isEmpty()) {
            const QString dir = QFileDialog::getExistingDirectory(this, "Destination folder");
            if (!dir.isEmpty()) {
                m_folder->insertItem(index, dir, dir);
                m_folder->setCurrentIndex(index);
            } else {
                m_folder->setCurrentIndex(0);
            }
        }
    });
    m_ready = new NoticeCard;
    ui.readyCardLayout->addWidget(m_ready);
    m_save = ui.savePdfButton;
    m_save->setObjectName("primary");
    m_print = ui.printButton;
    m_print->setObjectName("secondary");
    m_recent = new RecentExportPanel;
    ui.recentExportLayout->addWidget(m_recent);

    connect(m_save, &QPushButton::clicked, this, &ReportPage::savePdf);
    connect(m_print, &QPushButton::clicked, this, &ReportPage::printReport);
    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(250);                                      // the session publishes on every keystroke
    connect(m_refreshTimer, &QTimer::timeout, this, &ReportPage::refreshPreview);
    connect(ctx, &AppContext::snapshotChanged, this, [this] { if (isVisible()) m_refreshTimer->start(); });
    connect(ctx, &AppContext::runsChanged, this, [this] { if (isVisible()) m_refreshTimer->start(); });
    connect(ctx, &AppContext::notesSaved, this, [this] { if (isVisible()) m_refreshTimer->start(); });
    connect(ctx, &AppContext::exportRecorded, this, &ReportPage::refreshRecent);

    StatusInfo st;
    st.left = "Report ready";
    st.mid = "A4 · PDF";
    st.right = "Local files only";
    setStatusInfo(st, false);

    refreshPreview();
    refreshRecent();
}

void ReportPage::showEvent(QShowEvent* event) {
    BasePage::showEvent(event);
    refreshPreview();                                                      // the screens changed while we were away
}

void ReportPage::setSources(const QList<ExperimentPage*>& pages) {
    m_sources = pages;
    refreshPreview();
}

QString ReportPage::suggestedName() const {
    return QString::fromStdString(reportFileStem(QDate::currentDate().toString("yyyy-MM-dd").toStdString()));
}

QString ReportPage::destinationFolder() const { return m_folder->currentData().toString(); }

ReportOptions ReportPage::options(int imageWidth) const {
    ReportOptions o;
    const bool arabic = LanguageManager::instance().isArabic();
    const QLocale dateLocale = arabic ? QLocale(QLocale::Arabic, QLocale::Egypt) : QLocale::c();
    o.generated = dateLocale.toString(QDateTime::currentDateTime(), "d MMMM yyyy · HH:mm").toStdString();
    const QString sessionName = QFileInfo(m_ctx->sessionFile).fileName();
    o.sessionName = (sessionName.isEmpty() ? localized(QStringLiteral("Unsaved session")) : sessionName).toStdString();
    o.title = localized(QStringLiteral("PHY104 Laboratory Report")).toStdString();
    o.subtitle = localized(QStringLiteral("Optical Assistant · PHY 104 · Optics")).toStdString();
    o.rightToLeft = arabic;
    o.labels.mastheadOrganization = localized(QStringLiteral("PHY104 · OPTICS LABORATORY")).toStdString();
    o.labels.mastheadReport = localized(QStringLiteral("LAB REPORT")).toStdString();
    o.labels.generated = localized(QStringLiteral("GENERATED")).toStdString();
    o.labels.session = localized(QStringLiteral("SESSION")).toStdString();
    o.labels.experiments = localized(QStringLiteral("EXPERIMENTS")).toStdString();
    o.labels.result = localized(QStringLiteral("RESULT")).toStdString();
    o.labels.figureFormat = localized(QStringLiteral("Figure %1.")).toStdString();
    o.labels.opticalDiagramFormat = localized(QStringLiteral("Optical diagram — %1.")).toStdString();
    o.labels.dataChartFormat = localized(QStringLiteral("Data chart — %1.")).toStdString();
    o.labels.experimentalParameters = localized(QStringLiteral("Experimental parameters")).toStdString();
    o.labels.sessionNotes = localized(QStringLiteral("Session notes")).toStdString();
    o.labels.savedCalculations = localized(QStringLiteral("Saved calculations")).toStdString();
    o.labels.tableExperiment = localized(QStringLiteral("Experiment")).toStdString();
    o.labels.tableInputs = localized(QStringLiteral("Inputs")).toStdString();
    o.labels.tableResult = localized(QStringLiteral("Result")).toStdString();
    o.labels.nothingSelected = localized(QStringLiteral("Nothing selected yet: open an experiment or tick more sections.")).toStdString();
    o.labels.footerFormat = localized(QStringLiteral("PHY104 Optical Assistant · %1")).toStdString();
    o.labels.pageFormat = localized(QStringLiteral("Page %1 of %2")).toStdString();
    o.notes = m_chkNotes->isChecked();
    o.registers = m_chkRegs->isChecked();
    o.diagrams = m_chkDiagrams->isChecked();
    o.charts = m_chkCharts->isChecked();
    o.runs = m_chkRuns->isChecked();
    o.imageWidth = imageWidth;
    return o;
}

ReportContent ReportPage::collect(QMap<QString, QImage>& images) const {
    ReportContent content;
    content.notes = m_ctx->log.notes(); // Student-authored Markdown is intentionally never passed through i18n.
    content.runs = m_ctx->log.runs();
    for (RunRecord& run : content.runs) {
        run.experiment = localized(run.experiment);
        run.description = localized(run.description);
        run.result = localized(run.result);
    }
    for (ExperimentPage* page : m_sources) {
        if (!page) continue;
        const QString name = page->experimentName();
        const std::string sourceName = name.toStdString();
        const Snapshot* snap = m_ctx->log.snapshot(sourceName);
        if (!snap) continue;
        ReportSection section;
        section.title = localized(sourceName);
        section.headline = localized(snap->headline);
        section.parameters = snap->parameters;
        for (auto& [label, value] : section.parameters) {
            label = localized(label);
            value = localized(value);
        }
        section.registers = snap->registers;
        for (LcdRegister& reg : section.registers) {
            reg.title = localized(reg.title);
            reg.resultLabel = localized(reg.resultLabel);
            reg.resultValue = localized(reg.resultValue);
            for (std::string& line : reg.lines) line = localized(line);
            for (std::string& line : reg.extraLines) line = localized(line);
        }
        if (m_chkDiagrams->isChecked()) {
            const QString key = QString::fromStdString(reportImageKey("diagram", sourceName));
            QImage image = grabWidget(page->diagramCanvas());
            if (!image.isNull()) {
                section.diagramKey = key.toStdString();
                images.insert(key, image);
            }
        }
        if (m_chkCharts->isChecked()) {
            const QString key = QString::fromStdString(reportImageKey("chart", sourceName));
            QImage image = grabWidget(page->chartWidget());
            if (!image.isNull()) {
                section.chartKey = key.toStdString();
                images.insert(key, image);
            }
        }
        content.sections.push_back(section);
    }
    return content;
}

void ReportPage::refreshPreview() {
    QMap<QString, QImage> images;
    const ReportContent content = collect(images);
    m_html = QString::fromStdString(reportHtml(content, options(380)));       // preview column width
    m_sections = static_cast<int>(content.sections.size());

    QTextDocument* doc = m_preview->document();
    doc->clear();
    for (auto it = images.cbegin(); it != images.cend(); ++it)
        doc->addResource(QTextDocument::ImageResource, QUrl(it.key()), it.value());
    QTextCursor cursor(doc);
    cursor.insertHtml(m_html);

    m_previewMeta->setText(m_sections == 1 ? "1 EXPERIMENT" : QString("%1 EXPERIMENTS").arg(m_sections));
    const bool anything = m_sections > 0 || (m_chkNotes->isChecked() && !content.notes.empty()) ||
                          (m_chkRuns->isChecked() && !content.runs.empty());
    m_save->setEnabled(anything);
    m_print->setEnabled(anything);
    if (!anything)
        m_ready->setNotice("Nothing to print yet", "Run a calculation, write a note or tick another section.", true);
    else
        m_ready->setNotice(QString("%1 experiment(s) · %2 run(s)").arg(m_sections).arg(static_cast<int>(content.runs.size())),
                           "Diagrams and charts are captured from the live screens, so what you see is what prints.", false);
}

bool ReportPage::renderTo(QPrinter& printer) {
    QMap<QString, QImage> images;
    const ReportContent content = collect(images);
    ReportOptions o = options(520);                                          // A4 text width
    o.showMasthead = false;                                                  // the repeated QPainter header replaces the HTML masthead

    // Use a 96-dpi logical page so the HTML's pixel-sized diagrams remain readable; PDF text stays vector.
    printer.setResolution(96);
    printer.setFullPage(true);
    const QRectF pagePoints = printer.pageLayout().paintRect(QPageLayout::Point);
    const qreal pixelsPerPoint = static_cast<qreal>(printer.resolution()) / 72.0;
    const QRectF page(pagePoints.left() * pixelsPerPoint, pagePoints.top() * pixelsPerPoint,
                      pagePoints.width() * pixelsPerPoint, pagePoints.height() * pixelsPerPoint);
    constexpr qreal headerHeight = 30.0;
    constexpr qreal footerHeight = 26.0;
    const qreal bodyHeight = page.height() - headerHeight - footerHeight;
    if (page.width() < 120 || bodyHeight < 120) return false;

    QTextDocument doc;
    QFont bodyFont;
    bodyFont.setFamilies(ThemeManager::uiFontFamilies());
    bodyFont.setPointSize(10);
    doc.setDefaultFont(bodyFont);
    doc.setDocumentMargin(0);
    doc.setPageSize(QSizeF(page.width(), bodyHeight));
    for (auto it = images.cbegin(); it != images.cend(); ++it)
        doc.addResource(QTextDocument::ImageResource, QUrl(it.key()), it.value());
    QTextCursor cursor(&doc);
    cursor.insertHtml(QString::fromStdString(reportHtml(content, o)));
    const int pages = std::max(1, doc.pageCount());

    QPainter painter;
    if (!painter.begin(&printer)) return false;
    const QColor accent("#07566b"), rule("#cbd5e1"), muted("#52606d");
    for (int pageIndex = 0; pageIndex < pages; ++pageIndex) {
        if (pageIndex > 0 && !printer.newPage()) { painter.end(); return false; }
        painter.save();
        painter.setPen(accent);
        QFont headerFont;
        headerFont.setFamilies(ThemeManager::uiFontFamilies());
        headerFont.setPointSize(8);
        headerFont.setWeight(QFont::DemiBold);
        painter.setFont(headerFont);
        const QRectF header(page.left(), page.top() + 2, page.width(), headerHeight - 4);
        const Qt::Alignment leading = o.rightToLeft ? Qt::AlignRight : Qt::AlignLeft;
        const Qt::Alignment trailing = o.rightToLeft ? Qt::AlignLeft : Qt::AlignRight;
        painter.drawText(header, leading | Qt::AlignVCenter, QString::fromStdString(o.labels.mastheadOrganization));
        painter.drawText(header, trailing | Qt::AlignVCenter, QString::fromStdString(o.labels.mastheadReport));
        painter.setPen(QPen(rule, 0.8));
        const qreal headerRuleY = page.top() + headerHeight - 1;
        painter.drawLine(QPointF(page.left(), headerRuleY), QPointF(page.right(), headerRuleY));

        painter.save();
        painter.setClipRect(QRectF(page.left(), page.top() + headerHeight, page.width(), bodyHeight));
        painter.translate(page.left(), page.top() + headerHeight - pageIndex * bodyHeight);
        doc.drawContents(&painter, QRectF(0, pageIndex * bodyHeight, page.width(), bodyHeight));
        painter.restore();

        painter.setPen(QPen(rule, 0.8));
        const qreal footerRuleY = page.bottom() - footerHeight + 1;
        painter.drawLine(QPointF(page.left(), footerRuleY), QPointF(page.right(), footerRuleY));
        painter.setPen(muted);
        QFont footerFont;
        footerFont.setFamilies(ThemeManager::uiFontFamilies());
        footerFont.setPointSize(8);
        painter.setFont(footerFont);
        const QRectF footer(page.left(), footerRuleY + 2, page.width(), footerHeight - 2);
        const QString footerText = QString::fromStdString(o.labels.footerFormat).arg(QString::fromStdString(o.generated));
        const QString pageText = QString::fromStdString(o.labels.pageFormat).arg(pageIndex + 1).arg(pages);
        painter.drawText(footer, leading | Qt::AlignVCenter, footerText);
        painter.drawText(footer, trailing | Qt::AlignVCenter, pageText);
        painter.restore();
    }
    painter.end();
    return true;
}

void ReportPage::savePdf() {
    const QString dir = destinationFolder();
    if (dir.isEmpty()) {
        emit m_ctx->message("Choose a destination folder first");
        return;
    }
    const QString path = ExportService::outputPath(dir, m_fileName->text(), suggestedName(), ".pdf");
    QDir().mkpath(dir);

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(m_paper->current() == 1 ? QPageLayout::Landscape : QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    if (!renderTo(printer)) {
        emit m_ctx->message(QString(" ") + path);
        return;
    }

    const ExportRecord rec = ExportService::makeRecord(
        path, "PDF report", "report", static_cast<int>(m_ctx->log.runs().size()), m_sections);
    m_ctx->recordExport(rec);
    emit m_ctx->message(QString(" ") + QFileInfo(path).fileName());
    refreshRecent();
}

void ReportPage::printReport() {
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(m_paper->current() == 1 ? QPageLayout::Landscape : QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle("Print laboratory report");
    if (dialog.exec() != QDialog::Accepted) return;
    renderTo(printer);
}

void ReportPage::refreshRecent() {
    m_recent->setRecord(m_ctx->log.lastExport());
}

} // namespace phy104
