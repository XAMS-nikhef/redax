#ifndef _V1724_HH_
#define _V1724_HH_

#include <unistd.h>
#include <cstring>

#include <stdint.h>
#include <CAENVMElib.h>
#include <vector>

#include <iostream>
#include "MongoLog.hh"
#include "Options.hh"

using namespace std;

class V1724{

 public:
  V1724(MongoLog *log, Options *options);
  virtual ~V1724();

  int Init(int link, int crate, int bid, unsigned int address=0);
  int64_t ReadMBLT(u_int32_t *&buffer);
  int WriteRegister(unsigned int reg, unsigned int value);
  unsigned int ReadRegister(unsigned int reg);
  int ConfigureBaselines(std::vector<u_int16_t> &dac_values,
          std::map<std::string, std::vector<double>>& cal_values,
			 int nominal_value=16000,
			 int ntries=100,
             bool calibrate=true);
  int GetClockCounter(u_int32_t timestamp);
  int End();

  int bid(){
    return fBID;
  };

  int LoadDAC(vector<u_int16_t>dac_values, vector<bool> &update_dac);

  // Acquisition Control
  int SINStart();
  int SoftwareStart();
  int AcquisitionStop();
  bool EnsureReady(int ntries, int sleep);
  bool EnsureStarted(int ntries, int sleep);
  bool EnsureStopped(int ntries, int sleep);
  u_int32_t GetAcquisitionStatus();
  u_int32_t GetHeaderTime(u_int32_t *buff, u_int32_t size);
  
  int fNsPerSample;
  std::map<std::string, int> DataFormatDefinition;

protected:
  // Some values for base classes to override 
  unsigned int fAqCtrlRegister;
  unsigned int fAqStatusRegister;
  unsigned int fSwTrigRegister;
  unsigned int fResetRegister;
  unsigned int fChStatusRegister;
  unsigned int fChDACRegister;
  unsigned int fNChannels;

private:

  bool MonitorRegister(u_int32_t reg, u_int32_t mask, int ntries,
		       int sleep, u_int32_t val=1);
  Options *fOptions;
  int fBoardHandle;
  int fLink, fCrate, fBID;  
  unsigned int fBaseAddress;

  // Stuff for clock reset tracking
  u_int32_t clock_counter;
  u_int32_t last_time;
  bool seen_under_5;
  bool seen_over_15;

  MongoLog *fLog;

};


#endif
