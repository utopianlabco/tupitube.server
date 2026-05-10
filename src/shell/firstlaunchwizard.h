#ifndef FIRSTLAUNCHWIZARD_H
#define FIRSTLAUNCHWIZARD_H

#include <QWizard>
#include <QWizardPage>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>

class StudentPage : public QWizardPage {
    Q_OBJECT
public:
    StudentPage(QWidget *parent = nullptr);
    QLineEdit *studentUsernameEdit;
    QLineEdit *studentFullNameEdit;
    QLineEdit *studentPasswordEdit;
    QLineEdit *studentConfirmPasswordEdit;
    QCheckBox *isCreatorCheck;
    QLineEdit *studentClassEdit;
    QLabel *passwordMismatchLabel;
    bool isComplete() const override;
private slots:
    void onFieldChanged();
};

class FirstLaunchWizard : public QWizard {
    Q_OBJECT
public:
    explicit FirstLaunchWizard(QWidget *parent = nullptr);
protected:
    void showEvent(QShowEvent *event) override;
};

#endif // FIRSTLAUNCHWIZARD_H
