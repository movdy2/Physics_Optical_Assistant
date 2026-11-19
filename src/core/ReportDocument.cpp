#include "ReportDocument.h"

#include <algorithm>
#include <cctype>

namespace phy104 {
namespace {

std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out += c;
        }
    }
    return out;
}

std::string formatLabel(std::string format, const std::string& token, const std::string& value) {
    const std::size_t at = format.find(token);
    if (at != std::string::npos) format.replace(at, token.size(), value);
    return format;
}

/// **bold** and `code` inside text that has already been escaped.
std::string inlineMarkup(const std::string& text) {
    std::string out;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text.compare(i, 2, "**") == 0) {
            const std::size_t end = text.find("**", i + 2);
            if (end != std::string::npos) {
                out += std::string("<b>") + text.substr(i + 2, end - i - 2) + "</b>";
                i = end + 1;
                continue;
            }
        }
        if (text[i] == '`') {
            const std::size_t end = text.find('`', i + 1);
            if (end != std::string::npos) {
                out += std::string("<span style=\"font-family:monospace\">") + text.substr(i + 1, end - i - 1) + "</span>";
                i = end;
                continue;
            }
        }
        out += text[i];
    }
    return out;
}

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::string current;
    for (char c : text) {
        if (c == '\n') {
            if (!current.empty() && current.back() == '\r') current.pop_back();
            lines.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty() && current.back() == '\r') current.pop_back();
    if (!current.empty()) lines.push_back(current);
    return lines;
}

} // namespace

std::string markdownToHtml(const std::string& markdown) {
    std::string out;
    bool inList = false;
    auto closeList = [&out, &inList] {
        if (inList) {
            out += "</ul>";
            inList = false;
        }
    };
    for (std::string line : splitLines(markdown)) {
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.erase(line.begin());
        if (line.empty()) {
            closeList();
            continue;
        }
        // Inside a report the note headings start one level below the document's own h2 sections.
        if (line.rfind("### ", 0) == 0) {
            closeList();
            out += std::string("<h5>") + inlineMarkup(escape(line.substr(4))) + "</h5>";
        } else if (line.rfind("## ", 0) == 0) {
            closeList();
            out += std::string("<h4>") + inlineMarkup(escape(line.substr(3))) + "</h4>";
        } else if (line.rfind("# ", 0) == 0) {
            closeList();
            out += std::string("<h3>") + inlineMarkup(escape(line.substr(2))) + "</h3>";
        } else if (line.rfind("- ", 0) == 0 || line.rfind("* ", 0) == 0) {
            if (!inList) {
                out += "<ul>";
                inList = true;
            }
            out += std::string("<li>") + inlineMarkup(escape(line.substr(2))) + "</li>";
        } else {
            closeList();
            out += std::string("<p>") + inlineMarkup(escape(line)) + "</p>";
        }
    }
    closeList();
    return out;
}

std::string reportImageKey(const std::string& kind, const std::string& experiment) {
    std::string slug;
    for (char c : experiment) {
        if (std::isalnum(static_cast<unsigned char>(c))) slug += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        else if (!slug.empty() && slug.back() != '-') slug += '-';
    }
    while (!slug.empty() && slug.back() == '-') slug.pop_back();
    return std::string("img://") + kind + std::string("-") + slug;
}

std::string reportFileStem(const std::string& isoDate) {
    std::string out = "PHY104_report";
    if (!isoDate.empty()) out += std::string("_") + isoDate;
    for (std::string::size_type i = 0; i < out.size(); ++i)
        if (out[i] == ' ' || out[i] == '/' || out[i] == ':' || out[i] == ',') out[i] = '-';
    return out;
}

