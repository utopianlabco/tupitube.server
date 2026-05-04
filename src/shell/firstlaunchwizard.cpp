#include "firstlaunchwizard.h"
#include <QWizardPage>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>

// --- Class Page ---
class ClassPage : public QWizardPage {
public:
    ClassPage(QWidget *parent = nullptr) : QWizardPage(parent) {
        setTitle("Create Class");
        QFormLayout *layout = new QFormLayout;
        classNameEdit = new QLineEdit;
        layout->addRow("Class Name:", classNameEdit);
        setLayout(layout);
        registerField("className*", classNameEdit);
    }
    QLineEdit *classNameEdit;
};

// --- Period Page ---
class PeriodPage : public QWizardPage {
public:
    PeriodPage(QWidget *parent = nullptr) : QWizardPage(parent) {
        setTitle("Create Period");
        QFormLayout *layout = new QFormLayout;
        periodNameEdit = new QLineEdit;
        layout->addRow("Period Name:", periodNameEdit);
        setLayout(layout);
        registerField("periodName*", periodNameEdit);
    }
    QLineEdit *periodNameEdit;
};

// --- Student Page ---
class StudentPage : public QWizardPage {
public:
    StudentPage(QWidget *parent = nullptr) : QWizardPage(parent) {
        setTitle("Create Student");
        QFormLayout *layout = new QFormLayout;
        studentNameEdit = new QLineEdit;
        layout->addRow("Student Name:", studentNameEdit);
        setLayout(layout);
        registerField("studentName*", studentNameEdit);
    }
    QLineEdit *studentNameEdit;
};

// --- FirstLaunchWizard Implementation ---
FirstLaunchWizard::FirstLaunchWizard(QWidget *parent) : QWizard(parent) {
    setWindowTitle("First Launch Setup Wizard");
    addPage(new ClassPage);
    addPage(new PeriodPage);
    addPage(new StudentPage);
}
