#include "DAQController.hh"
#include <functional>

// Status:
// 0-idle
// 1-arming
// 2-armed
// 3-running
// 4-error

DAQController::DAQController(MongoLog *log, std::string hostname){
  fLog=log;
  fOptions = NULL;
  fStatus = DAXHelpers::Idle;
  fReadLoop = false;
  fNProcessingThreads=8;
  fBufferLength = 0;
  fRawDataBuffer = NULL;
  fDatasize=0.;
  fHostname = hostname;
}

DAQController::~DAQController(){
  if(fProcessingThreads.size()!=0)
    CloseProcessingThreads();
}

std::string DAQController::run_mode(){
  if(fOptions == NULL)
    return "None";
  try{
    return fOptions->GetString("name");
  }
  catch(const std::exception &e){
    return "None";
  }
}

int DAQController::InitializeElectronics(Options *options, std::vector<int>&keys,
		std::map<int, std::map<std::string, std::vector<double>>>&dac_values){

  End();
  
  fOptions = options;
  fNProcessingThreads = fOptions->GetNestedInt("processing_threads."+fHostname, 8);  
  fLog->Entry(MongoLog::Local, "Beginning electronics initialization with %i threads",
	      fNProcessingThreads);

  // Initialize digitizers
  fStatus = DAXHelpers::Arming;
  for(auto d : fOptions->GetBoards("V17XX", fHostname)){
    fLog->Entry(MongoLog::Local, "Arming new digitizer %i", d.board);

    V1724 *digi;
    if(d.type == "V1724_MV")
      digi = new V1724_MV(fLog, fOptions);
    // else if(d.type == "V1730")
    // digi = new V1730(fLog, fOptions);
    else
      digi = new V1724(fLog, fOptions);

    
    if(digi->Init(d.link, d.crate, d.board, d.vme_address)==0){
	fDigitizers[d.link].push_back(digi);
	fDataPerDigi[digi->bid()] = 0;

	if(std::find(keys.begin(), keys.end(), d.link) == keys.end()){
	  fLog->Entry(MongoLog::Local, "Defining a new optical link at %i", d.link);
	  keys.push_back(d.link);
	}    
	fLog->Entry(MongoLog::Debug, "Initialized digitizer %i", d.board);
	
	int write_success = 0;
	write_success += digi->WriteRegister(0xEF24, 0x1);
	write_success += digi->WriteRegister(0xEF00, 0x30);
	if(write_success!=0){
	  fLog->Entry(MongoLog::Error,
		      "Digitizer %i unable to load pre-registers",
		      digi->bid());
	  fStatus = DAXHelpers::Idle;
	  return -1;
	}
    }
    else{
      fLog->Entry(MongoLog::Warning, "Failed to initialize digitizer %i", d.board);
      fStatus = DAXHelpers::Idle;
      return -1;
    }
  }

  fLog->Entry(MongoLog::Local, "Sleeping for two seconds");
  // For the sake of sanity and sleeping through the night,
  // do not remove this statement.
  sleep(2); // <-- this one. Leave it here.
  // Seriously. This sleep statement is absolutely vital.
  fLog->Entry(MongoLog::Local, "That felt great, thanks.");

  unsigned i = 0;
  vector<thread*> init_threads;
  init_threads.reserve(fDigitizers.size());
  vector<int> rets;
  rets.reserve(fDigitizers.size());
  // Parallel digitizer programming to speed baselining
  for( auto& link : fDigitizers ) {
    rets.push_back(1);
    init_threads.push_back(new thread(&DAQController::InitLink, this,
	  std::ref(link.second), std::ref(dac_values), std::ref(rets[i])));
    i++;

  }
  for (i = 0; i < init_threads.size(); i++) {
    init_threads[i]->join();
    delete init_threads[i];
  }
  if (std::any_of(rets.begin(), rets.end(), [](int i) {return i != 0;})) {
    fLog->Entry(MongoLog::Warning, "Encountered errors during digitizer programming");
    fStatus = DAXHelpers::Idle;
    return -1;
  } else
    fLog->Entry(MongoLog::Debug, "Digitizer programming successful");
  fOptions->UpdateDAC(dac_values);

  for(auto const& link : fDigitizers ) {
    for(auto digi : link.second){
      if(fOptions->GetInt("run_start", 0) == 1)
	digi->SINStart();
      else
	digi->AcquisitionStop();
    }
  }
  sleep(1);
  fStatus = DAXHelpers::Armed;

  fLog->Entry(MongoLog::Local, "Arm command finished, returning to main loop");


  return 0;
}

