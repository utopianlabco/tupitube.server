#include "gradebookdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QLabel>
#include <QTextStream>
#include <QFile>
#include <QDate>
#include <QSet>
#include <QLineEdit>

// Column indices
static const int COL_STUDENT   = 0;
static const int COL_ROLE      = 1;
static const int COL_PROJECT   = 2;
static const int COL_PERIOD    = 3;
static const int COL_GRADE     = 4;
static const int COL_COMMENTS  = 5;

GradeBookDialog::GradeBookDialog(DatabaseHandler *db,
                                 const DatabaseHandler::ClassInfo &classInfo,
                                 QWidget *parent)
    : QDialog(parent), m_db(db), m_class(classInfo), m_loading(false)
{
    setWindowTitle(tr("Grade Book — %1").arg(m_class.name));
    setMinimumSize(820, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Top bar: period filter + text search
    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->addWidget(new QLabel(tr("Period:")));
    m_periodCombo = new QComboBox();
    m_periodCombo->addItem(tr("All Periods"), -1);
    for (const DatabaseHandler::PeriodInfo &p : m_db->getAllPeriods())
        m_periodCombo->addItem(p.name, p.periodId);
    topBar->addWidget(m_periodCombo);
    topBar->addSpacing(16);
    topBar->addWidget(new QLabel(tr("Search:")));
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("Filter by student, project, grade…"));
    m_searchEdit->setClearButtonEnabled(true);
    topBar->addWidget(m_searchEdit);
    mainLayout->addLayout(topBar);

    // Table
    m_table = new QTableWidget(0, 6);
    m_table->setHorizontalHeaderLabels({tr("Student"), tr("Role"), tr("Project"), tr("Period"),
                                        tr("Grade"), tr("Comments")});
    m_table->horizontalHeader()->setSectionResizeMode(COL_STUDENT,   QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_ROLE,      QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_PROJECT,   QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(COL_PERIOD,    QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_GRADE,     QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(COL_COMMENTS,  QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    mainLayout->addWidget(m_table);

    // Bottom bar: export + save/close
    QHBoxLayout *bottomBar = new QHBoxLayout();
    m_exportButton = new QPushButton(tr("Export CSV"));
    bottomBar->addWidget(m_exportButton);
    bottomBar->addStretch();
    m_saveButton = new QPushButton(tr("Save"));
    m_saveButton->setEnabled(false);
    QPushButton *closeButton = new QPushButton(tr("Close"));
    bottomBar->addWidget(m_saveButton);
    bottomBar->addWidget(closeButton);
    mainLayout->addLayout(bottomBar);

    connect(m_periodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GradeBookDialog::reload);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &GradeBookDialog::applyFilter);
    connect(m_table, &QTableWidget::cellChanged, this, &GradeBookDialog::onCellChanged);
    connect(m_saveButton, &QPushButton::clicked, this, &GradeBookDialog::saveChanges);
    connect(m_exportButton, &QPushButton::clicked, this, &GradeBookDialog::exportCsv);
    connect(closeButton, &QPushButton::clicked, this, [this]() {
        if (!m_pending.isEmpty()) {
            auto btn = QMessageBox::question(this, tr("Unsaved Changes"),
                tr("You have unsaved grade changes. Save before closing?"),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                QMessageBox::Save);
            if (btn == QMessageBox::Cancel)
                return;
            if (btn == QMessageBox::Save)
                saveChanges();
        }
        accept();
    });

    reload();
}

void GradeBookDialog::reload()
{
    m_pending.clear();
    m_saveButton->setEnabled(false);
    buildTable();
    applyFilter(m_searchEdit->text());
}

void GradeBookDialog::applyFilter(const QString &text)
{
    const QString pattern = text.trimmed();
    for (int row = 0; row < m_table->rowCount(); row++) {
        if (pattern.isEmpty()) {
            m_table->setRowHidden(row, false);
            continue;
        }
        bool match = false;
        for (int col = 0; col < m_table->columnCount(); col++) {
            QTableWidgetItem *it = m_table->item(row, col);
            if (it && it->text().contains(pattern, Qt::CaseInsensitive)) {
                match = true;
                break;
            }
        }
        m_table->setRowHidden(row, !match);
    }
}

void GradeBookDialog::buildTable()
{
    m_loading = true;
    m_table->setRowCount(0);

    int filterPeriodId = m_periodCombo->currentData().toInt();

    // All students in this class
    QList<DatabaseHandler::StudentInfo> allStudents = m_db->getAllStudents();
    QList<DatabaseHandler::StudentInfo> classStudents;
    for (const auto &s : allStudents) {
        if (s.classId == m_class.classId)
            classStudents.append(s);
    }

    // All projects (we'll match by ownerId and classId)
    QList<DatabaseHandler::ProjectRecord> allProjects = m_db->getAllProjects();

    // Build a map: studentId -> list of (project, isOwner) entries
    struct ProjectEntry { DatabaseHandler::ProjectRecord project; bool isOwner; };
    QHash<int, QList<ProjectEntry>> studentEntries;

    // Fast lookup: which students belong to this class
    QSet<int> classStudentIds;
    for (const auto &s : classStudents)
        classStudentIds.insert(s.studentId);

    for (const auto &p : allProjects) {
        if (p.classId != m_class.classId)
            continue;
        if (filterPeriodId != -1 && p.periodId != filterPeriodId)
            continue;
        // Owner
        studentEntries[p.ownerId].append({p, true});
        // Collaborators who are members of this class
        for (const auto &collab : m_db->getProjectCollaborators(p.projectId)) {
            if (classStudentIds.contains(collab.studentId))
                studentEntries[collab.studentId].append({p, false});
        }
    }

    for (const auto &student : classStudents) {
        if (!studentEntries.contains(student.studentId)) {
            // Student has no project (under this filter) — show a placeholder row
            int row = m_table->rowCount();
            m_table->insertRow(row);

            auto *nameItem = new QTableWidgetItem(student.studentname);
            nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
            m_table->setItem(row, COL_STUDENT, nameItem);

            auto *roleItem = new QTableWidgetItem("—");
            roleItem->setFlags(roleItem->flags() & ~Qt::ItemIsEditable);
            m_table->setItem(row, COL_ROLE, roleItem);

            auto *projItem = new QTableWidgetItem(tr("(no project)"));
            projItem->setFlags(projItem->flags() & ~Qt::ItemIsEditable);
            projItem->setForeground(QColor("#888888"));
            m_table->setItem(row, COL_PROJECT, projItem);

            auto *periodItem = new QTableWidgetItem("—");
            periodItem->setFlags(periodItem->flags() & ~Qt::ItemIsEditable);
            m_table->setItem(row, COL_PERIOD, periodItem);

            auto *gradeItem = new QTableWidgetItem("—");
            gradeItem->setFlags(gradeItem->flags() & ~Qt::ItemIsEditable);
            m_table->setItem(row, COL_GRADE, gradeItem);

            auto *commentsItem = new QTableWidgetItem("");
            commentsItem->setFlags(commentsItem->flags() & ~Qt::ItemIsEditable);
            m_table->setItem(row, COL_COMMENTS, commentsItem);

            applyRowStyle(row, QString());
        } else {
            for (const auto &entry : studentEntries[student.studentId]) {
                const auto &project = entry.project;
                int row = m_table->rowCount();
                m_table->insertRow(row);

                auto *nameItem = new QTableWidgetItem(student.studentname);
                nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
                // Store IDs for saving: projectId in UserRole, studentId in UserRole+1
                nameItem->setData(Qt::UserRole,     project.projectId);
                nameItem->setData(Qt::UserRole + 1, student.studentId);
                nameItem->setData(Qt::UserRole + 2, project.periodId);
                m_table->setItem(row, COL_STUDENT, nameItem);

                auto *roleItem = new QTableWidgetItem(entry.isOwner ? tr("Owner") : tr("Collaborator"));
                roleItem->setFlags(roleItem->flags() & ~Qt::ItemIsEditable);
                m_table->setItem(row, COL_ROLE, roleItem);

                auto *projItem = new QTableWidgetItem(project.title);
                projItem->setFlags(projItem->flags() & ~Qt::ItemIsEditable);
                m_table->setItem(row, COL_PROJECT, projItem);

                auto *periodItem = new QTableWidgetItem(project.periodName);
                periodItem->setFlags(periodItem->flags() & ~Qt::ItemIsEditable);
                m_table->setItem(row, COL_PERIOD, periodItem);

                // Load existing grade
                DatabaseHandler::GradeInfo gi = m_db->getGrade(project.projectId, student.studentId);
                QString gradeText = gi.found ? gi.grade : QString();
                QString commentsText = gi.found ? gi.comments : QString();

                auto *gradeItem = new QTableWidgetItem(gradeText);
                m_table->setItem(row, COL_GRADE, gradeItem);

                auto *commentsItem = new QTableWidgetItem(commentsText);
                m_table->setItem(row, COL_COMMENTS, commentsItem);

                applyRowStyle(row, gradeText);
            }
        }
    }

    m_loading = false;
}

void GradeBookDialog::applyRowStyle(int row, const QString &grade)
{
    // No project or no grade → amber tint; graded → no tint
    bool hasProject = m_table->item(row, COL_STUDENT) &&
                      m_table->item(row, COL_STUDENT)->data(Qt::UserRole).isValid() &&
                      m_table->item(row, COL_STUDENT)->data(Qt::UserRole).toInt() > 0;
    bool hasGrade = !grade.isEmpty();

    QColor bg;
    if (!hasProject) {
        bg = QColor("#555555"); // gray — no project
    } else if (!hasGrade) {
        bg = QColor("#7a6000"); // amber — ungraded
    } else {
        bg = QColor(); // default
    }

    if (!bg.isValid())
        return;

    for (int c = 0; c < m_table->columnCount(); c++) {
        QTableWidgetItem *it = m_table->item(row, c);
        if (it)
            it->setBackground(bg);
    }
}

void GradeBookDialog::onCellChanged(int row, int column)
{
    if (m_loading)
        return;
    if (column != COL_GRADE && column != COL_COMMENTS)
        return;

    QTableWidgetItem *nameItem = m_table->item(row, COL_STUDENT);
    if (!nameItem || !nameItem->data(Qt::UserRole).isValid())
        return; // no-project row — not editable

    int projectId = nameItem->data(Qt::UserRole).toInt();
    int studentId = nameItem->data(Qt::UserRole + 1).toInt();
    int periodId  = nameItem->data(Qt::UserRole + 2).toInt();
    if (projectId <= 0)
        return;

    QString grade    = m_table->item(row, COL_GRADE)    ? m_table->item(row, COL_GRADE)->text()    : QString();
    QString comments = m_table->item(row, COL_COMMENTS) ? m_table->item(row, COL_COMMENTS)->text() : QString();

    // Update or insert into pending list
    for (PendingGrade &pg : m_pending) {
        if (pg.projectId == projectId && pg.studentId == studentId) {
            pg.grade    = grade;
            pg.comments = comments;
            m_saveButton->setEnabled(true);
            applyRowStyle(row, grade);
            return;
        }
    }
    m_pending.append({projectId, studentId, grade, comments, periodId, m_class.classId});
    m_saveButton->setEnabled(true);
    applyRowStyle(row, grade);
}

void GradeBookDialog::saveChanges()
{
    // teacherStudentId = 0 (admin/teacher is not a student record in this schema)
    for (const PendingGrade &pg : m_pending) {
        m_db->saveGrade(pg.projectId, pg.studentId, 0,
                        pg.periodId, pg.classId, pg.grade, pg.comments);
    }
    m_pending.clear();
    m_saveButton->setEnabled(false);
}

void GradeBookDialog::exportCsv()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Export Grade Book"),
        QString("gradebook_%1.csv").arg(m_class.name),
        tr("CSV Files (*.csv)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export Failed"), tr("Could not open file for writing."));
        return;
    }

    QTextStream out(&file);
    out << "Student,Role,Project,Period,Grade,Comments\n";
    for (int row = 0; row < m_table->rowCount(); row++) {
        QStringList fields;
        for (int col = 0; col < m_table->columnCount(); col++) {
            QString val = m_table->item(row, col) ? m_table->item(row, col)->text() : QString();
            // Quote fields that contain commas or quotes
            if (val.contains(',') || val.contains('"') || val.contains('\n')) {
                val.replace("\"", "\"\"");
                val = "\"" + val + "\"";
            }
            fields << val;
        }
        out << fields.join(",") << "\n";
    }
    file.close();
}
