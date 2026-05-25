#ifndef GRADEBOOKDIALOG_H
#define GRADEBOOKDIALOG_H

#include "databasehandler.h"

#include <QDialog>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class GradeBookDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GradeBookDialog(DatabaseHandler *db,
                             const DatabaseHandler::ClassInfo &classInfo,
                             QWidget *parent = nullptr);

private slots:
    void reload();
    void applyFilter(const QString &text);
    void onCellChanged(int row, int column);
    void saveChanges();
    void exportCsv();

private:
    void buildTable();
    void applyRowStyle(int row, const QString &grade);

    DatabaseHandler *m_db;
    DatabaseHandler::ClassInfo m_class;

    QComboBox *m_periodCombo;
    QLineEdit  *m_searchEdit;
    QTableWidget *m_table;
    QPushButton *m_saveButton;
    QPushButton *m_exportButton;

    // Pending edits: key = (projectId, studentId), value = {grade, comments}
    struct PendingGrade {
        int projectId;
        int studentId;
        QString grade;
        QString comments;
        int periodId;
        int classId;
    };
    QList<PendingGrade> m_pending;
    bool m_loading; // suppress cellChanged during programmatic fill
};

#endif // GRADEBOOKDIALOG_H
