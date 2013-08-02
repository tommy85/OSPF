OSPF
====

Network: OSPF algorithm implementation in routers

Compile application:
After extract the files, use "make" command to compile the application.

Run application:
nse: 
Example:
./nse-linux386 129.97.167.51 2222

router1: ./router 1 129.97.167.54 2222 2223
router2: ./router 2 129.97.167.54 2222 2224  
router3: ./router 3 129.97.167.54 2222 2225	
router4: ./router 4 129.97.167.54 2222 2226	
router5: ./router 5 129.97.167.54 2222 2227	


The program is built on MAC OS 10.8.3 machine with software XCODE version 4.6.1.

The test environment is :
nse on linux008:
./nse-linux386 129.97.167.51 2222
routers on linux024:
router1: ./router 1 129.97.167.54 2222 2223
router2: ./router 2 129.97.167.54 2222 2224	
router3: ./router 3 129.97.167.54 2222 2225	
router4: ./router 4 129.97.167.54 2222 2226	
router5: ./router 5 129.97.167.54 2222 2227	


version of make:
	GNU Make 3.81
version of compiler:
	gcc version 4.2.1


	
 
