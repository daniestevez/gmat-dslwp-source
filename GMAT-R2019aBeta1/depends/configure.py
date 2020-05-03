import os
import sys
import tarfile
import struct
import platform

cspice_version = 'N0065'
wx_build = True
wx_version = '3.0.4'
xerces_version = '3.1.4'
osx_min_version = '10.10'
vs_version = '12'

gmat_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__))) #Path to gmat folder
depends_path = gmat_path + '/depends'	#Path to depends folder
logs_path = depends_path + '/logs'	#Path to depends/logs folder

# Create path variables
bin_path = 		depends_path + '/bin'
f2c_path = 		depends_path + '/f2c'
cspice_path = 		depends_path + '/cspice'
wxWidgets_path = 	depends_path + '/wxWidgets'
xerces_path = 		depends_path + '/xerces'
sofa_path = 		depends_path + '/sofa'
tsplot_path = 		depends_path + '/tsPlot'
pcre_path = 		depends_path + '/pcre'

if sys.platform == 'darwin':
	cspice_path = cspice_path + '/macosx'
elif sys.platform == 'win32':
	cspice_path = cspice_path + '/windows'
else:
	cspice_path = cspice_path + '/linux'

# Load the Visual Studio path settings
def setup_windows():
	# 64-bit; change to x86 for 32-bit
	vs_arch = 'x86_amd64'
	vs_tools = 'vs' + vs_version + '0comntools'
	vs_path = os.getenv(vs_tools)
	syscall = vs_path + '\\..\\..\\VC\\vcvarsall.bat'
	os.system('\"' + syscall + '\"' + " " + vs_arch + ' & set > vsEnvironment.txt')
	
	# Now parse the VC environment
	f = open('vsEnvironment.txt', 'r')
	env = f.read().splitlines()
	f.close()

	for line in env:
		pair = line.split('=', 1)
		# print ("--> Setting " + pair[0] + " to " + pair[1])
		os.environ[pair[0]] = pair[1]

	# and delete the temporary settings file
	os.remove('vsEnvironment.txt')

def download_depends():
	wx_build = True
	# Download xerces if it doesn't already exist
	if not os.path.exists(xerces_path):
		os.chdir(depends_path)

		# Download and extract xerces
		print('Downloading Xerces-C ' + xerces_version + '...')
		os.system('curl http://archive.apache.org/dist/xerces/c/3/sources/xerces-c-' + xerces_version + '.tar.gz > xerces.tar.gz')
		tar = tarfile.open('xerces.tar.gz', 'r:gz')
		tar.extractall()
		tar.close()
		os.remove('xerces.tar.gz')

		# Rename the extracted xerces directory to be the proper path
		os.rename(depends_path + '/xerces-c-' + xerces_version, xerces_path)

	# Create directories and download wxWidgets if needed
	if not os.path.exists(wxWidgets_path + '/wxWidgets-' + wx_version) and wx_build:
		# If on Windows, downloading is done as part of the build_wxWidgets function, since that isn't always necessary
		if not sys.platform == 'win32':
			# Create & change directories
			if not os.path.exists(wxWidgets_path):
				os.mkdir(wxWidgets_path)
			os.chdir(wxWidgets_path)
	
			# Download wxWidgets source
			print('Downloading wxWidgets ' + wx_version + '...')
			os.system('curl -Lk https://github.com/wxWidgets/wxWidgets/releases/download/v' + wx_version + '/wxWidgets-' + wx_version + '.tar.bz2 > wxWidgets.tar.bz2')
			tar = tarfile.open('wxWidgets.tar.bz2', 'r:bz2')
			tar.extractall()
			tar.close()
			os.remove('wxWidgets.tar.bz2')
	
			# Make sure wxWidgets was downloaded
			if not os.path.exists(wxWidgets_path + '/wxWidgets-' + wx_version):
				print('Error in wxWidgets-' + wx_version + ' download.')
				wx_build = False

	if not os.path.exists(cspice_path):
		# Create & change directories
		os.mkdir(depends_path + '/cspice')
		os.mkdir(cspice_path)
		os.chdir(cspice_path)

		if sys.platform == 'darwin':
			cspice_type = 'MacIntel_OSX_AppleC'
		else:
			cspice_type = 'PC_Linux_GCC'
			
		# Determine 32 or 64 bit system
		if struct.calcsize("P") * 8 == 32:
			cspice_dir = 'cspice32'
			cspice_bit = '32bit'
		else:
			cspice_dir = 'cspice64'
			cspice_bit = '64bit'
			
		if sys.platform == 'win32':
			# Download and extract Spice for Windows (32/64-bit)
			print('Downloading ' + cspice_bit + ' CSPICE ' + cspice_version + '...')
			os.system('curl ftp://naif.jpl.nasa.gov/pub/naif/misc/toolkit_N0065/C/PC_Windows_VisualC_' + cspice_bit + '/packages/cspice.zip > cspice.zip')
			os.system(depends_path + '/bin/7za/7za.exe x cspice.zip > nul') 
			os.rename('cspice', cspice_dir)
			os.remove('cspice.zip')
		else:
			# Download and extract Spice for Mac/Linux (32/64-bit)
			print('Downloading ' + cspice_bit + ' CSPICE ' + cspice_version + '...')
			os.system('curl ftp://naif.jpl.nasa.gov/pub/naif/misc/toolkit_"' + cspice_version + '"/C/"' + cspice_type + '"_' + cspice_bit + '/packages/cspice.tar.Z > cspice.tar.Z')
			os.system('gzip -d cspice.tar.Z')
			os.system('tar -xf cspice.tar')
			os.system('mv cspice ' + cspice_dir)
			os.remove('cspice.tar')



