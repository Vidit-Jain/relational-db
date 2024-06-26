#include"semanticParser.h"

void executeCommand();

void executeCLEAR();
void executeCOMPUTE();
void executeCROSS();
void executeDISTINCT();
void executeEXPORT();
void executeGROUPBY();
void executeINDEX();
void executeJOIN();
void executeLIST();
void executeLOAD();
void executePRINT();
void executePROJECTION();
void executeRENAME();
void executeSELECTION();
void executeSORT();
void executeSOURCE();
void executeSYMMETRY();
void executeTRANSPOSE();
void executeORDERBY();

bool evaluateBinOp(int value1, int value2, BinaryOperator binaryOperator);
void printRowCount(int rowCount);