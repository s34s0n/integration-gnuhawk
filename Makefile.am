#
# This file is protected by Copyright. Please refer to the COPYRIGHT file 
# distributed with this source distribution.
# 
# This file is part of GNUHAWK.
# 
# GNUHAWK is free software: you can redistribute it and/or modify is under the 
# terms of the GNU General Public License as published by the Free Software 
# Foundation, either version 3 of the License, or (at your option) any later 
# version.
# 
# GNUHAWK is distributed in the hope that it will be useful, but WITHOUT ANY 
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR 
# A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

# You should have received a copy of the GNU General Public License along with 
# this program.  If not, see http://www.gnu.org/licenses/.
#
 
pkgconfigdir = $(libdir)/pkgconfig
dist_pkgconfig_DATA = gnuhawk.pc

xmldir = $(prefix)
dist_xml_DATA = gnuhawk.spd.xml

m4dir = $(datadir)/gnuhawk
dist_m4_DATA = bld/GNUHAWK_PKG.m4

if COMPONENTS
SUBDIRS = gnuradio/volk include src components qa
else
SUBDIRS = gnuradio/volk include src
endif

