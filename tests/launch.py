#!/usr/bin/env python3

import tempfile
import atexit
import shutil
import os
import yaml

global tmpdir

class TestDef(object):

    def __init__(self, name, prepare, user, cmd, exitcode, stdout, stderr):
        self.name = name
        self.prepare = prepare
        self.user = user
        self.cmd = cmd
        self.exitcode = exitcode
        self.stdout = stdout
        self.stderr = stderr

def clean_test_env():
    print("Removing tmp directory %s" % (tmpdir))
    shutil.rmtree(tmpdir)

def init_test_env():
    global tmpdir
    tmpdir = tempfile.mkdtemp()
    print("Created tmp directory %s" % (tmpdir))
    atexit.register(clean_test_env)

def load_tests_defs():

    tests = []
    print("load tests from defs.yml")
    with open('defs.yml', 'r') as stream:
        try:
            tests_y = yaml.safe_load(stream)
            for xtest in tests_y['tests']:
                tests.append(TestDef(xtest['name'],
                                     xtest['prepare'],
                                     xtest['user'],
                                     xtest['cmd'],
                                     xtest['exitcode'],
                                     xtest['stdout'],
                                     xtest['stderr']))
        except yaml.YAMLError as exc:
            print(exc)

    return tests

def run_tests(tests):
    pass


if __name__ == '__main__':

    init_test_env()
    tests = load_tests_defs()
    run_tests(tests)
