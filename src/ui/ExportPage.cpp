#include "ExportPage.h"
#include "AppContext.h"
#include "Components.h"
#include "DataExport.h"
#include "ExportService.h"
#include "RecentExportPanel.h"
#include "ReportPage.h"
#include "ThemeManager.h"
#include "ui_ExportPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextDocument>
#include <cmath>

namespace phy104 {

ExportPage::ExportPage(AppContext* ctx, QWidget* parent)
    : BasePage("Export", "CSV, Excel and PDF reports / Prepare complete data files or a formatted laboratory report",
               ctx, parent) {
    // ---------------------------------------------------------------- Designer-defined series workspace; custom controls stay live C++ widgets
    auto* exportFormHost = new QWidget;
    Ui::ExportPage ui;
    ui.setupUi(exportFormHost);
    content()->addWidget(exportFormHost, 1);

    m_exportModes = new ModeTabs({"Series data", "PDF report"});
    m_exportModes->setObjectName("exportModeTabs");
    ui.exportModesLayout->addWidget(m_exportModes);
    m_workspace = ui.exportWorkspace;

    ui.seriesPanel->setObjectName("panel");
    ui.seriesHeader->setObjectName("panelHeader");
    ui.seriesBody->setObjectName("panelBody");
    ui.seriesTitle->setObjectName("panelTitle");
    ui.seriesMeta->setObjectName("panelMeta");
    m_listMeta = ui.seriesMeta;
    m_series = ui.seriesList;

    ui.descriptionPanel->setObjectName("panel");
    ui.descriptionHeader->setObjectName("panelHeader");
    ui.descriptionBody->setObjectName("panelBody");
    ui.descriptionTitle->setObjectName("panelTitle");
    ui.descriptionMeta->setObjectName("panelMeta");
    m_descTitle = ui.descriptionName;
    m_descTitle->setObjectName("sectionTitle");
    m_descBody = ui.descriptionText;
    m_descBody->setObjectName("hint");
    m_descNote = ui.descriptionNote;
    m_descNote->setObjectName("hint");
    m_descColumns = ui.descriptionColumns;
    m_descColumns->setObjectName("caption");

    ui.previewPanel->setObjectName("panel");
    ui.previewHeader->setObjectName("panelHeader");
    ui.previewBody->setObjectName("panelBody");
    ui.previewTitle->setObjectName("panelTitle");
    ui.previewMeta->setObjectName("panelMeta");
    m_previewTitle = ui.previewTitle;
    m_previewMeta = ui.previewMeta;
    m_table = ui.previewTable;
    m_table->verticalHeader()->hide();
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_previewFoot = ui.previewFoot;
    m_previewFoot->setObjectName("caption");

    ui.settingsPanel->setObjectName("panel");
    ui.settingsHeader->setObjectName("panelHeader");
    ui.settingsBody->setObjectName("panelBody");
    ui.settingsTitle->setObjectName("panelTitle");
    ui.settingsMeta->setObjectName("panelMeta");
    m_format = new ModeTabs({"Excel (.xls)", "CSV (.csv)"});
    m_format->setObjectName("seriesFormatTabs");
    ui.formatTabsLayout->addWidget(m_format);
    ui.fileNameLabel->setObjectName("fieldLabel");
    ui.folderLabel->setObjectName("fieldLabel");
    ui.includeLabel->setObjectName("caption");
    m_fileName = new UnitField(".xls");
    ui.fileNameLayout->addWidget(m_fileName);
    m_folder = ui.folderCombo;
    m_folder->addItem("Documents / PHY104", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/PHY104");
    m_folder->addItem("Desktop", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    m_folder->addItem("Choose folder…", QString());
    m_chkParams = ui.parametersCheck;
    m_chkSteps = ui.stepsCheck;
    m_chkData = ui.dataCheck;
    m_chkNotes = ui.notesCheck;
    m_ready = new NoticeCard;
    ui.readyCardLayout->addWidget(m_ready);
    m_primary = ui.primaryButton;
    m_primary->setObjectName("primary");
    m_secondary = ui.secondaryButton;
    m_secondary->setObjectName("secondary");
    m_print = ui.printButton;
    m_print->setObjectName("secondary");
    m_recent = new RecentExportPanel;
    ui.recentExportLayout->addWidget(m_recent);

    connect(m_folder, &QComboBox::activated, this, [this](int index) {
        if (m_folder->itemData(index).toString().isEmpty()) {
            const QString dir = QFileDialog::getExistingDirectory(this, "Destination folder");
            if (!dir.isEmpty()) { m_folder->insertItem(index, dir, dir); m_folder->setCurrentIndex(index); }
            else m_folder->setCurrentIndex(0);
        }
    });

    connect(m_exportModes, &ModeTabs::currentChanged, this, &ExportPage::onExportPaneChanged);
    connect(m_format, &ModeTabs::currentChanged, this, &ExportPage::onFormatChanged);
    connect(m_series, &QListWidget::currentRowChanged, this, [this](int) { refreshPreview(); });
    connect(m_series, &QListWidget::itemDoubleClicked, this, [this] { m_primary->click(); });
    for (auto* c : {m_chkParams, m_chkSteps, m_chkData, m_chkNotes}) connect(c, &QCheckBox::toggled, this, [this] { refreshPreview(); });
    connect(m_primary, &QPushButton::clicked, this, &ExportPage::exportPrimary);
    connect(m_secondary, &QPushButton::clicked, this, &ExportPage::exportSecondary);
    connect(m_print, &QPushButton::clicked, this, &ExportPage::printTable);
    connect(ctx, &AppContext::snapshotChanged, this, &ExportPage::refreshSeries);
    connect(ctx, &AppContext::exportRecorded, this, &ExportPage::refreshRecent);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] { refreshPreview(); refreshRecent(); });

    StatusInfo st;
    st.left = "Ready to export";
    st.mid = "Excel + CSV";
    st.right = "Local files only";
    setStatusInfo(st, false);

    onFormatChanged();
    onExportPaneChanged();
    refreshSeries();
    refreshRecent();
}

void ExportPage::setReportPage(ReportPage* reportPage) {
    if (!reportPage || m_reportPage) return;
    m_reportPage = reportPage;
    m_workspace->addWidget(reportPage);
    connect(reportPage, &BasePage::stateChanged, this, [this] {
        if (m_exportModes->current() == 1 && m_reportPage)
            setStatusInfo(m_reportPage->statusInfo(), m_reportPage->isPaused());
    });
    onExportPaneChanged();
}

void ExportPage::showDataExport() {
    m_exportModes->setCurrent(0);
    onExportPaneChanged();
}

void ExportPage::showPdfReport() {
    m_exportModes->setCurrent(1);
    onExportPaneChanged();
}

void ExportPage::onExportPaneChanged() {
    int pane = m_exportModes->current();
    if (pane == 1 && !m_reportPage) {
        m_exportModes->setCurrent(0);
        pane = 0;
    }
    m_workspace->setCurrentIndex(pane);
    if (pane == 1) {
        setStatusInfo(m_reportPage->statusInfo(), m_reportPage->isPaused());
        return;
    }
    StatusInfo st;
    st.left = "Ready to export";
    st.mid = m_format->current() == 0 ? "Excel workbook" : "CSV file";
    st.right = "Local files only";
    setStatusInfo(st, false);
}

QString ExportPage::destinationFolder() const { return m_folder->currentData().toString(); }

const Snapshot* ExportPage::currentSnapshot() const {
    const QListWidgetItem* item = m_series->currentItem();
    if (!item) return nullptr;
    return m_ctx->log.snapshot(item->data(Qt::UserRole).toString().toStdString());
}

void ExportPage::refreshSeries() {
    const QString keep = m_series->currentItem() ? m_series->currentItem()->data(Qt::UserRole).toString() : QString();
    {
        const QSignalBlocker block(m_series);
        m_series->clear();
        for (const auto& [name, snap] : m_ctx->log.snapshots()) {
            if (snap.series.rows.empty()) continue;
            auto* item = new QListWidgetItem(QString::fromStdString(snap.series.name) + "\n" +
                                             QString("%1 rows · %2 columns").arg(snap.series.rows.size()).arg(snap.series.headers.size()));
            item->setData(Qt::UserRole, QString::fromStdString(name));
            m_series->addItem(item);
        }
        int row = 0;
        for (int i = 0; i < m_series->count(); ++i)
            if (m_series->item(i)->data(Qt::UserRole).toString() == keep) { row = i; break; }
        if (m_series->count() > 0) m_series->setCurrentRow(row);
    }
    m_listMeta->setText(QString("%1 SERIES").arg(m_series->count()));
    refreshPreview();
}

void ExportPage::refreshPreview() {
    const Snapshot* snap = currentSnapshot();
    m_table->clear();
    if (!snap || snap->series.rows.empty()) {
        m_previewTitle->setText("Full preview");
        m_previewMeta->setText("no data");
        m_table->setRowCount(0);
        m_table->setColumnCount(0);
        m_previewFoot->setText("Run a calculation on an experiment page: its table is published here automatically.");
        m_descTitle->setText("No data series yet");
        m_descBody->setText("Every experiment page publishes its data series while you type, so this list fills up as you work.");
        m_descNote->setText(QString());
        m_descColumns->setText(QString());
        m_ready->setNotice("Nothing to export", "Choose or create a series that has rows.", true);
        m_primary->setEnabled(false);
        m_secondary->setEnabled(false);
        m_print->setEnabled(false);
        return;
    }

    const DataSeries& s = snap->series;
    const bool excel = m_format->current() == 0;
    const Theme& th = ThemeManager::instance().theme();

    // ---- what the series contains
    m_descTitle->setText(QString::fromStdString(s.name));
    m_descBody->setText(QString::fromStdString(s.description));
    m_descNote->setText(QString::fromStdString(s.note));
    QStringList columns;
    for (const auto& h : s.headers) columns << QString::fromStdString(h);
    m_descColumns->setText(QString("\n") + columns.join(" · "));

    // ---- every row, full width
    m_previewTitle->setText(QString(" ") + QString::fromStdString(s.id));
    m_previewMeta->setText(QString("%1 ROWS × %2 COLUMNS").arg(s.rows.size()).arg(s.headers.size()));
    const int rows = static_cast<int>(s.rows.size());
    const int cols = static_cast<int>(s.headers.size());
    m_table->setColumnCount(cols);
    m_table->setRowCount(rows);
    for (int c = 0; c < cols; ++c) {
        auto* h = new QTableWidgetItem(QString::fromStdString(s.headers[c]));
        h->setForeground(th.amber);
        m_table->setHorizontalHeaderItem(c, h);
    }
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const double v = s.rows[r][c];
            auto* item = new QTableWidgetItem(std::isfinite(v) ? QString::number(v, 'f', c == 0 ? 2 : 4) : "n/a");
            item->setForeground(std::isfinite(v) ? th.text : th.dim);
            m_table->setItem(r, c, item);
        }
        m_table->setRowHeight(r, 26);
    }
    QStringList analysis;
    for (const auto& [name, value] : s.analysis)
        analysis << QString::fromStdString(name) + QString(" ") + QString::fromStdString(value);
    QString foot = QString("%1 rows × %2 columns · every row shown · full precision retained").arg(rows).arg(cols);
    if (!analysis.isEmpty()) foot += QString(" ") + analysis.join(" · ");
    m_previewFoot->setText(foot);

    // ---- file name + what the export will contain
    const QString base = QString::fromStdString(DataExport::suggestedFileName(s, ""));
    if (m_fileName->text().isEmpty() || m_fileName->text().startsWith("PHY104_")) m_fileName->setText(base);
    const ExportOptions opt{m_chkParams->isChecked(), m_chkSteps->isChecked(), m_chkData->isChecked(), m_chkNotes->isChecked()};
    const QString info = excel ? QString::fromStdString(DataExport::describeWorkbook(m_ctx->log, s, opt))
                               : QString("%1 rows · %2 columns · UTF-8").arg(rows).arg(cols);
    m_ready->setNotice(excel ? "Workbook ready" : "CSV ready", info + ". Numeric cells use full precision.", false);
    m_primary->setEnabled(true);
    m_print->setEnabled(true);
    m_secondary->setEnabled(true);
}

