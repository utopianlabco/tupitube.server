/***************************************************************************
 *   Project TupiTube Server                                               *
 *   Project Contact: info@tupitube.com                                    *
 *   Project Website: http://www.tupitube.com                              *
 *                                                                         *
 *   Developers:                                                           *
 *   2025:                                                                 *
 *    Utopian Lab Development Team                                         *
 *   2010:                                                                 *
 *    Gustav Gonzalez                                                      *
 *   ---                                                                   *
 *   KTooN's versions:                                                     *
 *   2006:                                                                 *
 *    David Cuadrado                                                       *
 *    Jorge Cuadrado                                                       *
 *   2003:                                                                 *
 *    Fernado Roldan                                                       *
 *    Simena Dinas                                                         *
 *                                                                         *
 *   License:                                                              *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "firstlaunchwizard.h"

#include <QScreen>
#include <QApplication>
#include <QShowEvent>
#include <QWizardPage>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include <QPushButton>
#include <QStyle>
#include <QSpinBox>
#include <QDateEdit>
#include <QCheckBox>
#include <QDebug>

// --- Class Page ---
class ClassPage : public QWizardPage {
public:
    ClassPage(QWidget *parent = nullptr) : QWizardPage(parent) {
        setTitle("Create Class");
        QFormLayout *layout = new QFormLayout;
        classNameEdit = new QLineEdit;
        classYearSpin = new QSpinBox;
        classYearSpin->setRange(2000, 2100);
        classYearSpin->setValue(QDate::currentDate().year());
        classDescEdit = new QLineEdit;
        layout->addRow("Class Name:", classNameEdit);
        layout->addRow("Year:", classYearSpin);
        layout->addRow("Description:", classDescEdit);
        setLayout(layout);
        registerField("className*", classNameEdit);
        registerField("classYear", classYearSpin);
        registerField("classDesc", classDescEdit);
    }
    QLineEdit *classNameEdit;
    QSpinBox *classYearSpin;
    QLineEdit *classDescEdit;
};

// --- Period Page ---
class PeriodPage : public QWizardPage {
public:
    PeriodPage(QWidget *parent = nullptr) : QWizardPage(parent) {
        setTitle("Create Period");
        QFormLayout *layout = new QFormLayout;
        periodNameEdit = new QLineEdit;
        periodYearSpin = new QSpinBox;
        periodYearSpin->setRange(2000, 2100);
        periodYearSpin->setValue(QDate::currentDate().year());
        startDateEdit = new QDateEdit(QDate::currentDate());
        startDateEdit->setCalendarPopup(true);
        endDateEdit = new QDateEdit(QDate::currentDate());
        endDateEdit->setCalendarPopup(true);
        layout->addRow("Period Name:", periodNameEdit);
        layout->addRow("Year:", periodYearSpin);
        layout->addRow("Start Date:", startDateEdit);
        layout->addRow("End Date:", endDateEdit);
        setLayout(layout);
        registerField("periodName*", periodNameEdit);
        registerField("periodYear", periodYearSpin);
        registerField("periodStartDate", startDateEdit);
        registerField("periodEndDate", endDateEdit);
    }
    QLineEdit *periodNameEdit;
    QSpinBox *periodYearSpin;
    QDateEdit *startDateEdit;
    QDateEdit *endDateEdit;
};

// --- Student Page ---

StudentPage::StudentPage(QWidget *parent) : QWizardPage(parent) {
    setTitle("Create Student");
    QFormLayout *layout = new QFormLayout;
    studentUsernameEdit = new QLineEdit;
    studentFullNameEdit = new QLineEdit;
    studentPasswordEdit = new QLineEdit;
    studentPasswordEdit->setEchoMode(QLineEdit::Password);
    studentConfirmPasswordEdit = new QLineEdit;
    studentConfirmPasswordEdit->setEchoMode(QLineEdit::Password);
    isCreatorCheck = new QCheckBox("Is Creator");
    isCreatorCheck->setChecked(true);
    studentClassEdit = new QLineEdit;
    studentClassEdit->setReadOnly(true); // Will be set from class page after wizard is constructed
    passwordMismatchLabel = new QLabel;
    passwordMismatchLabel->setStyleSheet("color: red");
    passwordMismatchLabel->setVisible(false);
    layout->addRow("Username:", studentUsernameEdit);
    layout->addRow("Full Name:", studentFullNameEdit);
    layout->addRow("Password:", studentPasswordEdit);
    layout->addRow("Confirm Password:", studentConfirmPasswordEdit);
    layout->addRow(passwordMismatchLabel);
    layout->addRow("Class:", studentClassEdit);
    layout->addRow(isCreatorCheck);
    setLayout(layout);
    registerField("studentUsername*", studentUsernameEdit);
    registerField("studentFullName*", studentFullNameEdit);
    registerField("studentPassword", studentPasswordEdit);
    // Always enabled by default, do not show checkbox
    registerField("studentIsEnabled", new QWidget, "visible", "visible"); // dummy, not used
    registerField("studentIsCreator", isCreatorCheck);
    registerField("studentClass", studentClassEdit);

    // Connect signals to update completeness
    connect(studentUsernameEdit, &QLineEdit::textChanged, this, &StudentPage::onFieldChanged);
    connect(studentFullNameEdit, &QLineEdit::textChanged, this, &StudentPage::onFieldChanged);
    connect(studentPasswordEdit, &QLineEdit::textChanged, this, &StudentPage::onFieldChanged);
    connect(studentConfirmPasswordEdit, &QLineEdit::textChanged, this, &StudentPage::onFieldChanged);
    connect(studentClassEdit, &QLineEdit::textChanged, this, &StudentPage::onFieldChanged);
}

bool StudentPage::isComplete() const {
    bool allFilled = !studentUsernameEdit->text().isEmpty()
        && !studentFullNameEdit->text().isEmpty()
        && !studentPasswordEdit->text().isEmpty()
        && !studentConfirmPasswordEdit->text().isEmpty()
        && !studentClassEdit->text().isEmpty();
    bool passwordsMatch = studentPasswordEdit->text() == studentConfirmPasswordEdit->text();
    return allFilled && passwordsMatch;
}

void StudentPage::onFieldChanged() {
    // Show/hide password mismatch label
    bool showMismatch = !studentPasswordEdit->text().isEmpty()
        && !studentConfirmPasswordEdit->text().isEmpty()
        && studentPasswordEdit->text() != studentConfirmPasswordEdit->text();
    passwordMismatchLabel->setVisible(showMismatch);
    passwordMismatchLabel->setText(showMismatch ? "Passwords do not match" : "");
    emit completeChanged();
}

// --- FirstLaunchWizard Implementation ---
FirstLaunchWizard::FirstLaunchWizard(QWidget *parent) : QWizard(parent) {
    setWindowTitle("First Launch Setup Wizard");
    ClassPage *classPage = new ClassPage;
    PeriodPage *periodPage = new PeriodPage;
    StudentPage *studentPage = new StudentPage;
    addPage(classPage);
    addPage(periodPage);
    addPage(studentPage);

    // Set the class name in the student page after the class page is completed
    connect(this, &QWizard::currentIdChanged, this, [this, classPage, studentPage](int id) {
        if (id == 2) { // Student page
            studentPage->studentClassEdit->setText(classPage->classNameEdit->text());
        }
    });

    // Replace Next and Back button text with arrow icons
    QPushButton *nextBtn = new QPushButton;
    nextBtn->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    setButton(QWizard::NextButton, nextBtn);

    QPushButton *backBtn = new QPushButton;
    backBtn->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    setButton(QWizard::BackButton, backBtn);
}

void FirstLaunchWizard::showEvent(QShowEvent *event) {
    QWizard::showEvent(event);
    // Center the wizard on the primary screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = screenGeometry.x() + (screenGeometry.width() - width()) / 2;
        int y = screenGeometry.y() + (screenGeometry.height() - height()) / 2;
        move(x, y);
    }
}

FirstLaunchWizard::~FirstLaunchWizard() {
    qDebug() << "[FirstLaunchWizard] Destructor called.";
}

void FirstLaunchWizard::reject() {
    qDebug() << "[FirstLaunchWizard] reject() called. Dialog will close with Rejected.";
    QWizard::reject();
}
