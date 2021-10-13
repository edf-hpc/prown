#!/usr/bin/env python3

import os
import sys
import yaml
import hashlib
import pwd
import subprocess

tmpdir = None
projects_dir = '/var/tmp/projects'

class TestDef(object):

    def __init__(self, name, prepare, user, cmdargs, exitcode, stdout, stderr):
        self.name = name
        self.prepare = prepare
        self.user = user
        self.cmdargs = cmdargs
        self.exitcode = exitcode
        self.stdout = stdout.encode('utf-8')
        self.stderr = stderr.encode('utf-8')

def init_test_env():

    global tmpdir
    global prown_path
    global projects_dir

    tmpdir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
    print("tmp directory is %s" % (tmpdir))
    prown_path = os.path.join(tmpdir, 'src', 'prown')

    # write prown configuration file
    with open('/etc/prown.conf', 'w+') as prown_fh:
        prown_fh.write("PROJECT_DIR %s\n" % (projects_dir))

    os.mkdir(projects_dir)

def load_tests_defs():

    tests = []
    print("load tests from defs.yml")
    with open('defs.yml', 'r', encoding='utf8') as stream:
        try:
            tests_y = yaml.safe_load(stream)
            for xtest in tests_y['tests']:
                tests.append(TestDef(xtest['name'],
                                     xtest['prepare'],
                                     xtest['user'],
                                     xtest['cmdargs'],
                                     xtest['exitcode'],
                                     xtest['stdout'],
                                     xtest['stderr']))
        except yaml.YAMLError as exc:
            print(exc)

    return tests

def cmp_output(output, captured, expected):

    expected = expected.replace(b"$TMPDIR$", tmpdir.encode('utf-8'))
    if captured != expected:
        print("----- [captured %s begin] -----" % (output))
        sys.stdout.buffer.write(captured)
        print("----- [captured %s end] -----" % (output))
        print("----- [expected %s begin] -----" % (output))
        sys.stdout.buffer.write(expected)
        print("----- [expected %s end] -----" % (output))
        return 1
    return 0

def run_tests(tests):
    for test in tests:

        if test.prepare:
            script_path = os.path.join(tmpdir, hashlib.sha224(test.name.encode('utf-8')).hexdigest() + '.sh')
            with open(script_path, 'w+') as script_fh:
                script_fh.write(test.prepare)
            subprocess.run(['/bin/sh', script_path], cwd=projects_dir)

        uid = pwd.getpwnam(test.user).pw_uid

        os.seteuid(uid)
        cmd = [ prown_path ]
        if test.cmdargs:
            cmd.extend(test.cmdargs.split(' '))
        print("running cmd %s" % (str(cmd)))
        run = subprocess.run(cmd, cwd=projects_dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env={ 'LANG': 'C'})
        if run.returncode != test.exitcode:
            print("test %s failed, exit code %d is different from expected exit code %d" % (test.name, run.returncode, test.exitcode))
        if cmp_output('stdout', run.stdout, test.stdout):
            print("test %s failed, stdout is not conform" % (test.name))
        if cmp_output('stderr', run.stderr, test.stderr):
            print("test %s failed, stderr is not conform" % (test.name))

        os.seteuid(0)

    pass


if __name__ == '__main__':

    init_test_env()
    tests = load_tests_defs()
    run_tests(tests)