int DAQController::Start(){
  if(fOptions->GetInt("run_start", 0) == 0){
    for( auto const& link : fDigitizers ){      
      for(auto digi : link.second){

	// Ensure digitizer is ready to start
	if(digi->EnsureReady(1000, 1000)!= true){
	  fLog->Entry(MongoLog::Warning, "Digitizer not ready to start after sw command sent");
	  return -1;
	}

	// Send start command
	digi->SoftwareStart();

	// Ensure digitizer is started
	if(digi->EnsureStarted(1000, 1000)!=true){
	  fLog->Entry(MongoLog::Warning,
		      "Timed out waiting for acquisition to start after SW start sent");
	  return -1;
	}
      }
    }
  }
  fStatus = DAXHelpers::Running;
  return 0;
}

int DAQController::Stop(){

  std::cout<<"Deactivating boards"<<std::endl;
  for( auto const& link : fDigitizers ){
    for(auto digi : link.second){
      digi->AcquisitionStop();

      // Ensure digitizer is stopped
      if(digi->EnsureStopped(1000, 1000) != true){
	//if(digi->MonitorRegister(0x8104, 0x4, 1000, 1000, 0x0) != true){
	fLog->Entry(MongoLog::Warning,
		    "Timed out waiting for acquisition to stop after SW stop sent");
          return -1;
      }
    }
  }
  fLog->Entry(MongoLog::Debug, "Stopped digitizers");

  fReadLoop = false; // at some point.
  fStatus = DAXHelpers::Idle;
  return 0;
}
void DAQController::End(){
  Stop();
  std::cout<<"Closing Processing Threads"<<std::endl;
  CloseProcessingThreads();
  std::cout<<"Closing Digitizers"<<std::endl;
  for( auto const& link : fDigitizers ){    
    for(auto digi : link.second){
      digi->End();
      delete digi;
    }
  } 
  fDigitizers.clear();
  fStatus = DAXHelpers::Idle;

  if(fRawDataBuffer != NULL){
    fLog->Entry(MongoLog::Warning, "Deleting uncleard buffer of size %i",
		fRawDataBuffer->size());	       
    for(unsigned int i=0; i<fRawDataBuffer->size(); i++){
      delete[] (*fRawDataBuffer)[i].buff;
    }
    delete fRawDataBuffer;
    fRawDataBuffer = NULL;
  }

  std::cout<<"Finished end"<<std::endl;
}

void* DAQController::ReadThreadWrapper(void* data, int link){
  DAQController *dc = static_cast<DAQController*>(data);
  dc->ReadData(link);
  return dc;
}  

void DAQController::ReadData(int link){
  fReadLoop = true;
  
  // Raw data buffer should be NULL. If not then maybe it was not cleared since last time
  if(fRawDataBuffer != NULL){
    fLog->Entry(MongoLog::Debug, "Raw data buffer being brute force cleared.");	       
    for(unsigned int x=0;x<fRawDataBuffer->size(); x++){
      delete[] (*fRawDataBuffer)[x].buff;
    }
    delete fRawDataBuffer;
    fBufferLength=0;
    fRawDataBuffer = NULL;
  }
  
  u_int32_t lastRead = 0; // bytes read in last cycle. make sure we clear digitizers at run stop
  long int readcycler = 0;
  while(fReadLoop){
    
    vector<data_packet> local_buffer;
    for(unsigned int x=0; x<fDigitizers[link].size(); x++){

      // Every 1k reads check board status
      if(readcycler%10000==0){
	readcycler=0;
	u_int32_t data = fDigitizers[link][x]->GetAcquisitionStatus();
	std::cout<<"Board "<<fDigitizers[link][x]->bid()<<" has status "<<hex<<data<<dec<<std::endl;
      }
      data_packet d;
      d.buff=NULL;
      d.size=0;
      d.bid = fDigitizers[link][x]->bid();
      d.size = fDigitizers[link][x]->ReadMBLT(d.buff);

      lastRead += d.size;
      
      if(d.size<0){
	//LOG ERROR
	if(d.buff!=NULL)
	  delete[] d.buff;
	break;
      }
      if(d.size>0){
	d.header_time = fDigitizers[link][x]->GetHeaderTime(d.buff, d.size);
	d.clock_counter = fDigitizers[link][x]->GetClockCounter(d.header_time);
	fDatasize += d.size;
	fDataPerDigi[d.bid] += d.size;
	local_buffer.push_back(d);
      }
    }
    if(local_buffer.size()!=0)
      AppendData(local_buffer);
    local_buffer.clear();
    readcycler++;
    usleep(1);
  }

}


