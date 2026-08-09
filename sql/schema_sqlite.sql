-- TupiTube Server Definitive SQLite Schema
-- Database: tupitube.db
-- For classroom sessions with up to 30 students
-- Merged and updated: August 7, 2026
-- Collaboration architecture: durable commands, revisions, and project events

PRAGMA foreign_keys=on;

-- Class table
CREATE TABLE IF NOT EXISTS tupitube_class (
    class_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(50) NOT NULL,         -- e.g., '7B'
    year INTEGER NOT NULL,             -- e.g., 2026
    description TEXT
);

-- Period table
CREATE TABLE IF NOT EXISTS tupitube_period (
    period_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(50) NOT NULL,         -- e.g., 'Semester 1'
    year INTEGER NOT NULL,             -- e.g., 2026
    start_date DATE,
    end_date DATE
);

-- Student table
CREATE TABLE IF NOT EXISTS tupitube_student (
    student_id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(100),
    studentname VARCHAR(50) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    is_enabled INTEGER DEFAULT 1,
    is_creator INTEGER DEFAULT 1,
    projects_public_policy INTEGER DEFAULT 0,
    files_public_policy INTEGER DEFAULT 0,
    works_public_policy INTEGER DEFAULT 0,
    class_id INTEGER NOT NULL,
    created_at DATETIME DEFAULT (datetime('now')),
    updated_at DATETIME DEFAULT (datetime('now')),
    FOREIGN KEY (class_id) REFERENCES tupitube_class(class_id) ON DELETE RESTRICT
);

-- Project table
CREATE TABLE IF NOT EXISTS tupitube_project (
    project_id INTEGER PRIMARY KEY AUTOINCREMENT,
    title VARCHAR(100) NOT NULL,
    description TEXT,
    student_id INTEGER NOT NULL,
    filename VARCHAR(255) NOT NULL,
    is_public INTEGER DEFAULT 0,
    is_shared INTEGER DEFAULT 0,
    class_id INTEGER NOT NULL,
    period_id INTEGER NOT NULL,
    group_project INTEGER DEFAULT 0, -- 0: individual, 1: group

    -- Server-authoritative collaboration state.
    -- current_revision advances after each durably committed project command.
    -- snapshot_revision identifies the revision represented by the current .tup snapshot.
    current_revision INTEGER NOT NULL DEFAULT 0 CHECK (current_revision >= 0),
    snapshot_revision INTEGER NOT NULL DEFAULT 0 CHECK (snapshot_revision >= 0),
    snapshot_checksum TEXT, -- Optional checksum of the durable .tup snapshot
    snapshot_updated_at DATETIME,

    created_at DATETIME DEFAULT (datetime('now')),
    updated_at DATETIME DEFAULT (datetime('now')),
    last_rendered_at DATETIME, -- Timestamp of last successful render
    FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id) ON DELETE RESTRICT,
    FOREIGN KEY (class_id) REFERENCES tupitube_class(class_id) ON DELETE RESTRICT,
    FOREIGN KEY (period_id) REFERENCES tupitube_period(period_id) ON DELETE RESTRICT
);

-- Project-Student join table (for group projects)
CREATE TABLE IF NOT EXISTS tupitube_project_student (
    project_id INTEGER NOT NULL,
    student_id INTEGER NOT NULL,
    PRIMARY KEY (project_id, student_id),
    FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id) ON DELETE RESTRICT,
    FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id) ON DELETE RESTRICT
);

-- Collection table (for storyboards and folders)
CREATE TABLE IF NOT EXISTS tupitube_collection (
    collection_id INTEGER PRIMARY KEY AUTOINCREMENT,
    parent_id INTEGER,
    type VARCHAR(20),
    title VARCHAR(100),
    topics TEXT,
    description TEXT,
    student_id INTEGER NOT NULL,
    is_public INTEGER DEFAULT 0,
    visits INTEGER DEFAULT 0,
    likes INTEGER DEFAULT 0,
    project_id INTEGER,
    path VARCHAR(255),
    slug VARCHAR(100),
    created_at DATETIME DEFAULT (datetime('now')),
    updated_at DATETIME DEFAULT (datetime('now')),
    FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id) ON DELETE RESTRICT,
    FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id) ON DELETE RESTRICT,
    FOREIGN KEY (parent_id) REFERENCES tupitube_collection(collection_id) ON DELETE RESTRICT
);

