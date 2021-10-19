#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Prown is a simple tool developed to give users the possibility to
# own projects (files and repositories).
# Copyright (C) 2021 EDF SA.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
import codecs
import sys
import textwrap

TESTS_DEFS_YML='tests/defs.yml'

tmpdir = None
projects_dir = '/var/tmp/projects'

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

class User(object):

    def __init__(self, name, uid, primary_group):
        self.name = name
        self.uid = uid
        self.primary_group = primary_group

class UsersGroup(object):

    def __init__(self, name, gid):
        self.name = name
        self.gid = gid
        self.users = []

class UsersDB(object):

    def __init__(self, first_uid, first_gid):
        self.next_uid = first_uid
        self.next_gid = first_gid
        self.groups = []
        self.users = []

    def add_user(self, user, primary_group):

        group = self.add_group(primary_group, [])
        print("new user %s in group %s [%d]" % (user, group.name, self.next_uid))
        self.users.append(User(user, self.next_uid, group))
        self.next_uid+=1

    def add_group(self, group_name, users):

        group = None
        for xgroup in self.groups:
            if xgroup.name == group_name:
                print("group %s [%d] already exist" % (group_name, xgroup.gid))
                group = xgroup
                break

        if group is None:
            print("new group %s [%d]" % (group_name, self.next_gid))
            group = UsersGroup(group_name, self.next_gid)
            self.next_gid+=1
            self.groups.append(group)

        for user in users:
            for xuser in self.users:
                if xuser.name == user:
                    print("adding user %s in group %s" % (user, group_name))
                    group.users.append(xuser)

        return group

class TestDef(object):

    def __init__(self, name, prepare, tree, user, cmd, shell, exitcode, stat, stdout, stderr):
        self.name = name
        self.prepare = prepare
        self.tree = tree
        self.user = user
        self.cmd = cmd
        self.shell = shell
        self.exitcode = exitcode
        self.stat = stat
        self.stdout = stdout
        self.stderr = stderr

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
        with open('/etc/shadow', 'a') as shadow_fh:
            with open('/etc/group', 'a') as group_fh:
                for group in usersdb.groups:
                    line = "%s:x:%u:%s\n" % (group.name, group.gid, ','.join([ user.name for user in group.users if group.name != user.primary_group.name ]))
                    print("group line: %s" % (line), end='')
                    group_fh.write(line)
                    for user in group.users:
                         line = "%s:x:%u:%u::/tmp:/bin/false\n" % (user.name, user.uid, user.primary_group.gid)
                         print("passwd line: %s" % (line), end='')
                         passwd_fh.write(line)
                         line = "%s::::::::\n" % (user.name)
                         shadow_fh.write(line)

def warn(msg):
    print(bcolors.FAIL + msg + bcolors.ENDC)

def success(msg):
    print(bcolors.OKGREEN + msg + bcolors.ENDC)

def load_userdb():

    usersdb = None
    print("load userdb from %s" % (TESTS_DEFS_YML))
    with open(TESTS_DEFS_YML, 'r', encoding='utf8') as stream:
        try:
            usersdb_y = yaml.safe_load(stream)['usersdb']
            usersdb = UsersDB(usersdb_y['first_uid'], usersdb_y['first_gid'])

            for user_group in usersdb_y['users']:
                (user, group) = user_group.split(':')
                usersdb.add_user(user, group)
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
                                     xtest.get('tree', False),
                                     xtest['user'],
                                     xtest['cmd'],
                                     xtest.get('shell', False),
                                     xtest['exitcode'],
                                     xtest.get('stat'),
                                     xtest['stdout'],
                                     xtest['stderr']))
        except yaml.YAMLError as exc:
            print(exc)

    return tests

def check_stat(test):

    msg = ""

    if not test.stat:
        return None

    for path, perms in test.stat.items():
        if path[0] == '/':
            abspath = path
        else:
            # if the path is not absolute, consider it is relative to projects directory
            abspath = os.path.join(projects_dir, path)
        filestat = os.lstat(abspath)
        if 'owner' in perms:
            found = pwd.getpwuid(filestat.st_uid).pw_name
            if found != perms['owner']:
                msg += "owner check on file %s failed (%s!=%s)\n" % (path, found, perms['owner'])
        if 'group' in perms:
            found = grp.getgrgid(filestat.st_gid).gr_name
            if found != perms['group']:
                msg += "group check on file %s failed (%s!=%s)\n" % (path, found, perms['group'])
        if 'mode' in perms:
            found = oct(stat.S_IMODE(filestat.st_mode))
            if found != perms['mode']:
                msg += "mode check on file %s failed (%s!=%s)\n" % (path, found, perms['mode'])

    if not len(msg):
        return None
    return msg

