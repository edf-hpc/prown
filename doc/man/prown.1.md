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
the « administrator groups » of this project. The « project administrator
groups » are the union of the group owner of the project root directory and the
groups having a POSIX ACL with write permission attached to the project root
directory. Note that ACL entries for individual users and ACL entries without
write permission are ignored.

For exemple, considering a project directory */path/to/awesome* owned by
_physic_ group, all members of this group can use **prown** in
*/path/to/awesome*. If */path/to/awesome* directory also has an ACL entry with
write permission for _engineering_ group, all members of this group can also
use **prown** in */path/to/awesome*.

Files paths in arguments **prown** can be absolute or relative paths.

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

By default, **prown** does not display anything except errors when encountered.
The option `-v, --verbose` can be used to display all modified paths along with
runtime information.

Some options are available:

`-h, --help`

:   Display the help message and exit

`-v, --verbose`

:   Display modified paths and more information

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
