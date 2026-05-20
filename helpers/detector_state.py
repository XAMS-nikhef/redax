import numpy as np

from pymongo import MongoClient
import datetime
import argparse

def read_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('--active', choices='true false'.split(), help='Run mode', required=True)
    parser.add_argument('--mode', help='Document Options name', required=True)
    parser.add_argument('--comment', help='Comment', type=str, default='Do you see this comment? Awesome, good job!', required=False)
    parser.add_argument('--stop_after', help='Minutes until the run automatically restarts. Default=60', required=False, type=int, default=60)
    parser.add_argument('--softstop', help='Stop the run after this value. Default=30', required=False, type=int, default=30)
    parser.add_argument('--user', help='User', required=False,default="dipede")
    args = parser.parse_args()

    if (args.active != 'true') and (args.active != 'false'):
        print('read_arguments:: ERROR Wrong mode selected. mode=',args.active)
        exit(-1)

    return args

def main():
    
    #
        # read the command line arguments
    #

    args = read_arguments()

    client = MongoClient("mongodb://user:password@127.0.0.1:27017/admin")
    db = client['daq']
    collection = db['detector_control_new']

#     idoc = { 
#               "time" : "datetime.now()",
#               "detector" : "xams", 
#               "active" : args.active,
#               "stop_after" : args.stop, #How many minutes until the run automatically restarts
#               "mode" : args.mode,
# #          "number": args.number,
# #          "run_identifier": "{:06d}".format(args.number),
#               "user" : args.user,
#               "comment" : " ",
#               "remote": args.remote
#       } 

    idoc_active = { 
        "detector" : 'xams',
        "key": 'xams.active',
        "field": 'active',
        "value": args.active,
        "user": args.user,
        "time": datetime.datetime.now(),   
    } 

    idoc_mode = {
    "detector" : 'xams',
    "key": 'xams.mode',
    "field": 'mode',
    "value": args.mode,
    "user": args.user,
    "time":  datetime.datetime.now(),   
    }

    idoc_comment = {
    "detector" : 'xams',
    "key": 'xams.comment',
    "field": 'comment',
    "value": args.comment,
    "user": args.user,
    "time":  datetime.datetime.now(),   
    }

    idoc_softstop = {
    "detector" : 'xams',
    "key": 'xams.softstop',
    "field": 'softstop',
    "value": args.softstop,
    "user": args.user,
    "time":  datetime.datetime.now(),   
    }

    idoc_stop_after_doc = {
    "detector" : 'xams',
    "key": 'xams.stop_after',
    "field": 'stop_after',
    "value": args.stop_after,
    "user": args.user,
    "time":  datetime.datetime.now(),   
    }

    collection.insert_one(idoc_active)
    collection.insert_one(idoc_mode)
    collection.insert_one(idoc_comment)
    collection.insert_one(idoc_softstop)
    collection.insert_one(idoc_stop_after_doc)



if __name__ == '__main__':
    main()