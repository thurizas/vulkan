#include "logger.h"

#include<cstdarg>
#include<cstring>
#include<map>

CLogger* CLogger::m_pThis = nullptr;


/************************************************************************************************************************
 * function : 
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   :  (GKHuber)
************************************************************************************************************************/
CLogger*  CLogger::getInstance()
{
  if (nullptr == m_pThis)
    m_pThis = new CLogger;

  return m_pThis;
}



/************************************************************************************************************************
 * function :
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   :  (GKHuber)
************************************************************************************************************************/
void CLogger::delInstance()
{
  delete m_pThis;
}



/************************************************************************************************************************
 * function :
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   :  (GKHuber)
************************************************************************************************************************/
void CLogger::regOutDevice(int nWhich, fnct callback)
{
  std::pair<mapType::iterator, bool>   retPair;
  retPair = m_mapCallbacks.insert(std::pair<int, fnct>(nWhich, callback));
  if (!retPair.second)
  {
    outMsg(0, CLogger::level::ERR, "failed to insert outdevice #%d", nWhich);
  }
}



/************************************************************************************************************************
 * function :
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   :  (GKHuber)
************************************************************************************************************************/
void CLogger::outMsg(int nWhich, int level, const char* fmt, ...)
{
  mapType::iterator  iter = m_mapCallbacks.find(nWhich);

  if ((iter != m_mapCallbacks.end()) && ((*iter).second)) // did we find the entry, and is it a valid callback.
  {
    if(m_level <= level)      // only do work if level of message is greater than or equal to report level
    {
      va_list     args, args_copy;
      va_start(args, fmt);

      // make a copy of the argument list so we can reuse it...
      va_copy(args_copy, args);

      // do a trial vsnprintf to get the needed length
      int len = vsnprintf(nullptr, 0, fmt, args_copy);
      va_end(args_copy);

      // now that we know how big of a buffer we need, allocate it
      char* line = new char[len + 1];
      memset((void*)line, '\0', len + 1);

      vsnprintf(line, len + 1, fmt, args);

      // shoot message out to device...
      (*iter).second(line);

      va_end(args);
      delete[] line;
    }
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// private functions
/************************************************************************************************************************
 * function :
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   :  (GKHuber)
************************************************************************************************************************/
CLogger::CLogger()
{

}


/************************************************************************************************************************
 * function :
 *
 * abstract  :
 *
 * parameters:
 *
 * returns   :
 *
 * written   :  (GKHuber)
************************************************************************************************************************/
CLogger::~CLogger()
{

}