std::map<int, u_int64_t> DAQController::GetDataPerDigi(){
  // Return a map of data transferred per digitizer since last update
  // and clear the private map
  std::map <int, u_int64_t>retmap;
  for(auto const &kPair : fDataPerDigi){
    retmap[kPair.first] = (u_int64_t)(fDataPerDigi[kPair.first]);
    fDataPerDigi[kPair.first] = 0;
  }
  return retmap;
}

std::map<std::string, int> DAQController::GetDataFormat(){
  for( auto const& link : fDigitizers )    
    for(auto digi : link.second)
      return digi->DataFormatDefinition;
  return std::map<std::string, int>();
}

void DAQController::AppendData(vector<data_packet> &d){
  // Blocks!
  fBufferMutex.lock();
  if(fRawDataBuffer==NULL)
    fRawDataBuffer = new std::vector<data_packet>();
  fRawDataBuffer->insert( fRawDataBuffer->end(), d.begin(), d.end() );
  u_int64_t bl = 0;
  for(unsigned int x=0; x<fRawDataBuffer->size(); x++){
    bl += (*fRawDataBuffer)[x].size;
  }
  fBufferLength = bl; 
  fBufferMutex.unlock();  
}

int DAQController::GetData(std::vector <data_packet> *&retVec){
  // Check once, is it worth locking mutex?
  retVec=NULL;
  if(fBufferLength==0)
    return 0;
  if(!fBufferMutex.try_lock())
    return 0;

  int ret = 0;
  // Check again, is there still data?
  if(fRawDataBuffer != NULL && fRawDataBuffer->size()>0){

    // Pass ownership to calling function
    retVec = fRawDataBuffer;
    fRawDataBuffer = NULL;

    ret = retVec->size();
    fBufferLength = 0;
  }
  fBufferMutex.unlock();
  return ret;
}
  

void* DAQController::ProcessingThreadWrapper(void* data){
  StraxInserter *mi = static_cast<StraxInserter*>(data);
  mi->ReadAndInsertData();
  return data;
}

bool DAQController::CheckErrors(){

  // This checks for errors from the threads by checking the
  // error flag in each object. It's appropriate to poll this
  // on the order of ~second(s) and initialize a STOP in case
  // the function returns "true"

  for(unsigned int i=0; i<fProcessingThreads.size(); i++){
    if(fProcessingThreads[i].inserter->CheckError()){
      fLog->Entry(MongoLog::Error, "Error found in processing thread.");
      fStatus=DAXHelpers::Error;
      return true;
    }
  }
  return false;
}

void DAQController::OpenProcessingThreads(){

  for(int i=0; i<fNProcessingThreads; i++){
    processingThread p;
    //p.inserter = new MongoInserter();
    p.inserter = new StraxInserter();
    p.inserter->Initialize(fOptions, fLog, this, fHostname);
    p.pthread = new std::thread(ProcessingThreadWrapper,
			       static_cast<void*>(p.inserter));
    fProcessingThreads.push_back(p);
  }

}

void DAQController::CloseProcessingThreads(){

  for(unsigned int i=0; i<fProcessingThreads.size(); i++){
    fProcessingThreads[i].inserter->Close();
    fProcessingThreads[i].pthread->join();
        
    delete fProcessingThreads[i].pthread;
    delete fProcessingThreads[i].inserter;
  }
  fProcessingThreads.clear();
}