def build_xerces():
	# Windows-specific build
	if sys.platform == 'win32':
		print('********** Configuring Xerces-C++ ' + xerces_version + ' **********')

		# Determine 32 or 64 bit system
		if struct.calcsize("P") * 8 == 32:
			xerces_outdir = xerces_path + '/Build/Win32'
			xerces_arch = 'Win32'
		else:
			xerces_outdir = xerces_path + '/Build/Win64'
			xerces_arch = 'x64'
		
		# Build Xerces if the directory doesn't already exist
		if not os.path.exists(xerces_outdir):
			print('-- Compiling debug Xerces. This could take a while...')
			os.chdir(xerces_path + '/projects/win32/VC' + vs_version + '/xerces-all/XercesLib')

			os.system('msbuild /m /property:Configuration=\"Static Debug\";Platform=' + xerces_arch + \
				' /property:TargetName=xerces-c_3D' + \
				' /property:ForceImportBeforeCppTargets=\"' + depends_path + '\\bin\\xerces\\XercesLibOverride.prop\" ' + \
				'XercesLib.vcxproj > ' + logs_path + '\\xerces_build_debug.log 2>&1')
			
			print('-- Compiling release Xerces. This could take a while...')
			os.system('msbuild /m /property:Configuration=\"Static Release\";Platform=' + xerces_arch + \
				' /property:TargetName=xerces-c_3' + \
				' /property:ForceImportBeforeCppTargets=\"' + depends_path + '\\bin\\xerces\\XercesLibOverride.prop\" ' + \
				'XercesLib.vcxproj > ' + logs_path + '\\xerces_build_release.log 2>&1')
		else:
			print('-- Xerces already configured')
		return
		
	# Out-of-source xerces build/install locations
	if sys.platform == 'darwin':
		xerces_build_path = xerces_path + '/cocoa-build'
		xerces_install_path = xerces_path + '/cocoa-install'
	else:
		xerces_build_path = xerces_path + '/linux-build'
		xerces_install_path = xerces_path + '/linux-install'

	# Find a test file to check if xerces has already been installed
	xerces_test_file = xerces_install_path + '/lib/libxerces-c.a'

	# Build xerces if the test file doesn't already exist
	if os.path.exists(xerces_test_file):
		print('Xerces ' + xerces_version + ' already configured')
	else:
		os.mkdir(xerces_build_path)
		os.chdir(xerces_build_path)

		# For users who compile GMAT on multiple platforms side-by-side.
		# Running Windows configure.bat causes Mac/Linux configure scripts
		# to have missing permissions.
		os.system('chmod u+x ../configure')
		os.system('chmod u+x ../config/*')

		if sys.platform == 'darwin':
			# Xerces needs these flags on OSX
			OSXFLAGS = '-mmacosx-version-min=' + osx_min_version
		else:
			OSXFLAGS = ''

		COMMONXERCESFLAGS = '--disable-shared --disable-netaccessor-curl --disable-transcoder-icu --disable-msgloader-icu'

		print('Configuring Xerces ' + xerces_version + ' debug library. This could take a while...')
		COMMONCFLAGS = '-O0 -g -fPIC ' + OSXFLAGS
		os.system('../configure ' + COMMONXERCESFLAGS + ' CFLAGS="' + COMMONCFLAGS + '" CXXFLAGS="' + COMMONCFLAGS + '" --prefix="' + xerces_install_path + '" > "' + logs_path + '/xerces_configure_debug.log" 2>&1')

		makeFlag = os.system('make -j8 > "' + logs_path + '/xerces_build_debug.log" 2>&1')
		if makeFlag == 0:
			os.system('make install > "' + logs_path + '/xerces_install_debug.log" 2>&1')
			os.rename(xerces_install_path + '/lib/libxerces-c.a', xerces_install_path + '/lib/libxerces-cd.a')
			os.system('make clean > /dev/null 2>&1')
		else:
			print('Xerces debug build failed. Fix errors and try again.')
			return

		print('Configuring Xerces ' + xerces_version + ' release library. This could take a while...')
		COMMONCFLAGS = '-O2 -fPIC ' + OSXFLAGS
		os.system('../configure ' + COMMONXERCESFLAGS + ' CFLAGS="' + COMMONCFLAGS + '" CXXFLAGS="' + COMMONCFLAGS + '" --prefix="' + xerces_install_path + '" > "' + logs_path + '/xerces_configure_release.log" 2>&1')

		makeFlag = os.system('make -j8 > "' + logs_path + '/xerces_build_release.log" 2>&1')
		if makeFlag == 0:
			os.system('make install > "' + logs_path + '/xerces_install_release.log" 2>&1')
			os.chdir('..')
			os.system('rm -Rf ' + xerces_build_path)
		else:
			print('Xerces release build failed. Fix errors and try again.')
			return

