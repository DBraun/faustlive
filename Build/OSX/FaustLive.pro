######################################################################
# FAUSTLIVE.PRO
######################################################################

# THANKS TO QMAKE, THIS FILE WILL PRODUCE THE MAKEFILE OF FAUSTLIVE APPLICATION
# IT DESCRIBES ALL THE LINKED LIBRAIRIES, COMPILATION OPTIONS, THE SOURCES TO BE COMPILED

TEMPLATE = app

## Application Settings
OBJECTS_DIR += ../../src/objectsFolder
MOC_DIR += ../../src/objectsFolder
RCC_DIR += ../../src/Utilities

QMAKE_EXTRA_TARGETS += all

CONFIG -= x86_64
CONFIG += exceptions rtti

## QT libraries needed
QT+=widgets
QT+=core
QT+=gui
QT+=network

TARGET = FaustLive
DEFINES += APP_VERSION=\\\"1.2\\\"

## Images/Examples and other needed resources
RESOURCES   = ../../Resources/application.qrc
RESOURCES 	+= ../../Resources/unix.qrc
ICON = ../../Resources/Images/FaustLiveIcon.icns
QMAKE_INFO_PLIST = FaustLiveInfo.plist

####### INCLUDES PATHS && LIBS PATHS

DEPENDPATH += /usr/local/include/faust/gui
INCLUDEPATH += .
INCLUDEPATH += /opt/local/include
INCLUDEPATH += ../../src/Audio
INCLUDEPATH += ../../src/MenusAndDialogs
INCLUDEPATH += ../../src/MainStructure
INCLUDEPATH += ../../src/Network
INCLUDEPATH += ../../src/Utilities
	
LLVMLIBS = $$system($$system(which llvm-config) --libs)
LLVMDIR = $$system($$system(which llvm-config) --ldflags)

LIBS+=-L/usr/local/lib/faust -L/usr/lib/faust
LIBS+= $$LLVMDIR
LIBS+=-lfaust
LIBS+= $$LLVMLIBS

LIBS+=-lqrencode
LIBS+=-lmicrohttpd
LIBS+=-lcrypto
	  
LIBS+=-lHTTPDFaust
LIBS+=-lOSCFaust -loscpack
LIBS+=-L/opt/local/lib

DEFINES += HTTPCTRL
DEFINES += QRCODECTRL
DEFINES += OSCVAR

########## DEFINES/LIBS/SOURCES/... 
########## depending on audio drivers | remote processing feature

equals(REMVAR, 1){
	DEFINES += REMOTE
	LIBS+=-lfaustremote
	LIBS+=-lcurl
	LIBS+=-llo
	HEADERS += ../../src/MenusAndDialogs/FLStatusBar.h 
	SOURCES += ../../src/MenusAndDialogs/FLStatusBar.cpp
}

equals(CAVAR, 1){
	message("COREAUDIO LINKED")
	LIBS+= -L/opt/local/lib -framework CoreAudio -framework AudioUnit -framework CoreServices
	DEFINES += COREAUDIO
	INCLUDEPATH += ../../src/Audio/CA
	
	HEADERS += 	../../src/Audio/CA/CA_audioFactory.h\
				../../src/Audio/CA/CA_audioSettings.h\
				../../src/Audio/CA/CA_audioManager.h\
				../../src/Audio/CA/CA_audioFader.h 
				
	SOURCES += 	../../src/Audio/CA/CA_audioFactory.cpp \
				../../src/Audio/CA/CA_audioSettings.cpp \
				../../src/Audio/CA/CA_audioManager.cpp 
}else{
	message("COREAUDIO NOT LINKED")
}

equals(JVAR, 1){
	message("JACK LINKED")
	LIBS+= -ljack
	DEFINES += JACK
	
	INCLUDEPATH += ../../src/Audio/JA
		
	HEADERS += 	../../src/Audio/JA/JA_audioFactory.h \
				../../src/Audio/JA/JA_audioSettings.h \
				../../src/Audio/JA/JA_audioManager.h \
				../../src/Audio/JA/JA_audioFader.h \
	
	SOURCES += 	../../src/Audio/JA/JA_audioSettings.cpp \
				../../src/Audio/JA/JA_audioManager.cpp \
				../../src/Audio/JA/JA_audioFactory.cpp \
				../../src/Audio/JA/JA_audioFader.cpp 
}else{
	message("JACK NOT LINKED")
}	

equals(NJVAR, 1){
	message("NETJACK LINKED")
	LIBS += -ljacknet
	DEFINES += NETJACK
	
	INCLUDEPATH += ../../src/Audio/NJ
	
	HEADERS += 	../../src/Audio/NJ/NJ_audioFactory.h \
				../../src/Audio/NJ/NJ_audioSettings.h \
				../../src/Audio/NJ/NJ_audioManager.h \
				../../src/Audio/NJ/NJ_audioFader.h 
	
	SOURCES += 	../../src/Audio/NJ/NJ_audioFactory.cpp \
				../../src/Audio/NJ/NJ_audioSettings.cpp \
				../../src/Audio/NJ/NJ_audioManager.cpp \
				../../src/Audio/NJ/NJ_audioFader.cpp 
}else{
	message("NETJACK NOT LINKED")
}		

