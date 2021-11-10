# Prown: manage project files owners and permissions

## Purpose

**Prown** is a tool designed to give users the possibility to get ownership of
files in _projects directories_ on POSIX filesystems under some conditions. In
[EDF](edf) conventions on HPC supercomputers, _project directories_ are
directories shared by groups of users on filesystems. Project directories and
inner files have specific ACL (eg. [POSIX ACL][usenix]) to control user/group
permissions on the files of this project.

On POSIX operating systems (eg. Linux), users cannot change the permissions of
files (mode with `chmod` or ACL with `setfacl`) unless their are owner of this
file (or [`CAP_FOWNER` capability][capabilities]). Users cannot get file
ownership (with `chown`). The process performing the `chown()` system call must
have [`CAP_CHOWN` capability][capabilities].

**Prown** is designed to give a *project administrator group* of users the
possibility to get ownership of files its project directory on POSIX
filesystems, so they can then change permissions on these files. The *project
administator group* is the group owner of the project directory.

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
ords, the directories containing project directories. **Prown** can only change
owner on the files under these directories. The directories are declared one
per line prefixed by *PROJECT\_DIR* keyword. All lines that do not start with
this keyword are ignored.

## Usage

For **prown** command usage documentation, please read
[prown(1) manpage](doc/man/prown1.md). It also contains details of its
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

## Licence

**Prown** is distributed under the terms of the GPL v3 licence.
