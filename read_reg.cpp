#include <iostream>
#include <CAENVMElib.h>
#include <CAENVMEtypes.h>
#include <sstream>
#include <string>

int main(int argc, char** argv){

  if (argc != 5 && argc != 6) {
    std::cout<<"Usage: " << argv[0] << " link crate <r|w> register [value]\n";
    return 1;
  }
  int ret;
  CVBoardTypes BType = cvV2718;
  int handle = -1;
  int link = atoi(argv[1]);
  int crate = atoi(argv[2]);
  char op = argv[3][0];
  ret = CAENVME_Init(BType, link, crate, &handle);
  if(ret != cvSuccess){
    std::cout<<"Failed to initialize digitizer: "<<ret<<std::endl;
    exit(0);
  }
  //else
  //  std::cout<<"Initialized digitizer"<<std::endl;

  int val = 0;
  if (argc == 6) {
    std::stringstream s(argv[5]);
    s >> std::hex >> val;
  }
  int reg = 0xF080;
  std::stringstream s(argv[4]);
  s >> std::hex >> reg;
  if (op == 'r') {
    ret = CAENVME_ReadCycle(handle,reg,&val,cvA32_U_DATA,cvD32);
    if(ret != cvSuccess){
      std::cout<<"Failed to read 0x"<<std::hex<<reg<<std::dec<<": "<<ret<<std::endl;
    }
    std::cout<<"Read 0x" <<std::hex<<reg<<" as 0x"<<val<<std::dec<<std::endl;
  } else if (op == 'w') {
    ret = CAENVME_WriteCycle(handle, reg, &val, cvA32_U_DATA, cvD32);
    if(ret != cvSuccess){
      std::cout<<"Failed to write to 0x"<<std::hex<<reg<<std::dec<<": "<<ret<<std::endl;
    }
    std::cout<<"Wrote to 0x" <<std::hex<<reg<<std::dec<<std::endl;
  }

  CAENVME_End(handle);
  return 0;
}
