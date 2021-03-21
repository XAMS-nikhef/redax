import configparser
import argparse
import logging
import logging.handlers
import threading
import signal
import datetime

from MongoConnect import MongoConnect
from DAQController import DAQController, STATUS


class SignalHandler(object):
    def __init__(self):
        self.event = threading.Event()
        signal.signal(signal.SIGINT, self.interrupt)
        signal.signal(signal.SIGTERM, self.interrupt)

    def interrupt(self, *args):
        self.event.set()


def main():

    # Parse command line
    parser = argparse.ArgumentParser(description='Manage the DAQ')
    parser.add_argument('--config', type=str, help='Path to your configuration file',
            default='config.ini')
    parser.add_argument('--log', type=str, help='Logging level', default='DEBUG',
            choices=['DEBUG','INFO','WARNING','ERROR','CRITICAL'])
    args = parser.parse_args()
    config = configparser.ConfigParser()
    config.read(args.config)
    
    # Logger
    logger = logging.getLogger('main')
    # Stream Logger
    h = logging.StreamHandler()    
    f = logging.Formatter(fmt='%(asctime)s | %(levelname)s | %(message)s')
    h.setFormatter(f)
    logger.addHandler(h)
    # File Handler Logger
    h = logging.handlers.TimedRotatingFileHandler('Log_files/log_', when='midnight', utc=True, backupCount=7) 
    h.setFormatter(f) 
    logger.addHandler(h)
    # Logger levels
    logger.setLevel(getattr(logging, args.log))
    
    # Declare database object
    MongoConnector = MongoConnect(config, logger)

    # Declare a 'brain' object. This will cache info about the DAQ state in order to
    # solve the want/have decisions. The deciding functions also go here. It needs a
    # mongo connector because it has to pull options files
    DAQControl = DAQController(config, MongoConnector, logger)
    sleep_period = int(config['DEFAULT']['PollFrequency'])
    sh = SignalHandler()

    while(sh.event.is_set() == False):
        
        sh.event.wait(sleep_period)

        # Get most recent check-in from all connected hosts
        if MongoConnector.GetUpdate():
            continue
        latest_status = MongoConnector.latest_status

        # Get most recent goal state from database. Users will update this from the website.
        goal_state = MongoConnector.GetWantedState()
        if goal_state is None:
            continue

        # Print an update
        for detector in latest_status.keys():
            state = 'ACTIVE' if goal_state[detector]['active'] == 'true' else 'INACTIVE'
            msg = (f'The {detector} should be {state} and is '
                  f'{latest_status[detector]["status"].name}')
            
            if latest_status[detector]['number'] != -1:
                msg += f'({latest_status[detector]["number"]})'
            logger.debug(msg)

        # Decision time. Are we actually in our goal state? If not what should we do?
        DAQControl.SolveProblem(latest_status, goal_state)

        # Time to report back
        MongoConnector.UpdateAggregateStatus()
    MongoConnector.Quit()
    return


if __name__ == '__main__':
    main()
