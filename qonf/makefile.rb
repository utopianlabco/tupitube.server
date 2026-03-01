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

require 'rbconfig'

module RQonf
    class Makefile

        def self.setArgs(paths)
            @options = paths 
        end

        def self.findMakefiles(path)
            makefiles = []
            Dir.foreach(path) { |f|
                file = "#{path}/#{f}"
                
                if File.stat(file).directory?
                    if not f =~ /^\./
                        makefiles.concat findMakefiles(file)
                    end
                elsif f.downcase == "makefile"
                    makefiles << file
                end
            }
            
            makefiles
        end
        
        def self.override(makefile)
            newmakefile = ""
            File.open(makefile, "r") { |f|
                      lines = f.readlines
                      index = 0
                      while index < lines.size
                            line = lines[index]
                            if line.include? "INSTALL_ROOT" then
                               newmakefile += "#{line.gsub(/\$\(INSTALL_ROOT\)/, @options['prefix'])}"
                            elsif line.include? "TUPI_DIR" then
                               newmakefile += "#{line.gsub(/\$\(TUPI_DIR\)/, @options['with-tupi-dir'])}"
                            else
                               newmakefile += line
                            end
                            index += 1
                      end
            }

            File.open(makefile, "w") { |f|
                f << newmakefile
            }
        end
    end
end


