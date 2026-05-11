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
    ~FirstLaunchWizard();
protected:
    void showEvent(QShowEvent *event) override;
    void reject() override;
signals:
    void wizardCanceled();
};

#endif // FIRSTLAUNCHWIZARD_H
