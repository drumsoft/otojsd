#!/bin/sh

curl -s -X POST http://localhost:14609/ --data-binary @"$1" 1>&2
