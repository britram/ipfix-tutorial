#!/bin/sh

./sim_temprh.sh | ./ipfix_temprh 1 | nc $1 $2
