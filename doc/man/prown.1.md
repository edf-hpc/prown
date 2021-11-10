% prown(1)

# NAME

prown - Change project files owner

# SYNOPSYS

`prown [-hv] FILE1 [FILE2 … [FILEn]]`

# DESCRIPTION

**Prown** is a tool designed to give users ownership of files in projects
directories. Project directories are subdirectories of directories declared in
**prown** configuration file (see **FILES** section).

Users are granted to use **prown** in a project directory if they are member of
this project « administrator group » of this project. The « project
administrator group » is the group owner of the project directory. For exemple,
considering a project directory */path/to/awesome* owned by group _physic_,
all members of the _physic_ group can use **prown** in */path/to/awesome*.

Files paths in arguments can be absolute or relative paths.

If multiple files are given in arguments, **prown** changes owner of all the
given files.

If a directory is given in arguments, **prown** changes owner of all the files
recursively in this directory.

**Prown** does not dereferences symbolic links. If it encounters a symbolic
link, it changes the owner of the source but it does not modify the target,
even if this target is inside the same project directory. Users are expected to
**prown** the target of symbolic links explicitely.

**Prown** does not change owner of project directories roots. If a user runs
**prown** with project directory root path in argument, it sets user as owner
of all the files in this project directory except the project root directory
itself.

Additionally to changing the owner of the files, **prown** also ensures the
group class permissions have read and write bits enabled (similarly to
`chmod g+rw`). When POSIX ACL are defined, the group class permissions map the
mask entry when POSIX ACL are defined. The goal is to ensure permissions on
named user and group ACL entries are mostly effectives after *prown*
processing.

Some options are available:

`-h, --help`

:   Show the help message and exit

`-v, --verbose`

:   Show more details about modifications on files

# EXAMPLES

Considering a parent directory */path/to* declared in **prown** configuration
file (see **FILES**), here are some examples of **prown** commands:

    $ prown /path/to/awesome

Change ownership of all files recursively in _awesome_ project directory.

    $ cd /path/to
    $ prown awesome

Change ownership of all files recursively in _awesome_ project directory, using
relative path.

    $ prown /path/to/awesome/file

Change ownership of a particular file in _awesome_ project directory.

    $ prown /path/to/awesome/subdir

Change ownership of all files recursively in a subdirectory of _awesome_
project directory.

# FILES

*/etc/prown.conf*

: System configuration file with list of parent directories of project
directories or, in other words, the directories containing project directories.
*Prown* can only change owner on the files under these directories. For more
details about the syntax of this file, please refer to *prown* README.md file.

# COPYRIGHT

Copyright © 2021 EDF SA. The author of Prown is CCN-HPC team of EDF SA company.

Prown is a free software distributed under the terms of the GPL v3 licence.
