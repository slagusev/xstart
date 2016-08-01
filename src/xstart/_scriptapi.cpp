#include <corela.h>
#include <strtrim.h>
#include <time.h>
#include "_machine.h"
#include "Rect.h"

int argc;
char **argv;

#ifndef _WIN32
#define _popen popen
#define _pclose pclose
#endif

int GM_CDECL Script_GetVersion(gmThread* a_thread) {
	a_thread->PushNewString(VERSION);
	return GM_OK;
}

int GM_CDECL Script_LoadConfig(gmThread* a_thread) {
	GM_CHECK_STRING_PARAM(key, 0);

	// always use config.ini
	INI* ini = INIOpen("config.ini", true);
	if(!ini) {
		Log(LOG_ERROR, "No config.ini found!");
		a_thread->PushNull();
		return GM_OK;
	}

	// always use first section
	INISECTION* sec = INIEnumSections(ini, 0);
	if(!sec) {
		sec = INIGetSection(ini, "config", true);

		if(!sec) {
			Log(LOG_ERROR, "No [config] section found in config.ini!");
			INIClose(ini, true);
			a_thread->PushNull();
			return GM_OK;
		}
	}

	// get entry
	INIENTRY* ent = INIGetEntry(sec, key, true, "");
	if(!ent) {
		Log(LOG_ERROR, "Entry '%s' not found in config.ini!");
		INIClose(ini, true);
		a_thread->PushNull();
		return GM_OK;
	}
	char entStr[4097];
	INIGetEntryStr(ent, entStr, 4096);

	Log(LOG_INFO, "Loaded config '%s' is '%s'.", key, entStr);

	// return result
	INIClose(ini, false);
	a_thread->PushNewString(entStr);
	return GM_OK;
}

int GM_CDECL Script_SaveConfig(gmThread* a_thread) {
	GM_CHECK_STRING_PARAM(key, 0);
	GM_CHECK_STRING_PARAM(value, 1);

	Log(LOG_INFO, "Saving config '%s' as '%s'.", key, value);

	// always use config.ini
	INI* ini = INIOpen("config.ini", true);
	if(!ini) {
		Log(LOG_ERROR, "Creating config.ini failed!");
		return GM_OK;
	}

	// always use first section
	INISECTION* sec = INIEnumSections(ini, 0);
	if(!sec) {
		sec = INIGetSection(ini, "config", true);

		if(!sec) {
			Log(LOG_ERROR, "Creating section in config.ini failed!");
			INIClose(ini, true);
			a_thread->PushNull();
			return GM_OK;
		}
	}

	// set entry
	INIENTRY* ent = INIGetEntry(sec, key, true, value);
	if(!ent) {
		Log(LOG_ERROR, "Creating entry in config.ini failed!");
		INIClose(ini, true);
		a_thread->PushNull();
		return GM_OK;
	}
	INISetEntryStr(ent, value);

	// save config.ini
	INIClose(ini, true);

	return GM_OK;
}

int GM_CDECL Script_Ascii(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(1);
	gmType t = a_thread->ParamType(0);
	if(t == GM_STRING) {
		GM_CHECK_STRING_PARAM(asc, 0);
		a_thread->PushInt(asc[0]);
	} else {
		GM_CHECK_INT_PARAM(c, 0);
		char ascii[4];
		//sprintf(ascii, "%c", c);
		ascii[0] = c;
		ascii[1] = 0;
		a_thread->PushNewString(ascii);
	}
	return GM_OK;
}

int GM_CDECL Script_System(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(sys, 0);
	Log(LOG_DEBUG, "Running system(%s) ...", sys);
	FILE* pipe = _popen(sys, "r");
	if(!pipe) {
		Log(LOG_ERROR, "Error while executing system command '%s'!", sys);
		return GM_OK;
	}

	char buffer[256];
	std::string result;
	/*while(!feof(pipe)) {
		if(fgets(buffer, 255, pipe) != NULL) {
			result += buffer;
		}
	}*/
	int n;
	while(n = fread(buffer, 1, 255, pipe)) {
		buffer[n] = '\0';
		result += buffer;
	}
	_pclose(pipe);
	a_thread->PushNewString(result.c_str());
	return GM_OK;
}

