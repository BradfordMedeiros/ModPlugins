#ifndef MODPLUGIN_SQLPARSETEST
#define MODPLUGIN_SQLPARSETEST

#include <iostream>
#include <exception>
#include "./sqlparse.h"

void tokenize1();
void tokenize2();
void tokenize3();
void tokenize4();
void tokenize5();
void tokenize6();
void tokenize7();

void lexTestSelect1();
void lexTestSelect2();
void lexTestSelect3();
void lexTestSelect4();
void lexTestSelectSplice();
void lexTestSelectSpliceWeirdSpacing();
void lexTestInsert1();
void lexTestOperators();

void testParserComplete();
void testParserIncomplete();

void testCompileSqlCreateTable();
void testCompileSqlDropTable();
void testCompileSqlSelect();
void testCompileSqlUpdate();

#endif