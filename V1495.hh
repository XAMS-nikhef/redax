#ifndef _V1495_HH_
#define _V1495_HH_

#include <CAENVMElib.h>
#include "MongoLog.hh"
#include "Options.hh"
#include "V1724.hh"

class MongoLog;
class Options;

class V1495{

public:
      V1495(std::shared_ptr<MongoLog>&, std::shared_ptr<Options>&, int, int, unsigned);
      virtual ~V1495();
      int WriteReg(unsigned int reg, unsigned int value);

private:
      int fBoardHandle, fBID;
      unsigned int fBaseAddress;
      std::shared_ptr<Options> fOptions;
      std::shared_ptr<MongoLog> fLog;

};
#endif
