#!/bin/bash
# Uruchamianie z menu Ports w Knulli. Log: /userdata/dawpol3000/log.txt
cd /userdata/dawpol3000 || exit 1
./dawpol3000 > log.txt 2>&1
