//
//  FLEffect.cpp
//  
//
//  Created by Sarah Denoux on 30/05/13.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

// An Effect takes care of the compilation of a DSP. Moreover, it is notify in case, the DSP has been modified. 

#include "FLEffect.h"

#define LIBRARY_PATH "/Resources/Libs/"

#include "faust/llvm-dsp.h"

#ifdef REMOTE
#include "faust/remote-dsp.h"
#endif

/***********************************EFFECT IMPLEMENTATION*********************************/

FLEffect::FLEffect(bool recallVal, string sourceFile, string name, bool isLocal){
    
    fFactory = NULL;
    fOldFactory = NULL;
    fWatcher = NULL;
    fSynchroTimer = NULL;
    fSource = sourceFile;
    fName = name;
    fForceSynchro = false;
    fRecalled = recallVal; //Effect is build in a recall situation
    
    fIsLocal = isLocal;
    fIpMachineRemote = "localhost";
    fPortMachineRemote = 0;

}

FLEffect::~FLEffect(){
    
    delete fSynchroTimer;
    delete fWatcher;
    //    printf("deleting factory = %p\n", factory);
    
    if(fIsLocal)
        deleteDSPFactory(fFactory);
    else
        deleteRemoteDSPFactory(fRemoteFactory);
        
}


//Initialisation of the effect. From a source, it extracts the source file, the name and builds the factory
//currentSVGFolder = where to create the SVG-Folder tied to the factory 
//currentIRFolder = where to save the bitcode tied to the factory
//Compilation Options = needed to build the llvm factory
//Error = if the initialisation fails, the function returns false + the buffer is filled

bool FLEffect::init(const string& currentSVGFolder, const string& currentIRFolder ,string compilationMode, int optValue, string& error, const string& IPremote, int portremote){
    
    printf("FICHIER SOURCE = %s\n", fSource.c_str());
    
    fCompilationOptions = compilationMode;
    fOpt_level = optValue;
    fIpMachineRemote = IPremote;
    fPortMachineRemote = portremote;

    if(buildFactory(kCurrentFactory, error, currentSVGFolder, currentIRFolder)){
        
        //Initializing watcher looking for changes on DSP through text editor 
        fWatcher = new QFileSystemWatcher(this);
        fSynchroTimer = new QTimer(fWatcher);
        connect(fSynchroTimer, SIGNAL(timeout()), this, SLOT(effectModified()));
        
        connect(fWatcher, SIGNAL(fileChanged(const QString)), this, SLOT(reset_Timer(const QString)));
        
        return true;
    }
    
    else
        return false;
}

//---------------FACTORY ACTIONS

