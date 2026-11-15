#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace phy104 {

struct SpreadsheetCell {
    std::variant<std::monostate, double, std::string> value;
    bool bold = false;

    SpreadsheetCell() = default;
    SpreadsheetCell(double v, bool isBold = false) : value(v), bold(isBold) {}
    SpreadsheetCell(const std::string& text, bool isBold = false) : value(text), bold(isBold) {}
    SpreadsheetCell(const char* text, bool isBold = false) : value(std::string(text)), bold(isBold) {}
};

struct SpreadsheetSheet {
    std::string name;
    std::vector<std::vector<SpreadsheetCell>> rows;
};

/// Writes Excel 2003 SpreadsheetML, an XML spreadsheet format accepted by Excel and LibreOffice.
/// The export UI uses the conventional .xls suffix; strings are UTF-8 and numeric cells remain numeric.
class XlsWriter final {
public:
    static std::vector<std::uint8_t> build(const std::vector<SpreadsheetSheet>& sheets);
    static std::string xmlEscape(const std::string& text);
};

} // namespace phy104
