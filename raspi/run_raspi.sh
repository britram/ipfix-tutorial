#!/bin/sh

./dht_temprh 2302 4 | ./ipfix_temprh 1 | nc $1