# Pendant2019

Firmware for 2019 DuckPond pendant.

Prerequisites on Ubuntu:

```
> sudo apt install arm-none-eabi-gcc cmake
```

Prerequisites on OSX:

```
> brew install cmake
> brew install arm-none-eabi-gcc
```

Emulator build:

```
> git clone https://github.com/tinic/Pendant2019.git
> cd Pendant2019
> mkdir build
> cd build

> cmake -DEMULATOR_BUILD=1 ..
> make -j
> ./Pendant2019
```

Device build:

```
> sudo apt install arm-none-eabi-gcc cmake
> git clone https://github.com/tinic/Pendant2019.git
> cd Pendant2019
> mkdir build
> cd build

> cmake ..
> make -j
> make install
```