equals(ALVAR, 1){
	message("ALSA LINKED")
	LIBS += -lasound
	DEFINES += ALSA
	
	INCLUDEPATH += ../../src/Audio/AL
	
	HEADERS += 	../../src/Audio/AL/AL_audioFactory.h \
				../../src/Audio/AL/AL_audioSettings.h \
				../../src/Audio/AL/AL_audioManager.h \
				../../src/Audio/AL/AL_audioFader.h \
	
	SOURCES += 	../../src/Audio/AL/AL_audioFactory.cpp \
				../../src/Audio/AL/AL_audioSettings.cpp \
				../../src/Audio/AL/AL_audioManager.cpp 
}else{
	message("ALSA NOT LINKED")
}		


equals(PAVAR, 1){
	message("PORT AUDIO LINKED")
	
	LIBS += -lportaudio	
	DEFINES += PORTAUDIO
	
	INCLUDEPATH += ../../src/Audio/PA
	
	HEADERS += 	../../src/Audio/PA/PA_audioFactory.h \
				../../src/Audio/PA/PA_audioSettings.h \
				../../src/Audio/PA/PA_audioManager.h \
				../../src/Audio/PA/PA_audioFader.h \
	
	SOURCES += 	../../src/Audio/PA/PA_audioFader.cpp \
				../../src/Audio/PA/PA_audioFactory.cpp \
				../../src/Audio/PA/PA_audioSettings.cpp \
				../../src/Audio/PA/PA_audioManager.cpp 
}else{
	message("PORT AUDIO NOT LINKED")
}		

########## HEADERS AND SOURCES OF PROJECT

HEADERS +=  ../../src/Utilities/utilities.h \
			../../src/Audio/AudioSettings.h \
			../../src/Audio/AudioManager.h \
			../../src/Audio/AudioFactory.h \
			../../src/Audio/AudioCreator.h \
			../../src/Audio/AudioFader_Interface.h \
            ../../src/Audio/AudioFader_Implementation.h \
			../../src/Audio/FJUI.h \
			../../src/MenusAndDialogs/FLToolBar.h \
			../../src/MenusAndDialogs/FLrenameDialog.h \
			../../src/MenusAndDialogs/FLHelpWindow.h \
			../../src/MenusAndDialogs/FLPresentationWindow.h \
			../../src/MenusAndDialogs/FLErrorWindow.h \
			../../src/MenusAndDialogs/FLMessageWindow.h \
            ../../src/MenusAndDialogs/FLExportManager.h \
            ../../src/MenusAndDialogs/FLPreferenceWindow.h \
            ../../src/MainStructure/FLWindow.h \
            ../../src/MainStructure/FLComponentWindow.h \
            ../../src/MainStructure/FLSettings.h \
            ../../src/MainStructure/FLWinSettings.h \
            ../../src/MainStructure/FLFileWatcher.h \
            ../../src/Network/FLServerHttp.h \
            ../../src/MainStructure/FLSessionManager.h \
            ../../src/MainStructure/FLApp.h \
            ../../src/MenusAndDialogs/SimpleParser.h \
			../../src/Network/HTTPWindow.h \
			/usr/local/include/faust/gui/faustqt.h

SOURCES += 	../../src/Utilities/utilities.cpp \
			../../src/Audio/AudioCreator.cpp \
            ../../src/Audio/AudioFader_Implementation.cpp \
        	../../src/MenusAndDialogs/FLToolBar.cpp \
            ../../src/MenusAndDialogs/FLrenameDialog.cpp \
			../../src/MenusAndDialogs/FLHelpWindow.cpp \
			../../src/MenusAndDialogs/FLPresentationWindow.cpp \
            ../../src/MenusAndDialogs/FLErrorWindow.cpp \
            ../../src/MenusAndDialogs/FLMessageWindow.cpp \
            ../../src/MenusAndDialogs/FLExportManager.cpp \
            ../../src/MainStructure/FLWindow.cpp \
            ../../src/MainStructure/FLComponentWindow.cpp \
            ../../src/MainStructure/FLSettings.cpp \
            ../../src/MainStructure/FLWinSettings.cpp \
            ../../src/MainStructure/FLFileWatcher.cpp \
            ../../src/MenusAndDialogs/FLPreferenceWindow.cpp \
            ../../src/MainStructure/FLSessionManager.cpp \
            ../../src/MainStructure/FLApp.cpp \
			../../src/Network/FLServerHttp.cpp \
			../../src/Network/HTTPWindow.cpp \
			../../src/MenusAndDialogs/SimpleParser.cpp \
			../../src/Utilities/main.cpp



