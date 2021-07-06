#ifndef _OPTIONS_HH_
#define _OPTIONS_HH_

#include <string>
#include <vector>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <map>
#include <mongocxx/pool.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/document/value.hpp>

struct BoardType{
  int link;
  int crate;
  int board;
  std::string type;
  std::string host;
  unsigned int vme_address;
};

struct RegisterType{
  std::string reg;
  std::string val;
};

struct CrateOptions{
  float pulser_freq;
  int neutron_veto;
  int muon_veto;
  int led_trigger;
  int s_in;
};

struct HEVOptions{
    int signal_threshold;
    int sign;
    int rise_time_cut;
    int static_veto_duration;
    int dynamic_veto_limit;
    int integration_threshold;
    int rho_0;
    int rho_1;
    int rho_2;
    int rho_3;
    int window;
    int prescaling;
    int component_status;
    int width_cut;
    int delay;
    std::string address;
    std::string required;

};

struct fax_options_t {
  int tpc_size;
  double rate;
  double drift_speed;
  double e_absorbtion_length;
};

class MongoLog;

class Options{

public:
  Options(std::shared_ptr<MongoLog>&, std::string, std::string, mongocxx::collection*, std::shared_ptr<mongocxx::pool>&, std::string, std::string);
  ~Options();

  int GetInt(std::string, int=-1);
  long int GetLongInt(std::string, long int=-1);
  double GetDouble(std::string, double=-1);
  std::string GetString(std::string, std::string="");
  std::string GetNestedString(std::string, std::string="");
  std::string Hostname() {return fHostname;}
  std::string Detector() {return fDetector;}

  std::vector<BoardType> GetBoards(std::string);
  std::vector<RegisterType> GetRegisters(int, bool=false);
  std::vector<uint16_t> GetDAC(int, int, uint16_t);
  std::vector<int> GetBLTalloc();
  int GetV1495Opts(std::map<std::string, int>&);
  int GetCrateOpt(CrateOptions &ret);
  int GetHEVOpt(HEVOptions &ret);
  int16_t GetChannel(int, int);
  int GetNestedInt(std::string, int=-1);
  std::vector<uint16_t> GetThresholds(int);
  int GetFaxOptions(fax_options_t&);
  uint16_t GetSingleDAC(int, int, uint16_t);

  void UpdateDAC(std::map<int, std::vector<uint16_t>>&);

private:
  int Load(std::string, mongocxx::collection*, std::string);
  bsoncxx::document::view bson_options;
  bsoncxx::document::value *bson_value;
  std::shared_ptr<MongoLog> fLog;
  std::string fHostname;
  std::string fDetector;
  std::shared_ptr<mongocxx::pool> fPool;
  mongocxx::pool::entry fClient; // yes
  mongocxx::database fDB;
  mongocxx::collection fDAC_collection;
  bsoncxx::document::value fDAC_cache;
};

#endif
