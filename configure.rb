#!/usr/bin/ruby

require './qonf/configure'
require './qonf/info'

def usage
    puts <<_EOH_
   Use: ./configure [options]
    options:
        --help, -h                          : Show this message
        --prefix=[prefix]                   : Set prefix
        --with-debug                        : Enable debug
        --with-tupitube-dir=[tupitube home] : Set TupiTube Desk home
        --with-data-dir=[data path]         : Set TupiTube Server repository path (database and animation files)
_EOH_
    exit 0
end

begin
    conf = RQonf::Configure.new(ARGV)

    if conf.hasArgument?("help") or conf.hasArgument?("h")
       usage()
    end

    tupitube_dir = conf.argumentValue("with-tupitube-dir")
    
    if tupitube_dir.to_s.empty?
        tupitube_dir = ENV["TUPITUBE_HOME"]
    end
    
    if tupitube_dir.to_s.empty?
        usage()
    end

    conf.verifyQtVersion("5.0.0", "")
    
    tupidir = File.expand_path(tupitube_dir)

    config = RQonf::Config.new
    config.addModule("core")
    config.addModule("gui")
    config.addModule("svg")
    config.addModule("xml")
    config.addModule("network")

    config.addLib("-ltupifwgui")
    config.addLib("-ltupifwcore")
    config.addLib("-L#{tupidir}/lib/tupitube -L#{tupidir}/lib/tupitube/plugins -ltupi -ltupistore -ltupibase -ltupiffmpegplugin")
 
    config.addIncludePath(tupidir + "/include/tupi")
    config.addIncludePath(tupidir + "/include/tupistore")
    config.addIncludePath(tupidir + "/include/tupibase")
    config.addIncludePath(tupidir + "/include/tupicore")
    config.addIncludePath(tupidir + "/include/tupigui")
    config.addIncludePath(tupidir + "/include/tupiffmpeg")
    config.addIncludePath("/usr/local/include/quazip")

    Info.info << "Debug support... "

    if conf.hasArgument?("with-debug")
       config.addDefine("TUP_DEBUG")
       print "[ \033[92mON\033[0m ]\n"
    else
       config.addDefine("TUP_NODEBUG")
       config.addOption("silent")
       print "[ \033[91mOFF\033[0m ]\n"
    end

    data_dir = conf.argumentValue("with-data-dir")
    if !data_dir.to_s.empty?
       data_dir = File.expand_path(data_dir)
       config.addDefine("DEFAULT_DATA_PATH=\\\"#{data_dir}\\\"")
       Info.info << "Data directory... [ \033[92m#{data_dir}\033[0m ]\n"
    end

    unix = config.addScope("unix")
    unix.addVariable("MOC_DIR", ".moc")
    unix.addVariable("UI_DIR", ".ui")
    unix.addVariable("OBJECTS_DIR", ".obj")

    config.save("tupitube_config.pri")

    # Compile translation files (.ts -> .qm)
    Info.info << "Compiling translations... "
    translations_dir = "src/shell/data/translations"
    ts_files = Dir.glob("#{translations_dir}/*.ts")
    if ts_files.any?
        ts_files.each do |ts_file|
            system("lrelease #{ts_file} 2>/dev/null")
        end
        print "[ \033[92mOK\033[0m ]\n"
    else
        print "[ \033[93mNO FILES\033[0m ]\n"
    end

    conf.createMakefiles
    
rescue => err
    Info.error << "Configure failed. error was: #{err.message}\n"
    if $DEBUG
        puts err.backtrace
    end
end

