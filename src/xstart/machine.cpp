#include <corela.h>
#include <strtrim.h>
#include <gm/gmThread.h>
#include <gm/gmMachine.h>
#include <gm/gmStringLib.h>
#include <gm/gmMathLib.h>
#include <gm/gmDebug.h>
#include "ScriptObject.h"
#include "machine.h"

/*
#ifdef _WIN32
#ifndef STDCALL
#define STDCALL __stdcall
#else
#define STDCALL
#endif
#endif

typedef void (STDCALL* fpError)(int errorNumber, const char* errorMessage);
typedef char* (STDCALL* fpAlloc)(unsigned long memoryNeeded);
char* STDCALL AStyleMain(const char* pSourceIn, const char* pOptions, fpError fpErrorHandler, fpAlloc fpMemoryAlloc);
*/

typedef struct CLASS_CONSTRUCTOR {
	std::string typeName;
	gmCFunction ctorFunction;
} CLASS_CONSTRUCTOR;

gmMachine* machine;  // game monkey engine machine instance
gmType GM_TYPE_OBJECT = GM_NULL;  // script object types (will be set by GM)
std::map<std::string,SCRIPT_FUNCTION_DATA> functions;
std::vector<std::string> functionsOrder;
std::map<std::string,CLASS_CONSTRUCTOR> classes;
std::vector<std::string> classesOrder;


/******************************************************************************
* MachineRegisterFunction
*******************************************************************************/
void MachineRegisterFunction(const char* name, gmCFunction fn, std::string declaration, std::string help) {
	if(fn) machine->RegisterLibraryFunction(name, fn);

	// add function pointer to "functions" list
	SCRIPT_FUNCTION_DATA data;
	data.fn = 0;
	data.params = PARAM_THREAD;
	data.declaration = declaration;
	data.help = help;
	functions[std::string(name)] = data;
	functionsOrder.push_back(std::string(name));
}


/******************************************************************************
* MachineRegisterClass
*******************************************************************************/
void MachineRegisterClass(const char* typeName, gmCFunction ctor) {
	CLASS_CONSTRUCTOR cc;
	cc.typeName = typeName;
	cc.ctorFunction = ctor;
	classes[std::string(typeName)] = cc;
	classesOrder.push_back(std::string(typeName));
}


/******************************************************************************
* MachineGlobalHelp
*******************************************************************************/
std::string MachineGlobalHelp() {
	std::string out;
	out += "GLOBAL\t\t\t\n\n";

	std::vector<std::string>::iterator i;
	for(i = functionsOrder.begin(); i != functionsOrder.end(); i++) {
		SCRIPT_FUNCTION_DATA fd = functions[*i];
		out += *i + '\t' + fd.declaration + '\t' + fd.help + "\t\n";
	}

	out += "\n";
	return out;
}


/******************************************************************************
* MachineCreateClassInstance
*******************************************************************************/
int MachineCreateClassInstance(gmThread* a_thread, const char* typeName) {
	a_thread->m_numParameters--;
	if(classes.count(typeName) == 0) {
		Log(LOG_ERROR, "Creating instance of '%s' failed. No such class found!", typeName);
		return 0;
	}
	return classes[typeName].ctorFunction(a_thread);
}


/******************************************************************************
* MachineCreateClassInstance
*******************************************************************************/
std::string MachineGetClassList() {
	std::string out;

	std::vector<std::string>::iterator i;
	for(i = classesOrder.begin(); i != classesOrder.end(); i++) {
		CLASS_CONSTRUCTOR cc = classes[*i];
		out += cc.typeName + "\n";
	}

	out += "\n";
	return out;
}


