# Parser-GPIO-STM32F407
Subject: Microprocessor systems in telecommunications, Practice problem

## How to run
### Clone the repository
```
git clone https://github.com/aTTiny73/multilogger.git
```
### Cd into cloned repository and run upload.sh script
```
sh upload.sh
```
### Instal picocom with command
```
sudo apt install picocom
```
### Connect USB/USART dongle pins RX->PA2 , TX->PA3 and plug in the dongle into PC, run the following command:
```
picocom /dev/ttyUSB0 -b 115200
```
### Enter the help command to see supported commands.
```
----------------------------------|Help|------------------------------------|
Command: pbtn 
 Returns current state of user push button. 
----------------------------------------------------------------------------|
Command: led s N S , Example: led s 0 1 , led s 1 0 
 Sets the N(0,1,2,3) led into static state (0,1).
----------------------------------------------------------------------------|
Command: led b N O P , Example: led b 0 300 1000, led b 1 500 1000 
 Sets the N(0,1,2,3) led into blink mode, O(Ontime ms), P(Period ms). 
----------------------------------------------------------------------------|
Command: led p N D , Example: led p 0 50 
 Sets the N(0,1,2,3) led into PWM mode, D(Duty cycle 0-100).
----------------------------------------------------------------------------|
Command: mapb P , Example: mapb PA3 
 Sets user defined pin into a input mode.
----------------------------------------------------------------------------|
Command: map1 P , Example: map1 PA3 
 Changes the output pin of led 0 when calling the (led s 0) command .
----------------------------------------------------------------------------|
Command: help 
 Shows all commands with examples.
----------------------------------------------------------------------------|
```
