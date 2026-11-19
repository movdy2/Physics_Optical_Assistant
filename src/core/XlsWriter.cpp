#include "XlsWriter.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace phy104 {
namespace {

std::string number(double value) {
    char buffer[40];
    std::snprintf(buffer, sizeof buffer, "%.15g", value);
    return buffer;
}

std::string safeSheetName(const std::string& requested) {
    std::string name = requested.empty() ? "Sheet" : requested;
    for (char& ch : name)
        if (ch == ':' || ch == '\\' || ch == '/' || ch == '?' || ch == '*' || ch == '[' || ch == ']') ch = '_';
    if (name.size() > 31) name.resize(31);
    return name;
}

std::string sheetXml(const SpreadsheetSheet& sheet) {
    std::size_t columns = 0;
    for (const auto& row : sheet.rows) columns = std::max(columns, row.size());
    columns = std::max<std::size_t>(columns, 1);

    std::string xml = std::string("<Worksheet ss:Name=\"") + XlsWriter::xmlEscape(safeSheetName(sheet.name)) + "\"><Table ss:ExpandedColumnCount=\"" +
                      std::to_string(columns) + std::string("\" ss:ExpandedRowCount=\"") + std::to_string(sheet.rows.size()) + "\" x:FullColumns=\"1\" x:FullRows=\"1\">";
    for (const auto& row : sheet.rows) {
        xml += "<Row>";
        for (const SpreadsheetCell& cell : row) {
            const std::string style = cell.bold ? " ss:StyleID=\"Header\"" : "";
            if (const auto* text = std::get_if<std::string>(&cell.value)) {
                xml += std::string("<Cell") + style + "><Data ss:Type=\"String\" xml:space=\"preserve\">" +
                       XlsWriter::xmlEscape(*text) + "</Data></Cell>";
            } else if (const auto* value = std::get_if<double>(&cell.value)) {
                if (std::isfinite(*value)) {
                    xml += std::string("<Cell") + style + std::string("><Data ss:Type=\"Number\">") + number(*value) + "</Data></Cell>";
                } else {
                    xml += std::string("<Cell") + style + "><Data ss:Type=\"String\">n/a</Data></Cell>";
                }
            } else {
                xml += "<Cell/>";
            }
        }
        xml += "</Row>";
    }
    xml += "</Table></Worksheet>";
    return xml;
}

} // namespace

std::string XlsWriter::xmlEscape(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.size());
    for (const unsigned char ch : text) {
        switch (ch) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        default:
            // XML 1.0 cannot contain most C0 control characters.
            if (ch >= 0x20 || ch == '\t' || ch == '\n' || ch == '\r') escaped.push_back(static_cast<char>(ch));
            break;
        }
    }
    return escaped;
}

std::vector<std::uint8_t> XlsWriter::build(const std::vector<SpreadsheetSheet>& sheets) {
    std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                      "<?mso-application progid=\"Excel.Sheet\"?>\n"
                      "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\" "
                      "xmlns:o=\"urn:schemas-microsoft-com:office:office\" "
                      "xmlns:x=\"urn:schemas-microsoft-com:office:excel\" "
                      "xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\" "
                      "xmlns:html=\"http://www.w3.org/TR/REC-html40\">"
                      "<DocumentProperties xmlns=\"urn:schemas-microsoft-com:office:office\">"
                      "<Author>PHY104 Optical Assistant</Author><Version>1.0</Version></DocumentProperties>"
                      "<Styles>"
                      "<Style ss:ID=\"Default\" ss:Name=\"Normal\"><Alignment ss:Vertical=\"Bottom\"/>"
                      "<Font ss:FontName=\"Calibri\" ss:Size=\"11\"/><Interior/><NumberFormat/><Protection/></Style>"
                      "<Style ss:ID=\"Header\"><Font ss:Bold=\"1\"/><Interior ss:Color=\"#F2C879\" ss:Pattern=\"Solid\"/>"
                      "<Alignment ss:Vertical=\"Center\" ss:WrapText=\"1\"/></Style>"
                      "</Styles>";

    const std::vector<SpreadsheetSheet> fallback{{"Empty", {{SpreadsheetCell("Nothing selected")}}}};
    const auto& outputSheets = sheets.empty() ? fallback : sheets;
    for (const SpreadsheetSheet& sheet : outputSheets) xml += sheetXml(sheet);
    xml += "</Workbook>";

    return {xml.begin(), xml.end()};
}

} // namespace phy104
