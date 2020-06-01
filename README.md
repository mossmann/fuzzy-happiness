## Make sure you have a GCC cross-compiler for ARM installed.  For example, on Ubuntu:
apt install gcc-arm-none-eabi

## Make sure you have a copy of the GreatFET repo:
git clone --recurse-submodules https://github.com/greatscottgadgets/greatfet.git

## Assuming the GreatFET tree is located at ~/greatfet
export GREATFET_PATH=~/greatfet/

## configure:
mkdir build
cd build
cmake ..

## build:
make

## hold the DFU button while pressing and releasing the RESET button, then release the DFU button, then program with:
gf fw -d -w led-demo_external.bin
