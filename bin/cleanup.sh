#!/bin/bash
cd server

#read the currenrly running server process id from pid.txt
SERVER_ID=$(< pid.txt)

kill -9 $SERVER_ID