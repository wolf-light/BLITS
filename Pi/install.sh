#!/bin/bash

python -m venv venv

source activate.sh

# Pip often needs to be upgraded after creating virtual environment
pip install --upgrade pip

# Install dependencies from requirements.txt
pip install -r requirements.txt
