import os
from pymongo import MongoClient
import argparse
import datetime


def main(coll):
    parser = argparse.ArgumentParser()
    parser.add_argument('--command', choices='arm stop start'.split(), required=True, help='The command')
    parser.add_argument('--number', type=int, default=1, help='Run number')
    parser.add_argument('--mode', help='Run mode', required=True)
    parser.add_argument('--host', nargs='+', default=[os.uname()[1]], help="Hosts to issue to")
    parser.add_argument('--active', choices='true false'.split(), help='Run mode', required=True)


    args = parser.parse_args()
    if not isinstance(args.host, (list, tuple)):
        args.host = [args.host]

    doc = {
            "command": args.command,
            "number": args.number,
            "mode": args.mode,
            "host": args.host,
            "user": os.getlogin(),
            "run_identifier": '%06i' % args.number,
            "createdAt": datetime.datetime.utcnow(),
            "acknowledged": {h:0 for h in args.host}
            }
    coll.insert_one(doc)
    return

if __name__ == '__main__':

    with MongoClient("mongodb://user:%s@127.0.0.1:27017/admin" % os.environ['MONGO_PASSWORD']) as client:
        try:
            main(client['daq']['control'])
        except Exception as e:
            print('%s: %s' % (type(e), e))
