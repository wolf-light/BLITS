# Intructions and Assumuptions 

## Assumptions 
- Python3 is installed and runs using `python`
- The system has access to the internet or cached versions of the newest packages specified in requirements.txt
- The system has serial ports

## Instructions

1. Environment Setup
   1. Create virtual environment
        - `source install.sh`
        - This also installs dependencies so go to step 2
   2. Install dependencies
      -   This only needs to be done if you haven't done it yet or there are new dependencies (ie. a change to requirements.txt)
      - `pip install -r requirements.txt`   
   3. Activate environment run one of the following
      - `source activate.sh`
      - On windows: `source venv/Scripts/Activate`
      - On other: `source venv/bin/activate`
2. Running the script
    - ```source run.sh```
    - `python STIS.py`
3. Using the Interface
    