//#include <Windows.h>
int GM_CDECL Script_System_Async(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(sys, 0);
	Log(LOG_DEBUG, "Running start(%s) ...", sys);
	//	CreateProcessA(NULL, sys, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, NULL, NULL);
#ifdef _WIN32
//	std::string cmd = std::string("start ") + sys;
	PROCESS_INFORMATION procInfo;

	STARTUPINFO startupInfo;
	memset(&startupInfo, 0, sizeof(STARTUPINFO));
	startupInfo.cb = sizeof(STARTUPINFO);
	startupInfo.dwX = CW_USEDEFAULT;
	startupInfo.dwY = CW_USEDEFAULT;
	startupInfo.dwXSize = CW_USEDEFAULT;
	startupInfo.dwYSize = CW_USEDEFAULT;
	startupInfo.dwXCountChars = STARTF_USESIZE;
	startupInfo.dwYCountChars = STARTF_USESIZE;
	startupInfo.dwFillAttribute = FOREGROUND_BLUE;

	CreateProcessA(NULL, (LPSTR)sys, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &startupInfo, &procInfo);
#else
	std::string cmd = sys + std::string(" &");
	system(cmd.c_str());
#endif
	return GM_OK;
}

int GM_CDECL Script_Exit(gmThread* th) {
	MachineDestroy();
	exit(0);
	return GM_OK;
}

int GM_CDECL Script_Print(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(out, 0);
	printf("%s",out);
	fflush(stdout);
	return GM_OK;
}

int GM_CDECL Script_PrintLine(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(out, 0);
	printf("%s\n", out);
	fflush(stdout);
	return GM_OK;
}

int GM_CDECL Script_Log(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(out, 0);
	Log(LOG_SCRIPT, out);
	return GM_OK;
}

int GM_CDECL Script_Warning(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(out, 0);
	Log(LOG_WARNING, out);
	return GM_OK;
}

int GM_CDECL Script_Error(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(out, 0);
	Log(LOG_ERROR, out);
	return GM_OK;
}

int GM_CDECL Script_Fatal(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(out, 0);
	Log(LOG_FATAL, out);
	exit(-1);
	return GM_OK;
}

int GM_CDECL Script_Popup(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(info, 0);
	Log(LOG_POPUP, info);
	return GM_OK;
}

int GM_CDECL Script_Ask(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(ask, 0);
	printf("%s", ask);
	fflush(stdout);

	char answer[1024*10];
	memset((void*)answer, 0, 1024*10);

	fgets(answer, 1024*10-1, stdin);
	trim(answer);

	a_thread->PushNewString(answer);

	return GM_OK;
}

int GM_CDECL Script_GetTime(gmThread* a_thread) {
	float t = (float)TimeGet();
	a_thread->PushFloat(t);
	return GM_OK;
}

int GM_CDECL Script_GetMemUsage(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(0);
	a_thread->PushInt(machine->GetCurrentMemoryUsage());
	return GM_OK;
}

int GM_CDECL Script_Collect(gmThread* a_thread) {
	if(a_thread->GetNumParams() == 0) {
		machine->CollectGarbage(true);
	} else {
		GM_CHECK_INT_PARAM(full,0);
		machine->CollectGarbage(full==1);
	}
	return GM_OK;
}

int GM_CDECL Script_Redirect(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(redTo, 0);
	freopen(redTo, "wb", stdout);
	return GM_OK;
}

int GM_CDECL Script_GlobalHelp(gmThread* a_thread) {
	std::string help = MachineGlobalHelp();
	a_thread->PushNewString(help.c_str());
	return GM_OK;
}

int GM_CDECL Script_ClassList(gmThread* a_thread) {
	std::string list = MachineGetClassList();
	a_thread->PushNewString(list.c_str());
	return GM_OK;
}

int GM_CDECL Script_Random(gmThread* a_thread) {
	if(a_thread->ParamType(0) == GM_INT) {
		int scalei;
		a_thread->ParamInt(0, scalei, 1);
		//a_thread->PushInt( ((gmfloat)rand() / (gmfloat)RAND_MAX) * scalei);
		a_thread->PushInt(roundf((float)rand() / (float)RAND_MAX * scalei));
		return GM_OK;
	}

	gmfloat scale = 1.0;
	if(a_thread->GetNumParams() >= 1) {
		a_thread->ParamFloatOrInt(0, scale, 1.0);
	}
	a_thread->PushFloat( (gmfloat)rand() / (gmfloat)RAND_MAX * scale);
	return GM_OK;
}