-- Work table (animations, images, frames)
CREATE TABLE IF NOT EXISTS tupitube_work (
    work_id INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id INTEGER,
    project_id INTEGER,
    collection_id INTEGER,
    type_id INTEGER,
    type VARCHAR(20), -- e.g., 'animation', 'image'
    title VARCHAR(100),
    content TEXT,
    topics TEXT,
    tags TEXT,
    description TEXT,
    filename VARCHAR(255),
    is_public INTEGER DEFAULT 0,
    enabled INTEGER DEFAULT 0,
    visits INTEGER DEFAULT 0,
    duration REAL DEFAULT 0,
    portrait INTEGER DEFAULT 0,
    mobile INTEGER DEFAULT 0,
    rendered INTEGER DEFAULT 0, -- 1 if up-to-date, 0 if needs rendering
    uploaded INTEGER DEFAULT 0,
    render_status VARCHAR(20) DEFAULT 'pending', -- 'pending', 'success', 'failed'
    render_message TEXT, -- error or status message
    render_output VARCHAR(255), -- path to rendered file (e.g., MP4)
    last_rendered_at DATETIME, -- Timestamp of last render for this work
    created_at DATETIME DEFAULT (datetime('now')),
    updated_at DATETIME DEFAULT (datetime('now')),
    FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id) ON DELETE RESTRICT,
    FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id) ON DELETE RESTRICT,
    FOREIGN KEY (collection_id) REFERENCES tupitube_collection(collection_id) ON DELETE RESTRICT
);

-- Collaboration table
CREATE TABLE IF NOT EXISTS tupitube_collaboration (
    collaboration_id INTEGER PRIMARY KEY AUTOINCREMENT,
    student_id INTEGER NOT NULL,
    project_id INTEGER NOT NULL,
    permission_level INTEGER DEFAULT 1,
    created_at DATETIME DEFAULT (datetime('now')),
    FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id) ON DELETE RESTRICT,
    FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id) ON DELETE RESTRICT,
    UNIQUE(student_id, project_id)
);

-- Grade table
CREATE TABLE IF NOT EXISTS tupitube_grade (
    grade_id INTEGER PRIMARY KEY AUTOINCREMENT,
    project_id INTEGER NOT NULL,
    student_id INTEGER NOT NULL,
    teacher_student_id INTEGER DEFAULT 0,
    period_id INTEGER NOT NULL,
    class_id INTEGER NOT NULL,
    grade TEXT NOT NULL,
    comments TEXT,
    created_at DATETIME DEFAULT (datetime('now')),
    updated_at DATETIME DEFAULT (datetime('now')),
    FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id) ON DELETE RESTRICT,
    FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id) ON DELETE RESTRICT,
    FOREIGN KEY (period_id) REFERENCES tupitube_period(period_id) ON DELETE RESTRICT,
    FOREIGN KEY (class_id) REFERENCES tupitube_class(class_id) ON DELETE RESTRICT,
    UNIQUE (project_id, student_id, teacher_student_id, period_id, class_id)
);


-- Durable project command journal.
-- This table is the persistent source for command idempotency and command results.
-- The in-memory CommandResultRegistry may cache rows from this table, but must not
-- be treated as authoritative across project close/reopen or server restart.
CREATE TABLE IF NOT EXISTS tupitube_project_command (
    project_id INTEGER NOT NULL,
    command_id TEXT NOT NULL,

    -- Origin and concurrency metadata.
    student_id INTEGER,
    client_id TEXT,
    command_type VARCHAR(100),
    base_revision INTEGER NOT NULL DEFAULT 0 CHECK (base_revision >= 0),
    depends_on_command_id TEXT,

    -- Hash of the canonical command request. This allows the server to detect
    -- accidental reuse of a command_id with different content without storing
    -- large request bodies (for example, base64 encoded imported images).
    request_hash TEXT,

    -- Lifecycle/result state. Terminal states are committed, rejected, and failed.
    status VARCHAR(20) NOT NULL
        CHECK (status IN ('received', 'queued', 'processing', 'committed', 'rejected', 'failed')),
    error_code VARCHAR(100),
    message TEXT,

    -- Set only when the command has durably changed authoritative project state.
    committed_revision INTEGER CHECK (committed_revision IS NULL OR committed_revision >= 0),

    created_at DATETIME NOT NULL DEFAULT (datetime('now')),
    updated_at DATETIME NOT NULL DEFAULT (datetime('now')),
    completed_at DATETIME,

    PRIMARY KEY (project_id, command_id),
    FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id) ON DELETE RESTRICT,
    FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id) ON DELETE SET NULL,
    FOREIGN KEY (project_id, depends_on_command_id)
        REFERENCES tupitube_project_command(project_id, command_id) ON DELETE RESTRICT,

    CHECK (
        (status = 'committed' AND committed_revision IS NOT NULL)
        OR
        (status <> 'committed' AND committed_revision IS NULL)
    )
);

