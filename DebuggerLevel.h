#ifndef M_DEBUG_H
#define M_DEBUG_H

#define DEBUGGER_KEY_HIGH "[D-HIGH] "
#define DEBUGGER_KEY_MEDIUM "[D-MEDIUM] "
#define DEBUGGER_KEY_LOW "[D-LOW] "

#define DEBUG_LVL_DISABLED 0
#define DEBUG_LVL_LOW 1
#define DEBUG_LVL_MEDIUM 2
#define DEBUG_LVL_HIGH 3

#define DEBUGGER_PRINT_HIGH(str, ...) {\
	if (M_DEBUG > DEBUG_LVL_MEDIUM) {\
		fprintf(stdout, DEBUGGER_KEY_HIGH  str "\n", ##__VA_ARGS__);\
	}\
}

#define DEBUGGER_PRINT_MEDIUM(str, ...) {\
	if (M_DEBUG > DEBUG_LVL_LOW) {\
		fprintf(stdout, DEBUGGER_KEY_MEDIUM str "\n", ##__VA_ARGS__);\
	}\
}

#define DEBUGGER_PRINT_LOW(str, ...) {\
	if (M_DEBUG > DEBUG_LVL_DISABLED) {\
		fprintf(stdout, DEBUGGER_KEY_LOW str "\n", ##__VA_ARGS__);\
	}\
}


#define DEBUGGER_INFO() {\
	if (M_DEBUG > DEBUG_LVL_DISABLED) {\
		fprintf(stdout, "----------------------Debugger info----------------------\n");\
		fprintf(stdout, " Debugger Enabled\n");\
		fprintf(stdout, " Debugger lvl: %d ", M_DEBUG);\
		switch (M_DEBUG) {\
			case DEBUG_LVL_LOW:\
				fprintf(stdout, "LOW \n");\
				break;\
			case DEBUG_LVL_MEDIUM: \
				fprintf(stdout, "MEDIUM \n");\
				break;\
			case DEBUG_LVL_HIGH: \
				fprintf(stdout, "HIGH \n");\
				break;\
			default:\
				fprintf(stdout, "unimplemented debugger lvl! \n");\
		}\
		fprintf(stdout, " \n");\
		fprintf(stdout, " notes: should disable Debugger before release\n");\
		fprintf(stdout, "-------------------------------------------------------------\n");\
	}\
}

#ifndef M_DEBUG
#define M_DEBUG DEBUG_LVL_HIGH
#endif

#endif