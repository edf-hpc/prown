# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [4.0] - 2021-12-09

### added

- Tests framework with automated tests for many corner cases
- Added a manpage page prown(1)
- Support of i18n with translation in french
- Support of ACL to extend list of project administator groups
- Automatic code formating and styling with indent

### fixed

- Many tiny fixes thanks to static analysis with cppcheck and extra compiler
  warnings
- Test group membership based on GID instead of group name
- Fix bug due to successive allocation of string on the stack when prown is
  called with multiple paths in arguments.

### changed

- All output are in english by default, except french is set in environment
- Better messages to more easily understand prown granting logic and actual
  modifications on files
- Write errors messages on stderr
- Rename some symbols in code to match concept naming explained in
  documentation
- More generic Makefile to match packaging build systems standards expectations

## [3.8] - 2021-06-10

### fixed

- fix recursivity

### changed

- Better error in permissions

## [3.7] - 2021-05-07

### fixed

- fix bug in symbolic link handling

### changed

- align code lines

### fixed

- typo in example conf

## [3.6] - 2020-02-21

### changed

- chmod with only g+rw to keep old file permissions

## [3.5] - 2020-02-20

### added

- add execute right for group owner when prowning

### fixed

- fix error when prowning an empty subdirectory file

## [3.4] - 2020-02-04

### Changed

- print correct message error if user doesn't specify a path in project list 

## [3.3] - 2020-01-22

### Added

- recursive owning when when directory is specified
- more verbosity, to show owning steps
- fix segmentation fault error due to config file line size 
