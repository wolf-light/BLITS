#!/bin/bash
if [[ "$VIRTUAL_ENV" != "" ]]; then
    echo "You are in a virtual environment."
else
    echo "You are not in a virtual environment."
    source activate.sh
fi



python STIS.py