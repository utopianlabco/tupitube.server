-- TupiTube Server SQLite Schema
-- Database: tupitube.db
-- For classroom sessions with up to 30 students

-- User table
CREATE TABLE IF NOT EXISTS tupitube_user (
    user_id INTEGER PRIMARY KEY AUTOINCREMENT,
    username VARCHAR(50) NOT NULL UNIQUE,
    name VARCHAR(100),
    password VARCHAR(255) NOT NULL,
    is_enabled INTEGER DEFAULT 1,
    is_creator INTEGER DEFAULT 1,
    projects_public_policy INTEGER DEFAULT 0,
    files_public_policy INTEGER DEFAULT 0,
    works_public_policy INTEGER DEFAULT 0,
    created_at DATETIME DEFAULT (datetime('now')),
    updated_at DATETIME DEFAULT (datetime('now'))
);

-- Project table
CREATE TABLE IF NOT EXISTS tupitube_project (
    project_id INTEGER PRIMARY KEY AUTOINCREMENT,
    title VARCHAR(100) NOT NULL,
    description TEXT,
    owner_id INTEGER NOT NULL,
    filename VARCHAR(255) NOT NULL,
    is_public INTEGER DEFAULT 0,
    is_shared INTEGER DEFAULT 0,
    created_at DATETIME DEFAULT (datetime('now')),
    updated_at DATETIME DEFAULT (datetime('now')),
    FOREIGN KEY (owner_id) REFERENCES tupitube_user(user_id)
);

-- Collection table (for storyboards and folders)
CREATE TABLE IF NOT EXISTS tupitube_collection (
    collection_id INTEGER PRIMARY KEY AUTOINCREMENT,
    parent_id INTEGER,
    type VARCHAR(20),
    title VARCHAR(100),
    topics TEXT,
    description TEXT,
    owner_id INTEGER NOT NULL,
    is_public INTEGER DEFAULT 0,
    visits INTEGER DEFAULT 0,
    likes INTEGER DEFAULT 0,
    project_id INTEGER,
    path VARCHAR(255),
    slug VARCHAR(100),
    created_at DATETIME DEFAULT (datetime('now')),
    updated_at DATETIME DEFAULT (datetime('now')),
    FOREIGN KEY (owner_id) REFERENCES tupitube_user(user_id),
    FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id),
    FOREIGN KEY (parent_id) REFERENCES tupitube_collection(collection_id)
);

-- Work table (animations, images, frames)
CREATE TABLE IF NOT EXISTS tupitube_work (
    work_id INTEGER PRIMARY KEY AUTOINCREMENT,
    owner_id INTEGER,
    project_id INTEGER,
    collection_id INTEGER,
    type_id INTEGER,
    type VARCHAR(20),
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
    rendered INTEGER DEFAULT 0,
    uploaded INTEGER DEFAULT 0,
    created_at DATETIME DEFAULT (datetime('now')),
    updated_at DATETIME DEFAULT (datetime('now')),
    FOREIGN KEY (owner_id) REFERENCES tupitube_user(user_id),
    FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id),
    FOREIGN KEY (collection_id) REFERENCES tupitube_collection(collection_id)
);

-- Collaboration table
CREATE TABLE IF NOT EXISTS tupitube_collaboration (
    collaboration_id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    project_id INTEGER NOT NULL,
    permission_level INTEGER DEFAULT 1,
    created_at DATETIME DEFAULT (datetime('now')),
    FOREIGN KEY (user_id) REFERENCES tupitube_user(user_id),
    FOREIGN KEY (project_id) REFERENCES tupitube_project(project_id),
    UNIQUE(user_id, project_id)
);

-- Log table
CREATE TABLE IF NOT EXISTS tupitube_log (
    log_id INTEGER PRIMARY KEY AUTOINCREMENT,
    type VARCHAR(50),
    filename VARCHAR(255),
    ip VARCHAR(45),
    date DATETIME DEFAULT (datetime('now'))
);

-- User table for HumHub compatibility (optional)
CREATE TABLE IF NOT EXISTS user (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username VARCHAR(50) NOT NULL UNIQUE
);

-- Create indexes for better performance
CREATE INDEX IF NOT EXISTS idx_project_owner ON tupitube_project(owner_id);
CREATE INDEX IF NOT EXISTS idx_project_filename ON tupitube_project(filename);
CREATE INDEX IF NOT EXISTS idx_work_owner ON tupitube_work(owner_id);
CREATE INDEX IF NOT EXISTS idx_work_project ON tupitube_work(project_id);
CREATE INDEX IF NOT EXISTS idx_collection_owner ON tupitube_collection(owner_id);
CREATE INDEX IF NOT EXISTS idx_collection_slug ON tupitube_collection(slug);
CREATE INDEX IF NOT EXISTS idx_collaboration_user ON tupitube_collaboration(user_id);
CREATE INDEX IF NOT EXISTS idx_collaboration_project ON tupitube_collaboration(project_id);
CREATE INDEX IF NOT EXISTS idx_user_username ON tupitube_user(username);

-- Insert default admin user (password: admin123)
-- Note: In production, change this password immediately!
INSERT OR IGNORE INTO tupitube_user (username, name, password, is_enabled, is_creator) 
VALUES ('admin', 'Administrator', 'admin123', 1, 1);

-- Enable WAL mode for better concurrent access (recommended for classroom use)
PRAGMA journal_mode=WAL;
PRAGMA foreign_keys=ON;
