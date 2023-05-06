cmake_minimum_required(VERSION 3.8.0 FATAL_ERROR)

project(FaustLive)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#######################################
get_filename_component(ROOT ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set (SRCDIR  	 ${ROOT}/src)
set (RSRCDIR  	 ${ROOT}/Resources)
set (LOCALLIBDIR ${ROOT}/lib)

#######################################
# versions management
file (STRINGS "${ROOT}/version.txt" VERSION)
message (STATUS "Generates FaustLive version ${VERSION}")

#######################################
# Options 
option ( REMOTE 		"Includes remote computing" off )
option ( QT6 			"Uses Qt6" off )

set (CMAKE_CXX_STANDARD 11)

#######################################
# check for pkg-config 
# used later for libsndfile and libmicrohttpd
if (NOT WIN32)
include(FindPkgConfig)
if (NOT PKG_CONFIG_FOUND)
	message (FATAL "Cannot find pkg-config")
endif()
endif()


#######################################
# Faust part
if(DEFINED FAUST_PATH_IS_ABSOLUTE AND FAUST_PATH_IS_ABSOLUTE MATCHES "1")
# We can't execute an arm64 faust executable on a GitHub x86 runner,
# so we make some strong assumptions about where the include and libs are.
cmake_path(GET FAUST PARENT_PATH FAUST_DIR)
cmake_path(GET FAUST_DIR PARENT_PATH FAUST_DIR)
set(FAUST_INCLUDE_DIRS ${FAUST_DIR}/include)
set(FAUSTLIB ${FAUST_DIR}/lib)
string ( STRIP ${FAUST_INCLUDE_DIRS} FAUST_INCLUDE_DIRS )
message("USING FAUST_INCLUDE_DIRS ${FAUST_INCLUDE_DIRS}")
else()
if (NOT DEFINED FAUST)
	set (FAUST faust)
endif()
message (STATUS "Using Faust as ${FAUST}")
execute_process (COMMAND ${FAUST} --includedir OUTPUT_VARIABLE FINC)
string ( STRIP ${FINC} FAUST_INCLUDE_DIRS )
execute_process (COMMAND ${FAUST} --libdir OUTPUT_VARIABLE FLIB)
string ( STRIP ${FLIB} FAUSTLIB )

message (STATUS "Using Faust libraries from ${FAUSTLIB}")
endif()

# compiler dependent settings
if (WIN32)
	set (FAUST_LIBRARIES ${FAUSTLIB}/libfaustwithllvm.lib ${FAUSTLIB}/libOSCFaust.lib ${FAUSTLIB}/libHTTPDFaust.lib)
	set (WINSRC ${MAXSDK}/max-includes/common/dllmain_win.c ${MAXSDK}/max-includes/common/commonsyms.c)
else()
	
	if(DEFINED FAUST_PATH_IS_ABSOLUTE AND FAUST_PATH_IS_ABSOLUTE MATCHES "1")
	set (FAUST_LIBRARIES ${FAUSTLIB}/libfaustwithllvm.a ${FAUSTLIB}/libOSCFaust.a ${FAUSTLIB}/libHTTPDFaust.a)
	else()
	execute_process( COMMAND faust-config --system-libs OUTPUT_VARIABLE FC_OUTPUT RESULT_VARIABLE FC_RESULT)
	string(REPLACE " " ";" FC_OUTPUT ${FC_OUTPUT})
	string(STRIP "${FC_OUTPUT}" FC_OUTPUT)
	set (FAUST_LIBRARIES ${FAUSTLIB}/libfaustwithllvm.a ${FC_OUTPUT} -L/opt/local/lib ${FAUSTLIB}/libOSCFaust.a ${FAUSTLIB}/libHTTPDFaust.a)
	endif()

endif()

#######################################
# Qt settings
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTORCC ON)
if (QT6)
	find_package(Qt6 COMPONENTS Core Gui Widgets Network REQUIRED)
	set (QTINCDIRS 	${Qt6Core_INCLUDE_DIRS} ${Qt6Gui_INCLUDE_DIRS} ${Qt6Widgets_INCLUDE_DIRS} ${Qt6Network_INCLUDE_DIRS})
	set (QTLIBS 	Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)
	message (STATUS "Using Qt version ${Qt6Core_VERSION}")
