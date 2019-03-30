#! python3
#
# W600 make gzip file
# Copyright (c) 2018 Winner Micro Electronic Design Co., Ltd.
# All rights reserved.s
import sys
import gzip

file_name = sys.argv[1]

with open(file_name, 'rb') as f_in:
    with gzip.open(file_name + '.gz', 'wb') as f_out:
        f_out.writelines(f_in)
