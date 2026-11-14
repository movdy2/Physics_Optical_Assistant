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
                out += "<b>" + text.substr(i + 2, end - i - 2) + "</b>";
                i = end + 1;
                continue;
            }
        }
        if (text[i] == '`') {
            const std::size_t end = text.find('`', i + 1);
            if (end != std::string::npos) {
                out += "<span style=\"font-family:monospace\">" + text.substr(i + 1, end - i - 1) + "</span>";
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
            out += "<h5>" + inlineMarkup(escape(line.substr(4))) + "</h5>";
        } else if (line.rfind("## ", 0) == 0) {
            closeList();
            out += "<h4>" + inlineMarkup(escape(line.substr(3))) + "</h4>";
        } else if (line.rfind("# ", 0) == 0) {
            closeList();
            out += "<h3>" + inlineMarkup(escape(line.substr(2))) + "</h3>";
        } else if (line.rfind("- ", 0) == 0 || line.rfind("* ", 0) == 0) {
            if (!inList) {
                out += "<ul>";
                inList = true;
            }
            out += "<li>" + inlineMarkup(escape(line.substr(2))) + "</li>";
        } else {
            closeList();
            out += "<p>" + inlineMarkup(escape(line)) + "</p>";
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
    return "img://" + kind + "-" + slug;
}

std::string reportFileStem(const std::string& isoDate) {
    std::string out = "PHY104_report";
    if (!isoDate.empty()) out += "_" + isoDate;
    for (std::string::size_type i = 0; i < out.size(); ++i)
        if (out[i] == ' ' || out[i] == '/' || out[i] == ':' || out[i] == ',') out[i] = '-';
    return out;
}

std::string reportHtml(const ReportContent& content, const ReportOptions& o) {
    const std::string width = std::to_string(std::max(120, o.imageWidth));
    std::string h;
    h.reserve(6144);

    // ---------------------------------------------------------------- academic report style
    h += "<html><head><style>";
    h += "body { font-family: 'Inter', 'Segoe UI', 'DejaVu Sans', sans-serif; font-size: 10pt; color: #202b38; margin: 0; }";
    h += "h1 { font-size: 23pt; margin: 8pt 0 2pt 0; color: #102a43; }";
    h += "h2 { font-size: 13pt; margin: 16pt 0 5pt 0; color: #07566b; border-bottom: 1px solid #cbd5e1; padding-bottom: 3pt; }";
    h += "h3 { font-size: 10pt; margin: 9pt 0 3pt 0; color: #334e68; }";
    h += "h4, h5 { font-size: 9.5pt; margin: 6pt 0 2pt 0; color: #52606d; }";
    h += "p { margin: 3pt 0; } ul { margin: 3pt 0 3pt 14pt; }";
    h += "table { border-collapse: collapse; } td, th { padding: 3pt 6pt; font-size: 9pt; vertical-align: top; }";
    h += "th { color: #52606d; text-align: left; }";
    h += ".masthead td { color: #07566b; font-size: 8pt; letter-spacing: 1pt; border-bottom: 2px solid #07566b; padding: 0 0 5pt 0; }";
    h += ".metadata td { background: #f1f5f8; border: 1px solid #d9e2ec; padding: 6pt 8pt; }";
    h += ".meta-label { color: #627d98; font-size: 7.5pt; }";
    h += ".summary { background: #edf6f7; border-left: 3px solid #0b7285; padding: 5pt 8pt; }";
    h += ".figure-caption { color: #627d98; font-size: 8pt; margin-top: 0; }";
    h += ".parameter-label { color: #52606d; }";
    if (o.rightToLeft)
        h += "body { direction: rtl; } th { text-align: right; } .summary { border-left: 0; border-right: 3px solid #0b7285; } .masthead td { letter-spacing: 0; }";
    h += "</style></head><body";
    h += o.rightToLeft ? " dir=\"rtl\">" : ">";

    // ---------------------------------------------------------------- report identity and session metadata
    if (o.showMasthead) {
        h += "<table class=\"masthead\" width=\"100%\"><tr><td><b>" + escape(o.labels.mastheadOrganization) + "</b></td>";
        h += "<td align=\"right\">" + escape(o.labels.mastheadReport) + "</td></tr></table>";
    }
    h += "<h1>" + escape(o.title) + "</h1>";
    h += "<p style=\"color:#52606d; margin-bottom:7pt\">" + escape(o.subtitle) + "</p>";
    h += "<table class=\"metadata\" width=\"100%\"><tr>";
    h += "<td><span class=\"meta-label\">" + escape(o.labels.generated) + "</span><br><b>" + escape(o.generated) + "</b></td>";
    h += "<td><span class=\"meta-label\">" + escape(o.labels.session) + "</span><br><b>" + escape(o.sessionName) + "</b></td>";
    h += "<td><span class=\"meta-label\">" + escape(o.labels.experiments) + "</span><br><b>" + std::to_string(content.sections.size()) + "</b></td>";
    h += "</tr></table><hr>";

    // ---------------------------------------------------------------- numbered experiments and figure captions
    int index = 0;
    int figure = 0;
    for (const ReportSection& s : content.sections) {
        ++index;
        h += "<h2>" + std::to_string(index) + ". " + escape(s.title) + "</h2>";
        if (!s.headline.empty())
            h += "<p class=\"summary\"><b>" + escape(o.labels.result) + "</b><br>" + escape(s.headline) + "</p>";
        if (o.diagrams && !s.diagramKey.empty()) {
            ++figure;
            h += "<p><img src=\"" + escape(s.diagramKey) + "\" width=\"" + width + "\"></p>";
            const std::string figureLabel = formatLabel(o.labels.figureFormat, "%1", std::to_string(figure));
            const std::string diagramLabel = formatLabel(o.labels.opticalDiagramFormat, "%1", s.title);
            h += "<p class=\"figure-caption\"><b>" + escape(figureLabel) + "</b> " + escape(diagramLabel) + "</p>";
        }
        if (o.charts && !s.chartKey.empty()) {
            ++figure;
            h += "<p><img src=\"" + escape(s.chartKey) + "\" width=\"" + width + "\"></p>";
            const std::string figureLabel = formatLabel(o.labels.figureFormat, "%1", std::to_string(figure));
            const std::string chartLabel = formatLabel(o.labels.dataChartFormat, "%1", s.title);
            h += "<p class=\"figure-caption\"><b>" + escape(figureLabel) + "</b> " + escape(chartLabel) + "</p>";
        }
        if (o.registers && !s.parameters.empty()) {
            h += "<h3>" + escape(o.labels.experimentalParameters) + "</h3><table width=\"100%\">";
            for (std::size_t i = 0; i < s.parameters.size(); i += 2) {
                h += "<tr><th class=\"parameter-label\">" + escape(s.parameters[i].first) + "</th><td>" + escape(s.parameters[i].second) + "</td>";
                if (i + 1 < s.parameters.size())
                    h += "<th class=\"parameter-label\">" + escape(s.parameters[i + 1].first) + "</th><td>" + escape(s.parameters[i + 1].second) + "</td>";
                else
                    h += "<td></td><td></td>";
                h += "</tr>";
            }
            h += "</table>";
        }
        if (o.registers) {
            for (const LcdRegister& r : s.registers) {
                h += "<table width=\"100%\" style=\"margin-top:5pt\">";
                h += "<tr><td colspan=\"2\" style=\"background:#e5eef2; color:#102a43\"><b>" + escape(r.title) + "</b></td></tr>";
                for (const std::string& line : r.lines) h += "<tr><td colspan=\"2\">" + escape(line) + "</td></tr>";
                if (!r.resultLabel.empty() || !r.resultValue.empty())
                    h += "<tr><td style=\"color:#52606d\">" + escape(r.resultLabel) + "</td><td align=\"right\"><b>" +
                         escape(r.resultValue) + "</b></td></tr>";
                for (const std::string& line : r.extraLines) h += "<tr><td colspan=\"2\">" + escape(line) + "</td></tr>";
                h += "</table>";
            }
        }
    }

    // ---------------------------------------------------------------- notes
    if (o.notes && !content.notes.empty()) {
        h += "<h2>" + escape(o.labels.sessionNotes) + "</h2>";
        h += markdownToHtml(content.notes);
    }

    // ---------------------------------------------------------------- saved runs
    if (o.runs && !content.runs.empty()) {
        h += "<h2>" + escape(o.labels.savedCalculations) + "</h2>";
        h += "<table width=\"100%\">";
        h += "<tr><th>#</th><th>" + escape(o.labels.tableExperiment) + "</th><th>" + escape(o.labels.tableInputs) +
             "</th><th>" + escape(o.labels.tableResult) + "</th></tr>";
        for (const RunRecord& r : content.runs)
            h += "<tr><td>" + std::to_string(r.id) + "</td><td>" + escape(r.experiment) + "</td><td>" + escape(r.description) +
                 "</td><td>" + escape(r.result) + "</td></tr>";
        h += "</table>";
    }

    if (index == 0 && (!o.notes || content.notes.empty()) && (!o.runs || content.runs.empty()))
        h += "<p style=\"color:#52606d\">" + escape(o.labels.nothingSelected) + "</p>";

    h += "</body></html>";
    return h;
}

} // namespace phy104
