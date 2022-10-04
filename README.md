# Table of contents

1. Installation
2. Configuration file
3. Running the transmitter
4. Sending data

## Step 1: Installation
### 1.1 Getting the source code
````
git clone --recurse-submodules -b qrd-tx https://github.com/nakolos/srsRAN.git

cd srsRAN/

git submodule update

mkdir build && cd build
````

### 1.2 Build setup
``
cmake -DCMAKE_INSTALL_PREFIX=/usr -GNinja ..
``

### 1.3 Building
``
ninja
``

### 1.4 Installing
``
sudo ninja install
``

### 1.5 Install configs
``
sudo ./srsran_install_configs.sh
``

## Step 2: Adjust configuration file
After the installtion, you have to adjust the enb config file for your desired frequency, bandwith, tx gain, ...

Edit it by typing:
``
sudo vi /root/.config/srsran/enb.conf
``

## Step 3: Run the transmitter
Starting the transmitter requires the follwing 3 steps:
1. Starting the MBMS-Gateway
2. Starting the EPC
3. Starting the ENB

### 1. Starting the MBMS-Gateway
``
sudo srsmbms
``

The MBMS-GW receives multicast packets on one tunnel interface, packages them to GTP-U-Packets and sends them to ENB over another tunnel interface.
The command above creates the sgi_mb interface (you could see it by entering ``ifconfig`` for example). In order for the incoming data to be routed correctly, a route has to be added:

``
sudo route add -net 239.11.4.0 netmask 255.255.255.0 dev sgi_mb
``

You can use any multicast route. 

### 2. Start the EPC
``
sudo srsepc
``

### 3. Start the ENB
````
cd srsRAN/build

sudo srsenb/src/srsenb
````

After that the transmitter is running. 

## Step 4: Sending data
The transmitter is running and is ready to receive a multicast stream. Now you can, for example, transcode a local .mp4 file to rtp with ffmpeg:

``
ffmpeg -stream_loop -1 -re -i <Input-file> -vcodec copy -an -f rtp_mpegts udp://239.11.4.50:9988
``