def cmp_output(output, captured, expected):

    report_error = False
    if expected is None:
        if len(captured):
            report_error = True
            expected = "ø\n"
    else:
        expected = expected.replace("$TMPDIR$", tmpdir)
        expected = expected.replace("$UID$", str(os.getuid()))
        if captured != expected:
           report_error = True
    if report_error:
        msg  = "----- [captured %s begin] -----\n" % (output)
        msg += captured
        msg += "-----  [captured %s end]  -----\n" % (output)
        msg += "----- [expected %s begin] -----\n" % (output)
        msg += expected
        msg += "-----  [expected %s end]  -----\n" % (output)
        return msg

    return None

def run_test(test):

    uid = pwd.getpwnam(test.user).pw_uid
    gid = pwd.getpwnam(test.user).pw_gid

    # set supplementary groups, just like after user login
    os.setgroups(os.getgrouplist(test.user, gid))
    os.setgid(gid)
    os.setuid(uid)

    cmd = test.cmd.replace('$BIN$', prown_path)

    if not test.shell:
        cmd = cmd.split(' ')

    status = 0
    errors = []
    run = subprocess.run(cmd, shell=test.shell, cwd=projects_dir, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env={ 'LANG': 'C'})
    if run.returncode != test.exitcode:
         errors.append("exit code %d is different from expected exit code %d\n" % (run.returncode, test.exitcode))

    msg = cmp_output('stdout', run.stdout.decode('utf-8'), test.stdout)
    if msg:
         errors.append("stdout is not conform:\n%s" % (msg))
    msg = cmp_output('stderr', run.stderr.decode('utf-8'), test.stderr)
    if msg:
         errors.append("stderr is not conform:\n%s" % (msg))

    if len(errors):
       warn(u"❌test « %s » failed, errors:" % (test.name))
       for error in errors:
           warn(textwrap.indent(error, '  + ', lambda line: True))
       status = 1

    # at this point, we cannot tell if the test is successful as the parent
    # with root permissions must check stats.

    # terminate child process
    sys.exit(status)

def run_tests(tests):

    nb_tests = 0
    nb_errors = 0
    for test in tests:
        nb_tests += 1
        if test.prepare:
            script_path = os.path.join(tmpdir, hashlib.sha224(test.name.encode('utf-8')).hexdigest() + '.sh')
            with open(script_path, 'w+') as script_fh:
                script_fh.write(test.prepare)
            subprocess.run(['/bin/sh', script_path], cwd=projects_dir)

        if test.tree:
            print("tree before:")
            subprocess.run(['tree', '-u', '-p', '-A', '-C', '-l', '-g', projects_dir])

        pid = os.fork()
        if pid:
            status = os.wait()
            nb_errors += status[1]>>8
        else:
            run_test(test)
            # child leaves the program in run_test()

        # now check stats as root
        msg = check_stat(test)
        if not nb_errors:
           if msg:
               warn(u"❌test « %s » failed, errors:" % (test.name))
               nb_errors+=1
           else:
               success(u"✓ test « %s » succeeded" % (test.name))
        if msg:
           warn(textwrap.indent("expected stats are not conform:\n%s" % (msg), '  + '))

        if test.tree:
            print("tree after:")
            subprocess.run(['tree', '-u', '-p', '-A', '-C', '-l', '-g', projects_dir])

        # remove tests data in projects directory
        for path in glob.glob(os.path.join(projects_dir, '*')):
            shutil.rmtree(path)

    print("%d tests | %d ok | %d failed" % (nb_tests, (nb_tests-nb_errors), nb_errors))


if __name__ == '__main__':

    usersdb = load_userdb()
    init_test_env(usersdb)
    tests = load_tests_defs()
    run_tests(tests)