-- Append-only authoritative project event log.
-- A single committed command may emit more than one event; event_index preserves
-- their deterministic order inside the command's resulting project revision.
CREATE TABLE IF NOT EXISTS tupitube_project_event (
    event_id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_uuid TEXT NOT NULL UNIQUE,
    project_id INTEGER NOT NULL,
    command_id TEXT NOT NULL,
    revision INTEGER NOT NULL CHECK (revision > 0),
    event_index INTEGER NOT NULL DEFAULT 0 CHECK (event_index >= 0),
    event_type VARCHAR(100) NOT NULL,
    payload TEXT,
    created_at DATETIME NOT NULL DEFAULT (datetime('now')),

    FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id) ON DELETE RESTRICT,
    FOREIGN KEY (project_id, command_id)
        REFERENCES tupitube_project_command(project_id, command_id) ON DELETE RESTRICT,
    UNIQUE (project_id, revision, event_index)
);

-- Log table
CREATE TABLE IF NOT EXISTS tupitube_log (
    log_id INTEGER PRIMARY KEY AUTOINCREMENT,
    type VARCHAR(50),
    filename VARCHAR(255),
    ip VARCHAR(45),
    date DATETIME DEFAULT (datetime('now'))
);

-- Chat messages table (for teacher review)
CREATE TABLE IF NOT EXISTS tupitube_chat (
    chat_id INTEGER PRIMARY KEY AUTOINCREMENT,
    project_id INTEGER,
    student_id INTEGER NOT NULL,
    studentname VARCHAR(50) NOT NULL,
    message TEXT NOT NULL,
    message_type VARCHAR(20) DEFAULT 'chat',
    created_at DATETIME DEFAULT (datetime('now')),
    FOREIGN KEY (student_id) REFERENCES tupitube_student(student_id) ON DELETE RESTRICT,
    FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id) ON DELETE RESTRICT
);

-- Student table for HumHub compatibility (optional)
CREATE TABLE IF NOT EXISTS student (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    studentname VARCHAR(50) NOT NULL UNIQUE
);

-- Indexes for new/updated tables
CREATE INDEX IF NOT EXISTS idx_student_class ON tupitube_student(class_id);
CREATE INDEX IF NOT EXISTS idx_project_class ON tupitube_project(class_id);
CREATE INDEX IF NOT EXISTS idx_project_period ON tupitube_project(period_id);
CREATE INDEX IF NOT EXISTS idx_grade_period ON tupitube_grade(period_id);
CREATE INDEX IF NOT EXISTS idx_grade_class ON tupitube_grade(class_id);
CREATE INDEX IF NOT EXISTS idx_project_owner ON tupitube_project(student_id);
CREATE INDEX IF NOT EXISTS idx_project_filename ON tupitube_project(filename);
CREATE INDEX IF NOT EXISTS idx_work_owner ON tupitube_work(student_id);
CREATE INDEX IF NOT EXISTS idx_work_project ON tupitube_work(project_id);
CREATE INDEX IF NOT EXISTS idx_collection_owner ON tupitube_collection(student_id);
CREATE INDEX IF NOT EXISTS idx_collection_slug ON tupitube_collection(slug);
CREATE INDEX IF NOT EXISTS idx_collaboration_student ON tupitube_collaboration(student_id);
CREATE INDEX IF NOT EXISTS idx_collaboration_project ON tupitube_collaboration(project_id);
CREATE INDEX IF NOT EXISTS idx_student_studentname ON tupitube_student(studentname);
CREATE INDEX IF NOT EXISTS idx_chat_project ON tupitube_chat(project_id);
CREATE INDEX IF NOT EXISTS idx_chat_student ON tupitube_chat(student_id);
CREATE INDEX IF NOT EXISTS idx_chat_created ON tupitube_chat(created_at);
CREATE INDEX IF NOT EXISTS idx_grade_project ON tupitube_grade(project_id);
CREATE INDEX IF NOT EXISTS idx_grade_student ON tupitube_grade(student_id);
CREATE INDEX IF NOT EXISTS idx_grade_teacher ON tupitube_grade(teacher_student_id);

-- Collaboration command/event indexes
CREATE INDEX IF NOT EXISTS idx_project_command_status
    ON tupitube_project_command(project_id, status);
CREATE INDEX IF NOT EXISTS idx_project_command_created
    ON tupitube_project_command(project_id, created_at);
CREATE INDEX IF NOT EXISTS idx_project_command_dependency
    ON tupitube_project_command(project_id, depends_on_command_id);
CREATE INDEX IF NOT EXISTS idx_project_command_revision
    ON tupitube_project_command(project_id, committed_revision);
CREATE INDEX IF NOT EXISTS idx_project_event_revision
    ON tupitube_project_event(project_id, revision, event_index);
CREATE INDEX IF NOT EXISTS idx_project_event_command
    ON tupitube_project_event(project_id, command_id);
CREATE INDEX IF NOT EXISTS idx_project_event_type
    ON tupitube_project_event(project_id, event_type);

