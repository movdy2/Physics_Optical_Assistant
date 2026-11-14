#include "LanguageManager.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QFile>
#include <QGroupBox>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QRegularExpression>
#include <QSet>
#include <QVector>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSignalBlocker>
#include <QStringList>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QTreeWidget>
#include <QWidget>

#include <algorithm>
#include <functional>
#include <utility>

namespace phy104 {
namespace {

QSettings* languageSettings() {
    static QSettings settings("PHY104", "PHY104OpticalAssistant");
    return &settings;
}

const QHash<QString, QString>& arabicCatalog() {
    static const QHash<QString, QString> catalog = [] {
        QHash<QString, QString> result{
        {QStringLiteral("Dashboard"), QStringLiteral("لوحة التحكم")},
        {QStringLiteral("Refraction & TIR"), QStringLiteral("الانكسار والانعكاس الكلي")},
        {QStringLiteral("Lenses & Mirrors"), QStringLiteral("العدسات والمرايا")},
        {QStringLiteral("Prisms"), QStringLiteral("الموشورات")},
        {QStringLiteral("Wave Interference (Double-Slit)"), QStringLiteral("تداخل الموجات (الشق المزدوج)")},
        {QStringLiteral("Wave Interference"), QStringLiteral("تداخل الموجات")},
        {QStringLiteral("Polarization (Malus & Brewster)"), QStringLiteral("الاستقطاب (مالوس وبروستر)")},
        {QStringLiteral("Single-Slit Diffraction"), QStringLiteral("حيود الشق الواحد")},
        {QStringLiteral("Diffraction Grating"), QStringLiteral("محزوز الحيود")},
        {QStringLiteral("Thin-Film Interference"), QStringLiteral("تداخل الأغشية الرقيقة")},
        {QStringLiteral("Michelson Interferometry"), QStringLiteral("تداخل ميكلسون")},
        {QStringLiteral("Newton's Rings"), QStringLiteral("حلقات نيوتن")},
        {QStringLiteral("Thick Lens & Aberration"), QStringLiteral("العدسة السميكة والزيغ")},
        {QStringLiteral("Notes"), QStringLiteral("الملاحظات")},
        {QStringLiteral("Equation Lab"), QStringLiteral("مختبر المعادلات")},
        {QStringLiteral("Study Hub"), QStringLiteral("مركز الدراسة")},
        {QStringLiteral("Formula Solver"), QStringLiteral("محلّل المعادلات")},
        {QStringLiteral("Calculator"), QStringLiteral("الحاسبة")},
        {QStringLiteral("Export"), QStringLiteral("تصدير")},
        {QStringLiteral("EXPERIMENTS"), QStringLiteral("التجارب")},
        {QStringLiteral("TOOLS"), QStringLiteral("الأدوات")},
        {QStringLiteral("LAB SESSION"), QStringLiteral("جلسة المختبر")},
        {QStringLiteral("● ENGINE READY"), QStringLiteral("● المحرك جاهز")},
        {QStringLiteral("● CALCULATION PAUSED"), QStringLiteral("● الحساب متوقف مؤقتًا")},
        {QStringLiteral("Ready"), QStringLiteral("جاهز")},
        {QStringLiteral("Double precision"), QStringLiteral("دقة مزدوجة")},
        {QStringLiteral("Angles measured from normal"), QStringLiteral("تُقاس الزوايا من العمود المقام")},
        {QStringLiteral("⌨ Keyboard"), QStringLiteral("⌨ لوحة المفاتيح")},
        {QStringLiteral("☾ Dark"), QStringLiteral("☾ داكن")},
        {QStringLiteral("☀ Light"), QStringLiteral("☀ فاتح")},
        {QStringLiteral("Qt 6 · Offline"), QStringLiteral("Qt 6 · دون اتصال")},
        {QStringLiteral("PHY104 / Semester 02"), QStringLiteral("PHY104 / الفصل 02")},
        {QStringLiteral("Save"), QStringLiteral("حفظ")},
        {QStringLiteral("Save session"), QStringLiteral("حفظ الجلسة")},
        {QStringLiteral("Export: PDF report, Excel (.xlsx) or CSV (.csv)"), QStringLiteral("تصدير: تقرير PDF أو Excel (.xlsx) أو CSV (.csv)")},
        {QStringLiteral("PDF report…"), QStringLiteral("تقرير PDF…")},
        {QStringLiteral("Excel (.xlsx)…"), QStringLiteral("ملف Excel (.xlsx)…")},
        {QStringLiteral("CSV (.csv)…"), QStringLiteral("ملف CSV (.csv)…")},
        {QStringLiteral("Print…"), QStringLiteral("طباعة…")},
        {QStringLiteral("Save PDF"), QStringLiteral("حفظ PDF")},
        {QStringLiteral("Open folder"), QStringLiteral("فتح المجلد")},
        {QStringLiteral("Copy value"), QStringLiteral("نسخ القيمة")},
        {QStringLiteral("Delete selected run"), QStringLiteral("حذف التشغيل المحدد")},
        {QStringLiteral("Open notes"), QStringLiteral("فتح الملاحظات")},
        {QStringLiteral("Export data"), QStringLiteral("تصدير البيانات")},
        {QStringLiteral("Export PNG…"), QStringLiteral("تصدير PNG…")},
        {QStringLiteral("Reset parameters"), QStringLiteral("إعادة ضبط القيم")},
        {QStringLiteral("Calculate && Simulate"), QStringLiteral("احسب وحاكِ")},
        {QStringLiteral("Calculate & Simulate"), QStringLiteral("احسب وحاكِ")},
        {QStringLiteral("Calculate &&\nSimulate"), QStringLiteral("احسب وحاكِ")},
        {QStringLiteral("Send to\nNotes"), QStringLiteral("إرسال إلى\nالملاحظات")},
        {QStringLiteral("Solve"), QStringLiteral("حل")},
        {QStringLiteral("Clear"), QStringLiteral("مسح")},
        {QStringLiteral("Generate summary"), QStringLiteral("إنشاء ملخص")},
        {QStringLiteral("Close (Esc)"), QStringLiteral("إغلاق (Esc)")},
        {QStringLiteral("Recent calculations"), QStringLiteral("الحسابات الأخيرة")},
        {QStringLiteral("Recent"), QStringLiteral("الأخيرة")},
        {QStringLiteral("Compare"), QStringLiteral("مقارنة")},
        {QStringLiteral("Run A"), QStringLiteral("التشغيل أ")},
        {QStringLiteral("Run B"), QStringLiteral("التشغيل ب")},
        {QStringLiteral("Lab notebook"), QStringLiteral("دفتر المختبر")},
        {QStringLiteral("AUTOSAVED"), QStringLiteral("حُفظ تلقائيًا")},
        {QStringLiteral("Review formula references, record observations and export your measured data."), QStringLiteral("راجع مراجع المعادلات، وسجّل الملاحظات، وصدّر بيانات القياس.")},
        {QStringLiteral("No saved calculations yet: press Calculate & Simulate"), QStringLiteral("لا توجد حسابات محفوظة بعد: اضغط «احسب وحاكِ».")},
        {QStringLiteral("Open any of the 11 experiment screens"), QStringLiteral("افتح أيًا من شاشات التجارب الإحدى عشرة")},
        {QStringLiteral("Browse all 11"), QStringLiteral("استعراض التجارب الـ11")},
        {QStringLiteral("Your optical bench / choose an experiment or resume a saved run"), QStringLiteral("منضدتك البصرية / اختر تجربة أو استأنف تشغيلًا محفوظًا")},
        {QStringLiteral("EXPERIMENTS"), QStringLiteral("التجارب")},
        {QStringLiteral("Geometrical & wave optics"), QStringLiteral("البصريات الهندسية والموجية")},
        {QStringLiteral("SAVED CALCULATIONS"), QStringLiteral("الحسابات المحفوظة")},
        {QStringLiteral("Current lab session"), QStringLiteral("جلسة المختبر الحالية")},
        {QStringLiteral("DATA SERIES"), QStringLiteral("سلاسل البيانات")},
        {QStringLiteral("Ready for Excel / CSV"), QStringLiteral("جاهز لـ Excel / CSV")},
        {QStringLiteral("EXPERIMENT MODULES"), QStringLiteral("وحدات التجارب")},
        {QStringLiteral("Recent export"), QStringLiteral("آخر تصدير")},
        {QStringLiteral("No export yet"), QStringLiteral("لا يوجد تصدير بعد")},
        {QStringLiteral("The file you export last is remembered here and in the .phy session."), QStringLiteral("يُحفظ آخر ملف تصدير هنا وفي جلسة .phy.")},
        {QStringLiteral("No data series yet"), QStringLiteral("لا توجد سلاسل بيانات بعد")},
        {QStringLiteral("Every experiment page publishes its data series while you type, so this list fills up as you work."), QStringLiteral("تنشر كل صفحة تجربة سلسلة بياناتها أثناء إدخال القيم، وستمتلئ هذه القائمة مع العمل.")},
        {QStringLiteral("Run a calculation on an experiment page: its table is published here automatically."), QStringLiteral("أجرِ حسابًا في صفحة تجربة لنشر جدولها هنا تلقائيًا.")},
        {QStringLiteral("Nothing to export"), QStringLiteral("لا توجد بيانات للتصدير")},
        {QStringLiteral("Nothing to export yet"), QStringLiteral("لا توجد بيانات للتصدير بعد")},
        {QStringLiteral("Choose a destination folder first"), QStringLiteral("اختر مجلد الوجهة أولًا")},
        {QStringLiteral("Choose or create a series that has rows."), QStringLiteral("اختر سلسلة تحتوي على صفوف أو أنشئها.")},
        {QStringLiteral("Series data"), QStringLiteral("بيانات السلسلة")},
        {QStringLiteral("PDF report"), QStringLiteral("تقرير PDF")},
        {QStringLiteral("Data series"), QStringLiteral("سلسلة البيانات")},
        {QStringLiteral("Series description"), QStringLiteral("وصف السلسلة")},
        {QStringLiteral("Full preview"), QStringLiteral("معاينة كاملة")},
        {QStringLiteral("Export settings"), QStringLiteral("إعدادات التصدير")},
        {QStringLiteral("Excel (.xlsx)"), QStringLiteral("Excel (.xlsx)")},
        {QStringLiteral("CSV (.csv)"), QStringLiteral("CSV (.csv)")},
        {QStringLiteral("Documents / PHY104"), QStringLiteral("المستندات / PHY104")},
        {QStringLiteral("Choose folder…"), QStringLiteral("اختيار مجلد…")},
        {QStringLiteral("INCLUDE IN WORKBOOK"), QStringLiteral("تضمين في المصنف")},
        {QStringLiteral("Input parameters and units"), QStringLiteral("قيم الإدخال ووحداتها")},
        {QStringLiteral("Step-by-step results"), QStringLiteral("النتائج خطوة بخطوة")},
        {QStringLiteral("Measurement data && fit"), QStringLiteral("بيانات القياس والملاءمة")},
        {QStringLiteral("Session notes"), QStringLiteral("ملاحظات الجلسة")},
        {QStringLiteral("File name"), QStringLiteral("اسم الملف")},
        {QStringLiteral("Destination folder"), QStringLiteral("مجلد الوجهة")},
        {QStringLiteral("Ready to export"), QStringLiteral("جاهز للتصدير")},
        {QStringLiteral("Excel + CSV"), QStringLiteral("Excel + CSV")},
        {QStringLiteral("Local files only"), QStringLiteral("ملفات محلية فقط")},
        {QStringLiteral("Excel workbook"), QStringLiteral("مصنف Excel")},
        {QStringLiteral("CSV file"), QStringLiteral("ملف CSV")},
        {QStringLiteral("Export workbook"), QStringLiteral("تصدير المصنف")},
        {QStringLiteral("Export CSV"), QStringLiteral("تصدير CSV")},
        {QStringLiteral("Export as CSV instead"), QStringLiteral("التصدير بصيغة CSV بدلًا من ذلك")},
        {QStringLiteral("Export workbook instead"), QStringLiteral("تصدير المصنف بدلًا من ذلك")},
        {QStringLiteral("Nothing to print: publish a sweep first"), QStringLiteral("لا توجد بيانات للطباعة: أنشئ مسحًا أولًا")},
        {QStringLiteral("Print the data table"), QStringLiteral("طباعة جدول البيانات")},
        {QStringLiteral("Physics Constants"), QStringLiteral("الثوابت الفيزيائية")},
        {QStringLiteral("Constants"), QStringLiteral("الثوابت")},
        {QStringLiteral("Search constants (name or symbol)..."), QStringLiteral("ابحث عن الثوابت (بالاسم أو الرمز)…")},
        {QStringLiteral("Select a constant to see its description."), QStringLiteral("اختر ثابتًا لعرض وصفه.")},
        {QStringLiteral("Optical Setups"), QStringLiteral("التجهيزات البصرية")},
        {QStringLiteral("Export diagram"), QStringLiteral("تصدير الرسم")},
        {QStringLiteral("LaTeX · math mode"), QStringLiteral("LaTeX · وضع الرياضيات")},
        {QStringLiteral("LaTeX source"), QStringLiteral("مصدر LaTeX")},
        {QStringLiteral("Insert template ▾"), QStringLiteral("إدراج قالب ▾")},
        {QStringLiteral("Snell's law"), QStringLiteral("قانون سنيل")},
        {QStringLiteral("Critical angle"), QStringLiteral("الزاوية الحرجة")},
        {QStringLiteral("Thin lens"), QStringLiteral("عدسة رقيقة")},
        {QStringLiteral("Newton's rings"), QStringLiteral("حلقات نيوتن")},
        {QStringLiteral("Double slit"), QStringLiteral("الشق المزدوج")},
        {QStringLiteral("Single slit"), QStringLiteral("الشق المفرد")},
        {QStringLiteral("Malus' law"), QStringLiteral("قانون مالوس")},
        {QStringLiteral("Keyboard · closed"), QStringLiteral("لوحة المفاتيح · مغلقة")},
        {QStringLiteral("Keyboard · open"), QStringLiteral("لوحة المفاتيح · مفتوحة")},
        {QStringLiteral("Live rendered equation"), QStringLiteral("معادلة معروضة مباشرة")},
        {QStringLiteral("Extracted variables"), QStringLiteral("المتغيرات المستخرجة")},
        {QStringLiteral("Known · integer"), QStringLiteral("معلوم · عدد صحيح")},
        {QStringLiteral("Known · > 0"), QStringLiteral("معلوم · > 0")},
        {QStringLiteral("Leave one value empty (or write  y = …) to choose the output."), QStringLiteral("اترك قيمة واحدة فارغة (أو اكتب y = …) لاختيار الناتج.")},
        {QStringLiteral("NO RESULT"), QStringLiteral("لا توجد نتيجة")},
        {QStringLiteral("Substitute:"), QStringLiteral("بالتعويض:")},
        {QStringLiteral("History"), QStringLiteral("السجل")},
        {QStringLiteral("Expression"), QStringLiteral("التعبير")},
        {QStringLiteral("Switch between degrees and radians"), QStringLiteral("التبديل بين الدرجات والراديان")},
        {QStringLiteral("Copy value"), QStringLiteral("نسخ القيمة")},
        {QStringLiteral("Search by name or ID…"), QStringLiteral("ابحث بالاسم أو المعرّف…")},
        {QStringLiteral("Search constants (name or symbol)..."), QStringLiteral("ابحث عن الثوابت (بالاسم أو الرمز)…")},
        {QStringLiteral("Chapter"), QStringLiteral("الفصل")},
        {QStringLiteral("Chapters"), QStringLiteral("الفصول")},
        {QStringLiteral("Chapter notes"), QStringLiteral("ملاحظات الفصل")},
        {QStringLiteral("Summary"), QStringLiteral("الملخص")},
        {QStringLiteral("Rules"), QStringLiteral("القواعد")},
        {QStringLiteral("Write a short recap, or generate an extractive draft from your notes…"), QStringLiteral("اكتب ملخصًا قصيرًا أو أنشئ مسودة مستخلصة من ملاحظاتك…")},
        {QStringLiteral("Write physics notes in Markdown…\n\n# Title   ## Chapter   ### Formula name   #### Exam warning\n- bullet points      $ n₁ sin θ₁ = n₂ sin θ₂ $ for formulas"), QStringLiteral("اكتب ملاحظات الفيزياء بصيغة Markdown…\n\n# العنوان   ## الفصل   ### اسم المعادلة   #### تنبيه للاختبار\n- نقاط      $ n₁ sin θ₁ = n₂ sin θ₂ $ للمعادلات")},
        {QStringLiteral("Write plain-text notes for the selected chapter…"), QStringLiteral("اكتب ملاحظات نصية للفصل المحدد…")},
        {QStringLiteral("Select a chapter to write its notes."), QStringLiteral("اختر فصلًا لكتابة ملاحظاته.")},
        {QStringLiteral("Generate summary"), QStringLiteral("إنشاء ملخص")},
        {QStringLiteral("Autosave on"), QStringLiteral("الحفظ التلقائي مفعّل")},
        {QStringLiteral("MARKDOWN · FORMULA JOTTER"), QStringLiteral("ملاحظات Markdown · صيغ ومعادلات")},
        {QStringLiteral("Scientific keyboard: types LaTeX formulas straight into the note"), QStringLiteral("لوحة المفاتيح العلمية: تُدرج صيغ LaTeX مباشرة في الملاحظة")},
        {QStringLiteral("Save PDF writes the file directly into the folder above; Print… opens the system dialog."), QStringLiteral("يحفظ «حفظ PDF» الملف مباشرة في المجلد أعلاه؛ ويفتح «طباعة…» نافذة النظام.")},
        {QStringLiteral("The whole document is built from the session: nothing leaves this computer."), QStringLiteral("يُنشأ المستند كاملًا من الجلسة؛ ولا تغادر أي بيانات هذا الجهاز.")},
        {QStringLiteral("Include"), QStringLiteral("تضمين")},
        {QStringLiteral("INCLUDE"), QStringLiteral("تضمين")},
        {QStringLiteral("PAPER"), QStringLiteral("الورق")},
        {QStringLiteral("Select a constant to see its description."), QStringLiteral("اختر ثابتًا لعرض وصفه.")},
        {QStringLiteral("Element type"), QStringLiteral("نوع العنصر")},
        {QStringLiteral("Lens type"), QStringLiteral("نوع العدسة")},
        {QStringLiteral("Mirror type"), QStringLiteral("نوع المرآة")},
        {QStringLiteral("Experiment controls"), QStringLiteral("عناصر التحكم بالتجربة")},
        {QStringLiteral("Incident medium"), QStringLiteral("الوسط الساقط")},
        {QStringLiteral("Transmitted medium"), QStringLiteral("الوسط النافذ")},
        {QStringLiteral("Refractive index · n₁"), QStringLiteral("معامل الانكسار · n₁")},
        {QStringLiteral("Refractive index · n₂"), QStringLiteral("معامل الانكسار · n₂")},
        {QStringLiteral("Incidence angle · θ₁"), QStringLiteral("زاوية السقوط · θ₁")},
        {QStringLiteral("Incidence angle · i₁"), QStringLiteral("زاوية السقوط · i₁")},
        {QStringLiteral("Apex angle · A"), QStringLiteral("زاوية الرأس · A")},
        {QStringLiteral("Focal length · f"), QStringLiteral("البعد البؤري · f")},
        {QStringLiteral("Object distance · dₒ"), QStringLiteral("بعد الجسم · dₒ")},
        {QStringLiteral("Object distance · dₒ (from V₁)"), QStringLiteral("بعد الجسم · dₒ (من V₁)")},
        {QStringLiteral("Object height · hₒ"), QStringLiteral("ارتفاع الجسم · hₒ")},
        {QStringLiteral("Radius of curvature · R = 2f"), QStringLiteral("نصف قطر التكور · R = 2f")},
        {QStringLiteral("Wavelength · λ"), QStringLiteral("الطول الموجي · λ")},
        {QStringLiteral("Slit separation · d"), QStringLiteral("المسافة بين الشقين · d")},
        {QStringLiteral("Screen distance · L"), QStringLiteral("المسافة إلى الشاشة · L")},
        {QStringLiteral("Fringe order · m"), QStringLiteral("رتبة الهدب · m")},
        {QStringLiteral("Source preset"), QStringLiteral("إعداد المصدر")},
        {QStringLiteral("First radius · R₁"), QStringLiteral("نصف القطر الأول · R₁")},
        {QStringLiteral("Second radius · R₂"), QStringLiteral("نصف القطر الثاني · R₂")},
        {QStringLiteral("Lens index · n"), QStringLiteral("معامل العدسة · n")},
        {QStringLiteral("Surrounding index · n₀"), QStringLiteral("معامل الوسط المحيط · n₀")},
        {QStringLiteral("Centre thickness · t"), QStringLiteral("السماكة المركزية · t")},
        {QStringLiteral("Clear aperture · D"), QStringLiteral("قطر الفتحة · D")},
        {QStringLiteral("Clear aperture · D"), QStringLiteral("قطر الفتحة · D")},
        {QStringLiteral("Standard uncertainty (1σ). Enter 0 to treat this quantity as exact."), QStringLiteral("عدم اليقين المعياري (1σ). أدخل 0 لاعتبار هذه الكمية دقيقة.")},
        {QStringLiteral("Independent standard uncertainties; enter 0 when a value is treated as exact."), QStringLiteral("حالات عدم اليقين المعيارية مستقلة؛ أدخل 0 عندما تُعد القيمة دقيقة.")},
        {QStringLiteral("Measurement uncertainties · 1σ"), QStringLiteral("حالات عدم اليقين في القياس · 1σ")},
        {QStringLiteral("Reset parameters"), QStringLiteral("إعادة ضبط القيم")},
        {QStringLiteral("Restore the last-valid value, then calculate again."), QStringLiteral("استعادة آخر قيمة صحيحة ثم إعادة الحساب.")},
        {QStringLiteral("Enter a number or expression."), QStringLiteral("أدخل رقمًا أو تعبيرًا.")},
        {QStringLiteral("Enter numbers only (for example 1.5 or 0.5*mm)"), QStringLiteral("أدخل أرقامًا فقط (مثل 1.5 أو 0.5*mm)")},
        {QStringLiteral("Nothing to send yet: calculate first"), QStringLiteral("لا يوجد ما يُرسل بعد؛ أجرِ الحساب أولًا")},
        {QStringLiteral("Regression analysis"), QStringLiteral("تحليل الانحدار")},
        {QStringLiteral("Results / Snell's law"), QStringLiteral("النتائج / قانون سنيل")},
        {QStringLiteral("Results / total internal reflection"), QStringLiteral("النتائج / الانعكاس الكلي الداخلي")},
        {QStringLiteral("Total internal reflection / ray diagram"), QStringLiteral("الانعكاس الكلي الداخلي / مخطط الأشعة")},
        {QStringLiteral("Critical-angle sweep"), QStringLiteral("مسح الزاوية الحرجة")},
        {QStringLiteral("Ray diagram / two incidence cases"), QStringLiteral("مخطط الأشعة / حالتا السقوط")},
        {QStringLiteral("Thin lens / principal rays"), QStringLiteral("العدسة الرقيقة / الأشعة الرئيسية")},
        {QStringLiteral("Image calculation"), QStringLiteral("حساب الصورة")},
        {QStringLiteral("Image distance vs object distance"), QStringLiteral("بعد الصورة مقابل بعد الجسم")},
        {QStringLiteral("Prism ray path"), QStringLiteral("مسار الشعاع في الموشور")},
        {QStringLiteral("Deviation vs. incidence angle"), QStringLiteral("الانحراف مقابل زاوية السقوط")},
        {QStringLiteral("Deviation register"), QStringLiteral("سجل الانحراف")},
        {QStringLiteral("Wavefronts and interference pattern"), QStringLiteral("جبهات الموجة ونمط التداخل")},
        {QStringLiteral("Screen intensity profile"), QStringLiteral("منحنى شدة الشاشة")},
        {QStringLiteral("Fringe calculation"), QStringLiteral("حساب الأهداب")},
        {QStringLiteral("Thick-lens calculation"), QStringLiteral("حساب العدسة السميكة")},
        {QStringLiteral("Thick element / principal planes"), QStringLiteral("عنصر سميك / المستويان الرئيسيان")},
        {QStringLiteral("Focal length per wavelength"), QStringLiteral("البعد البؤري لكل طول موجي")},
        {QStringLiteral("No data series yet"), QStringLiteral("لا توجد سلاسل بيانات بعد")},
        {QStringLiteral("CSV, Excel and PDF reports / Prepare complete data files or a formatted laboratory report"), QStringLiteral("تقارير CSV وExcel وPDF / أنشئ ملفات بيانات كاملة أو تقريرًا مخبريًا منسقًا")},
        {QStringLiteral("The file you export last is remembered here and in the .phy session."), QStringLiteral("يُحفظ آخر ملف تصدير هنا وفي جلسة .phy.")},
        {QStringLiteral("Units are shown alongside their fields"), QStringLiteral("تظهر الوحدات بجانب حقولها")},
        {QStringLiteral("Page actions"), QStringLiteral("إجراءات الصفحة")},
        {QStringLiteral("Session saved"), QStringLiteral("تم حفظ الجلسة")},
        {QStringLiteral("Session restored"), QStringLiteral("تمت استعادة الجلسة")},
        {QStringLiteral("Nothing to compare yet: press Calculate & Simulate on two settings and they appear here."), QStringLiteral("لا توجد بيانات للمقارنة بعد: اضغط «احسب وحاكِ» لإعدادين لتظهر هنا.")},
        {QStringLiteral("These runs carry no sweep data (they were saved by an older version of the session file)."), QStringLiteral("لا تحتوي هذه التشغيلات على بيانات مسح (حُفظت بإصدار أقدم من ملف الجلسة).")},
        {QStringLiteral("Nothing to compare yet"), QStringLiteral("لا توجد بيانات للمقارنة بعد")},
        {QStringLiteral("Enter a number or expression."), QStringLiteral("أدخل رقمًا أو تعبيرًا.")},
        {QStringLiteral("Search by name or ID…"), QStringLiteral("ابحث بالاسم أو المعرّف…")},
        {QStringLiteral("Browse all 11"), QStringLiteral("استعراض التجارب الـ11")},
        {QStringLiteral("Keyboard · closed"), QStringLiteral("لوحة المفاتيح · مغلقة")},
        {QStringLiteral("Keyboard · open"), QStringLiteral("لوحة المفاتيح · مفتوحة")},
        {QStringLiteral("LaTeX · math mode"), QStringLiteral("LaTeX · وضع الرياضيات")},
        {QStringLiteral("LATEX WORKSPACE / Edit, preview & evaluate an optics equation"), QStringLiteral("مساحة LaTeX / حرّر معادلة بصرية واعرضها وقيّمها")},
        {QStringLiteral("Equation Lab"), QStringLiteral("مختبر المعادلات")},
        {QStringLiteral("Constants"), QStringLiteral("الثوابت")},
        {QStringLiteral("Study Hub"), QStringLiteral("مركز الدراسة")},
        {QStringLiteral("Notes"), QStringLiteral("الملاحظات")},
        {QStringLiteral("Export data"), QStringLiteral("تصدير البيانات")},
        {QStringLiteral("Calculator"), QStringLiteral("الحاسبة")},
        {QStringLiteral("Formula Solver"), QStringLiteral("محلّل المعادلات")},
        {QStringLiteral("Browse all 11"), QStringLiteral("استعراض التجارب الـ11")},
        {QStringLiteral("Polarization"), QStringLiteral("الاستقطاب")},
        {QStringLiteral("Thin lens"), QStringLiteral("عدسة رقيقة")},
        {QStringLiteral("Converging lens"), QStringLiteral("عدسة مجمِّعة")},
        {QStringLiteral("Diverging lens"), QStringLiteral("عدسة مفرِّقة")},
        {QStringLiteral("Concave mirror"), QStringLiteral("مرآة مقعّرة")},
        {QStringLiteral("Convex mirror"), QStringLiteral("مرآة محدّبة")},
        {QStringLiteral("Converging · biconvex"), QStringLiteral("مجمِّعة · محدّبة الوجهين")},
        {QStringLiteral("Diverging · biconcave"), QStringLiteral("مفرِّقة · مقعّرة الوجهين")},
        {QStringLiteral("Crown glass"), QStringLiteral("زجاج تاجي")},
        {QStringLiteral("Flint glass"), QStringLiteral("زجاج صواني")},
        {QStringLiteral("Biconvex · converging"), QStringLiteral("محدّبة الوجهين · مجمِّعة")},
        {QStringLiteral("Biconcave · diverging"), QStringLiteral("مقعّرة الوجهين · مفرِّقة")},
        {QStringLiteral("Reflection & Refraction"), QStringLiteral("الانعكاس والانكسار")},
        {QStringLiteral("Total Internal Reflection"), QStringLiteral("الانعكاس الكلي الداخلي")},
        {QStringLiteral("Prism & Deviation"), QStringLiteral("الموشور والانحراف")},
        {QStringLiteral("Spherical Mirror"), QStringLiteral("مرآة كروية")},
        {QStringLiteral("Young Double Slit"), QStringLiteral("شقّا يونغ")},
        {QStringLiteral("Experiment controls"), QStringLiteral("عناصر التحكم بالتجربة")},
        {QStringLiteral("Run"), QStringLiteral("تشغيل")},
        {QStringLiteral("Result"), QStringLiteral("النتيجة")},
        {QStringLiteral("Status"), QStringLiteral("الحالة")},
        {QStringLiteral("RUN"), QStringLiteral("التشغيل")},
        {QStringLiteral("EXPERIMENT"), QStringLiteral("التجربة")},
        {QStringLiteral("RESULT"), QStringLiteral("النتيجة")},
        {QStringLiteral("STATUS"), QStringLiteral("الحالة")},
        {QStringLiteral("RUN A"), QStringLiteral("التشغيل أ")},
        {QStringLiteral("RUN B"), QStringLiteral("التشغيل ب")},
        {QStringLiteral("SCALE: cm"), QStringLiteral("المقياس: cm")},
        {QStringLiteral("LIVE · 1:1"), QStringLiteral("مباشر · 1:1")},
        {QStringLiteral("SCHEMATIC · NOT TO SCALE"), QStringLiteral("رسم تخطيطي · غير مرسوم بمقياس")},
        {QStringLiteral("SYMMETRIC PASSAGE"), QStringLiteral("مسار متماثل")},
        {QStringLiteral("7 COLOURS · 400–700 NM"), QStringLiteral("7 ألوان · 400–700 nm")},
        {QStringLiteral("EXPERIMENT MODULES"), QStringLiteral("وحدات التجارب")},
        {QStringLiteral("EXPERIMENT 01 / Snell's law and the critical angle"), QStringLiteral("التجربة 01 / قانون سنيل والزاوية الحرجة")},
        {QStringLiteral("EXPERIMENT 02 / Gaussian optics · real-is-positive sign convention"), QStringLiteral("التجربة 02 / بصريات غاوس · اصطلاح الإشارة: الحقيقي موجب")},
        {QStringLiteral("EXPERIMENT 03 / Refraction through a prism · minimum deviation"), QStringLiteral("التجربة 03 / الانكسار في موشور · أقل انحراف")},
        {QStringLiteral("EXPERIMENT 04 / Young's double-slit experiment · coherent monochromatic light"), QStringLiteral("التجربة 04 / تجربة شقّي يونغ · ضوء أحادي اللون ومترابط")},
        {QStringLiteral("Current: (%1, %2) cm"), QStringLiteral("الحالي: (%1، %2) cm")},
        {QStringLiteral("LAB SESSION / %1"), QStringLiteral("جلسة المختبر / %1")},
        {QStringLiteral("Session saved · %1"), QStringLiteral("تم حفظ الجلسة · %1")},
        {QStringLiteral("Last saved · %1"), QStringLiteral("آخر حفظ · %1")},
        {QStringLiteral("Search by name or ID…"), QStringLiteral("ابحث بالاسم أو المعرّف…")},
        {QStringLiteral("Formulas"), QStringLiteral("المعادلات")},
        {QStringLiteral("Insert template ▾"), QStringLiteral("إدراج قالب ▾")},
        {QStringLiteral("Input parameters and units"), QStringLiteral("قيم الإدخال ووحداتها")},
        {QStringLiteral("Data series"), QStringLiteral("سلسلة البيانات")},
        {QStringLiteral("Series description"), QStringLiteral("وصف السلسلة")},
        {QStringLiteral("Destination folder"), QStringLiteral("مجلد الوجهة")},
        {QStringLiteral("Ready to export"), QStringLiteral("جاهز للتصدير")},
        {QStringLiteral("File name"), QStringLiteral("اسم الملف")},
        {QStringLiteral("Report contents"), QStringLiteral("محتويات التقرير")},
        {QStringLiteral("Preview"), QStringLiteral("معاينة")},
        {QStringLiteral("Paper"), QStringLiteral("الورق")},
        {QStringLiteral("PAPER"), QStringLiteral("الورق")},
        {QStringLiteral("DATA SERIES"), QStringLiteral("سلاسل البيانات")},
        {QStringLiteral("SAVED CALCULATIONS"), QStringLiteral("الحسابات المحفوظة")},
        {QStringLiteral("INCLUDE IN WORKBOOK"), QStringLiteral("تضمين في المصنف")},
        {QStringLiteral("Search constants (name or symbol)..."), QStringLiteral("ابحث عن الثوابت (بالاسم أو الرمز)…")},
        {QStringLiteral("Physics Constants"), QStringLiteral("الثوابت الفيزيائية")},
        {QStringLiteral("Keyboard"), QStringLiteral("لوحة المفاتيح")},
        {QStringLiteral("Dark"), QStringLiteral("داكن")},
        {QStringLiteral("Light"), QStringLiteral("فاتح")},
        {QStringLiteral("Please enter a number"), QStringLiteral("يرجى إدخال رقم")},
        {QStringLiteral("This field is required"), QStringLiteral("هذا الحقل مطلوب")},
        {QStringLiteral("No saved calculations yet"), QStringLiteral("لا توجد حسابات محفوظة بعد")},
        {QStringLiteral("Export diagram"), QStringLiteral("تصدير الرسم")},
        {QStringLiteral("PNG image (*.png)"), QStringLiteral("صورة PNG (*.png)")},
    };

    for (const QString& path : {QStringLiteral(":/i18n/ar-ui.json"),
                                QStringLiteral(":/i18n/ar-content.json"),
                                QStringLiteral(":/i18n/ar-lessons.json"),
                                QStringLiteral(":/i18n/ar-core.json")}) {
        QFile translations(path);
        if (!translations.open(QIODevice::ReadOnly)) continue;
        const QJsonDocument document = QJsonDocument::fromJson(translations.readAll());
        if (!document.isObject()) continue;
        const QJsonObject entries = document.object();
        for (auto it = entries.begin(); it != entries.end(); ++it)
            if (it.value().isString()) result.insert(it.key(), it.value().toString());
    }
    return result;
    }();
    return catalog;
}

const QHash<QString, QString>& englishCatalog() {
    static const QHash<QString, QString> reverse = [] {
        QHash<QString, QString> result;
        const auto& arabic = arabicCatalog();
        for (auto it = arabic.cbegin(); it != arabic.cend(); ++it) {
            if (!result.contains(it.value())) result.insert(it.value(), it.key());
        }
        return result;
    }();
    return reverse;
}

const QHash<QString, QString>& foldedArabicCatalog() {
    static const QHash<QString, QString> folded = [] {
        QHash<QString, QString> result;
        QSet<QString> ambiguous;
        const auto& arabic = arabicCatalog();
        for (auto it = arabic.cbegin(); it != arabic.cend(); ++it) {
            const QString key = it.key().toCaseFolded();
            const auto existing = result.constFind(key);
            if (existing == result.cend()) result.insert(key, it.value());
            else if (existing.value() != it.value()) ambiguous.insert(key);
        }
        for (const QString& key : ambiguous) result.remove(key);
        return result;
    }();
    return folded;
}

struct TranslationPattern {
    QRegularExpression expression;
    QString translation;
    QVector<int> argumentIndexes;
};

const QVector<TranslationPattern>& translationPatterns() {
    static const QVector<TranslationPattern> patterns = [] {
        QVector<TranslationPattern> result;
        static const QRegularExpression placeholder(QStringLiteral("%([1-9])"));
        const auto& catalog = arabicCatalog();
        QStringList keys = catalog.keys();
        std::sort(keys.begin(), keys.end(), [](const QString& a, const QString& b) {
            return a.size() == b.size() ? a < b : a.size() > b.size();
        });
        for (const QString& key : keys) {
            if (!key.contains(placeholder)) continue;
            QString expression = QStringLiteral("^");
            QVector<int> indexes;
            qsizetype copied = 0;
            auto matches = placeholder.globalMatch(key);
            while (matches.hasNext()) {
                const QRegularExpressionMatch match = matches.next();
                expression += QRegularExpression::escape(key.mid(copied, match.capturedStart() - copied));
                expression += QStringLiteral("(.+?)");
                indexes.push_back(match.captured(1).toInt());
                copied = match.capturedEnd();
            }
            expression += QRegularExpression::escape(key.mid(copied));
            expression += QLatin1Char('$');
            result.push_back({QRegularExpression(expression, QRegularExpression::CaseInsensitiveOption),
                              catalog.value(key), std::move(indexes)});
        }
        return result;
    }();
    return patterns;
}

QString translatePattern(const QString& source) {
    for (const TranslationPattern& pattern : translationPatterns()) {
        const QRegularExpressionMatch match = pattern.expression.match(source);
        if (!match.hasMatch()) continue;
        QString translated = pattern.translation;
        QHash<int, QString> arguments;
        for (qsizetype i = 0; i < pattern.argumentIndexes.size(); ++i)
            if (!arguments.contains(pattern.argumentIndexes[i])) arguments.insert(pattern.argumentIndexes[i], match.captured(static_cast<int>(i + 1)));
        for (auto it = arguments.cbegin(); it != arguments.cend(); ++it)
            translated.replace(QLatin1Char('%') + QString::number(it.key()), it.value());
        return translated;
    }
    return source;
}

const QVector<QPair<QRegularExpression, QString>>& embeddedTranslations() {
    static const QVector<QPair<QRegularExpression, QString>> translations = [] {
        QVector<QPair<QRegularExpression, QString>> result;
        const auto& catalog = arabicCatalog();
        QHash<QString, QString> keysByFolded;
        QHash<QString, QString> valuesByFolded;
        QSet<QString> ambiguous;
        for (auto it = catalog.cbegin(); it != catalog.cend(); ++it) {
            const QString& key = it.key();
            if (key.size() < 3 || key == it.value() || key.contains(QLatin1Char('%'))) continue;
            bool plainWords = true;
            int letters = 0;
            for (const QChar ch : key) {
                if (ch.isLetter()) {
                    if (ch.unicode() > 0x7f) { plainWords = false; break; }
                    ++letters;
                } else if (!(ch.isSpace() || ch.unicode() == 39 || ch == QLatin1Char('-'))) {
                    plainWords = false;
                    break;
                }
            }
            if (!plainWords || letters < 3) continue;
            const QString folded = key.toCaseFolded();
            if (!keysByFolded.contains(folded)) {
                keysByFolded.insert(folded, key);
                valuesByFolded.insert(folded, it.value());
            } else if (valuesByFolded.value(folded) != it.value()) {
                // A word such as "light" can be an optical noun or a theme name. Exact catalog
                // matches remain available, but ambiguous meanings are not guessed inside prose.
                ambiguous.insert(folded);
            }
        }
        QStringList keys;
        for (auto it = keysByFolded.cbegin(); it != keysByFolded.cend(); ++it)
            if (!ambiguous.contains(it.key())) keys.push_back(it.value());
        std::sort(keys.begin(), keys.end(), [](const QString& a, const QString& b) {
            return a.size() == b.size() ? a < b : a.size() > b.size();
        });
        for (const QString& key : keys) {
            const QString expression = QStringLiteral("(?<![A-Za-z0-9_])") + QRegularExpression::escape(key) +
                                       QStringLiteral("(?![A-Za-z0-9_])");
            result.push_back({QRegularExpression(expression, QRegularExpression::CaseInsensitiveOption),
                              valuesByFolded.value(key.toCaseFolded())});
        }
        return result;
    }();
    return translations;
}

QString translateEmbedded(const QString& source) {
    QString translated = source;
    for (const auto& entry : embeddedTranslations())
        translated.replace(entry.first, entry.second);
    return translated;
}

constexpr int kI18nSourceRole = Qt::UserRole + 0x5A0;
constexpr int kI18nLastRole = Qt::UserRole + 0x5A1;
constexpr int kI18nTipSourceRole = Qt::UserRole + 0x5A2;
constexpr int kI18nTipLastRole = Qt::UserRole + 0x5A3;

} // namespace

LanguageManager& LanguageManager::instance() {
    static LanguageManager manager;
    return manager;
}

void LanguageManager::install(QApplication& app) {
    if (m_app && m_app != &app) m_app->removeEventFilter(this);
    m_app = &app;
    const QString saved = languageSettings()->value("ui/language", "en").toString().toLower();
    m_language = saved == "ar" ? UiLanguage::Arabic : UiLanguage::English;
    app.installEventFilter(this);
    app.setLayoutDirection(isArabic() ? Qt::RightToLeft : Qt::LeftToRight);
}

void LanguageManager::setLanguage(UiLanguage language) {
    m_language = language;
    languageSettings()->setValue("ui/language", isArabic() ? "ar" : "en");
    if (m_app) m_app->setLayoutDirection(isArabic() ? Qt::RightToLeft : Qt::LeftToRight);
}

QString LanguageManager::text(const QString& source) const {
    if (source.isEmpty()) return source;
    if (isArabic()) {
        const QString appTitle = QStringLiteral("PHY104 Optical Assistant — ");
        if (source.startsWith(appTitle)) return QStringLiteral("مساعد PHY104 البصري — ") + source.mid(appTitle.size());
        const auto& catalog = arabicCatalog();
        const auto it = catalog.constFind(source);
        if (it != catalog.cend()) return it.value();
        const auto folded = foldedArabicCatalog().constFind(source.toCaseFolded());
        if (folded != foldedArabicCatalog().cend()) return folded.value();
        const QString patterned = translatePattern(source);
        if (patterned != source) return patterned;
        return translateEmbedded(source);
    }
    const QString arabicTitle = QStringLiteral("مساعد PHY104 البصري — ");
    if (source.startsWith(arabicTitle)) return QStringLiteral("PHY104 Optical Assistant — ") + source.mid(arabicTitle.size());
    const auto it = englishCatalog().constFind(source);
    return it == englishCatalog().cend() ? source : it.value();
}

void LanguageManager::translateValue(QObject* owner, const char* field, const QString& value,
                                     const std::function<void(const QString&)>& setter) {
    if (!owner) return;
    const QByteArray sourceProperty = QByteArray("i18nSource_") + field;
    const QByteArray lastProperty = QByteArray("i18nLast_") + field;
    const QByteArray languageProperty = QByteArray("i18nLanguage_") + field;
    const QVariant previousTarget = owner->property(lastProperty.constData());
    const int currentLanguage = isArabic() ? 1 : 0;
    if (previousTarget.isValid() && value == previousTarget.toString() &&
        owner->property(languageProperty.constData()).toInt() == currentLanguage)
        return; // paint events are frequent; an unchanged, already translated field needs no regex work
    const QString source = previousTarget.isValid() && value == previousTarget.toString()
        ? owner->property(sourceProperty.constData()).toString() : value;
    const QString translated = text(source);
    owner->setProperty(sourceProperty.constData(), source);
    owner->setProperty(lastProperty.constData(), translated);
    owner->setProperty(languageProperty.constData(), currentLanguage);
    if (translated != value && setter) setter(translated);
}

void LanguageManager::translateAction(QAction* action) {
    if (!action || action->property("i18nSkip").toBool()) return;
    translateValue(action, "text", action->text(), [action](const QString& v) { action->setText(v); });
    translateValue(action, "toolTip", action->toolTip(), [action](const QString& v) { action->setToolTip(v); });
    translateValue(action, "statusTip", action->statusTip(), [action](const QString& v) { action->setStatusTip(v); });
}

void LanguageManager::translateWidget(QWidget* widget) {
    if (!widget || widget->property("i18nSkip").toBool()) return;
    auto update = [this](QObject* owner, const char* field, const QString& value, const auto& setter) {
        translateValue(owner, field, value, setter);
    };
    auto updateItem = [this](const QString& value, const QVariant& sourceValue, const QVariant& targetValue,
                             const auto& setter, const auto& store) {
        const QString source = targetValue.isValid() && value == targetValue.toString() ? sourceValue.toString() : value;
        const QString translated = text(source);
        store(source, translated);
        if (translated != value) setter(translated);
    };

    if (auto* label = qobject_cast<QLabel*>(widget))
        update(label, "text", label->text(), [label](const QString& v) { label->setText(v); });
    if (auto* button = qobject_cast<QAbstractButton*>(widget)) {
        update(button, "text", button->text(), [button](const QString& v) { button->setText(v); });
        update(button, "toolTip", button->toolTip(), [button](const QString& v) { button->setToolTip(v); });
        update(button, "accessibleName", button->accessibleName(), [button](const QString& v) { button->setAccessibleName(v); });
    }
    if (auto* group = qobject_cast<QGroupBox*>(widget))
        update(group, "title", group->title(), [group](const QString& v) { group->setTitle(v); });
    update(widget, "windowTitle", widget->windowTitle(), [widget](const QString& v) { widget->setWindowTitle(v); });
    update(widget, "toolTip", widget->toolTip(), [widget](const QString& v) { widget->setToolTip(v); });

    if (auto* edit = qobject_cast<QLineEdit*>(widget))
        update(edit, "placeholder", edit->placeholderText(), [edit](const QString& v) { edit->setPlaceholderText(v); });
    if (auto* edit = qobject_cast<QTextEdit*>(widget))
        update(edit, "placeholder", edit->placeholderText(), [edit](const QString& v) { edit->setPlaceholderText(v); });
    if (auto* edit = qobject_cast<QPlainTextEdit*>(widget)) {
        update(edit, "placeholder", edit->placeholderText(), [edit](const QString& v) { edit->setPlaceholderText(v); });
        // Only the untouched app-provided starter note is eligible for localization. Once the
        // student edits it, the flag is cleared and their authored content is never rewritten.
        if (edit->property("i18nBuiltinContent").toBool()) {
            update(edit, "plainText", edit->toPlainText(), [edit](const QString& v) {
                const QSignalBlocker blocker(edit);
                edit->setPlainText(v);
            });
        }
    }

    if (auto* combo = qobject_cast<QComboBox*>(widget)) {
        const QSignalBlocker blocker(combo);
        for (int i = 0; i < combo->count(); ++i) {
            const QVariant source = combo->itemData(i, kI18nSourceRole);
            const QVariant target = combo->itemData(i, kI18nLastRole);
            updateItem(combo->itemText(i), source, target,
                [combo, i](const QString& v) { combo->setItemText(i, v); },
                [combo, i](const QString& s, const QString& t) {
                    combo->setItemData(i, s, kI18nSourceRole);
                    combo->setItemData(i, t, kI18nLastRole);
                });
        }
    }
    if (auto* tabs = qobject_cast<QTabWidget*>(widget)) {
        QTabBar* bar = tabs->findChild<QTabBar*>();
        for (int i = 0; i < tabs->count(); ++i) {
            const QVariant source = bar ? bar->tabData(i).toMap().value(QStringLiteral("i18nSource")) : QVariant();
            const QVariant target = bar ? bar->tabData(i).toMap().value(QStringLiteral("i18nLast")) : QVariant();
            updateItem(tabs->tabText(i), source, target,
                [tabs, i](const QString& v) { tabs->setTabText(i, v); },
                [bar, i](const QString& s, const QString& t) {
                    if (!bar) return;
                    QVariantMap data = bar->tabData(i).toMap();
                    data.insert(QStringLiteral("i18nSource"), s);
                    data.insert(QStringLiteral("i18nLast"), t);
                    bar->setTabData(i, data);
                });
        }
    }
    if (auto* tabBar = qobject_cast<QTabBar*>(widget)) {
        for (int i = 0; i < tabBar->count(); ++i) {
            const QVariantMap data = tabBar->tabData(i).toMap();
            updateItem(tabBar->tabText(i), data.value(QStringLiteral("i18nSource")), data.value(QStringLiteral("i18nLast")),
                [tabBar, i](const QString& v) { tabBar->setTabText(i, v); },
                [tabBar, i](const QString& s, const QString& t) {
                    QVariantMap updated = tabBar->tabData(i).toMap();
                    updated.insert(QStringLiteral("i18nSource"), s);
                    updated.insert(QStringLiteral("i18nLast"), t);
                    tabBar->setTabData(i, updated);
                });
        }
    }
    if (auto* list = qobject_cast<QListWidget*>(widget)) {
        for (int i = 0; i < list->count(); ++i) {
            auto* item = list->item(i);
            updateItem(item->text(), item->data(kI18nSourceRole), item->data(kI18nLastRole),
                [item](const QString& v) { item->setText(v); },
                [item](const QString& s, const QString& t) { item->setData(kI18nSourceRole, s); item->setData(kI18nLastRole, t); });
            updateItem(item->toolTip(), item->data(kI18nTipSourceRole), item->data(kI18nTipLastRole),
                [item](const QString& v) { item->setToolTip(v); },
                [item](const QString& s, const QString& t) { item->setData(kI18nTipSourceRole, s); item->setData(kI18nTipLastRole, t); });
        }
    }
    if (auto* table = qobject_cast<QTableWidget*>(widget)) {
        const auto updateTableItem = [&updateItem](QTableWidgetItem* item) {
            if (!item) return;
            updateItem(item->text(), item->data(kI18nSourceRole), item->data(kI18nLastRole),
                [item](const QString& v) { item->setText(v); },
                [item](const QString& s, const QString& t) { item->setData(kI18nSourceRole, s); item->setData(kI18nLastRole, t); });
        };
        for (int row = 0; row < table->rowCount(); ++row)
            for (int col = 0; col < table->columnCount(); ++col) updateTableItem(table->item(row, col));
        for (int col = 0; col < table->columnCount(); ++col) updateTableItem(table->horizontalHeaderItem(col));
        for (int row = 0; row < table->rowCount(); ++row) updateTableItem(table->verticalHeaderItem(row));
    }
    if (auto* tree = qobject_cast<QTreeWidget*>(widget)) {
        const int columns = tree->columnCount();
        const auto updateTreeItem = [&updateItem, columns](QTreeWidgetItem* item) {
            if (!item) return;
            for (int col = 0; col < columns; ++col) {
                const QVariant source = item->data(col, kI18nSourceRole);
                const QVariant target = item->data(col, kI18nLastRole);
                updateItem(item->text(col), source, target,
                    [item, col](const QString& v) { item->setText(col, v); },
                    [item, col](const QString& s, const QString& t) {
                        item->setData(col, kI18nSourceRole, s);
                        item->setData(col, kI18nLastRole, t);
                    });
            }
        };
        updateTreeItem(tree->headerItem());
        std::function<void(QTreeWidgetItem*)> translateItem = [&](QTreeWidgetItem* item) {
            if (!item) return;
            updateTreeItem(item);
            for (int i = 0; i < item->childCount(); ++i) translateItem(item->child(i));
        };
        for (int i = 0; i < tree->topLevelItemCount(); ++i) translateItem(tree->topLevelItem(i));
    }

    if (auto* menu = qobject_cast<QMenu*>(widget)) {
        update(menu, "menuTitle", menu->title(), [menu](const QString& v) { menu->setTitle(v); });
        for (QAction* action : menu->actions()) translateAction(action);
    }
    for (QAction* action : widget->actions()) translateAction(action);
}

void LanguageManager::apply(QWidget* root) {
    if (!root) return;
    if (m_app) m_app->setLayoutDirection(isArabic() ? Qt::RightToLeft : Qt::LeftToRight);
    root->setLayoutDirection(isArabic() ? Qt::RightToLeft : Qt::LeftToRight);
    m_applying = true;
    translateWidget(root);
    const auto widgets = root->findChildren<QWidget*>();
    for (QWidget* widget : widgets) {
        translateWidget(widget);
        widget->update(); // custom-painted diagrams and charts also redraw in the selected language
    }
    m_applying = false;
    root->updateGeometry();
    root->update();
}

bool LanguageManager::eventFilter(QObject* watched, QEvent* event) {
    if (m_applying || !event) return false;
    switch (event->type()) {
    case QEvent::Show:
    case QEvent::Polish:
    case QEvent::Paint:
    case QEvent::LanguageChange:
        if (auto* widget = qobject_cast<QWidget*>(watched)) {
            m_applying = true;
            translateWidget(widget);
            m_applying = false;
        }
        break;
    default:
        break;
    }
    return false;
}

} // namespace phy104
