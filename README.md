## Vimsicles

Vimsicles a simple alternatives for file transfer between devices (Windows, Linux and Android only) on LAN.
I used basic sockets to send data and used tar to archive and compress data (lossless).


The file transfer doesn't have encryption to improve performance .
I tested with little TSL encryption wile transfering files over 2 GB it starts burning so holding on encryption for now.
If you are running on Linux you need these to run this:


``
Cmake
g++
``


For Android



``
Go to Releases and install the apk from there
``


For windows you need 

``
Cmake
Ninja
tar
``

Since tar come prebuild with windows 10 since 17063(from 2017) you don't need to install tar for this.
If you don't have neither Cmake nor Ninja either download the exe's from there official sites 
or use winget (or chocolate your wish) a terminal based package install for this I use winget to install these since it comes 
prebuild with windows since 2021.




```
winget install cmake 
winget install Ninja-builder.ninja

```




**Future Plans**

[+] Add screen sharing between devices

[+] Add interaction similar to Moonlight and Teamviewer 

[+] Concurrent connection with multiple devices

[] Add Andriod as wiresless mouse or controller for linux /windows

[] Make the file transfer more secure without converting my phone into a hot brick
