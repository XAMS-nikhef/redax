import pymongo
from pymongo import MongoClient
from bson.objectid import ObjectId
import os


client = MongoClient("mongodb://user:%s@127.0.0.1:27017/admin"%os.environ["MONGO_PASSWORD"])
db = client['daq']
collection = db['options']

run_mode = {
	# Organitation Fields
	# "_id": ObjectId("5d30e9d05e13ab6116c43bf9"),
    "name": "software_update_V1730_part2",
    "user": "dipede",
    "description": "redax update for V1730",
    "detector" : "xams",
    # "mongo_uri": "mongodb://user:%s@127.0.0.1:27017/admin"%os.environ["MONGO_PASSWORD"],
    # "mongo_database": "daq",
    # "mongo_collection": "test_NaI",
    "run_start":0,
    "strax_chunk_overlap": 0.5,# 500000000,
    "strax_header_size": 31,
    "strax_output_path": "/home/xams/daq/redax/data",
    "strax_chunk_length": 5, #5000000000,
    "strax_fragment_length": 220,
    "baseline_dac_mode": "fixed",
    "baseline_value": 16000,
	"baseline_fixed_value":4000,
    "firmware_version": 0,
	"boards" : [{"crate": 1,
                "link": 0,
               "board": 770,
               "vme_address": "0",
               "type": "V1724",
               "host": "xams-daq_reader_0"}],
    "registers": [{'reg': 'EF24', 'val': '1','board': 770,'comment': 'Software reset'},
                  {'reg': 'EF1C','val': 'FF','board': 770, 'comment': 'Max number of events per blt'},
                  {'reg': 'EF00', 'val': '10', 'board': 770, 'comment': 'Readout control'},
                  {'reg': '8120', 'val': 'FF', 'board': 770, 'comment': 'Channel mask'},
                  {'reg': '8000','val': '310','board': 770,'comment': 'Board configuration'},
                  {'reg': '8038','val': '19','board': 770,'comment': 'Word in pretrigger window'},
                  {'reg': '8020','val': '32','board': 770,'comment': 'Event size register. Required for new FW. Words.'},
                  {'reg': '8078','val': '19','board': 770,'comment': 'Samples under threshold to close event.'},
                  {'reg': '8060', 'val': '12', 'board': 770, 'comment': 'Trigger threshold. '},
                  {'reg': '8080','val': '510000','board': 770,'comment': 'DPP register. 310000 - DPP on, 256 sample baseline.'}
	],
    'channels':{'770': [0, 1, 2, 3, 4, 5, 6, 7]},
    'thresholds': {'770': [100, 100, 100, 100, 100, 100, 100, 100]},

}

if collection.find_one({"name": run_mode['name']}) is not None:
    print("Please provide a unique name!")

try:
    collection.insert_one(run_mode)
except Exception as e:
    print("Insert failed. Maybe your JSON is bad. Error follows:")
    print(e)
