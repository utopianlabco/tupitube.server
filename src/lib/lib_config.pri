PRI_FILE = ../../tupitube_config.pri
exists ($$PRI_FILE) {
    include ($$PRI_FILE)
} else {
    error("[lib] Please, run configure first")
}
