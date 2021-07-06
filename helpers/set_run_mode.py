import pymongo
from pymongo import MongoClient
from bson.objectid import ObjectId
import os

def main():
    
    client = MongoClient(f'mongodb://{os.environ["MONGO_USER"]}:{os.environ["MONGO_PASSWORD"]}@127.0.0.1:27017/admin')
    db = client['daq']
    collection = db['options']

    run_mode = {
    
    # Organitation Fields
        "name": "Run0_V1724_V1730", #the first digitizer is the lead clock
        "user": "angevaare",
#        "number": 10,
#        "run_identifier": "000002",
        "description": "First test writing on Stoomboot",
        "detector" : "xams",

    # Software configuration options
        "run_start":0,
        'compressor': 'lz4',
        'processing_threads': {'xams-daq_reader_0': 1},
        'firmware_version': 0, #or 1?
        'baseline_dac_mode': 'fixed',
        'baseline_value': 16000,
        'baseline_fixed_value': 4000, 
        'detectors': {'xams-daq_reader_0': 'xams'},

    # Low-level diagnostic options
        'baseline_max_iterations': 2,
        'baseline_max_steps': 30,
        'baseline_adjustment_threshold': 10,
        'baseline_convergence_threshold': 3,
        'baseline_min_adjustment': 10,
        'baseline_rebin_log2': 2,
        'baseline_bins_around_max': 4,
        'baseline_fraction_around_max': 0.5,
        'baseline_triggers_per_step': 3,
        'baseline_ms_between_triggers': 10,
        'blt_safety_factor': 1.5,
        'buffer_safety_factor': 1.1,
        'buffer_type': 'dual',
        'do_sn_check': 1,
    
    # Electronics definition
        'boards' : [{'crate': 1, 'link': 0,'board': 770, 'vme_address': '0','type': 'V1724','host': 'xams-daq_reader_0'},
			{'crate': 0, 'link': 0,'board': 77, 'vme_address': '0','type': 'V1730','host': 'xams-daq_reader_0'}],
        'registers': [{'reg': 'EF24', 'val': '1','board': 770,'comment': 'Software reset'},
                      {'reg': 'EF1C','val': 'FF','board': 770, 'comment': 'Max number of events per blt'},
                      {'reg': 'EF00', 'val': '10', 'board': 770, 'comment': 'Readout control'},
                      {'reg': '8120', 'val': 'FF', 'board': 770, 'comment': 'Channel mask'},
                      {'reg': '8000','val': '310','board': 770,'comment': 'Board configuration'},
                      {'reg': '8038','val': '19','board': 770,'comment': 'Word in pretrigger window'},
                      {'reg': '8020','val': '32','board': 770,'comment': 'Event size register. Required for new FW. Words.'},
                      {'reg': '8078','val': '19','board': 770,'comment': 'Samples under threshold to close event.'},
                      {'reg': '8060', 'val': '12', 'board': 770, 'comment': 'Trigger threshold. '},
                      {'reg': '8080','val': '510000','board': 770,'comment': 'DPP register. 310000 - DPP on, 256 sample baseline.'},

		{'reg': 'EF24', 'val': '1','board': 77,'comment': 'Software reset'},
                       {'reg': 'EF1C','val': 'FF','board': 77, 'comment': 'Max number of events per blt'},
                      {'reg': 'EF00', 'val': '10', 'board': 77, 'comment': 'Readout control'},
                       {'reg': '8120', 'val': 'FF', 'board': 77, 'comment': 'Channel mask'},
                       {'reg': '8000','val': '310','board': 77,'comment': 'Board configuration'},
                        {'reg': '8038','val': '19','board': 77,'comment': 'Word in pretrigger window'},
                         {'reg': '8020','val': '32','board': 77,'comment': 'Event size register. Required for new FW. Words.'},
                         {'reg': '8078','val': '19','board': 77,'comment': 'Samples under threshold to close event.'},
                         {'reg': '8060', 'val': '12', 'board': 77, 'comment': 'Trigger threshold. '},
                        {'reg': '8080','val': '510000','board': 77,'comment': 'DPP register. 310000 - DPP on, 256 sample baseline.'}],
		# {'reg': '8080','val': '10000','board': 77,'comment': 'DPP register. 310000 - DPP on, fixed baseline on the register 8064.'}
		# {'reg': '8064','val': '2000','board': 77,'comment': 'Fixed Baseline.'}
		# {'reg': '807C','val': 1F4'','board': 77,'comment': 'Maximum Tail Duration in Samples number.'}],

              
        'channels':{'770': [0, 1, 2, 3, 4, 5, 6, 7],
			'77':[8, 9, 10, 11, 12, 13, 14, 15]},
       'thresholds': {'770': [100, 100, 100, 100, 100, 100, 100, 100], 
			'77': [100, 100, 100, 100, 100, 100, 100, 100]},    

    
    # Strax output options
        "strax_chunk_overlap": 0.5,
        "strax_header_size": 31,
        "strax_output_path": "/data/xenon/xams/run11",
        "strax_chunk_length": 5,
        "strax_fragment_length": 220,
        'strax_buffer_num_chunks': 3,
        'strax_chunk_phase_limit': 2,
        'strax_fragment_payload_bytes': 220,    
    }

    if collection.find_one({"name": run_mode['name']}) is not None:
        print("Please provide a unique name!")

    try:
        collection.insert_one(run_mode)
    
    except Exception as e:
        print("Insert failed. Maybe your JSON is bad. Error follows:")
        print(e)
    
if __name__ == '__main__':
    main()