def build_wxWidgets():
	# Windows-specific build
	if sys.platform == 'win32':
		print('********** Configuring wxWidgets ' + wx_version + ' **********')

		# Determine 32 or 64 bit system
		if struct.calcsize("P") * 8 == 32:
			wxtype = '_'
		else:
			wxtype = '_x64_'
			
		# Generate filenames to download
		wxhdr = 'wxWidgets-' + wx_version + '-headers.7z'
		wxdev = 'wxMSW-' + wx_version + '_vc' + vs_version + '0' + wxtype + 'Dev.7z'
		wxrel = 'wxMSW-' + wx_version + '_vc' + vs_version + '0' + wxtype + 'ReleaseDLL.7z'
		wx_path = 'wxWidgets/wsMSW-' + wx_version
		
		# Download wxWidgets files if they don't already exist
		if not os.path.exists(wxWidgets_path + '/wsMSW-' + wx_version + '/lib/vc' + vs_version + '0' + wxtype + 'dll'):
			os.mkdir(wxWidgets_path)
			os.chdir(wxWidgets_path)
			os.mkdir('wsMSW-' + wx_version)
			os.chdir('wsMSW-' + wx_version)
			
			print('-- Downloading wxWidgets headers')
			os.system('curl -Lk https://github.com/wxWidgets/wxWidgets/releases/download/v' + wx_version + '/' + wxhdr + ' > ' + wxhdr)
			
			print('-- Downloading wxWidgets debug libraries')
			os.system('curl -Lk https://github.com/wxWidgets/wxWidgets/releases/download/v' + wx_version + '/' + wxdev + ' > ' + wxdev)

			print('-- Downloading wxWidgets release libraries')
			os.system('curl -Lk https://github.com/wxWidgets/wxWidgets/releases/download/v' + wx_version + '/' + wxrel + ' > ' + wxrel)
			
			os.system(depends_path + '/bin/7za/7za.exe x ' + wxhdr + ' > nul')
			os.system(depends_path + '/bin/7za/7za.exe x ' + wxdev + ' > nul')
			os.system(depends_path + '/bin/7za/7za.exe x ' + wxrel + ' > nul')
			
			os.remove(wxhdr)
			os.remove(wxdev)
			os.remove(wxrel)
			
			os.chdir('lib')
			os.rename('vc' + vs_version + '0' + wxtype + 'dll', 'vc' + wxtype + 'dll')
			
			os.chdir(depends_path)
			
			return
		else:
			print('-- wxWidgets already configured')
			return
	else:		
		# Set build path based on version
		wx_path = wxWidgets_path + '/wxWidgets-' + wx_version
	
		# OS-specific vars
		if sys.platform == 'darwin':
			wx_build_path = wx_path + '/cocoa-build'
			wx_install_path = wx_path + '/cocoa-install'
			# Find a test file to see if wxWidgets has already been configured
			wx_test_file = wx_install_path + '/lib/libwx_baseu-3.0.dylib'

		else:
			wx_build_path = wx_path + '/gtk-build'
			wx_install_path = wx_path + '/gtk-install'
			# Find a test file to see if wxWidgets has already been configured
			wx_test_file = wx_install_path + '/lib/libwx_baseu-3.0.so'
	
		# Build wxWidgets if the test file doesn't already exist
		# Note that according to 
		#   http://docs.wxwidgets.org/3.0/overview_debugging.html
		# debugging features "are always available by default", so
		# we don't build a separate debug version here.
		# IF a debug version is required in the future, then this
		# if/else block should be repeated with the --enable-debug flag
		# added to mac & linux versions of the wx ./configure command
		if os.path.exists(wx_test_file):
			print('wxWidgets ' + wx_version + ' already configured')
		else:
			os.mkdir(wx_build_path)
			os.chdir(wx_build_path)
	
			print('Configuring wxWidgets ' + wx_version + '. This could take a while...')
	
			if sys.platform == 'darwin':
				# wxWidgets 3.0.2 has a compile error due to an incorrect
				# include file on OSX 10.10+. Apply patch to fix this.
				# See [GMT-5384] and http://goharsha.com/blog/compiling-wxwidgets-3-0-2-mac-os-x-yosemite/
				osx_ver = platform.mac_ver()[0]
				if osx_ver > '10.10.0' and wx_version == '3.0.2':
					os.system('sed -i.bk "s/WebKit.h/WebKitLegacy.h/" "' + wx_path + '/src/osx/webview_webkit.mm"')
	
				# wxWidgets needs these flags on OSX
				OSXFLAGS = '--with-osx_cocoa --with-macosx-version-min=' + osx_min_version
			else:
				OSXFLAGS = ''
	
			os.system('../configure ' + OSXFLAGS + ' --enable-unicode --with-opengl --prefix="' + wx_install_path + '" > "' + logs_path + '/wxWidgets_configure.log" 2>&1')
	
			# Compile, install, and clean wxWidgets
			makeFlag = os.system('make -j8 > "' + logs_path + '/wxWidgets_build.log" 2>&1')
	
			if makeFlag == 0:
				os.system('make install > "' + logs_path + '/wxWidgets_install.log" 2>&1')
				os.chdir('..')
				os.system('rm -Rf "' + wx_build_path + '"')
			else:
				print('wxWidgets build failed. Fix errors and try again.')
				return