void DAQController::InitLink(std::vector<V1724*>& digis,
    std::map<int, std::map<std::string, vector<double>>>& dacs, int& ret) {
  for(auto digi : digis){
    fLog->Entry(MongoLog::Local, "Beginning specific init for board %i", digi->bid());

    // Load DAC. n.b.: if you set the DAC value in your
    // ini file you'll overwrite the fancy stuff done here!
    vector<u_int16_t>dac_values(16, 0x0);

    // Multiple options here
    std::string BL_MODE = fOptions->GetString("baseline_dac_mode", "fixed");
    int success = 0, tries=0, max_iter=100, max_tries(5);
    bool calibrate=false;
    int nominal_baseline = fOptions->GetInt("baseline_value", 16000);
    std::map<std::string, std::vector<double>> board_dac_cal;
    board_dac_cal = dacs.count(digi->bid()) ? dacs[digi->bid()] : dacs[-1];
    if((BL_MODE == "fit") || (BL_MODE == "cached")){
      if (BL_MODE == "fit") {
        fLog->Entry(MongoLog::Local, "You're fitting baselines for digi %i",
            digi->bid());
        max_iter = 50;
        max_tries = 5;
        calibrate=true;
      } else {
        fLog->Entry(MongoLog::Local, "You're using cached baselines for digi %i",
            digi->bid());
        max_iter=1;
        max_tries = 1;
        calibrate=false;
      }
      // Try a few times since sometimes will not converge. If the function
      // returns -2 it means it crashed hard so don't bother trying again.
      do{
	fLog->Entry(MongoLog::Local, "Going into DAC routine. Try: %i", tries+1);
	success = digi->ConfigureBaselines(dac_values, board_dac_cal,
              nominal_baseline, max_iter, calibrate);
	tries++;
        calibrate = false; // only need to calibrate the first time
      } while(tries<max_tries && success==-1);
    }
    else if(BL_MODE != "fixed"){
      fLog->Entry(MongoLog::Warning, "Received unknown baseline mode. Fallback to fixed");
      BL_MODE = "fixed";
    }
    if(BL_MODE == "fixed"){
      int BLVal = fOptions->GetInt("baseline_fixed_value", 4000);
      fLog->Entry(MongoLog::Local, "Loading fixed baselines at value 0x%04x for digi %i",
		    BLVal, digi->bid());
      dac_values.assign(dac_values.size(), BLVal);
    }

    //int success = 0;
    std::cout<<"Baselines finished for digi "<<digi->bid()<<std::endl;
    if(success==-2){
      fLog->Entry(MongoLog::Warning, "Baselines failed with digi error");
      fStatus = DAXHelpers::Error;
      ret = -1;
      return;
    }
    else if(success!=0){
      fLog->Entry(MongoLog::Warning, "Baselines failed with timeout");
      fStatus = DAXHelpers::Idle;
      ret = -1;
      return;
    }

    fLog->Entry(MongoLog::Local, "Digi %i survived baseline mode. Going into register setting",
		digi->bid());

    for(auto regi : fOptions->GetRegisters(digi->bid())){
      unsigned int reg = DAXHelpers::StringToHex(regi.reg);
      unsigned int val = DAXHelpers::StringToHex(regi.val);
      success+=digi->WriteRegister(reg, val);
    }
    fLog->Entry(MongoLog::Local, "User registers finished for digi %i. Loading DAC.",
                  digi->bid());

    // Load the baselines you just configured
    vector<bool> update_dac(16, true);
    success += digi->LoadDAC(dac_values, update_dac);
    dacs[digi->bid()] = board_dac_cal;

    fLog->Entry(MongoLog::Local,
	"Setup finished for %i. Assuming not directly followed by an error, that's a wrap.",
        digi->bid());
    if(success!=0){
      //LOG
      fStatus = DAXHelpers::Idle;
      fLog->Entry(MongoLog::Warning, "Failed to configure digitizers.");
      ret = -1;
      return;
    }
  } // loop over digis per link
  ret = 0;
  return;
}