else()
	find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)
	set (QTINCDIRS 	${Qt5Core_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS})
	set (QTLIBS 	Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)
	message (STATUS "Using Qt version ${Qt5Core_VERSION}")
endif()

#######################################
# set sources and include dirs
file (GLOB FAUSTLIVE_SRC  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${SRCDIR}/*/*.cpp )
file (GLOB FAUSTLIVE_HEADERS  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}  ${SRCDIR}/*/*.h)
file (GLOB FAUSTLIVE_INCLUDE_DIRS  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${SRCDIR}/* "/usr/local/include")
set (FAUSTLIVE_HEADERS ${FAUSTLIVE_HEADERS} ${FAUST_INCLUDE_DIRS}/faust/gui/QTUI.h)
#message ("FAUSTLIVE_HEADERS ${FAUSTLIVE_HEADERS}")

#######################################
# manual libmicrohttpd settings
macro (set_pkg_config_path)
	set (PP $ENV{PKG_CONFIG_PATH})
	foreach(A ${PP})
		string (REPLACE "\\" "/" A1 ${A} )
		string (REGEX REPLACE "C:/msys[^/]*" "" A2 ${A1} )
		set (LP "${LP}:${A2}")
	endforeach()
	set (ENV{PKG_CONFIG_PATH} ${LP} )
endmacro()

#######################################
# libmicrohttpd settings
if (LIBMICROHTTPD_CFLAGS AND LIBMICROHTTPD_LDFLAGS)
	message(STATUS "Using manual cflags '${LIBMICROHTTPD_CFLAGS}' and ldflags '${LIBMICROHTTPD_LDFLAGS}' for libmicrohttpd")
else()
if (WIN32 AND MSVC)
	set (LMHDIR ${ROOT}/lib/libmicrohttpd/x64)
	set (LMHLIB ${LMHDIR}/libmicrohttpd.lib)
	set (INCLUDE ${INCLUDE} ${LMHDIR})
else ()
	find_package(PkgConfig REQUIRED)
	if (${PKG_CONFIG_FOUND})
		if (BUILD_HTTP_STATIC)
		set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
		list(APPEND PKG_CONFIG_EXECUTABLE "--static")  # append temporarily
		endif()
		pkg_check_modules(LMHD IMPORTED_TARGET libmicrohttpd)
		if (BUILD_HTTP_STATIC)
		list(POP_BACK PKG_CONFIG_EXECUTABLE)  # undo the append above
		endif()

		if (${LMHD_FOUND})
			foreach (A ${LMHD_CFLAGS})
				set(LIBMICROHTTPD__CFLAGS "${LIBMICROHTTPD__CFLAGS} ${A}")
			endforeach()
			foreach (A ${LMHD_LDFLAGS})
				set(LIBMICROHTTPD__LDFLAGS "${LIBMICROHTTPD__LDFLAGS} ${A}")
			endforeach()
			message (STATUS "Found libmicrohttpd ${LMHD_VERSION}")
		else ()
			message (WARNING "Cannot locate libmicrohttpd: skip ${TARGET} generation")
			return ()
		endif()
	else()
		message (WARNING "No pkg-config, cannot locate libmicrohttpd: skip ${TARGET} generation")
		return ()
	endif()
endif()
endif()

if ( WIN32 OR MSYS )
	if (MSVC)
		set (LIBMICROHTTPD__LDFLAGS "${LIBMICROHTTPD__LDFLAGS} ws2_32" )
	else()
		set (LIBMICROHTTPD__LDFLAGS "-lws2_32 ${LIBMICROHTTPD__LDFLAGS}" )
	endif()
endif()

#######################################
# libmicrohttpd settings
if (WIN32)
	set (FAUSTLIVE_DEFINITIONS ${FAUSTLIVE_DEFINITIONS} -D_WIN32 -DMHD_W32LIB)
	set (FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES} ${LMHLIB})
	set (FAUSTLIVE_INCLUDE_DIRS ${FAUSTLIVE_INCLUDE_DIRS} ${LMHDIR})
else()
	set (FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES} PkgConfig::LMHD)
endif()

#######################################
# sndfile support
if (WIN32)
	if (NOT DEFINED LIBSNDFILE)
		set ( LIBSNDFILE "C:/Program Files/Mega-Nerd/libsndfile" )
	endif()
	set (FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES}  winmm.lib ws2_32.lib ${LIBSNDFILE}/lib/sndfile.lib)
	set (FAUSTLIVE_INCLUDE_DIRS ${FAUSTLIVE_INCLUDE_DIRS}  ${LIBSNDFILE}/include)
	set (FAUSTLIVE_DEFINITIONS ${FAUSTLIVE_DEFINITIONS} -D_WIN32 -DUSEWINMAIN)
else()
	find_package(PkgConfig REQUIRED)
	if (DEFINED USE_STATIC_SNDFILE AND "${USE_STATIC_SNDFILE}" STREQUAL "1" )
		# We expect the user to have built libsndfile as a static lib
		# and for it to be accessible via `pkg-config --static libsndfile`
		set(SNDFILE_IS_STATIC 1)
		set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
		list(APPEND PKG_CONFIG_EXECUTABLE "--static")  # append temporarily
		pkg_check_modules(SNDFILE REQUIRED IMPORTED_TARGET sndfile)
		pkg_check_modules(FLAC REQUIRED IMPORTED_TARGET flac)
		pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbis)
		pkg_check_modules(OGG REQUIRED IMPORTED_TARGET ogg)
		pkg_check_modules(OPUS REQUIRED IMPORTED_TARGET opus)
		pkg_check_modules(MPG123 REQUIRED IMPORTED_TARGET libmpg123)
		list(POP_BACK PKG_CONFIG_EXECUTABLE)  # undo the append above
	else()
		# We expect the user to have installed sndfile with
		# brew install libsndfile
		set(SNDFILE_IS_STATIC 1)
		pkg_check_modules(SNDFILE REQUIRED IMPORTED_TARGET sndfile)
		pkg_check_modules(FLAC REQUIRED IMPORTED_TARGET flac)
		pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbis)
		pkg_check_modules(OGG REQUIRED IMPORTED_TARGET ogg)
		pkg_check_modules(OPUS REQUIRED IMPORTED_TARGET opus)
		pkg_check_modules(MPG123 REQUIRED IMPORTED_TARGET libmpg123)
	endif()
	set (FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES} -lcurl PkgConfig::SNDFILE PkgConfig::FLAC PkgConfig::VORBIS PkgConfig::OGG PkgConfig::OPUS PkgConfig::MPG123)
	set (FAUSTLIVE_INCLUDE_DIRS ${FAUSTLIVE_INCLUDE_DIRS} ${SNDFILE_INCLUDE_DIRS})
endif()

#######################################
# audio settings
if ((NOT DEFINED AUDIO) OR (AUDIO STREQUAL ""))
	if (WIN32)
		set(AUDIO portaudio)
	elseif (APPLE)
		set(AUDIO coreaudio)
	else()
		set(AUDIO jack)
	endif()
endif()

if (AUDIO STREQUAL coreaudio)
	message (STATUS "Using Core Audio drivers")
	set (FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES} "-framework CoreAudio" "-framework AudioUnit" "-framework CoreServices" "-framework CoreMIDI" )
	set (FAUSTLIVE_DEFINITIONS ${FAUSTLIVE_DEFINITIONS} -DCOREAUDIO)
	set (FAUSTLIVE_INCLUDE_DIRS ${FAUSTLIVE_INCLUDE_DIRS} ${SRCDIR}/Audio/CA)
	file (GLOB AUDIO_SRC  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${SRCDIR}/Audio/CA/*.cpp )
	file (GLOB AUDIO_HEADERS  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}  ${SRCDIR}/Audio/CA/*.h )

elseif (AUDIO STREQUAL portaudio)
	message (STATUS "Using PortAudio drivers")
	set (FAUSTLIVE_DEFINITIONS ${FAUSTLIVE_DEFINITIONS} -DPORTAUDIO)
	set (FAUSTLIVE_INCLUDE_DIRS ${FAUSTLIVE_INCLUDE_DIRS} ${SRCDIR}/Audio/PA ${ROOT}/lib/portaudio/include)
	file (GLOB AUDIO_SRC  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${SRCDIR}/Audio/PA/*.cpp )
	file (GLOB AUDIO_HEADERS  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}  ${SRCDIR}/Audio/PA/*.h )
	if (WIN32)
		set (FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES} ${ROOT}/lib/portaudio/lib/portaudio.lib )
		set (FAUSTLIVE_INCLUDE_DIRS ${FAUSTLIVE_INCLUDE_DIRS} ${LOCALLIBDIR}/portaudio/include)
	else()
		set (FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES} -lportaudio )
	endif()

elseif (AUDIO STREQUAL alsa)
	if (APPLE OR NOT UNIX)
		message (FATAL "Can't use alsa drivers on non Unix platform.")	
	endif()
	message (STATUS "Using Alsa drivers")
	set (FAUSTLIVE_DEFINITIONS ${FAUSTLIVE_DEFINITIONS} -DALSA)
	set (FAUSTLIVE_INCLUDE_DIRS ${FAUSTLIVE_INCLUDE_DIRS} ${SRCDIR}/Audio/AL)
	file (GLOB AUDIO_SRC  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${SRCDIR}/Audio/AL/*.cpp )
	file (GLOB AUDIO_HEADERS  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}  ${SRCDIR}/Audio/AL/*.h )
	set (FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES} -lasound )

elseif (AUDIO STREQUAL jack)
	message (STATUS "Using Jack Audio drivers")
	set (FAUSTLIVE_DEFINITIONS ${FAUSTLIVE_DEFINITIONS} -DJACK)
	set (FAUSTLIVE_INCLUDE_DIRS ${FAUSTLIVE_INCLUDE_DIRS} ${SRCDIR}/Audio/JA)
	file (GLOB AUDIO_SRC  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${SRCDIR}/Audio/JA/*.cpp )
	file (GLOB AUDIO_HEADERS  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}  ${SRCDIR}/Audio/JA/*.h )
	set (FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES} -ljack )

elseif (AUDIO STREQUAL netjack)
	message (STATUS "Using NetJack Audio drivers")
	set (FAUSTLIVE_DEFINITIONS ${FAUSTLIVE_DEFINITIONS} -DNETJACK)
	set (FAUSTLIVE_INCLUDE_DIRS ${FAUSTLIVE_INCLUDE_DIRS} ${SRCDIR}/Audio/JA)
	file (GLOB AUDIO_SRC  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${SRCDIR}/Audio/NJ_Master/*.cpp ${SRCDIR}/Audio/NJ_Slave/*.cpp )
	file (GLOB AUDIO_HEADERS  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${SRCDIR}/Audio/NJ_Master/*.h ${SRCDIR}/Audio/NJ_Slave/*.h )
	set (FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES} -ljacknet )

else ()
	message (WARNING "Unknown Audio driver '${AUDIO}'")
endif()


#######################################
set ( FAUSTLIVE_SRC 	${FAUSTLIVE_SRC} 	 ${AUDIO_SRC})
set ( FAUSTLIVE_HEADERS ${FAUSTLIVE_HEADERS} ${AUDIO_HEADERS})

set (INCLUDE_DIRS ${FAUSTLIVE_INCLUDE_DIRS} ${QTINCDIRS} ${FAUST_INCLUDE_DIRS})
set (FAUSTLIVE_DEFINITIONS ${FAUSTLIVE_DEFINITIONS} -DVERSION="${VERSION}" -DAPP_VERSION="2.8" -DHTTPCTRL -DQRCODECTRL)
set (FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES} ${FAUST_LIBRARIES} ${QTLIBS} ${LIBMICROHPPTD_LIBRARIES})
#message ("FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES}")


#######################################
# add ui and resources
set (FAUSTLIVE_RSRC ${RSRCDIR}/application.qrc ${RSRCDIR}/styles.qrc)
set (FAUSTLIVE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/rsrc/FaustLiveInfo.plist)
set (FAUSTLIVE_ICON_FILE ${RSRCDIR}/Images/FaustLiveIcon.icns)
set_source_files_properties(${FAUSTLIVE_RSRC} ${FAUSTLIVE_ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
set(FAUSTLIVE_SRC ${FAUSTLIVE_SRC} ${FAUSTLIVE_RSRC} ${FAUSTLIVE_ICON_FILE})

if (WIN32)
	enable_language(RC)
	set(FAUSTLIVE_SRC ${FAUSTLIVE_SRC} ${CMAKE_CURRENT_SOURCE_DIR}/rsrc/FaustLive.rc)
	set(FAUSTLIVE_EXE WIN32)
elseif(APPLE)
	set(FAUSTLIVE_EXE MACOSX_BUNDLE)
endif()

if(APPLE)
	set (OSXTARGET 10.11)
	set (CMAKE_C++_FLAGS -mmacosx-version-min=${OSXTARGET})
	set (CMAKE_LDFLAGS -mmacosx-version-min=${OSXTARGET})
	set (CMAKE_OSX_DEPLOYMENT_TARGET ${OSXTARGET})
	set (MACOSX_DEPLOYMENT_TARGET ${OSXTARGET})
	message (STATUS "MacOSX deployment target set to ${OSXTARGET}")
	set (FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES} -L/opt/local/lib )
elseif(UNIX)
	set (FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES} -lpthread -lasound )
endif()

#message ("FAUSTLIVE_LIBRARIES ${FAUSTLIVE_LIBRARIES}")

set (FAUSTLIVE_RPATH ${FAUSTLIVE_RPATH} "/usr/lib")
set (CMAKE_BUILD_RPATH ${FAUSTLIVE_RPATH})
set (CMAKE_SKIP_BUILD_RPATH FALSE)
#message ("FAUSTLIVE_RPATH ${FAUSTLIVE_RPATH}")

set (BINDIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
#######################################
# add FaustLive
set (faustlive FaustLive)
add_executable(${faustlive} ${FAUSTLIVE_EXE} ${FAUSTLIVE_SRC} ${FAUSTLIVE_HEADERS})
target_include_directories (${faustlive} PRIVATE ${INCLUDE_DIRS})
target_compile_definitions (${faustlive} PRIVATE ${FAUSTLIVE_DEFINITIONS})
target_link_libraries (${faustlive} PRIVATE ${FAUSTLIVE_LIBRARIES} )
set_target_properties (${faustlive} PROPERTIES
	BUNDLE true
	MACOSX_BUNDLE_BUNDLE_NAME ${faustlive}
	MACOSX_BUNDLE_ICON_FILE ${FAUSTLIVE_ICON_FILE}
	MACOSX_BUNDLE_INFO_PLIST ${FAUSTLIVE_INFO_PLIST}
	MACOSX_BUNDLE_BUNDLE_VERSION "v.${VERSION}"
	MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION}"
	MACOSX_BUNDLE_INFO_PLIST ${FAUSTLIVE_INFO_PLIST}
	RUNTIME_OUTPUT_DIRECTORY  ${BINDIR}
	RUNTIME_OUTPUT_DIRECTORY_RELEASE  ${BINDIR}
)
if (MSVC)
target_compile_options(${faustlive} PRIVATE /permissive-)
target_compile_options(${faustlive} PUBLIC "/Zc:__cplusplus")
endif ()

#######################################
# windows post processing
if (WIN32)
configure_file (${ROOT}/lib/portaudio/lib/portaudio.dll ${BINDIR}/portaudio_x64.dll COPYONLY)
configure_file (${LIBSNDFILE}/bin/sndfile.dll ${BINDIR}/sndfile.dll COPYONLY)
# add_custom_command(TARGET ${faustlive} POST_BUILD
#                    COMMAND windeployqt FaustLive.exe
#                    WORKING_DIRECTORY ${BINDIR})
endif()


#######################################
# install section
#######################################
# install paths 
if (APPLE)
	set (CMAKE_INSTALL_PREFIX $ENV{HOME}/FaustLive-${VERSION})
elseif(WIN32)
	set (HOME $ENV{USERPROFILE})
	string (REPLACE "\\" "/" UXHOME "${HOME}")
	set (CMAKE_INSTALL_PREFIX ${UXHOME}/FaustLive-${VERSION})
endif()
if (PACK)
	set (CMAKE_INSTALL_PREFIX .)
elseif (DEFINED PREFIX)
	set (CMAKE_INSTALL_PREFIX ${PREFIX})
endif()
message (STATUS "FaustLive install prefix set to ${CMAKE_INSTALL_PREFIX}")

if (APPLE OR WIN32)
	set (INSTALL_BIN 		${CMAKE_INSTALL_PREFIX})
	set (INSTALL_DOC 		${CMAKE_INSTALL_PREFIX}/Documentation)
else()
	set (INSTALL_BIN 		${CMAKE_INSTALL_PREFIX}/bin)
	set (INSTALL_SHARED 	${CMAKE_INSTALL_PREFIX}/share/)
	set (INSTALL_DOC 		${INSTALL_SHARED}/doc/faustlive)
	set (INSTALL_ICONS 		${INSTALL_SHARED}/icons/hicolor)
endif()

if (APPLE)
install ( CODE "message (STATUS \"Call macdeployqt\")"
		  CODE "execute_process ( COMMAND macdeployqt ${BINDIR}/FaustLive.app )"
)
endif()


####################################
# copy DLLs with windeployqt
# Find the path to windeployqt executable
# Create a custom command to run windeployqt after building the target
if (WIN32)

	find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "$ENV{Qt5_DIR}/../../../bin" REQUIRED)

    add_custom_command(TARGET ${faustlive}
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E env PATH="$ENV{Qt5_DIR}/../../../bin"
        "${WINDEPLOYQT_EXECUTABLE}"
        --no-compiler-runtime
        --no-angle
        --no-opengl-sw
        --no-system-d3d-compiler
        --no-webkit2
        # --no-webkit2-process
        # --no-translate
        --no-translations
        --no-quick-import
        # --no-quick-dialogs
        --qmldir "${CMAKE_SOURCE_DIR}"
        \"$<TARGET_FILE:FaustLive>\"
        COMMENT "Running windeployqt..."
    )
endif()


####################################
# install compiled resources
install ( TARGETS ${faustlive}
	RUNTIME DESTINATION ${INSTALL_BIN}
	BUNDLE DESTINATION  ${INSTALL_BIN}
)

if (WIN32)
install (
    DIRECTORY ${BINDIR}/ DESTINATION ${INSTALL_BIN}
    USE_SOURCE_PERMISSIONS
	PATTERN "FaustLive.exe" EXCLUDE
)
endif()

####################################
# install documentation
install (
    FILES ${ROOT}/Documentation/faust-quick-reference.pdf ${ROOT}/Documentation/UserManual.pdf DESTINATION ${INSTALL_DOC}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
if (APPLE OR WIN32)
install (
    FILES ${ROOT}/build/README.html DESTINATION ${INSTALL_BIN}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
else()
install (
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/rsrc/FaustLive.desktop DESTINATION ${INSTALL_SHARED}/applications/
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install (
    FILES ${RSRCDIR}/Images/FaustLiveIcon.png DESTINATION ${INSTALL_ICONS}/32x32/apps/
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install (
    FILES ${RSRCDIR}/Images/Faustlive.svg DESTINATION ${INSTALL_ICONS}/scalable/apps/
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
install (
    FILES ${RSRCDIR}/Images/Faustlive.xpm DESTINATION ${INSTALL_SHARED}/pixmaps/
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

endif()

####################################
# install VS redistributables
if (MSVC)
  set (CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${INSTALL_BIN})
  include (InstallRequiredSystemLibraries)
endif()


####################################
# deploy applications

set (PACKNAME FaustLive-${VERSION})
if(WIN32)
	if (PACK)
		set  (INSTALL_APPS ${CMAKE_CURRENT_SOURCE_DIR}/guidodir/_CPack_Packages/win64/NSIS64/${PACKNAME}-win64)
	endif()
	install ( CODE "message (STATUS \"Call windeployqt\")"
			  CODE "execute_process ( COMMAND windeployqt ${INSTALL_APPS}/FaustLive.exe )"
	)
endif()


####################################
# cpack section
####################################
set (CPACK_PACKAGE_NAME   "FaustLive")
set (CPACK_PACKAGE_VENDOR "GRAME")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "FAUSTLive - an environment for the design of Faust DSP.")
set (CPACK_PACKAGE_VERSION   ${VERSION})
set (CPACK_PACKAGE_VERSION_MAJOR   ${VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR   ${VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH   ${VERSION_PATCH})

set (CPACK_MONOLITHIC_INSTALL true)
set (CPACK_RESOURCE_FILE_LICENSE  ${ROOT}/GPL.txt)
set (CPACK_PACKAGE_INSTALL_DIRECTORY ${PACKNAME})

####################################
# NSIS specific section
set (CPACK_NSIS_INSTALL_ROOT "C:\\\\Program Files")
set (CPACK_NSIS_MODIFY_PATH on)
set (CPACK_NSIS_MUI_ICON "${ROOT}/Resources/Images/FaustLiveIcon.ico")
set (CPACK_GENERATOR NSIS64)

include (CPack)
