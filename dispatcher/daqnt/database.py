from pymongo import MongoClient
import os
from urllib.parse import quote_plus

def get_client(which='daq'):
    """
    Returns a MongoClient, connected to the requested server. If you haven't
    sourced the DAQ envs, you may run into problems.
    :param which: str, daq/runs, the deployment you want to connect to
    :returns: pymongo.MongoClient
    """
    pw = quote_plus(os.environ[f'MONGO_PASSWORD'])
    user = quote_plus(os.environ[f'MONGO_USER'])
    port = 27017
    return MongoClient(f'mongodb://{user}:{pw}@127.0.0.1:{port}/admin')