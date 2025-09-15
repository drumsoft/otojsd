#!/bin/sh
# %%%{CotEditorXInput=Selection}%%%

curl -s -X POST http://localhost:14609/ --data-binary @- 1>&2