//Creating the factory with the specific compilation options, in case of an error the buffer is filled
bool FLEffect::buildFactory(int factoryToBuild, string& error, string currentSVGFolder, string currentIRFolder){
    
    //+2 = Path to DSP + -svg to build the svg Diagram
    int argc = 1 + get_numberParameters(fCompilationOptions);
    
    char ** argv;
    argv = new char*[argc];
    
    //Parsing the compilationOptions from a string to a char**
    string copy = fCompilationOptions;
    for(int i=1; i<argc; i++){
        
        string parseResult = parse_compilationParams(copy);
        argv[i] = new char[parseResult.length()+1];
        
//        if(parseResult.compare("-double") != 0)  
            strcpy(argv[i], parseResult.c_str());
    }

    argv[0] = new char[5];
    strcpy(argv[0], "-svg");
    
    const char** argument = (const char**) argv;
    
    //The library path is where libraries like the scheduler architecture file are = Application Bundle/Resources
    char libraryPath[256];
    
#ifdef __APPLE__
    snprintf(libraryPath, 255, "%s%s", QFileInfo(QFileInfo( QCoreApplication::applicationFilePath()).absolutePath()).absolutePath().toStdString().c_str(), LIBRARY_PATH);
#endif
#ifdef __linux__
    snprintf(libraryPath, 255, "%s%s", QFileInfo( QCoreApplication::applicationFilePath()).absolutePath().toStdString().c_str(), LIBRARY_PATH);
#endif
    
    string path = currentIRFolder + "/" + fName;

    //To speed up the process of compilation, when a session is recalled, the DSP are re-compiled from Bitcode 
    
        llvm_dsp_factory* buildingFactory = NULL;
        remote_dsp_factory* buildingRemoteFactory = NULL;
    
//    if(fRecalled && QFileInfo(path.c_str()).exists() && fIsLocal){        
//        buildingFactory = readDSPFactoryFromBitcodeFile(path, "");
//        
//        printf("factory from IR = %p\n", factoryToBuild);
//    }
    
    fRecalled = false;
    
    if(buildingFactory == NULL && buildingRemoteFactory == NULL){
        
        std::string getError("");
        
        if(fIsLocal)        
            buildingFactory = createDSPFactoryFromFile(fSource, argc , argument, libraryPath, currentSVGFolder, "", getError, fOpt_level);
        else{
         
            printf("IP = %s\n", fIpMachineRemote.c_str());
            
            buildingRemoteFactory = createRemoteDSPFactoryFromFile(fSource, argc, argument, fIpMachineRemote, fPortMachineRemote, getError, fOpt_level);
        }
        
        error = getError;
        
        printf("ERROR OF FACTORY BUILD = %s\n", error.c_str());
        
        printf("FACTORY = %p\n", buildingRemoteFactory);
        
        delete [] argv;
        
        //The creation date is nedded in case the text editor sends a message of modification when actually the file has only been opened. It prevents recompilations for bad reasons
        fCreationDate = fCreationDate.currentDateTime();
        
        if(buildingFactory != NULL || buildingRemoteFactory != NULL){
            
            if(!QFileInfo(currentIRFolder.c_str()).exists()){
                QDir direct(currentIRFolder.c_str());
                direct.mkdir(currentIRFolder.c_str());
            }
            
            if(fIsLocal){
                //The Bitcode files are written at each compilation 
                writeDSPFactoryToBitcodeFile(buildingFactory, path);
                
                if(factoryToBuild == kCurrentFactory){
                    fFactory = buildingFactory;
                    printf("FFACTORY = %p\n", fFactory);
                }
                else
                    fOldFactory = buildingFactory;
            }
            else{
                if(factoryToBuild == kCurrentFactory)
                    fRemoteFactory = buildingRemoteFactory;
                else
                    fOldRemoteFactory = buildingRemoteFactory;
            }
                
            printf("BUILD FACTORY SUCCEEDEED\n");
            return true;
        }
        else{
            printf("FALSE IS RETURNED\n");
            return false;
        }
        //    printf("NEW FACTORY = %p\n", factory);
    }
    else{
        delete [] argv;
        fCreationDate = fCreationDate.currentDateTime();
        return true;
    }
    
}


//Re-Build of the factory from the source file
bool FLEffect::update_Factory(string& error, string currentSVGFolder, string currentIRFolder){
    
    if(fIsLocal){
        
        fOldFactory = fFactory;
        
        llvm_dsp_factory* factory_update = NULL;
        
        if(buildFactory(kChargingFactory, error, currentSVGFolder, currentIRFolder)){
            factory_update = fFactory;
            fFactory = fOldFactory;
            fOldFactory = factory_update;
            return true;
        }
        else
            return false;
    }
    else{
        
        fOldRemoteFactory = fRemoteFactory;
        
        remote_dsp_factory* factory_update = NULL;
        
        if(buildFactory(kChargingFactory, error, currentSVGFolder, currentIRFolder)){
            factory_update = fRemoteFactory;
            fRemoteFactory = fOldRemoteFactory;
            fOldRemoteFactory = factory_update;
            return true;
        }
        else
            return false;
    }
}