void ExportPage::refreshRecent() {
    m_recent->setRecord(m_ctx->log.lastExport());
}

void ExportPage::setFormat(bool excel) {
    m_format->setCurrent(excel ? 0 : 1);      // setCurrent does not emit currentChanged
    onFormatChanged();
    showDataExport();
}

void ExportPage::onFormatChanged() {
    const bool excel = m_format->current() == 0;
    m_fileName->setUnit(excel ? ".xls" : ".csv");
    m_primary->setText(excel ? "Export workbook" : "Export CSV");
    m_secondary->setText(excel ? "Export as CSV instead" : "Export workbook instead");
    for (auto* c : {m_chkParams, m_chkSteps, m_chkNotes}) c->setEnabled(excel);   // CSV is the series only
    refreshPreview();
    if (m_exportModes->current() == 0) onExportPaneChanged();
}

bool ExportPage::exportAs(bool excel) {
    const Snapshot* snap = currentSnapshot();
    if (!snap || snap->series.rows.empty()) { emit m_ctx->message("Nothing to export yet"); return false; }
    const DataSeries& s = snap->series;
    const QString dir = destinationFolder();
    if (dir.isEmpty()) { emit m_ctx->message("Choose a destination folder first"); return false; }

    const QString extension = excel ? ".xls" : ".csv";
    const QString fallback = QString::fromStdString(DataExport::suggestedFileName(s, extension.toStdString()));
    const QString path = ExportService::outputPath(dir, m_fileName->text(), fallback, extension);
    const ExportOptions options{m_chkParams->isChecked(), m_chkSteps->isChecked(),
                                m_chkData->isChecked(), m_chkNotes->isChecked()};
    const auto format = excel ? SeriesExportFormat::Workbook : SeriesExportFormat::Csv;
    QString error;
    if (!ExportService::writeSeriesFile(path, m_ctx->log, s, format, options, &error)) {
        emit m_ctx->message(error);
        return false;
    }

    const ExportRecord rec = ExportService::makeRecord(
        path, excel ? "Excel (.xls)" : "CSV (.csv)", s.id,
        static_cast<int>(s.rows.size()), static_cast<int>(s.headers.size()));
    m_ctx->recordExport(rec);
    refreshRecent();
    return true;
}

