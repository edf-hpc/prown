#!/usr/bin/env python3

import grp

if __name__ == '__main__':
    print("Running tests in isolated environment")
    print(grp.getgrnam('biology')[3])
