import numpy as np

from pymongo import MongoClient
import datetime
import argparse

def read_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('--active', choices='true false'.split(), help='Run mode', required=True)
    parser.add_argument('--mode', help='Document Options name', required=True)
    parser.add_argument('--stop', help='Minutes until the run automatically restarts. Default=60', required=False, type=int, default=60)
    parser.add_argument('--user', help='User', required=False,default="sdipede")
    parser.add_argument('--remote', help='Remote', required=False, default="false")
#    parser.add_argument('--number', help='Run number', type=int, default=1)
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
    collection = db['detector_control']

    idoc = { 
              "time" : "datetime.now()",
              "detector" : "xams", 
              "active" : args.active,
              "stop_after" : args.stop, #How many minutes until the run automatically restarts
              "mode" : args.mode,
#          "number": args.number,
#          "run_identifier": "{:06d}".format(args.number),
              "user" : args.user,
              "comment" : " ",
              "remote": args.remote
      } 
    collection.insert_one(idoc)

if __name__ == '__main__':
    main()