#ifdef _WIN32
int GM_CDECL Script_PlaySound(gmThread* a_thread) {
	GM_CHECK_STRING_PARAM(file, 0);
	SoundSimplePlay(file);
	return GM_OK;
}
#endif

int GM_CDECL Script_CreateInstance(gmThread* a_thread) {
	GM_CHECK_STRING_PARAM(className, 0);
	return MachineCreateClassInstance(a_thread, className);
}

int GM_CDECL Script_Pause(gmThread* a_thread) {
	GM_CHECK_FLOAT_OR_INT_PARAM(s, 0);
	TimeSleep(s);
	return GM_OK;
}

int GM_CDECL Script_Throw(gmThread* a_thread) {
	if(a_thread->GetNumParams() > 0) {
		if(a_thread->ParamType(0) == GM_STRING) {
			const char* error;
			a_thread->ParamString(0, error, "");
			GM_EXCEPTION_MSG(error);
		}
	}
	return GM_EXCEPTION;
}

int GM_CDECL Script_Callstack(gmThread* a_thread) {
	std::string callstack;

	//machine->GetLog().LogEntry(GM_NL"CALLSTACK:");
	gmStackFrame * frame = a_thread->m_frame;
	int base = a_thread->m_base;
	const gmuint8 * ip = a_thread->m_instruction;

	while(frame) {
		// get the function object
		gmVariable * fnVar = &a_thread->m_stack[base - 1];
		if(fnVar->m_type == GM_FUNCTION) {
			gmFunctionObject * fn = (gmFunctionObject *) GM_MOBJECT(a_thread->m_machine, fnVar->m_value.m_ref);
			//m_machine->GetLog().LogEntry("%3d: %s", fn->GetLine(ip), fn->GetDebugName());
			//callstack += "" + fn->GetLine(ip);
			callstack += ":";
			callstack += fn->GetDebugName();
			callstack += "\n";
		}
		base = frame->m_returnBase;
		ip = frame->m_returnAddress;
		frame = frame->m_prev;
	}
	a_thread->PushNewString(callstack.c_str());
	//m_machine->GetLog().LogEntry("");
	return GM_OK;
}

int GM_CDECL Script_Eval(gmThread* a_thread) {
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(script, 0);
	GM_INT_PARAM(now, 1, 1); // 2nd param is execute now flag
	gmVariable paramThis = a_thread->Param(2, gmVariable::s_null); // 3rd param is 'this'

	int id = GM_INVALID_THREAD;
	if( script ) {
		int errors = a_thread->GetMachine()->ExecuteString(script, &id, (now) ? true : false, NULL, &paramThis);
		if( errors ) { return GM_EXCEPTION; }
		a_thread->PushInt(id);
	}
	return GM_OK;
}

int GM_CDECL Script_InputFile(gmThread* a_thread) {
	std::string file = RequestFileName();
	if(file.length()) { a_thread->PushNewString(file.c_str()); }
	else { a_thread->PushNull(); }
	return GM_OK;
}

#ifdef _WIN32
int GM_CDECL Script_GetMonitorRect(gmThread* a_thread) {
	GM_CHECK_INT_PARAM(index, 0);
	CORECT _rc;
	GetMonitorRect(index, &_rc);
	Rect* rc = new Rect();
	rc->left = _rc.left;
	rc->top = _rc.top;
	rc->right = _rc.right;
	rc->bottom = _rc.bottom;
	return rc->ReturnThis(a_thread);
}
#endif

int GM_CDECL Script_EnableConsoleDebug(gmThread* a_thread) {
	GM_CHECK_INT_PARAM(level, 0);
	SetLogLevel(level);
	return GM_OK;
}

int GM_CDECL Script_EnableConsoleColors(gmThread* a_thread) {
	GM_CHECK_INT_PARAM(enable, 0);
	SetLogColors(enable);
	return GM_OK;
}

int GM_CDECL Script_GetArguments(gmThread* a_thread) {
	if (a_thread->GetNumParams() == 0) { a_thread->PushInt(argc); return GM_OK; }
	GM_CHECK_INT_PARAM(index, 0);
	if (index >= 0 && index < argc) { a_thread->PushNewString(argv[index]); }
	else { a_thread->PushNewString(""); }
	return GM_OK;
}

int GM_CDECL Script_FcloseAll(gmThread* a_thread) {
	// TODO: Properly fix file handles
	int closed = _fcloseall();
	if (closed) {
		Log(LOG_WARNING, "Closed %d open file streams.", closed);
	}
	return GM_OK;
}

