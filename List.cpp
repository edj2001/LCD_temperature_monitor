#include "List.h"

List::List(int lines)
{
  nextIndex = 0;
  iDisplay1 = 0;
  setDisplaySize(lines);
}

void List::displayNext()
//move to the next display line.
//return to the beginning of the list when 
//we reach the end.
{
    iDisplay1++;
    if ( iDisplay1 >= nextIndex )
    {
      iDisplay1 = 0;
    }
}

boolean List::isDisplay(int line)
//is there enough data to display line+1 lines?
{
  return ((iDisplay1 + line) < nextIndex);
  
}



void List::insert(int hours, int minutes, float data)
{
  if (nextIndex < maxsize)
  {
    DataList[nextIndex].Hour = hours;
    DataList[nextIndex].Minute = minutes;
    DataList[nextIndex].Data = int(data*10);
    nextIndex++;
  }
}

void List::setDisplaySize(int lines)
{
  displayLines = lines;
}

int List::displayHours(int line)
{
  return DataList[line+iDisplay1].Hour;
}

int List::displayMinutes(int line)
{
  return DataList[line+iDisplay1].Minute;
  
}

float List::displayValue(int line)
{
  return DataList[line+iDisplay1].Data/10.0;
  
}