def build_cspice():
	# Windows-specific build
	if sys.platform == 'win32':
		print('********** Configuring CSPICE **********')
		cspice_type = 'PC_Linux_GCC'
			
		# Determine 32 or 64 bit system
		if struct.calcsize("P") * 8 == 32:
			cspice_dir = 'cspice32'
			cspice_bit = '32bit'
		else:
			cspice_dir = 'cspice64'
			cspice_bit = '64bit'
		
		# Build CSPICE if cspiced.lib does not already exist
		if not os.path.exists(cspice_path + '/' + cspice_dir + '/lib/cspiced.lib'):
			os.chdir(cspice_path + '/' + cspice_dir + '/src/cspice')
			print('-- Compiling debug CSPICE. This could take a while...')
			os.system('cl /c /DEBUG /Z7 /MP -D_COMPLEX_DEFINED -DMSDOS -DOMIT_BLANK_CC -DNON_ANSI_STDIO -DUIOLEN_int *.c > ' + logs_path + '\cspice_build_debug.log 2>&1')
			os.system('link -lib /out:..\..\lib\cspiced.lib *.obj >> ' + logs_path + '\cspice_build_debug.log 2>&1')
			
			os.system('del *.obj')
			
			print('-- Compiling release CSPICE. This could take a while...')
			os.system('cl /c /O2 /MP -D_COMPLEX_DEFINED -DMSDOS -DOMIT_BLANK_CC -DNON_ANSI_STDIO -DUIOLEN_int *.c > ' + logs_path + '\cspice_build_release.log 2>&1')
			os.system('link -lib /out:..\..\lib\cspice.lib *.obj >> ' + logs_path + '\cspice_build_release.log 2>&1')
			
			os.system('del *.obj')
			
			os.chdir(depends_path)
		else:
			print('-- CSPICE already configured')
			
		return
		
	if struct.calcsize("P") * 8 == 32:
		spice_path = cspice_path + '/cspice32'
		TKCOMPILEARCH = '-m32'
		os.system('export TKCOMPILEARCH="' + TKCOMPILEARCH + '"')
	else:
		spice_path = cspice_path + '/cspice64'
		TKCOMPILEARCH = '-m64'
		os.system('export TKCOMPILEARCH="' + TKCOMPILEARCH + '"')

	if sys.platform == 'darwin':
		OSXFLAGS = '-mmacosx-version-min=' + osx_min_version
	else:
		OSXFLAGS = ''

	cspice_test_file = spice_path + '/lib/cspiced.a'

	if os.path.exists(cspice_test_file):
		print('CSPICE already configured')
	else:
		os.chdir(spice_path + '/src/cspice')

		#Compile debug CSPICE with integer uiolen [GMT-5044]
		print('Compiling CSPICE debug library. This could take a while...')
		os.system('export TKCOMPILEOPTIONS="' + TKCOMPILEARCH + ' -c -ansi ' + OSXFLAGS + ' -g -fPIC -DNON_UNIX_STDIO -DUIOLEN_int"')
		makeFlag = os.system('./mkprodct.csh > "' + logs_path + '/cspice_build_debug.log" 2>&1')

		if makeFlag == 0:
			os.system('mv ../../lib/cspice.a ../../lib/cspiced.a')
		else:
			print('CSPICE debug build failed. Fix errors and try again.')

		#Compile release CSPICE with integer uiolen [GMT-5044]
		print('Compiling CSPICE release library. This could take a while...')
		os.system('export TKCOMPILEOPTIONS="' + TKCOMPILEARCH + ' -c -ansi ' + OSXFLAGS + ' -O2 -fPIC -DNON_UNIX_STDIO -DUIOLEN_int"')
		makeFlag = os.system('./mkprodct.csh > "' + logs_path + '/cspice_build_release.log" 2>&1')

		if makeFlag != 0:
			print('CSPICE release build failed. Fix errors and try again.')

# Create log directory
if not os.path.exists(logs_path):
	os.mkdir(logs_path)

if sys.platform == 'win32':
	setup_windows()

download_depends()
build_xerces()
if wx_build:
	build_wxWidgets()
build_cspice()