/*int GM_CDECL Script_Format(gmThread* a_thread) {

}*/

void RegisterCommonAPI() {
	srand(time(0));
	MachineRegisterFunction("_help", Script_GlobalHelp);
	MachineRegisterFunction("_classlist", Script_ClassList);
	MachineRegisterFunction("_freeMemory", Script_Collect);
	MachineRegisterFunction("_getMemoryUsage", Script_GetMemUsage);
	MachineRegisterFunction("_fcloseAll", Script_FcloseAll);
	MachineRegisterFunction("version", Script_GetVersion, " {string} version()", "Returns the version string.");
	MachineRegisterFunction("exit", Script_Exit, "exit()", "Terminates the program immediately.");
	MachineRegisterFunction("throw", Script_Throw, "throw()", "Throws an exception.");
	MachineRegisterFunction("ascii", Script_Ascii, " {string} ascii({int} ascii)", "Returns the string character for the given ascii code.");
	MachineRegisterFunction("system", Script_System, " {string} system({string} command)", "Executes a command and redirects output via pipe so you can catch the output of the command.");
	MachineRegisterFunction("start", Script_System_Async, " {string} start({string} command)", "Executes a command without waiting for it to finish, thus returning immediately.");
	MachineRegisterFunction("random", Script_Random, " {float} random( (optional) {float} (or) {int} max)", "Returns a random number between 0 and 'max'. If 'max' is a floating point number, the result is a floating point number too, otherwise its an integer.");
	MachineRegisterFunction("time", Script_GetTime, " {float} time()", "Gets the elapsed time since the program start in seconds.");
	MachineRegisterFunction("print", Script_Print, "print({string})", "Outputs the string on the console, no newline is added.");
	MachineRegisterFunction("println", Script_PrintLine, "println({string})", "Outputs the string on the console, a newline is added.");
	MachineRegisterFunction("log", Script_Log, "log({string} message)", "Writes a log message to the console.");
	MachineRegisterFunction("warning", Script_Warning, "warning({string} error)", "Reports a warning on the console.");
	MachineRegisterFunction("error", Script_Error, "error({string} error)", "Reports a error on the console.");
	MachineRegisterFunction("fatal", Script_Fatal, "fatal({string} error)", "Reports a fatal error on the console and terminates the program immediately!");
	MachineRegisterFunction("popup", Script_Popup, "popup({string} info)", "Opens a message box with the given information.");
	MachineRegisterFunction("ask", Script_Ask, " {string} ask({string} question)", "Prompts the user for a line of input.");
	MachineRegisterFunction("redirect", Script_Redirect, "redirect({string} file)", "Redirects the console output to a file of the given name.");
	MachineRegisterFunction("load", Script_LoadConfig, " {string} get({string} key)", "Loads a config string.");
	MachineRegisterFunction("save", Script_SaveConfig, "save({string} key, {string} value)", "Saves a config string.");
	MachineRegisterFunction("instance", Script_CreateInstance, "{object} instance({string} class)", "Creates an instance from a class type name. Used for introspection.");
	MachineRegisterFunction("pause", Script_Pause, "pause( {float} s)", "Pauses the execution for the given time in seconds.");
	MachineRegisterFunction("eval", Script_Eval, "eval({string} code)", "Execute given code.");
	MachineRegisterFunction("callstack", Script_Callstack, "{string} callstack()", "Returns the current callback as string.");
	MachineRegisterFunction("debug", Script_EnableConsoleDebug, "debug({int} level)", "Enable/Disable console debug messages.");
	MachineRegisterFunction("colors", Script_EnableConsoleColors, "colors({int} enable)", "Enable/Disable console colors.");
	MachineRegisterFunction("arg", Script_GetArguments, "{string} arg({int} number)", "Get command-line arguments.");
#ifdef _WIN32
	MachineRegisterFunction("sound", Script_PlaySound, "sound({string} file)", "Plays a sound file. (This is a simple and inefficient way to play a sound file)");
	MachineRegisterFunction("askFile", Script_InputFile, "{string} askFile()", "Opens a dialog where the user can select a file.");
	MachineRegisterFunction("monitor", Script_GetMonitorRect, "[Rect] monitor({int} index)", "Gets the rectangle area of the given monitor in virtual screen space.");
#endif
}

