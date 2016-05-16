#!/bin/bash
apt-get update
apt-get install linux-headers-$(uname -r)
make
