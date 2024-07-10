<div style="display:flex;justify-content: center;">
  <img src="screenshots/logo.png">
</div>

#
---

# NMake
NMake is a simple solution to overcomplicated development environments. NMake can be used to generate basic C or C++ environments, or compile existing ones.

## Installation
We don't support Windows, and never plan to, if you still use Windows in 2024 please consider changing careers.

Linux & macOS
> ./Build.sh; ./Install.sh

To specify a path other than the default (NMake will check your PATH variable), you can use the following.
> ./Install.sh -p {YourPathHere}

## Usage
To create a new environment use the following command.

> nmake new {EnvironmentName} c++

To add to an existing environment use the following command. (NMake will automatically find the source and build directories)

> nmake add {EnvironmentName}

For more information check out the <a href="Documentation/Usage.md">Documentation</a>