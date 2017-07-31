#!/usr/bin/env python3

import json
import os
import sys

def discover(fname) :
    obj = {}
    obj["data"] = [ {"{#URL}": u} for u in json.loads(open(fname).read())["urls"] ]
    return obj

if "__main__" == __name__ :
    if len(sys.argv) != 2 :
        print("Usage : {} config_file .".format(sys.argv[0]))
        sys.exit(1)

    if not os.access(sys.argv[1], os.R_OK) :
        print("{} can not be read!".format(sys.argv[1]))
        sys.exit(1)
    
    print(json.dumps(discover(sys.argv[1])))
