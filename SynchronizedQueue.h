#ifndef SYNCHRONIZED__Q_H
#define SYNCHRONIZED__Q_H

void q_init(int size);

void q_push(char* fileNamePush, char* filePathPush);

void q_pop(char** outputFileName, char** outputFilePath);

#endif