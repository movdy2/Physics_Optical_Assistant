#pragma once

#include <QObject>
#include <QString>

class QAction;
class QApplication;
class QEvent;
class QWidget;

namespace phy104 {

enum class UiLanguage { English, Arabic };

/// Small offline UI catalog and runtime language switch. Arabic uses a right-to-left layout;
/// equations, symbols and numeric input remain unchanged.
class LanguageManager final : public QObject {
public:
    static LanguageManager& instance();

    void install(QApplication& app);
    UiLanguage language() const { return m_language; }
    bool isArabic() const { return m_language == UiLanguage::Arabic; }
    void setLanguage(UiLanguage language);
    QString text(const QString& source) const;
    void apply(QWidget* root);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    LanguageManager() = default;
    void translateWidget(QWidget* widget);
    void translateAction(QAction* action);

    QApplication* m_app = nullptr;
    UiLanguage m_language = UiLanguage::English;
    bool m_applying = false;
};

} // namespace phy104