#ifndef FIRSTLAUNCHWIZARD_H
#define FIRSTLAUNCHWIZARD_H

#include <QWizard>


class FirstLaunchWizard : public QWizard {
    Q_OBJECT
public:
    explicit FirstLaunchWizard(QWidget *parent = nullptr);
protected:
    void showEvent(QShowEvent *event) override;
};

#endif // FIRSTLAUNCHWIZARD_H
