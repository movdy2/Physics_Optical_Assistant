#include "ConstantsPage.h"
#include "ui_ConstantsPage.h"

#include <QHeaderView>
#include <QLineEdit>
#include <QTableWidget>

namespace phy104 {

ConstantsPage::ConstantsPage(const IConstantsRepository& repository, QWidget* parent)
    : QWidget(parent), m_constants(repository.loadAll()) {
    Ui::ConstantsPage form;
    form.setupUi(this);
    m_search = form.constantsSearch;
    m_table = form.constantsTable;
    m_description = form.constantDescription;
    m_table->verticalHeader()->setVisible(false);
    m_table->setRowCount(static_cast<int>(m_constants.size()));

    for (int row = 0; row < static_cast<int>(m_constants.size()); ++row) {
        const auto& c = m_constants[std::size_t(row)];
        m_table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(c.symbol)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(c.name)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::number(c.value, 'g', 12)));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(c.unit)));
    }
    m_table->resizeColumnsToContents();

    connect(m_search, &QLineEdit::textChanged, this, &ConstantsPage::applyFilter);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &ConstantsPage::onSelectionChanged);
}

int ConstantsPage::selectedIndex() const {
    const auto rows = m_table->selectionModel()->selectedRows();
    return rows.isEmpty() ? -1 : rows.first().row();
}

void ConstantsPage::applyFilter(const QString& text) {
    for (int row = 0; row < static_cast<int>(m_constants.size()); ++row) {
        const auto& c = m_constants[std::size_t(row)];
        const QString haystack = QString::fromStdString(c.symbol + std::string(" ") + c.name + std::string(" ") + c.description);
        m_table->setRowHidden(row, !haystack.contains(text, Qt::CaseInsensitive));
    }
}

void ConstantsPage::onSelectionChanged() {
    const int i = selectedIndex();
    if (i >= 0) m_description->setText(QString::fromStdString(m_constants[std::size_t(i)].description));
}

} // namespace phy104