/******************************************************************************
* MachineCreate
*******************************************************************************/
void MachineCreate(bool debug) {
	Log(LOG_DEBUG, "Creating Script Engine ...");
	machine = new gmMachine;
	machine->SetDebugMode(debug);

	// bind gamemonkeys default string library
	gmBindStringLib(machine);
	gmBindMathLib(machine);
	gmBindDebugLib(machine);

	// create object type (ScriptObject)
	GM_TYPE_OBJECT = machine->CreateUserType("Object");
	machine->RegisterUserCallbacks(GM_TYPE_OBJECT, Script_Object_Trace, Script_Object_Destruct, Script_Object_AsString);
	machine->RegisterTypeOperator(GM_TYPE_OBJECT, O_GETDOT, NULL, Script_Object_GetDot);
	machine->RegisterTypeOperator(GM_TYPE_OBJECT, O_SETDOT, NULL, Script_Object_SetDot);
	machine->RegisterTypeOperator(GM_TYPE_OBJECT, O_GETIND, NULL, Script_Object_GetIndex);
	machine->RegisterTypeOperator(GM_TYPE_OBJECT, O_SETIND, NULL, Script_Object_SetIndex);
	machine->RegisterTypeIterator(GM_TYPE_OBJECT, Script_Object_Iterator);
	machine->RegisterLibraryFunction("Object", _MachineNewClass<ScriptObject>);
}

bool LogThreadStatus(gmThread* thread, void* context) {
//	Log(LOG_INFO, "THREAD STATUS %d", thread->GetId());
	thread->LogCallStack();
	thread->LogLineFile();
	return true;
}

/******************************************************************************
* FindNextTimestampThreadCallback
*******************************************************************************/
gmuint32 _nextThreadTS = 0;
bool FindNextTimestampThreadCallback(gmThread* thread, void* context) {
	if(thread->GetState() == gmThread::SLEEPING) {
		gmuint32 t = thread->GetTimeStamp() - machine->GetTime();
		if(t < _nextThreadTS) _nextThreadTS = t;
		if(_nextThreadTS < 0) _nextThreadTS = 0;
		return true;
	}

	if(thread->GetState() == gmThread::RUNNING) {
		_nextThreadTS = 0;
		return true;
	}

	if(thread->GetState() == gmThread::SYS_YIELD) {
		_nextThreadTS = 0;
		return true;
	}

	return true;

}

// Allocate memory for the Artistic Style formatter.
char* STDCALL ASMemoryAlloc(unsigned long memoryNeeded) {
	char* buffer = new char[memoryNeeded];
	return buffer;
}
void  STDCALL ASErrorHandler(int errorNumber, const char* errorMessage) {
	Log(LOG_COMPILE, "astyle error %d:%s", errorNumber, errorMessage);
}

/******************************************************************************
* MachineRunFile
*******************************************************************************/
bool MachineRunFile(const char* file) {
	// temporary file
	char fileTemp[4096];

#ifdef _WIN32
	char* appdata = getenv("APPDATA");
	sprintf(fileTemp, "%s\\%s", appdata, "xstart.gm");
#else
	char* home = getenv("HOME");
	sprintf(fileTemp, "%s/%s", home, ".xstart.gm");	
#endif

	// load, preprocess and save script
	Log(LOG_DEBUG, "Preprocessing script ...");
	char* buffer = PreprocessScript(file, machine);
	Log(LOG_DEBUG, "Saving temporary script to '%s'...", fileTemp);
	FileSaveBuffer(fileTemp, buffer, strlen(buffer));
	free(buffer);

	// load temporary script file
	Log(LOG_DEBUG, "Loading temporary script ...");
	coDword fileSize = 0;
	//FileReadText(fileTemp, 0, &fileSize);
	//buffer = (char*)malloc(fileSize);
	//FileReadText(fileTemp, buffer, 0);
	FileReadText(file, 0, &fileSize);
	buffer = (char*)malloc(fileSize);
	FileReadText(file, buffer, 0);

	// use AStyle on buffer
	const char* options = "--add-one-line-brackets --keep-one-line-blocks --keep-one-line-statements";
	char* textOut = AStyleMain(buffer, options, ASErrorHandler, ASMemoryAlloc);
	free(buffer);
	buffer = textOut;

	// execute buffer
	Log(LOG_DEBUG, "Executing ...");
	//machine->AddSourceCode((char*)buffer, "_xstart.gm.bak");
	//int errors = machine->CheckSyntax((char*)buffer);
	int errors = machine->ExecuteString((char*)buffer, NULL, false, file/*"_xstart.gm.bak"*/);
	
	// show compile errors
	if(errors) {
		bool first = true;
		const char* message;

		while((message = machine->GetLog().GetEntry(first))) {
			char trimmedMessage[2048];
			strtrim(trimmedMessage, message);
			if(strlen(trimmedMessage) > 0) {
				Log(LOG_COMPILE, "%s:%s", file, trimmedMessage);
			}
		}

		machine->GetLog().Reset();

		//free(buffer);
		delete buffer;
		return false;
	}

	// execute and log errors
	int deltaTime = 0;
	int lastTime = (int)(TimeGet() * 1000.0);

	while(true) {
		// find timestamp for next thread
		_nextThreadTS = 2000;  // 2 seconds max to sleep
		machine->ForEachThread(FindNextTimestampThreadCallback, 0);
		if(_nextThreadTS > 2) { /*Log(LOG_INFO, "sleeping for %d", _nextThreadTS);*/ TimeSleep( (float)(_nextThreadTS) / 1000.0 ); }

		// execute script till next sleep/yield/exception
		int num_threads = machine->Execute(deltaTime);

		// Update delta time
		int curTime = (int)(TimeGet() * 1000.0);   //timeGetTime();
		deltaTime = curTime - lastTime;
		lastTime = curTime;

		// Dump run time errors to output
		bool first = true;
		const char* message;
		std::string errorLog = "";
		while((message = machine->GetLog().GetEntry(first))) {
			errorLog += message;
			/*char trimmedMessage[2048];
			strtrim(trimmedMessage, message);
			if(strlen(trimmedMessage) > 0) {
				Log(LOG_ERROR, "%s", trimmedMessage);
			}*/
		}
		if(errorLog.length() > 0) {
			/*machine->ForEachThread(LogThreadStatus, 0);
			first = true;
			while((message = machine->GetLog().GetEntry(first))) {
				errorLog += message;
			}*/
			Log(LOG_ERROR, "%s", errorLog.c_str());
		}
		machine->GetLog().Reset();
		FrameUpdate();

//		TimeSleep(double(10-deltaTime)/1000.0);
		if(num_threads <= 0) {
			break;
		}
	}

	// cleanup
	//free(buffer);
	delete buffer;
	return true;
}


