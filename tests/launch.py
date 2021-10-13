#!/usr/bin/env python3

import os
import sys
import shutil
import glob
import yaml
import hashlib
import pwd
import grp
import stat
import subprocess

TESTS_DEFS_YML='defs.yml'

tmpdir = None
projects_dir = '/var/tmp/projects'

class User(object):

    def __init__(self, name, uid):
        self.name = name
        self.uid = uid

class UsersGroup(object):

    def __init__(self, name, gid):
        self.name = name
        self.gid = gid
        self.users = []

    def add_user(self, name, uid):
        self.users.append(User(name,uid))

class UsersDB(object):

    def __init__(self, first_uid, first_gid):
        self.next_uid = first_uid
        self.next_gid = first_gid
        self.groups = []

    def add_group(self, group, users):

        new_group = UsersGroup(group, self.next_gid)
        self.next_gid+=1
        for user in users:
            new_group.add_user(user, self.next_uid)
            self.next_uid+=1
        self.groups.append(new_group)

class TestDef(object):

    def __init__(self, name, prepare, user, cmd, exitcode, stat, stdout, stderr):
        self.name = name
        self.prepare = prepare
        self.user = user
        self.cmd = cmd
        self.exitcode = exitcode
        self.stat = stat
        if stdout:
            self.stdout = stdout.encode('utf-8')
        else:
            self.stdout = None
        if stderr:
            self.stderr = stderr.encode('utf-8')
        else:
            self.stderr = None

def init_test_env(usersdb):

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

    # add user/groups in /etc/{passwd,group}
    with open('/etc/passwd', 'a') as passwd_fh:
        with open('/etc/group', 'a') as group_fh:
            for group in usersdb.groups:
                line = "%s:x:%u:%s\n" % (group.name, group.gid, ','.join([ user.name for user in group.users ]));
                group_fh.write(line)
                for user in group.users:
                     line = "%s:x:%u:%u::/tmp:/bin/false\n" % (user.name, user.uid, group.gid)
                     passwd_fh.write(line)

def load_userdb():

    usersdb = None
    print("load userdb from %s" % (TESTS_DEFS_YML))
    with open(TESTS_DEFS_YML, 'r', encoding='utf8') as stream:
        try:
            usersdb_y = yaml.safe_load(stream)['usersdb']
            usersdb = UsersDB(usersdb_y['first_uid'], usersdb_y['first_gid'])

            for group, users in usersdb_y['groups'].items():
                usersdb.add_group(group, users)

        except yaml.YAMLError as exc:
            print(exc)

    return usersdb

def load_tests_defs():

    tests = []
    print("load tests from %s" % (TESTS_DEFS_YML))
    with open(TESTS_DEFS_YML, 'r', encoding='utf8') as stream:
        try:
            tests_y = yaml.safe_load(stream)['tests']
            for xtest in tests_y:
                tests.append(TestDef(xtest['name'],
                                     xtest['prepare'],
                                     xtest['user'],
                                     xtest['cmd'],
                                     xtest['exitcode'],
                                     xtest.get('stat'),
                                     xtest['stdout'],
                                     xtest['stderr']))
        except yaml.YAMLError as exc:
            print(exc)

    return tests

def check_stat(test):

    ok = True
    if not test.stat:
        return ok

    for path, perms in test.stat.items():
        abspath = os.path.join(projects_dir, path)
        filestat = os.stat(abspath)
        if 'owner' in perms:
            found = pwd.getpwuid(filestat.st_uid).pw_name
            if found != perms['owner']:
                print("owner check on file %s failed (%s!=%s)" % (path, found, perms['owner']))
                ok = False
        if 'group' in perms:
            found = grp.getgrgid(filestat.st_gid).gr_name
            if found != perms['group']:
                print("group check on file %s failed (%s!=%s)" % (path, found, perms['group']))
                ok = False
        if 'mode' in perms:
            found = oct(stat.S_IMODE(filestat.st_mode))
            if found != perms['mode']:
                print("mode check on file %s failed (%s!=%s)" % (path, found, perms['mode']))
                ok = False

    return ok

def cmp_output(output, captured, expected):

    report_error = False
    if expected is None:
        if len(captured):
            report_error = True
            expected = "ø\n".encode('utf-8')
    else:
        expected = expected.replace(b"$TMPDIR$", tmpdir.encode('utf-8'))
        expected = expected.replace(b"$UID$", str(os.getuid()).encode('utf-8'))
        if captured != expected:
           report_error = True
    if report_error:
        print("----- [captured %s begin] -----" % (output))
        sys.stdout.buffer.write(captured)
        print("----- [captured %s end] -----" % (output))
        print("----- [expected %s begin] -----" % (output))
        sys.stdout.buffer.write(expected)
        print("----- [expected %s end] -----" % (output))
        return 1
    return 0

def run_test(test):

    uid = pwd.getpwnam(test.user).pw_uid
    gid = pwd.getpwnam(test.user).pw_gid

    os.setgid(gid)
    os.setuid(uid)

    cmd = test.cmd.replace('$BIN$', prown_path).split(' ')

    print("running cmd %s" % (str(cmd)))
    run = subprocess.run(cmd, cwd=projects_dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env={ 'LANG': 'C'})
    if run.returncode != test.exitcode:
         print("test %s failed, exit code %d is different from expected exit code %d" % (test.name, run.returncode, test.exitcode))
    if not check_stat(test):
         print("test %s failed, expected stats are not conform" % (test.name))
    if cmp_output('stdout', run.stdout, test.stdout):
         print("test %s failed, stdout is not conform" % (test.name))
    if cmp_output('stderr', run.stderr, test.stderr):
         print("test %s failed, stderr is not conform" % (test.name))

    # terminate child process
    sys.exit(0)

def run_tests(tests):

    for test in tests:

        if test.prepare:
            script_path = os.path.join(tmpdir, hashlib.sha224(test.name.encode('utf-8')).hexdigest() + '.sh')
            with open(script_path, 'w+') as script_fh:
                script_fh.write(test.prepare)
            subprocess.run(['/bin/sh', script_path], cwd=projects_dir)

        pid = os.fork()
        if pid:
            os.wait()
        else:
            run_test(test)
            # child leaves the program in run_test()

        # remove tests data in projects directory
        for path in glob.glob(os.path.join(projects_dir, '*')):
            shutil.rmtree(path)


if __name__ == '__main__':

    usersdb = load_userdb()
    init_test_env(usersdb)
    tests = load_tests_defs()
    run_tests(tests)
