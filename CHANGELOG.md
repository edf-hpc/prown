# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
