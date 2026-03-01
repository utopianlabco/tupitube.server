###########################################################################
#   Project TupiTube Server                                               #
#   Project Contact: info@tupitube.com                                    #
#   Project Website: http://www.tupitube.com                              #
#                                                                         #
#   Developers:                                                           #
#   2025:                                                                 #
#    Utopian Lab Development Team                                         #
#   2010:                                                                 #
#    Gustav Gonzalez                                                      #
#   ---                                                                   #
#   KTooN's versions:                                                     #
#   2006:                                                                 #
#    David Cuadrado                                                       #
#    Jorge Cuadrado                                                       #
#   2003:                                                                 #
#    Fernado Roldan                                                       #
#    Simena Dinas                                                         #
#                                                                         #
#   License:                                                              #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program.  If not, see <http://www.gnu.org/licenses/>. #
###########################################################################

# encoding: utf-8

require 'qonf/config'
require 'qonf/info'
require 'qonf/qmake'
require 'qonf/qonfexception'
require 'qonf/makefile'

module RQonf

class Configure
    attr_reader :qmake, :statusFile
    
    def initialize(args)
        @statusFile = Dir.getwd+"/configure.status"
        
        @tests = []
        @testsDir = Dir.getwd
        
        @options = {}
        parseArgs(args)
        
        @qmake = QMake.new
        @statusFile = Dir.getwd + "/configure.status"

        @tests = []
        @testsDir = Dir.getwd

        @options = {}
        parseArgs(args)

        @qmake = QMake.new
        @properties = {}

        setPath()
        Makefile::setArgs(@options)
    end

    def load_properties(properties_filename)
        File.open(properties_filename, 'r') do |properties_file|
            properties_file.read.each_line do |line|
                line.strip!
                if (line[0] != ?# and line[0] != ?=)
                    i = line.index('=')
                    if (i)
                        @properties[line[0..i - 1].strip] = line[i + 1..-1].strip
                    else
                        @properties[line] = ''
                    end
                end
            end
        end
        @properties
    end

    def hasProperty?(arg)
        @properties.has_key?(arg)
    end

    def propertyValue(arg)
        @properties[arg].to_s
    end

    def hasArgument?(arg)
        @options.has_key?(arg)
    end

    def argumentValue(arg)
        @options[arg].to_s
    end
    
    def setTestDir(dir)
        @testsDir = dir
    end
    
    def verifyQtVersion(minqtversion, qtdir)
        Info.info << "Checking for Qt >= " << minqtversion << "... "

        if @qmake.findQMake(minqtversion, true, qtdir)
           print "[ \033[92mOK\033[0m ]\n"
        else
           print "[ \033[91mFAILED\033[0m ]\n"
           raise QonfException.new("\033[91mInvalid Qt version\033[0m.\n   Please, upgrade to #{minqtversion} or higher (Visit: http://qt.nokia.com)")
        end
    end

    def createTests
        @tests.clear
        findTest(@testsDir)
    end
    
    def runTests(config, conf, debug, isLucid)
        @tests.each { |test|
            if not test.run(config, conf, debug, isLucid) and not test.optional
                raise QonfException.new("\033[91mMissing required dependency\033[0m")
            end
        }
    end
    
    def createMakefiles
        Info.info << "Creating makefiles..." << $endl

        if RUBY_PLATFORM.downcase.include?("darwin")
            qmakeLine = " 'CONFIG += console warn_on' 'CONFIG -= app_bundle' 'LIBS += -lavcodec -lavutil -lavformat -framework CoreFoundation -L/sw/lib' 'INCLUDEPATH += /sw/include'"
	    @qmake.run(qmakeLine, true)
        else
            @qmake.run("", true)
        end
        
        Info.info << "Updating makefiles and source code..." << $endl
        
        @makefiles = Makefile::findMakefiles(Dir.getwd)
        
        @makefiles.each { |makefile|
                           Makefile::override(makefile)
        }
    end
    
    private
    def parseArgs(args)

        optc = 0
        last_opt = ""

        while args.size > optc

          arg = args[optc].strip

          if arg =~ /^--([\w-]*)={0,1}([\W\w]*)/

             opt = $1.strip
             val = $2.strip
             @options[opt] = val
             last_opt = opt

          else

             # arg is an arg for option
             if not last_opt.to_s.empty? and @options[last_opt].to_s.empty?
                @options[last_opt] = arg
             else
                raise "Invalid option: #{arg}"
             end

          end

          optc += 1

        end
    end
    
    def findTest(path)
        if $DEBUG
            Info.warn << "Searching qonfs in: " << path << $endl
        end
        Dir.foreach(path) { |f|
            
            file = "#{path}/#{f}"

            if File.stat(file).directory?
                if not f =~ /^\./
                    findTest(file)
                end
            elsif file =~ /.qonf$/
                @tests << Test.new(file, @qmake)
            end
        }
    end

   private
    def setPath()
        if @options['prefix'].nil? then
           @options['prefix'] = "/usr/sbin"
        end

        if @options['with-tupitube-dir'].nil? then
           usage()
        end

        launcher_prefix = @options['prefix'] 
        tupitube_path = @options['with-tupitube-dir']

        # Check if running on Windows
        if RUBY_PLATFORM =~ /mingw|mswin|cygwin/i
            # Generate Windows .bat launcher
            newfile = "@echo off\r\n"
            newfile += "set TUPITUBE_HOME=" + tupitube_path + "\r\n"
            newfile += "set TUPITUBE_PLUGIN=" + tupitube_path + "\\lib\\tupitube\\plugins\r\n"
            newfile += "set PATH=%TUPITUBE_HOME%\\lib\\tupitube;%TUPITUBE_PLUGIN%;%PATH%\r\n"
            newfile += "\r\n"
            newfile += "\"%~dp0tupitube.server.bin.exe\"\r\n"

            launcher = File.open("bin/tupitube.server.bat", "w") { |f|
                       f << newfile
            }
        else
            # Generate Unix bash launcher
            newfile = "#!/bin/bash\n\n"
            newfile += "export DISPLAY=localhost:0.0\n"
            newfile += "/usr/bin/xhost localhost\n\n"
            newfile += "export QT_LIB=\"/usr/local/Qt5.14.1/5.14.1/gcc_64/lib\"\n"
            newfile += "export QUAZIP_LIB=\"/usr/local/quazip/lib\"\n"
            newfile += "export FFMPEG_LIB=\"/usr/local/ffmpeg/lib\"\n"
            newfile += "export TUPITUBE_HOME=\"" + tupitube_path + "\"\n"
            newfile += "export TUPITUBE_LIB=\"" + tupitube_path + "/lib/tupitube\"\n"
            newfile += "export TUPITUBE_PLUGIN=\"" + tupitube_path + "/lib/tupitube/plugins\"\n"
            newfile += "export SERVER_LIB=\"" + launcher_prefix + "/lib\"\n"
            newfile += "export LD_LIBRARY_PATH=\"$\{QT_LIB\}:$\{QUAZIP_LIB\}:$\{FFMPEG_LIB\}:$\{TUPITUBE_LIB\}:$\{TUPITUBE_PLUGIN\}:$\{SERVER_LIB\}:$LD_LIBRARY_PATH\"\n"
            newfile += "export QT_NO_GLIB=\"1\"\n"
            newfile += "BASE=`dirname $0`\n\n"
            newfile += "exec ${BASE}/tupitube.server.bin >& /tmp/debug.log"

            launcher = File.open("bin/tupitube.server", "w") { |f|
                       f << newfile
                       f.chmod(0755)
            }
        end

    end
end
end # module