/******************************************************************************
* MachineRun
*******************************************************************************/
bool MachineRun(FILE* in) {
	char line[1024*10]; // TODO: HOPE THIS IS ENOUGH...

	while(true) {
		if(in == stdin) {
			printf("\n>");
		}
		if(!fgets(line, 1024*10-1, stdin)) {
			break;
		}
		for(unsigned int i=0; i<strlen(line); i++) {
			if(line[i] == 10) {
				line[i] = 0;
			}
		}
		if(strlen(line) == 0) {
			continue;
		}

		// compile script, dont execute now
		int errors = machine->ExecuteString(line, NULL, false, "[input]");

		// show compile errors
		if(errors) {
			bool first = true;
			const char* message;

			while((message = machine->GetLog().GetEntry(first))) {
				char trimmedMessage[2048];
				strtrim(trimmedMessage, message);
				if(strlen(trimmedMessage) > 0) {
					Log(LOG_COMPILE, "%s", trimmedMessage);
				}
			}

			machine->GetLog().Reset();
		}

		// execute and log errors
		int deltaTime = 0;
		int lastTime = (int)(TimeGet() * 1000.0);   //timeGetTime();

		while(true) {
			int num_threads = machine->Execute(deltaTime);

			// Update delta time
			int curTime = (int)(TimeGet() * 1000.0);   //timeGetTime();
			deltaTime = curTime - lastTime;
			lastTime = curTime;

			// Dump run-time errors to output
			bool first = true;
			const char* message;
			while((message = machine->GetLog().GetEntry(first))) {
				char trimmedMessage[2048];
				strtrim(trimmedMessage, message);
				if(strlen(trimmedMessage) > 0) {
					Log(LOG_ERROR, "%s", trimmedMessage);
				}
			}
			machine->GetLog().Reset();
			FrameUpdate();
//#ifdef _WIN32
//			TimeSleep(double(10-deltaTime)/1000.0);
//#endif
			if(num_threads <= 0) {
				break;
			}
		}

	}

	return true;
}


/******************************************************************************
* MachineDestroy
*******************************************************************************/
int MachineDestroy() {
	machine->ResetAndFreeMemory();
	Script_Objects_Print();
	delete machine;
	return 0;
}