//Once the rebuild is complete, the former factory has to be deleted
void FLEffect::erase_OldFactory(){
    
    //    printf("delete Factory = %p\n", oldFactory);
    
    if(fIsLocal)
        deleteDSPFactory(fOldFactory);
//    else
        
}

//Get number of compilation options
int FLEffect::get_numberParameters(const string& compilOptions){
    
    string copy = compilOptions;
    
    int argc = 0;
    int pos = 0;
    
    if(copy.find("-") == string::npos){
        return 0;
    }
    
    while(copy.find(" ", pos+1) != string::npos){
        argc++;
        pos = copy.find(" ", pos+1);
    }
    
    return argc+1;
    
}

//Hand Made Parser = a ' ' means a separation between parameters. If there are none and still there are compilation Options = it's the last one but it has to be taken into account anyway.
//Returns : the first option found
//CompilOptions : the rest of the options are kept in
string FLEffect::parse_compilationParams(string& compilOptions){
    
    string returning = "";
    
    size_t pos = compilOptions.find(" ");
    
    if(pos != string::npos){
        returning = compilOptions.substr(0, pos);
        compilOptions.erase(0, pos+1);
    }
    else if(compilOptions.compare("") != 0)
        returning = compilOptions.substr(0, compilOptions.length());
    
    return returning;
}

//---------------WATCHER & FILE MODIFICATIONS ACTIONS

//When any action on the effect is performed, the watcher has to be stopped (and then re-launched) otherwise the synchronisation is called without good reason

void FLEffect::reset_Timer(const QString /*toto*/){
    
    //    printf("Reseting Timer\n");
    
    //If the signal is triggered multiple times in 2 second, only 1 is taken into account
    if(fSynchroTimer->isActive()){
        fSynchroTimer->stop();
        fSynchroTimer->start(2000);
    }
    else
        fSynchroTimer->start(2000);
}

void FLEffect::effectModified(){
    fSynchroTimer->stop();
    //    printf("Emission FLEffectChanged\n");
    emit effectChanged();
}

void FLEffect::stop_Watcher(){
    fWatcher->removePath(fSource.c_str());
    printf("PATH STOP WATCHING = %s\n", fSource.c_str());
}

void FLEffect::launch_Watcher(){
    
    printf("PATH WATCHED= %s\n", fSource.c_str());
    
    fWatcher->addPath(fSource.c_str());
}

//--------------ACCESSORS

string FLEffect::getSource(){
    return fSource;
}

void FLEffect::setSource(string file){
    fSource = file;
}

QDateTime FLEffect::get_creationDate(){
    return fCreationDate;
}

string FLEffect::getName(){
    
    return fName; 
}

void FLEffect::setName(string name){
    fName = name;
}

llvm_dsp_factory* FLEffect::getFactory(){
    return fFactory;
}

remote_dsp_factory* FLEffect::getRemoteFactory(){
    return fRemoteFactory;
}

string FLEffect::getCompilationOptions(){
    return fCompilationOptions;
}

void FLEffect::update_compilationOptions(string& compilOptions, int newOptValue){
    if(fCompilationOptions.compare(compilOptions) !=0 || fOpt_level != newOptValue){
        fCompilationOptions = compilOptions;
        fOpt_level = newOptValue;   
        //        printf("opt level = %i\n", opt_level);
        fForceSynchro = true;
        emit effectChanged();
    }
}

void FLEffect::update_remoteMachine(const string& ip, int port){
    
    fIpMachineRemote = ip;
    fPortMachineRemote = port;
    fForceSynchro = true;
    emit effectChanged();
}

int FLEffect::getOptValue(){
    return fOpt_level;
}

bool FLEffect::isSynchroForced(){
    return fForceSynchro;
}

void FLEffect::setForceSynchro(bool val){
    fForceSynchro = val;
}

bool FLEffect::isLocal(){
    return fIsLocal;
}

