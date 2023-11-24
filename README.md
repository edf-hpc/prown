# Prown: manage project files owners and permissions

## Purpose

**Prown** is a tool designed to give users the possibility to get ownership of
files in _projects directories_ on POSIX filesystems under some conditions. In
[EDF][edf] conventions on HPC supercomputers, _project directories_ are
directories shared by groups of users on filesystems. Project directories and
inner files have specific ACL (eg. [POSIX ACL][usenix]) to control user/group
permissions on the files of this project.

On POSIX operating systems (eg. Linux), users cannot change the permissions of
files (mode with `chmod` or ACL with `setfacl`) unless their are owner of this
file (or [`CAP_FOWNER` capability][capabilities]). Users cannot get file
ownership (with `chown`). The process performing the `chown()` system call must
have [`CAP_CHOWN` capability][capabilities].

**Prown** is designed to give *project administrator groups* of users the
possibility to get ownership of files in its project directory on POSIX
filesystems, so they can then change permissions on these files. The *project
administator groups* are the union of the group owner of the project root
directory and the groups having a POSIX ACL with write permission attached to
the project root directory.

Optionally, *Prown* usage can be granted only to authorized subset of group(s)
in *project administator groups*, named *authorized groups*!
See bellow for more details.


Here is a typical example, considering an _awesome_ project directory owned by
_physic_ group containing _alice_ and _bob_ users:

```
root@host ~ # stat --format=%U:%G /path/to/awesome
root:physic
root@host ~ # getent group physic
physic:*:1000:alice,bob
```

First, _alice_ creates a new file in _awesome_ project directory and adds
execution permission on this file:

```
alice@host ~ $ touch /path/to/awesome/data
alice@host ~ $ chmod u+x /path/to/awesome/data
```

But _bob_ also wants to execute this file! As _bob_ is member of the _physic_
project administrator group, he can use **prown** to add execution permission
to all the _physic_ group members:

```
bob@host ~ $ prown /path/to/awesome/data
bob@host ~ $ chgrp physic /path/to/awesome/data
bob@host ~ $ chmod g+x /path/to/awesome/data
```

Consider a third user _carol_, member of _engineering_ group but not in
_physic_ group:

```
root@host ~ # groups carol
carol : engineering
root@host ~ # getent group engineering
engineering:*:1001:carol
```

She can be granted to use **prown** in _awesome_ project directory by adding
a POSIX ACL group entry for the _engineering_ group with write permission on
the root directory of the project:

```
root@host ~ # setfacl -m group:engineering:rwx /path/to/awesome
```

Then:

```
carol@host ~ $ prown /path/to/awesome/data
```

By default, **Prown** proceed recursively, meaning all content of _awesome_
path provided will be changed!
For instance, imagine that user _alice_ have launched bellow commands:

```
alice@host ~ $ mkdir /path/to/awesome/subdir
alice@host ~ $ touch /path/to/awesome/subdir/file
```

When _carol_ launch **Prown** on path `/path/to/awesome/subdir`, by default,<br />
it's will subsequently changed also right on `/path/to/awesome/subdir/file`!

To avoid this default behaviour, needful switch must be use accordingly:

```
carol@host ~ $ prown -d /path/to/awesome/subdir
#OR# carol@host ~ $ prown --directory /path/to/awesome/subdir
```

It's also **optionally** possible to enforce **Prown** usage only for user
member of specific authorized groups, say _physic_ and _engineering_ one!

Consider a fourth user _marie_, member of _research_ group but not in either
_physic_ nor _engineering_ groups:

```
root@host ~ # groups marie
marie : research
root@host ~ # getent group research
research:*:1002:marie
```

Following same rule as _carol_ user, _marie_ **should** be granted to use
**prown** in _awesome_ project directory by adding a POSIX ACL group entry for
the _research_ group with write permission on the root directory of the project:

```
root@host ~ # setfacl -m group:research:rwx /path/to/awesome
```

But **Prown** must failed:

```
marie@host ~ $ prown /path/to/awesome/data
```

With bellow error:

```
User is NOT a valid member of authorized group physic (1000)
User is NOT a valid member of authorized group engineering (1001)
User is NOT a valid member of any authorized group!
```

[edf]: https://www.edf.fr/en/meta-home
[usenix]: https://www.usenix.org/legacy/publications/library/proceedings/usenix03/tech/freenix03/full_papers/gruenbacher/gruenbacher_html/main.html
[capabilities]: https://man7.org/linux/man-pages/man7/capabilities.7.html

## Quickstart

Compile **prown** source code:

```
$ make make
```

Then, as _root_, install **prown**:

```
# make install
```

Add `CAP_CHOWN` capability to the binary:

```
# setcap cap_chown+ep /usr/local/bin/prown
```

Setup the parent directory on the project directories:

```
# echo "PROJECT_DIR /path/to/projects" > /etc/prown.conf
```

## Configuration

**Prown** uses one system-wide configuration file `/etc/prown.conf`.

It contains the list of parent directories of project directories or, in other
words, the directories containing project directories. **Prown** can only change
owner on the files under these directories. The directories are declared one
per line prefixed by *PROJECT\_DIR* keyword. All lines that do not start with
this keyword are ignored.

At another part, authorization can be enforced with **optional** keyword
*AUTHORIZED\_GROUP*, one per line. When this keyword exist in `/etc/prown.conf`
file, user must be not only member of writable group, but also member of
groups list by *AUTHORIZED\_GROUP*,  to be able to use **Prown**.

## Usage

For **prown** command usage documentation, please read
[prown(1) manpage](doc/man/prown.1.md). It also contains details of its
behaviour with some special cases (symbolic links, directories, etc).

## Code maintainance

### Code style

**Prown** C code base follows styling rules controlled by this Makefile target:

```
make indent
```

When this command is run, **prown** C source code is reformated to follow all
styling rules defined by the project.

This requires GNU indent to be installed on your system, eg. on Debian:

```
sudo apt install indent
```

### Static checks

To realize static code analysis over **prown** source code, run this command:

```
make check
```

This runs `cppcheck` utility over **prown** source code and reports all bugs
and errors found.

This requires `cppcheck` to be installed on your system, eg. on Debian:

```
sudo apt install cppcheck
```

### Functionnal tests

To run functionnal tests, just run this command:

```
make tests
```

Tests require `sudo` to prepare the testing environment as root.

To perform the tests for many corner cases and the most complex behaviours,
multiple users and groups are needed. Prown comes with its own test framework
to prepare a test environment with fake users and groups.

This is realized with a first shell script `run.sh` that install **prown** in
`/tmp` and then runs `isolate` program. This one creates a Linux mount
namespace and setup an overlay with host root filesystem (RO) and a tmpfs (RW).
The `/tmp` directory is bind-mounted from the host to get access to **prown**
installation. The `isolate` program finally runs Python script `launch.py`.
This one first reads tests definitions in Yaml file `defs.yml`, then creates
the defined fake users and groups in `/etc/passwd`, `/etc/shadown` and
`/etc/groups` (in tmpfs) and runs all tests. The results are checked against
expected output, files owner/modes modifications, etc and finally reported.

### i18n

The gettext pot and po file for translation are automatically updated within
the makefile all target.

## Licence

**Prown** is distributed under the terms of the GPL v3 license.
