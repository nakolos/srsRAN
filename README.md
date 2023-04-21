# Table of contents

1. Install dependencies
2. Install SDR drivers
3. Build the transmitter
4. Run the transmitter
5. Send data

## Step 1: Install Dependencies

#### Ubuntu 22.04 LTS
````
sudo apt update
sudo apt install ssh g++ git libboost-atomic-dev libboost-thread-dev libboost-system-dev libboost-date-time-dev libboost-regex-dev libboost-filesystem-dev libboost-random-dev libboost-chrono-dev libboost-serialization-dev libwebsocketpp-dev openssl libssl-dev ninja-build libspdlog-dev libmbedtls-dev libboost-all-dev libconfig++-dev libsctp-dev libfftw3-dev vim libcpprest-dev libusb-1.0-0-dev net-tools smcroute python3-pip clang-tidy gpsd gpsd-clients libgps-dev
sudo snap install cmake --classic
sudo pip3 install cpplint
sudo pip3 install psutil
````

## Step 2: Install SDR drivers
### 2.1 Installing SDR drivers

````
sudo apt install libsoapysdr-dev soapysdr-tools
````

##### Using BladeRF with Soapy
For BladeRF the relevant package is named *soapysdr-module-bladerf*. Install it by running:
````
sudo apt install soapysdr-module-bladerf
````
Finally, install the BladeRF firmware:
````
sudo bladeRF-install-firmware
````

### 2.2 Check SDR availability
Check if the SDR can be found on your system
````
SoapySDRUtil --find
````

The output should look like this:
````
######################################################
##     Soapy SDR -- the SDR abstraction library     ##
######################################################
Found device 2
  backend = libusb
  device = 0x02:0x09
  driver = bladerf
  instance = 0
  label = BladeRF #0 [ANY]
  serial = ANY
````

## Step 3: Build the transmitter

### 3.1 Getting the source code
````
git clone --recurse-submodules -b qrd-tx https://github.com/nakolos/srsRAN.git

cd srsRAN/

git submodule update

mkdir build && cd build
````

### 3.2 Build setup
``
cmake -DCMAKE_INSTALL_PREFIX=/usr -GNinja ..
``

### 3.3 Building
``
ninja
``

### 3.4 Installing
``
sudo ninja install
``

### 3.5 Install configs
``
sudo ./srsran_install_configs.sh user
``

### 3.6: Adjust configuration files
After the installtion, you have to adjust the enb, rr, epc config files to your desired frequency, bandwith, tx gain, MNC, MCC ...

Alternatively, you can use our Config-templates:
````
cd srsRAN/
sudo cp Config-Template/* /root/.config/srsran/
````

You can still change the frequency, gain or whatever if you want to. 

Also make sure to copy the adapted sib.conf.mbsfn file to the build directory:
````
cd srsRAN/
cp sib.conf.mbsfn build/sib.conf.mbsfn
````

## Step 4: Run the transmitter
Starting the transmitter requires the follwing 3 steps:
1. Starting the MBMS-Gateway
2. Starting the EPC
3. Starting the ENB

### 4.1 Starting the MBMS-Gateway
``
sudo srsmbms
``

The MBMS-GW receives multicast packets on one tunnel interface, packages them to GTP-U-Packets and sends them to ENB over another tunnel interface.
The command above creates the sgi_mb interface (you could see it by entering ``ifconfig`` for example). In order for the incoming data to be routed correctly, a route has to be added:

``
sudo route add -net 239.11.4.0 netmask 255.255.255.0 dev sgi_mb
``

You can use any multicast route. 

### 4.2 Start the EPC
``
sudo srsepc
``

### 4.3 Start the ENB
````
cd srsRAN/build

sudo srsenb/src/srsenb
````

After that the transmitter is running. 

## Step 5: Sending data
The transmitter is running and is ready to receive a multicast stream. Now you can, for example, transcode a local .mp4 file to rtp with ffmpeg:

``
ffmpeg -stream_loop -1 -re -i <Input-file> -vcodec copy -an -f rtp_mpegts udp://239.11.4.50:9988
``
