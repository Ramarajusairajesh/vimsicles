## Vimsicles

Vimsicles a simple alternatives for file transfer between devices (Windows, Linux and Android only) on LAN.
I used basic sockets to send data and used tar to archive and compress data (lossless).


The file transfer doesn't have encryption to improve performance .
I tested with little TSL encryption wile transferring files over 2 GB it starts burning so holding on encryption for now.
If you are running on Linux you need these to run this:



For now this application stores any received files in Downloads/vimsicles folder.


````

Cmake

g++
````

To install the application :
```

git clone https://github.com/Ramarajusairajesh/Vimsicles
cd vimsicles/Linux
make all

#After installing you will get 3 binaries 
file_sender
file_reciever 
file_sender_test   #For testing purposes


#If you want to send use file_sender or if you want to recieve use file_reciever

#The default port is 8080 if you didn't specify the port and port 8080 is used by other software it may leads to not running the reciever server
```
```
```

For Android
I used gradle to build my packages into apk and used kotlin to create the app.
Requirement :Android 8+ (API 25+)


``
Go to Releases and install the apk from there
``

For windows you need 


````
git

Cmake

Ninja

tar


````

Since tar come prebuild with windows 10 since 17063(from 2017) you don't need to install tar for this.
If you don't have neither Cmake nor Ninja either download the exe's from there official sites 
or use winget (or chocolate your wish) a terminal based package install for this I use winget to install these since it comes  prebuild with windows since 2021.




```
winget install cmake 
winget install Ninja-builder.ninja

#For installing in windows:
git clone https://github.com/Ramarajusairajesh/vimsicles

cd vimsicles/Windows
mkdir build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
#It will create a Release folder with exe files reciever.exe and sender.exe
#For windows you need to copy the files that you want to send into a seperate folder since and send it for now 

```




**Future Plans**

[+] Add screen sharing between devices

[+] Add interaction similar to Moonlight and Teamviewer 

[+] Concurrent connection with multiple devices

[] Add Andriod as wiresless mouse or controller for linux /windows

[] Make the file transfer more secure without converting my phone into a hot brick
