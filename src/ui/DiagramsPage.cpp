#include "DiagramsPage.h"
#include "LatexRenderer.h"
#include "SvgDiagramView.h"
#include "ui_DiagramsPage.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>

namespace phy104 {
namespace {

struct DiagramViewInfo {
    QString title;
    QString resource;
    QString caption;
    QStringList equations;
};

struct DiagramInfo {
    QString title;
    QList<DiagramViewInfo> views;
};

const QList<DiagramInfo>& diagrams() {
    static const QList<DiagramInfo> list = {
        {"Refraction & TIR", {
            {"Refraction", ":/diagrams/refraction.svg",
             "The reflected angle equals the incident angle. Snell’s law sets the refracted angle when light crosses between media.",
             {"n_1\\sin\\theta_1=n_2\\sin\\theta_2", "\\theta_r=\\theta_1"}},
            {"Total Internal Reflection", ":/diagrams/tir.svg",
             "Above the critical angle, light travelling from higher to lower refractive index is totally reflected.",
             {"\\sin\\theta_c=\\frac{n_2}{n_1}", "n_1>n_2"}}
        }},
        {"Lenses & Mirrors", {
            {"Thin Lens", ":/diagrams/thin_lens.svg",
             "Principal rays locate the image formed by a thin lens. The sign of the image distance identifies a real or virtual image.",
             {"\\frac{1}{s}+\\frac{1}{s'}=\\frac{1}{f}", "m=-\\frac{s'}{s}", "P=\\frac{1}{f}"}},
            {"Spherical Mirror", ":/diagrams/spherical_mirror.svg",
             "For a spherical mirror, the paraxial focal length is half the radius of curvature.",
             {"\\frac{1}{s}+\\frac{1}{s'}=\\frac{1}{f}", "f=\\frac{R}{2}", "m=-\\frac{s'}{s}"}}
        }},
        {"Prisms", {
            {"Prism & Deviation", ":/diagrams/prism.svg",
             "The ray bends at both prism faces. At minimum deviation, the path is symmetric and the refractive index follows from A and δmin.",
             {"\\delta=\\theta_1+\\theta_2-A", "n=\\frac{\\sin((A+\\delta_{min})/2)}{\\sin(A/2)}"}}
        }},
        {"Wave Interference (Double-Slit)", {
            {"Young Double Slit", ":/diagrams/double_slit.svg",
             "Light from the two slits overlaps on the screen. Bright fringes occur when the path difference is an integer number of wavelengths.",
             {"d\\sin\\theta=m\\lambda", "\\Delta y=\\frac{\\lambda L}{d}"}}
        }},
        {"Polarization (Malus & Brewster)", {
            {"Polarizers & Brewster Angle", ":/diagrams/polarization.svg",
             "Malus’s law gives transmission through two polarizers. At Brewster’s angle, the reflected and refracted rays are perpendicular.",
             {"I=I_0\\cos^2\\theta", "\\tan\\theta_B=\\frac{n_2}{n_1}", "\\theta_B+\\theta_t=90^\\circ"}}
        }},
        {"Single-Slit Diffraction", {
            {"Single-Slit Pattern", ":/diagrams/single_slit.svg",
             "The central maximum is twice the width of each adjacent maximum. Minima occur at a sin θ = mλ.",
             {"a\\sin\\theta=m\\lambda", "w_0\\approx\\frac{2L\\lambda}{a}"}}
        }},
        {"Diffraction Grating", {
            {"Grating Orders", ":/diagrams/grating.svg",
             "A grating produces bright orders when the path difference between adjacent slits is an integer wavelength.",
             {"d\\sin\\theta_m=m\\lambda", "d=\\frac{1}{N}"}}
        }},
        {"Thin-Film Interference", {
            {"Thin-Film Reflections", ":/diagrams/thin_film.svg",
             "Reflections from the two film surfaces interfere. With one phase reversal, the reflected constructive condition is shifted by half a wavelength.",
             {"\\Delta=2nt\\cos\\theta_t", "2nt\\cos\\theta_t=(q+\\frac{1}{2})\\lambda"}}
        }},
        {"Michelson Interferometry", {
            {"Michelson Interferometer", ":/diagrams/michelson.svg",
             "The beam splitter sends light down perpendicular arms. Moving M2 changes the round-trip optical path difference and shifts the fringes.",
             {"\\Delta\\mathrm{OPD}=2n\\Delta x", "m=\\frac{2n\\Delta x}{\\lambda}"}}
        }},
        {"Newton's Rings", {
            {"Newton’s Rings", ":/diagrams/newton_rings.svg",
             "A curved lens on a flat plate creates a radially varying thin film. In reflected light, dark-ring radius squared is proportional to order.",
             {"r_m^2=\\frac{m\\lambda R}{n}", "t(r)\\approx\\frac{r^2}{2R}"}}
        }},
        {"Thick Lens & Aberration", {
            {"Thick Lens & Spherical Aberration", ":/diagrams/thick_lens.svg",
             "A thick lens has separated principal planes. Marginal and paraxial rays may focus at different points, producing spherical aberration.",
             {"\\frac{1}{f}=(n-1)\\left(\\frac{1}{R_1}-\\frac{1}{R_2}+\\frac{(n-1)d}{nR_1R_2}\\right)"}}
        }}
    };
    return list;
}

} // namespace

DiagramsPage::DiagramsPage(QWidget* parent) : QWidget(parent) {
    Ui::DiagramsPage form;
    form.setupUi(this);
    m_list = form.diagramList;
    m_viewSelector = form.diagramVariantSelector;
    m_title = form.sectionTitle;
    m_caption = form.diagramCaption;
    m_equations = form.equationsContainer;
    m_view = new SvgDiagramView(this);
    form.diagramViewLayout->addWidget(m_view);

    for (const auto& d : diagrams()) m_list->addItem(d.title);
    connect(form.exportDiagramButton, &QPushButton::clicked, this, &DiagramsPage::exportPng);

    connect(m_list, &QListWidget::currentRowChanged, this, &DiagramsPage::showDiagram);
    connect(m_viewSelector, qOverload<int>(&QComboBox::currentIndexChanged), this, &DiagramsPage::showDiagramView);
    m_list->setCurrentRow(0);
}

void DiagramsPage::showDiagram(int row) {
    if (row < 0 || row >= diagrams().size()) return;
    m_currentDiagram = row;
    const DiagramInfo& d = diagrams()[row];
    const QSignalBlocker blocker(m_viewSelector);
    m_viewSelector->clear();
    for (const auto& view : d.views) m_viewSelector->addItem(view.title);
    m_viewSelector->setVisible(d.views.size() > 1);
    showDiagramView(0);
}

void DiagramsPage::showDiagramView(int index) {
    if (m_currentDiagram < 0 || m_currentDiagram >= diagrams().size()) return;
    const DiagramInfo& d = diagrams()[m_currentDiagram];
    if (index < 0 || index >= d.views.size()) return;
    const DiagramViewInfo& view = d.views[index];
    m_view->load(view.resource);
    m_title->setText(view.title);
    m_caption->setText(view.caption);

    auto* layout = qobject_cast<QHBoxLayout*>(m_equations->layout());
    while (QLayoutItem* item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    for (const QString& tex : view.equations) {
        auto* w = new LatexWidget;
        w->setLatex(tex);
        w->setPointSize(17);
        w->setCardStyle(true);
        layout->addWidget(w);
    }
    layout->addStretch();
}

void DiagramsPage::exportPng() {
    const QString name = m_title->text().toLower().replace(" ", "_").replace("&", "and");
    const QString file = QFileDialog::getSaveFileName(this, "Export diagram", name + ".png", "PNG image (*.png)");
    if (file.isEmpty()) return;
    if (!m_view->exportPng(file, 2400))
        QMessageBox::warning(this, "Export", QString(" ") + file);
}

} // namespace phy104
