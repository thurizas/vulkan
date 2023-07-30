#ifndef _logger_h_
#define _logger_h_


#include <stdint.h>
#include <cstdint>
#include <map>
#include <functional>

// predefined output identifiers
static const uint32_t cmdLine = 0;
static const uint32_t fileLine = 1;

typedef void (*fnct)(char*);          //std::function<void(char*)>    fnct;
typedef std::map<int, fnct>           mapType;

class CLogger
{
public:
  enum level: std::int8_t{INFO = 1, DEBUG=2, WARNING=3, ERR=4, FATAL=5, NOTICE=6};
  enum outLoc:  std::int8_t { CONSOLE = 0, FILE = 1};

  void setLevel(int l){m_level = l;}

  static CLogger* getInstance();
  static void     delInstance();


  void     outMsg(int, int, const char*, ...);
  void     regOutDevice(int, fnct);

private:
    CLogger();
    ~CLogger();

    int             m_level;
    static CLogger* m_pThis;
    mapType         m_mapCallbacks;  
};

#endif



