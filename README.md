Prown, a tool to manage project owners and permissions
======================================================

Name
----

prown - Change project owner 

Synopsys
--------
    prown DIRECTORY(s)

Description
-----------

Prown is a simple tool developed to give users the possibility to own projects. It uses the configuration file in /etc/prown.conf to specify the projects directories. When a user specify one or multiple directories, Prown verify the user permissions and changes recursively the owner of the directory to that user. 
- User must be in the group owner of the directory or the file with Write access to become owner.
- User can't own the root directory of a project.
- User can't own directories outside the list of projects in config file. 

Tests
-----

To run tests, just run this command:

```
make tests
```

Tests require `sudo` to prepare the testing environment as root.

Licence
-------

Prown is distributed under the terms of the GPL v3 licence.
