# REDAX
D. Coderre, 2018, D. Masson, 2020. Please see license in LICENSE file.

See documentation here: [link](https://axfoundation.github.io/redax)
## Prerequisites

* mongodb cxx driver
* CAENVMElib v2.5+
* libblosc-dev
* liblz4-dev
* C++17-compatible compiler. Tested on gcc 7.3.0
* Driver for your CAEN PCI card
* A DAQ hardware setup (docs coming on xenon wiki)
* A MongoDB deployment you can write to
* tcl8.6-dev, and tcl-expect-dev (optional, for DDC10 support)

## Install

The instructions below assume **Ubuntu 18.04 LTS** and a fresh system. Adjust package names as needed for other distributions.

### 1. System packages

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    git \
    cmake \
    pkg-config \
    libssl-dev \
    libsasl2-dev \
    libblosc-dev \
    liblz4-dev \
    tcl8.6-dev \
    tcl-expect-dev   # only needed for DDC10 support
```

### 2. CAEN libraries

Download and install the following from [caen.it](http://www.caen.it):

- **CAENVMElib** v2.5 or later
- **Driver** for your CAEN PCI/PCIe optical link card (e.g. A2818, A3818)

Each package ships with an `install` script; follow the vendor instructions. After installation verify with:

```bash
ls /usr/lib/libCAENVME*
```

### 3. MongoDB C driver

The CXX driver (step 4) depends on the C driver.

```bash
# Download (pin a version for reproducibility)
wget https://github.com/mongodb/mongo-c-driver/releases/download/1.9.2/mongo-c-driver-1.9.2.tar.gz
tar -xvzf mongo-c-driver-1.9.2.tar.gz
cd mongo-c-driver-1.9.2

./configure --disable-automatic-init-and-cleanup
make -j$(nproc)
sudo make install
cd ..
```

### 4. MongoDB CXX driver

```bash
git clone https://github.com/mongodb/mongo-cxx-driver.git \
    --branch releases/stable --depth 1
cd mongo-cxx-driver
mkdir -p build && cd build

cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      ..

sudo make EP_mnmlstc_core   # install polyfill
make -j$(nproc)
sudo make install
cd ../..
```

After installation, confirm pkg-config can find the driver:

```bash
pkg-config --modversion libmongocxx
```

### 5. MongoDB server (if running locally)

Skip this step if you are connecting to an existing remote/cloud deployment.

Follow the [official MongoDB installation guide](https://www.mongodb.com/docs/manual/administration/install-on-linux/) for your OS. After installation:

```bash
sudo systemctl enable --now mongod
```

**Enable authentication** and create a DAQ user — see the [MongoDB security checklist](https://www.mongodb.com/docs/manual/administration/security-checklist/) and the [Database Setup](#database-setup) section below.

### 6. Python environment (dispatcher & monitor)

Python 3 is required. Using a virtual environment or [Anaconda](https://www.anaconda.com/) is recommended on shared systems.

```bash
pip install psutil pymongo
```

### 7. Clone and build redax

```bash
git clone https://github.com/AxFoundation/redax.git
cd redax
make -j$(nproc)
```

The resulting binary is `./redax`.

### 8. Initialize the DAQ database collections

A helper script creates the required capped/TTL collections:

```bash
python helpers/initialize_databases.py
```

Edit the script (or pass arguments) to point it at your MongoDB URI and database name before running.

## Starting the Reader Process
```
./redax --id <id> --uri <mongo_uri> [--reader | --cc ] [--db <database_name>] [--logdir <logging directory>] [--log-retention <days>] [--arm-delay <ms>] [--help]
```
The first argument is a unique ID that will identify your reader. This is important since your physical hardware setup needs to be associated with the software programs that will read the things out. So you probably want to map out ahead of time which reader processes will read out which optical links.

Three arguments are required. First is the ID of this instance, second is the full MongoDB URI, and third is one of 'reader' or 'cc', depending on if you want this instance to be a reader or a crate controller. Note: while this can run multiple times on a given host (one readout instance per link), it also works fine with all links in one instance (slightly better, even, because of overhead).

The ID number will create a unique process identifier for this instance of redax which appears as so: HOSTNAME_reader_ID. So if you run on host daq00 with ID 1 your process ID will be daq00_reader_1. This is the ID you use to address commands to this host.

Optionally, you can also specify the name of the database the program should look in for the various collections, where you want the log files written, how long to keep the log files around, and how long to wait between receiving an ARM command and actually beginning the arming sequence.

## Starting the Dispatcher (optional)

If you run with more than one readout process (this includes a crate controller) you should configure a dispatcher. The dispatcher handles communication with the user interface and translates human-level commands to the readout nodes. The provided example is a python script stored in the 'dispatcher' subdirectory, but is written for XENONnT, so YMMV.

At the moment the dispatcher is just a script, not an executable. So it can be run with:
`python dispatcher.py --config=options.ini`

The config option points to an ini file for the dispatcher. See the subdirectory for a readme detailing the options herein.

## Database Setup

You need to provide connectivity to a mongodb database using the URI.
This database should have the following collections.

**control:** is where commands go. Configure this as a capped or TTL collection, size approx 1MB (really not much needed). If you have no dispatcher you'll write directly to this collection, otherwise the dispatcher handles it.

**status:** should be configured as a capped or TTL collection. Each readout
node will write its status here every second or so.

**log:** The DAQ will log here.

**options:** is where settings docs go. When sending the 'arm' command
the name of the options file should be embedded in the command doc.
If the reader can't find an options doc with that name it won't be
able to arm the DAQ.

If you run with the dispatcher you additionally need a collection called **detector_control**, which will supercede control as the top-level user interface. The **control** collection will still be used by the dispatcher to control the readout nodes but the user should not use it in this case. If there is just one readout node you may want to skip the dispatcher and just directly control that node for simplicity.

## First steps: from nothing to starting a run

Install all prerequisites, a mongodb database, and the redax software as described above. If you have XENON wiki access there are some build notes on the DAQ page [here](https://xe1t-wiki.lngs.infn.it/doku.php?id=xenon:xenonnt:dsg:daq#reader wiki), however if you don't have access don't worry too much since everything is straight off google searches.

Consult the documentation as linked above.

If you want to configure your status collection as capped (as well as the 'aggregate_status', which stored dispatcher state history and 'system_monitor' for use with the optional monitor script) you can run helpers/initialize_databases.py.

Hook your digitizer up via optical link to your PC. Hopefully you're using our self-triggering firmware, if not you'll have to deal with configuring an external trigger for your digitizer yourself. This example will use just one digitizer.

Start the reader process as above.