void ExportPage::printTable() {
    const Snapshot* snap = currentSnapshot();
    if (!snap || snap->series.rows.empty()) { emit m_ctx->message("Nothing to print: publish a sweep first"); return; }
    QPrinter printer(QPrinter::HighResolution);
    printer.setDocName("PHY104 data");
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle("Print the data table");
    if (dialog.exec() != QDialog::Accepted) return;

    // the same table as the preview, as simple HTML (no CSS tricks: Qt's rich text engine prints it as it is shown)
    const DataSeries& series = snap->series;
    QString html = QString(">") + QString::fromStdString(series.name).toHtmlEscaped() + "</h2><p>"
                 + QString::fromStdString(series.description).toHtmlEscaped() + "</p>";
    if (!series.note.empty()) html += QString(">") + QString::fromStdString(series.note).toHtmlEscaped() + "</p>";
    if (!series.analysis.empty()) {
        html += "<h3>Analysis and uncertainty</h3><table border='1' cellspacing='0' cellpadding='4'>";
        for (const auto& [name, value] : series.analysis)
            html += QString(">") + QString::fromStdString(name).toHtmlEscaped() + "</th><td>" +
                    QString::fromStdString(value).toHtmlEscaped() + "</td></tr>";
        html += "</table>";
    }
    html += "<table border='1' cellspacing='0' cellpadding='4'><tr>";
    for (const std::string& h : series.headers) html += QString(">") + QString::fromStdString(h).toHtmlEscaped() + "</th>";
    html += "</tr>";
    for (const auto& row : snap->series.rows) {
        html += "<tr>";
        for (double v : row) html += QString("<td>") + (std::isnan(v) ? QString("—") : QString::number(v, 'g', 6)) + QString("</td>");
        html += "</tr>";
    }
    html += "</table>";
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);
    emit m_ctx->message(QString(" ") + QString::fromStdString(snap->series.name));
}

void ExportPage::exportPrimary() { exportAs(m_format->current() == 0); }
void ExportPage::exportSecondary() { exportAs(m_format->current() != 0); }

} // namespace phy104
