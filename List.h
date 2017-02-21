#ifndef _List_h
#define _List_h

#include "Arduino.h"
const int maxsize = 75;

typedef struct  DataElements { 
  byte Minute; 
  byte Hour; 
  int Data;
} 	DataElements;



class List
{
public:
  List(int lines);
  void displayNext();
  void insert(int hours, int minutes, float data);
  void setDisplaySize(int lines);
  boolean isDisplay(int line);
  int displayHours(int line);
  int displayMinutes(int line);
  float displayValue(int line);
  
private:
  int nextIndex;
  int last;
  int displayLines;
  int iDisplay1;
  int iDisplay2;
  DataElements DataList [maxsize];

} 
;


#endif /* _List_h */