std::string reportHtml(const ReportContent& content, const ReportOptions& o) {
    const std::string width = std::to_string(std::max(120, o.imageWidth));
    std::string h;
    h.reserve(6144);

    // ---------------------------------------------------------------- academic report style
    const bool rtl = o.rightToLeft;
    h += "<html><head><style>";
    h += "@page { size: A4; margin: 15mm 15mm 18mm 15mm; }";
    h += "body { font-family: 'Inter', 'Noto Sans Arabic', 'DejaVu Sans', sans-serif; font-size: 10pt; color: #202b38; margin: 0; }";
    if (rtl)
        h += "body { direction: rtl; unicode-bidi: embed; }";
    h += "h1 { font-size: 22pt; margin: 10pt 0 4pt 0; color: #102a43; page-break-after: avoid; }";
    h += "h2 { font-size: 13pt; margin: 18pt 0 6pt 0; color: #07566b; border-bottom: 1px solid #cbd5e1; padding-bottom: 3pt; page-break-after: avoid; }";
    h += "h3 { font-size: 10pt; margin: 10pt 0 4pt 0; color: #334e68; page-break-after: avoid; }";
    h += "h4, h5 { font-size: 9.5pt; margin: 7pt 0 3pt 0; color: #52606d; page-break-after: avoid; }";
    h += "p { margin: 3pt 0; } ul { margin: 3pt 0 3pt 16pt; }";
    h += "table { border-collapse: collapse; width: 100%; } td, th { padding: 3pt 6pt; font-size: 9pt; vertical-align: top; }";
    h += "th { color: #52606d; }";
    h += ".masthead { width: 100%; border-bottom: 2px solid #07566b; padding: 0 0 6pt 0; margin-bottom: 4pt; }";
    h += ".masthead td { color: #07566b; font-size: 8pt; letter-spacing: 1pt; padding: 0; }";
    h += ".metadata { width: 100%; background: #f1f5f8; border: 1px solid #d9e2ec; margin: 4pt 0; }";
    h += ".metadata td { padding: 6pt 8pt; }";
    h += ".meta-label { color: #627d98; font-size: 7.5pt; }";
    h += ".summary { background: #edf6f7; border-left: 3px solid #0b7285; padding: 5pt 8pt; page-break-inside: avoid; }";
    h += ".figure-caption { color: #627d98; font-size: 8pt; margin-top: 2pt; page-break-after: avoid; }";
    h += ".parameter-label { color: #52606d; }";
    h += ".figure { text-align: center; page-break-inside: avoid; margin: 6pt 0; }";
    h += ".figure img { max-width: 100%; height: auto; }";
    h += ".lcd-register { width: 100%; margin-top: 5pt; page-break-inside: avoid; }";
    h += ".lcd-register td.title { background: #e5eef2; color: #102a43; font-weight: bold; }";
    h += ".lcd-register td.line { padding: 1pt 6pt; }";
    h += ".lcd-register td.result { color: #52606d; }";
    h += ".lcd-register td.result-value { font-weight: bold; text-align: right; }";
    h += "hr { border: none; border-top: 1px solid #cbd5e1; margin: 8pt 0; page-break-after: avoid; }";
    h += ".section { page-break-before: always; page-break-inside: auto; }";
    h += ".section:first-child { page-break-before: avoid; }";
    if (rtl) {
        h += "td { text-align: right; }";
        h += ".summary { border-left: 0; border-right: 3px solid #0b7285; }";
        h += ".masthead td { letter-spacing: 0; }";
        h += ".lcd-register td.result-value { text-align: left; }";
    }
    h += "</style></head><body";
    h += rtl ? " dir=\"rtl\">" : ">";
    // ---------------------------------------------------------------- report identity and session metadata
    if (o.showMasthead) {
        h += std::string("<div class=\"masthead\"><table><tr><td><b>") + escape(o.labels.mastheadOrganization) + "</b></td>";
        h += std::string("<td align=\"right\">") + escape(o.labels.mastheadReport) + "</td></tr></table></div>";
    }
    h += std::string("<h1>") + escape(o.title) + "</h1>";
    h += std::string("<p style=\"color:#52606d; margin-bottom:7pt\">") + escape(o.subtitle) + "</p>";
    h += "<table class=\"metadata\"><tr>";
    h += std::string("<td><span class=\"meta-label\">") + escape(o.labels.generated) + std::string("</span><br><b>") + escape(o.generated) + "</b></td>";
    h += std::string("<td><span class=\"meta-label\">") + escape(o.labels.session) + std::string("</span><br><b>") + escape(o.sessionName) + "</b></td>";
    h += std::string("<td><span class=\"meta-label\">") + escape(o.labels.experiments) + std::string("</span><br><b>") + std::to_string(content.sections.size()) + "</b></td>";
    h += "</tr></table><hr>";

    // ---------------------------------------------------------------- numbered experiments and figure captions
    int index = 0;
    int figure = 0;
    for (const ReportSection& s : content.sections) {
        ++index;
        h += std::string("<div class=\"section\"><h2>") + std::to_string(index) + std::string(". ") + escape(s.title) + "</h2>";
        if (!s.headline.empty())
            h += std::string("<p class=\"summary\"><b>") + escape(o.labels.result) + std::string("</b><br>") + escape(s.headline) + "</p>";
        if (o.diagrams && !s.diagramKey.empty()) {
            ++figure;
            h += std::string("<div class=\"figure\"><p><img src=\"") + escape(s.diagramKey) + std::string("\" width=\"") + width + "\"></p>";
            const std::string figureLabel = formatLabel(o.labels.figureFormat, "%1", std::to_string(figure));
            const std::string diagramLabel = formatLabel(o.labels.opticalDiagramFormat, "%1", s.title);
            h += std::string("<p class=\"figure-caption\"><b>") + escape(figureLabel) + std::string("</b> ") + escape(diagramLabel) + "</p></div>";
        }
        if (o.charts && !s.chartKey.empty()) {
            ++figure;
            h += std::string("<div class=\"figure\"><p><img src=\"") + escape(s.chartKey) + std::string("\" width=\"") + width + "\"></p>";
            const std::string figureLabel = formatLabel(o.labels.figureFormat, "%1", std::to_string(figure));
            const std::string chartLabel = formatLabel(o.labels.dataChartFormat, "%1", s.title);
            h += std::string("<p class=\"figure-caption\"><b>") + escape(figureLabel) + std::string("</b> ") + escape(chartLabel) + "</p></div>";
        }
        if (o.registers && !s.parameters.empty()) {
            h += std::string("<h3>") + escape(o.labels.experimentalParameters) + "</h3><table class=\"lcd-register\">";
            for (std::size_t i = 0; i < s.parameters.size(); i += 2) {
                h += std::string("<tr><th class=\"parameter-label\">") + escape(s.parameters[i].first) + std::string("</th><td>") + escape(s.parameters[i].second) + "</td>";
                if (i + 1 < s.parameters.size())
                    h += std::string("<th class=\"parameter-label\">") + escape(s.parameters[i + 1].first) + std::string("</th><td>") + escape(s.parameters[i + 1].second) + "</td>";
                else
                    h += "<td></td><td></td>";
                h += "</tr>";
            }
            h += "</table>";
        }
        if (o.registers) {
            for (const LcdRegister& r : s.registers) {
                h += "<table class=\"lcd-register\">";
                h += std::string("<tr><td colspan=\"2\" style=\"background:#e5eef2; color:#102a43\"><b>") + escape(r.title) + "</b></td></tr>";
                for (const std::string& line : r.lines) h += std::string("<tr><td colspan=\"2\">") + escape(line) + "</td></tr>";
                if (!r.resultLabel.empty() || !r.resultValue.empty())
                    h += std::string("<tr><td style=\"color:#52606d\">") + escape(r.resultLabel) + "</td><td class=\"result-value\"><b>" +
                         escape(r.resultValue) + "</b></td></tr>";
                for (const std::string& line : r.extraLines) h += std::string("<tr><td colspan=\"2\">") + escape(line) + "</td></tr>";
                h += "</table>";
            }
        }
        h += "</div>";
    }

    // ---------------------------------------------------------------- notes
    if (o.notes && !content.notes.empty()) {
        h += std::string("<h2>") + escape(o.labels.sessionNotes) + "</h2>";
        h += markdownToHtml(content.notes);
    }

    // ---------------------------------------------------------------- saved runs
    if (o.runs && !content.runs.empty()) {
        h += std::string("<h2>") + escape(o.labels.savedCalculations) + "</h2>";
        h += "<table class=\"lcd-register\">";
        h += std::string("<tr><th>#</th><th>") + escape(o.labels.tableExperiment) + std::string("</th><th>") + escape(o.labels.tableInputs) +
             std::string("</th><th>") + escape(o.labels.tableResult) + "</th></tr>";
        for (const RunRecord& r : content.runs)
            h += std::string("<tr><td>") + std::to_string(r.id) + std::string("</td><td>") + escape(r.experiment) + std::string("</td><td>") + escape(r.description) +
                 std::string("</td><td>") + escape(r.result) + "</td></tr>";
        h += "</table>";
    }

    if (index == 0 && (!o.notes || content.notes.empty()) && (!o.runs || content.runs.empty()))
        h += std::string("<p style=\"color:#52606d\">") + escape(o.labels.nothingSelected) + "</p>";

    h += "</body></html>";
    return h;
}

} // namespace phy104
